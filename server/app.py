"""Account-scoped save storage. Run with ``uvicorn server.app:app``.

The SQLite file must be on a persistent, private volume. Set WARBLADE_DB_PATH and
WARBLADE_PUBLIC_ORIGIN in deployment; the latter is the exact browser origin.
"""

import hashlib
import ipaddress
import json
import logging
import math
import os
import re
import secrets
import sqlite3
import time
from collections import deque
from contextlib import contextmanager
from pathlib import Path
from threading import Lock
from urllib.parse import urlsplit

from argon2 import PasswordHasher
from argon2.exceptions import InvalidHashError, VerifyMismatchError
from fastapi import Depends, FastAPI, Header, HTTPException, Request, Response
from fastapi.responses import JSONResponse
from server import hiscores

MAX_FILE_BYTES = 2 * 1024 * 1024
MAX_ACCOUNT_BYTES = 64 * 1024 * 1024
MAX_FILES = 512
SESSION_SECONDS = 30 * 24 * 60 * 60
COOKIE = "warblade_session"
TOKEN = re.compile(r"[A-Za-z0-9_-]{43}\Z")
USERNAME = re.compile(r"[A-Za-z0-9][A-Za-z0-9_.-]{2,31}\Z")
SEGMENT = re.compile(r"[A-Za-z0-9][A-Za-z0-9 _.-]{0,99}\Z")
hasher = PasswordHasher()
DUMMY_HASH = hasher.hash("invalid-account-placeholder")
PERFORMANCE_LOG = logging.getLogger("uvicorn.error")
PERFORMANCE_FIELDS = frozenset(("version", "device", "browser", "phase", "frames",
                                "duration_ms", "fps", "mean_ms", "p95_ms", "max_ms",
                                "over_33_pct", "target_fps"))


def _performance_fields(data: bytes) -> dict:
    def unique_fields(pairs):
        fields = {}
        for key, value in pairs:
            if key in fields:
                raise ValueError("Duplicate field")
            fields[key] = value
        return fields

    try:
        fields = json.loads(data, object_pairs_hook=unique_fields)
    except (ValueError, TypeError, UnicodeDecodeError):
        raise HTTPException(400, "Invalid performance report") from None
    if not isinstance(fields, dict) or fields.keys() != PERFORMANCE_FIELDS:
        raise HTTPException(400, "Invalid performance report")
    for name, lower, upper in (("version", 1, 1), ("frames", 1, 20000),
                                ("target_fps", 1, 300)):
        if type(fields[name]) is not int or not lower <= fields[name] <= upper:
            raise HTTPException(400, "Invalid performance report")
    for name, choices in (("device", ("mobile", "desktop")),
                          ("browser", ("chromium", "firefox", "safari", "other")),
                          ("phase", ("menu", "play", "paused"))):
        if type(fields[name]) is not str or fields[name] not in choices:
            raise HTTPException(400, "Invalid performance report")
    for name, lower, upper in (("duration_ms", 1000, 120000), ("fps", 0, 300),
                                ("mean_ms", 0, 10000), ("p95_ms", 0, 10000),
                                ("max_ms", 0, 120000), ("over_33_pct", 0, 100)):
        value = fields[name]
        if (type(value) not in (int, float) or not lower <= value <= upper or
                not math.isfinite(value)):
            raise HTTPException(400, "Invalid performance report")
    if (abs(fields["fps"] - fields["frames"] * 1000 / fields["duration_ms"]) > 0.2 or
            abs(fields["mean_ms"] - fields["duration_ms"] / fields["frames"]) > 0.2 or
            fields["p95_ms"] > fields["max_ms"] or fields["mean_ms"] > fields["max_ms"]):
        raise HTTPException(400, "Invalid performance report")
    return fields


def _path(path: str) -> str:
    # A database key is never interpreted as a host filesystem path. Restricting
    # segments still prevents ambiguous URL encodings and accidental bad imports.
    if len(path.encode("utf-8")) > 240 or not path.startswith("warblade/"):
        raise HTTPException(400, "Invalid save path")
    parts = path.split("/")
    if len(parts) < 2 or len(parts) > 8 or any(
        part in (".", "..") or not SEGMENT.fullmatch(part) for part in parts
    ):
        raise HTTPException(400, "Invalid save path")
    return path


def _etag(version: str) -> str:
    return '"' + version + '"'


def _matches(if_match: str, version: str) -> bool:
    # The browser uses the JSON version; HTTP clients may use the quoted ETag.
    return if_match in (version, _etag(version))


def _positive_setting(name: str, fallback: int) -> int:
    value = int(os.environ.get(name, str(fallback)))
    if value < 1:
        raise ValueError(name + " must be positive")
    return value


def create_app(db_path: str | Path, public_origin: str, *, secure_cookie: bool | None = None,
               max_accounts: int | None = None, login_attempts: int | None = None,
               register_attempts: int | None = None, auth_window_seconds: int | None = None,
               trusted_proxy_ips: str | None = None, session_mode: str | None = None) -> FastAPI:
    session_mode = session_mode if session_mode is not None else os.environ.get("WARBLADE_SESSION_MODE", "cookie")
    if session_mode not in ("cookie", "memory"):
        raise ValueError("WARBLADE_SESSION_MODE must be cookie or memory")
    origin = urlsplit(public_origin)
    if (origin.scheme not in ("http", "https") or not origin.netloc or
            origin.path or origin.query or origin.fragment or origin.username or origin.password):
        raise ValueError("WARBLADE_PUBLIC_ORIGIN must be an exact HTTP(S) origin")
    if origin.scheme == "http" and origin.hostname not in ("localhost", "127.0.0.1", "::1"):
        raise ValueError("WARBLADE_PUBLIC_ORIGIN must use HTTPS outside localhost")
    if secure_cookie is None:
        secure_cookie = origin.scheme == "https"
    max_accounts = max_accounts if max_accounts is not None else _positive_setting("WARBLADE_MAX_ACCOUNTS", 100)
    login_attempts = login_attempts if login_attempts is not None else _positive_setting("WARBLADE_LOGIN_ATTEMPTS", 30)
    register_attempts = register_attempts if register_attempts is not None else _positive_setting("WARBLADE_REGISTER_ATTEMPTS", 10)
    auth_window_seconds = auth_window_seconds if auth_window_seconds is not None else _positive_setting("WARBLADE_AUTH_WINDOW_SECONDS", 900)
    if min(max_accounts, login_attempts, register_attempts, auth_window_seconds) < 1:
        raise ValueError("API limits must be positive")
    proxies = frozenset(ipaddress.ip_address(item.strip()) for item in
                        (trusted_proxy_ips if trusted_proxy_ips is not None else
                         os.environ.get("WARBLADE_TRUSTED_PROXY_IPS", "")).split(",") if item.strip())
    db_path = str(db_path)
    Path(db_path).parent.mkdir(parents=True, exist_ok=True)

    @contextmanager
    def connection():
        db = sqlite3.connect(db_path, timeout=10, isolation_level=None)
        db.row_factory = sqlite3.Row
        db.execute("PRAGMA foreign_keys=ON")
        db.execute("PRAGMA busy_timeout=10000")
        try:
            yield db
        finally:
            db.close()

    with connection() as db:
        db.execute("PRAGMA journal_mode=WAL")
        db.executescript("""
            CREATE TABLE IF NOT EXISTS accounts (
                id INTEGER PRIMARY KEY, username TEXT NOT NULL UNIQUE COLLATE NOCASE,
                password_hash TEXT NOT NULL
            );
            CREATE TABLE IF NOT EXISTS sessions (
                token_hash BLOB PRIMARY KEY, account_id INTEGER NOT NULL
                    REFERENCES accounts(id) ON DELETE CASCADE,
                expires_at INTEGER NOT NULL
            );
            CREATE INDEX IF NOT EXISTS sessions_expiry ON sessions(expires_at);
            CREATE TABLE IF NOT EXISTS saves (
                account_id INTEGER NOT NULL REFERENCES accounts(id) ON DELETE CASCADE,
                path TEXT NOT NULL, data BLOB NOT NULL, version TEXT NOT NULL,
                PRIMARY KEY (account_id, path)
            );
            CREATE TABLE IF NOT EXISTS auth_attempts (
                peer_ip TEXT NOT NULL, action TEXT NOT NULL,
                window_start INTEGER NOT NULL, attempts INTEGER NOT NULL,
                PRIMARY KEY (peer_ip, action)
            );
            CREATE TABLE IF NOT EXISTS score_attempts (
                account_id INTEGER PRIMARY KEY REFERENCES accounts(id) ON DELETE CASCADE,
                window_start INTEGER NOT NULL, attempts INTEGER NOT NULL
            );
        """)
        hiscores.init(db)

    api = FastAPI(docs_url=None, redoc_url=None, openapi_url=None)

    @api.middleware("http")
    async def prevent_cache(request: Request, call_next):
        response = await call_next(request)
        response.headers["Cache-Control"] = "private, no-store"
        if session_mode == "memory" and request.url.path == "/api/me" and COOKIE in request.cookies:
            # Expire cookies left by older clients, including on unauthorized
            # requests. Authentication still requires an explicit bearer token.
            response.delete_cookie(COOKIE, path="/", httponly=True,
                                   secure=secure_cookie, samesite="lax")
        return response

    @api.get("/healthz")
    def healthz():
        return Response("ok\n", media_type="text/plain")

    def client_ip(request: Request) -> str:
        peer = ipaddress.ip_address(request.client.host)
        if peer in proxies:
            forwarded = request.headers.get("x-real-ip", "")
            try:
                # A proxy must overwrite this single header. Commas and malformed
                # values never become keys in the attempt table.
                return str(ipaddress.ip_address(forwarded))
            except ValueError:
                raise HTTPException(400, "Invalid client address") from None
        return str(peer)

    def limit_auth(request: Request, action: str, maximum: int):
        peer_ip = client_ip(request)
        now = int(time.time())
        with connection() as db:
            db.execute("BEGIN IMMEDIATE")
            db.execute("DELETE FROM auth_attempts WHERE window_start<?",
                       (now - auth_window_seconds,))
            row = db.execute("SELECT attempts FROM auth_attempts WHERE peer_ip=? AND action=?",
                             (peer_ip, action)).fetchone()
            if row and row["attempts"] >= maximum:
                raise HTTPException(429, "Too many attempts")
            if row is None:
                # Bound persistent metadata even under requests from many source IPs.
                count = db.execute("SELECT count(*) FROM auth_attempts").fetchone()[0]
                if count >= 1024:
                    raise HTTPException(429, "Too many attempts")
            db.execute("INSERT INTO auth_attempts VALUES (?,?,?,1) "
                       "ON CONFLICT(peer_ip,action) DO UPDATE SET attempts=attempts+1",
                       (peer_ip, action, now))
            db.commit()

    def check_origin(request: Request):
        if request.headers.get("origin") != public_origin:
            raise HTTPException(403, "Origin denied")

    # Telemetry is independent of accounts and the persistent database. Sliding
    # windows bound both per-peer traffic and total log volume; keys expire in RAM.
    performance_peers = {}
    performance_global = deque()
    performance_lock = Lock()

    @api.post("/api/performance", dependencies=[Depends(check_origin)])
    async def performance(request: Request):
        length = request.headers.get("content-length", "")
        if length.isdigit() and int(length) > 1024:
            raise HTTPException(413, "Request too large")
        data = bytearray()
        async for chunk in request.stream():
            if len(data) + len(chunk) > 1024:
                raise HTTPException(413, "Request too large")
            data.extend(chunk)
        fields = _performance_fields(data)
        try:
            peer = client_ip(request)
        except ValueError:
            raise HTTPException(400, "Invalid client address") from None
        now = time.monotonic()
        with performance_lock:
            for address, accepted in list(performance_peers.items()):
                while accepted and accepted[0] <= now - 60:
                    accepted.popleft()
                if not accepted:
                    del performance_peers[address]
            while performance_global and performance_global[0] <= now - 60:
                performance_global.popleft()
            accepted = performance_peers.get(peer)
            if (len(performance_global) >= 120 or (accepted is not None and len(accepted) >= 3) or
                    (accepted is None and len(performance_peers) >= 1024)):
                raise HTTPException(429, "Too many performance reports")
            if accepted is None:
                accepted = performance_peers[peer] = deque()
            accepted.append(now)
            performance_global.append(now)
        PERFORMANCE_LOG.info(json.dumps({"event": "warblade_performance", **fields},
                                        separators=(",", ":"), allow_nan=False))
        return Response(status_code=204)

    def session_token(request: Request) -> str:
        if session_mode == "cookie" and COOKIE in request.cookies:
            return request.cookies[COOKIE]
        # Cookie-mode rollout also accepts existing memory-only page sessions.
        # New browser logins receive only cookies; a cookie takes precedence.
        # Never fall back to ambient browser cookies in memory-only mode.
        scheme, separator, token = request.headers.get("authorization", "").partition(" ")
        return token if separator and scheme.lower() == "bearer" else ""

    def account(request: Request) -> int:
        token = session_token(request)
        if not TOKEN.fullmatch(token):
            raise HTTPException(401, "Authentication required")
        digest = hashlib.sha256(token.encode("ascii")).digest()
        with connection() as db:
            row = db.execute("SELECT account_id FROM sessions WHERE token_hash=? AND expires_at>?",
                             (digest, int(time.time()))).fetchone()
        if row is None:
            raise HTTPException(401, "Authentication required")
        return row["account_id"]

    def new_session(db: sqlite3.Connection, account_id: int, response: Response):
        token = secrets.token_urlsafe(32)
        db.execute("INSERT INTO sessions VALUES (?,?,?)",
                   (hashlib.sha256(token.encode("ascii")).digest(), account_id,
                    int(time.time()) + SESSION_SECONDS))
        if session_mode == "cookie":
            response.set_cookie(COOKIE, token, max_age=SESSION_SECONDS, httponly=True,
                                secure=secure_cookie, samesite="lax", path="/")
        return token

    async def credentials(request: Request) -> tuple[str, str]:
        if request.headers.get("content-length", "").isdigit() and int(request.headers["content-length"]) > 4096:
            raise HTTPException(413, "Request too large")
        data = await request.body()
        if len(data) > 4096:
            raise HTTPException(413, "Request too large")
        try:
            import json
            fields = json.loads(data)
            username, password = fields["username"], fields["password"]
        except (ValueError, TypeError, KeyError):
            raise HTTPException(400, "Invalid credentials") from None
        if (not isinstance(username, str) or not USERNAME.fullmatch(username) or
                not isinstance(password, str) or not 8 <= len(password) <= 256):
            raise HTTPException(400, "Invalid credentials")
        return username, password

    @api.post("/api/register", dependencies=[Depends(check_origin)])
    async def register(request: Request, response: Response):
        limit_auth(request, "register", register_attempts)
        username, password = await credentials(request)
        with connection() as db:
            if db.execute("SELECT count(*) FROM accounts").fetchone()[0] >= max_accounts:
                raise HTTPException(403, "Registration unavailable")
        password_hash = hasher.hash(password)
        with connection() as db:
            try:
                db.execute("BEGIN IMMEDIATE")
                if db.execute("SELECT count(*) FROM accounts").fetchone()[0] >= max_accounts:
                    raise HTTPException(403, "Registration unavailable")
                cur = db.execute("INSERT INTO accounts(username,password_hash) VALUES (?,?)",
                                 (username, password_hash))
                token = new_session(db, cur.lastrowid, response)
                db.commit()
            except sqlite3.IntegrityError:
                db.rollback()
                raise HTTPException(409, "Username unavailable") from None
        return {"username": username, **({"token": token} if session_mode == "memory" else {})}

    @api.post("/api/login", dependencies=[Depends(check_origin)])
    async def login(request: Request, response: Response):
        limit_auth(request, "login", login_attempts)
        username, password = await credentials(request)
        with connection() as db:
            row = db.execute("SELECT id, username, password_hash FROM accounts WHERE username=?",
                             (username,)).fetchone()
            try:
                hasher.verify(row["password_hash"] if row else DUMMY_HASH, password)
            except (VerifyMismatchError, InvalidHashError):
                raise HTTPException(401, "Invalid username or password") from None
            if row is None:
                raise HTTPException(401, "Invalid username or password")
            db.execute("BEGIN IMMEDIATE")
            token = new_session(db, row["id"], response)
            db.commit()
        return {"username": row["username"], **({"token": token} if session_mode == "memory" else {})}

    @api.post("/api/logout", dependencies=[Depends(check_origin)])
    def logout(request: Request, response: Response):
        if session_mode == "memory":
            account(request)
        token = session_token(request)
        if TOKEN.fullmatch(token):
            with connection() as db:
                db.execute("DELETE FROM sessions WHERE token_hash=?",
                           (hashlib.sha256(token.encode("ascii")).digest(),))
        if session_mode == "cookie":
            response.delete_cookie(COOKIE, path="/")
        return {"ok": True}

    @api.get("/api/me")
    def me(request: Request, response: Response):
        try:
            account_id = account(request)
        except HTTPException as error:
            if session_mode != "cookie" or error.status_code != 401:
                raise
            if COOKIE in request.cookies:
                response.delete_cookie(COOKIE, path="/", httponly=True,
                                       secure=secure_cookie, samesite="lax")
            return {"username": None}
        with connection() as db:
            row = db.execute("SELECT username FROM accounts WHERE id=?", (account_id,)).fetchone()
        return {"username": row["username"]}

    @api.get("/api/saves")
    def list_saves(account_id: int = Depends(account)):
        with connection() as db:
            rows = db.execute("SELECT path, length(data) AS size, version FROM saves "
                              "WHERE account_id=? ORDER BY path", (account_id,)).fetchall()
        return {"files": [dict(row) for row in rows]}

    @api.get("/api/hiscores")
    def get_hiscores(account_id: int = Depends(account)):
        with connection() as db:
            version, data = hiscores.current(db)
        return Response(data, media_type="application/octet-stream",
                        headers={"ETag": _etag(str(version)), "X-Content-Type-Options": "nosniff"})

    @api.post("/api/hiscores", dependencies=[Depends(check_origin)])
    async def post_hiscores(request: Request, if_match: str | None = Header(None),
                            account_id: int = Depends(account)):
        if if_match is None or not re.fullmatch(r'(?:[0-9]{1,12}|"[0-9]{1,12}")', if_match):
            raise HTTPException(428, "High-score revision required")
        base_version = int(if_match.strip('"'))
        if request.headers.get("content-length", "").isdigit() and int(request.headers["content-length"]) > MAX_FILE_BYTES:
            raise HTTPException(413, "High-score file too large")
        data = bytearray()
        async for chunk in request.stream():
            if len(data) + len(chunk) > MAX_FILE_BYTES:
                raise HTTPException(413, "High-score file too large")
            data.extend(chunk)
        try:
            hiscores.unpack(data)
            with connection() as db:
                db.execute("BEGIN IMMEDIATE")
                now = int(time.time())
                rate = db.execute("SELECT window_start,attempts FROM score_attempts WHERE account_id=?",
                                  (account_id,)).fetchone()
                if rate and now - rate["window_start"] < 3600 and rate["attempts"] >= 120:
                    raise HTTPException(429, "Too many high-score submissions")
                if rate and now - rate["window_start"] < 3600:
                    db.execute("UPDATE score_attempts SET attempts=attempts+1 WHERE account_id=?", (account_id,))
                else:
                    db.execute("INSERT INTO score_attempts(account_id,window_start,attempts) VALUES (?,?,1) "
                               "ON CONFLICT(account_id) DO UPDATE SET window_start=excluded.window_start,attempts=1",
                               (account_id, now))
                version, board = hiscores.merge(db, account_id, base_version, data)
                db.commit()
        except ValueError as exc:
            raise HTTPException(400, str(exc)) from None
        except LookupError as exc:
            raise HTTPException(412, str(exc)) from None
        return Response(board, media_type="application/octet-stream",
                        headers={"ETag": _etag(str(version)), "X-Content-Type-Options": "nosniff"})

    @api.get("/api/saves/{path:path}")
    def get_save(path: str, account_id: int = Depends(account)):
        path = _path(path)
        if path == "warblade/warblade_132.his":
            raise HTTPException(404, "Use the shared high-score endpoint")
        with connection() as db:
            row = db.execute("SELECT data, version FROM saves WHERE account_id=? AND path=?",
                             (account_id, path)).fetchone()
        if row is None:
            raise HTTPException(404, "Save not found")
        return Response(row["data"], media_type="application/octet-stream",
                        headers={"ETag": _etag(row["version"]), "X-Content-Type-Options": "nosniff",
                                 "Cache-Control": "no-store"})

    @api.put("/api/saves/{path:path}", dependencies=[Depends(check_origin)])
    async def put_save(path: str, request: Request, if_match: str | None = Header(None),
                       account_id: int = Depends(account)):
        path = _path(path)
        if session_mode == "memory" and re.fullmatch(
                r"warblade/(?:profiles|backup)/profile(?!000\.)[^/]*", path, re.IGNORECASE):
            raise HTTPException(400, "Only one game profile is allowed per account")
        if path == "warblade/warblade_132.his":
            raise HTTPException(400, "Use the shared high-score endpoint")
        if if_match is None:
            raise HTTPException(428, "If-Match required")
        if request.headers.get("content-length", "").isdigit() and int(request.headers["content-length"]) > MAX_FILE_BYTES:
            raise HTTPException(413, "Save too large")
        data = bytearray()
        async for chunk in request.stream():
            if len(data) + len(chunk) > MAX_FILE_BYTES:
                raise HTTPException(413, "Save too large")
            data.extend(chunk)
        version = secrets.token_hex(16)
        with connection() as db:
            db.execute("BEGIN IMMEDIATE")
            row = db.execute("SELECT version, length(data) AS size FROM saves WHERE account_id=? AND path=?",
                             (account_id, path)).fetchone()
            if (row is None and if_match != "*") or (row is not None and not _matches(if_match, row["version"])):
                raise HTTPException(412, "Save revision conflict")
            totals = db.execute("SELECT count(*) AS files, coalesce(sum(length(data)),0) AS bytes "
                                "FROM saves WHERE account_id=?", (account_id,)).fetchone()
            if totals["files"] + (row is None) > MAX_FILES or totals["bytes"] - (row["size"] if row else 0) + len(data) > MAX_ACCOUNT_BYTES:
                raise HTTPException(413, "Save quota exceeded")
            db.execute("INSERT INTO saves(account_id,path,data,version) VALUES (?,?,?,?) "
                       "ON CONFLICT(account_id,path) DO UPDATE SET data=excluded.data,version=excluded.version",
                       (account_id, path, bytes(data), version))
            db.commit()
        return JSONResponse({"version": version}, headers={"ETag": _etag(version), "Cache-Control": "no-store"})

    @api.delete("/api/saves/{path:path}", dependencies=[Depends(check_origin)])
    def delete_save(path: str, if_match: str | None = Header(None), account_id: int = Depends(account)):
        path = _path(path)
        if path == "warblade/warblade_132.his":
            raise HTTPException(400, "Use the shared high-score endpoint")
        if if_match is None:
            raise HTTPException(428, "If-Match required")
        with connection() as db:
            db.execute("BEGIN IMMEDIATE")
            row = db.execute("SELECT version FROM saves WHERE account_id=? AND path=?",
                             (account_id, path)).fetchone()
            if row is None:
                raise HTTPException(404, "Save not found")
            if not _matches(if_match, row["version"]):
                raise HTTPException(412, "Save revision conflict")
            db.execute("DELETE FROM saves WHERE account_id=? AND path=?", (account_id, path))
            db.commit()
        return {"ok": True}

    return api


def _from_environment() -> FastAPI:
    # SQLite creates the DB, WAL and shared-memory files at process startup.
    # Keep them private even if an operator later loosens the parent directory.
    os.umask(0o077)
    return create_app(os.environ.get("WARBLADE_DB_PATH", "/var/lib/warblade/warblade.sqlite3"),
                      os.environ.get("WARBLADE_PUBLIC_ORIGIN", "http://localhost:8080"))


app = _from_environment()

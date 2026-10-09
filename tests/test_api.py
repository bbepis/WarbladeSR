"""Account and private save API behavior, with a temporary persistent SQLite DB."""
import os
import sqlite3
import tempfile
import atexit
import stat
import struct
import zlib
import pytest
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

_import_db_dir = tempfile.TemporaryDirectory(prefix="warblade-api-test-")
atexit.register(_import_db_dir.cleanup)
os.environ.setdefault("WARBLADE_DB_PATH", str(Path(_import_db_dir.name) / "import.sqlite3"))
os.environ.setdefault("WARBLADE_PUBLIC_ORIGIN", "http://localhost:8080")

from fastapi.testclient import TestClient
from server import app as api_module
from server import hiscores

ORIGIN = "http://localhost:8080"
PATH = "/api/saves/warblade/profiles/profile000.acc"


def client(tmp_path, **settings):
    return TestClient(api_module.create_app(tmp_path / "state.sqlite3", ORIGIN, **settings),
                      client=("127.0.0.1", 50000))


def register(c, name):
    res = c.post("/api/register", json={"username": name, "password": "long password 123"},
                 headers={"Origin": ORIGIN})
    assert res.status_code == 200, res.text
    return res


def test_password_length_boundaries(tmp_path):
    c = client(tmp_path)
    headers = {"Origin": ORIGIN}
    for password in ("1234567", "x" * 257):
        for endpoint in ("register", "login"):
            assert c.post(f"/api/{endpoint}", json={"username": "Boundary", "password": password},
                          headers=headers).status_code == 400
    for name, password in (("Minimum", "12345678"), ("Maximum", "x" * 256)):
        fields = {"username": name, "password": password}
        assert c.post("/api/register", json=fields, headers=headers).status_code == 200
        assert c.post("/api/logout", headers=headers).status_code == 200
        assert c.post("/api/login", json=fields, headers=headers).status_code == 200


@pytest.mark.parametrize("session_mode", ["cookie", "memory"])
def test_username_case_variants_share_account_and_saves(tmp_path, session_mode):
    c = client(tmp_path, session_mode=session_mode)
    headers = {"Origin": ORIGIN}
    fields = {"username": "RkJ", "password": "Eight123"}
    registered = c.post("/api/register", json=fields, headers=headers)
    assert registered.status_code == 200
    if session_mode == "memory":
        c.headers["Authorization"] = "Bearer " + registered.json()["token"]
    assert c.put(PATH, content=b"same player progress",
                 headers={**headers, "If-Match": "*"}).status_code == 200
    assert c.post("/api/logout", headers=headers).status_code == 200
    c.headers.pop("Authorization", None)

    for name in ("rkj", "RKJ", "rKj"):
        # Spelling case never creates another identity or redirects its saves.
        assert c.post("/api/register", json={**fields, "username": name},
                      headers=headers).status_code == 409
        assert c.post("/api/login", json={"username": name, "password": "eight123"},
                      headers=headers).status_code == 401
        logged_in = c.post("/api/login", json={**fields, "username": name}, headers=headers)
        assert logged_in.status_code == 200
        assert logged_in.json()["username"] == "RkJ"
        if session_mode == "memory":
            c.headers["Authorization"] = "Bearer " + logged_in.json()["token"]
        assert c.get("/api/me").json() == {"username": "RkJ"}
        assert c.get(PATH).content == b"same player progress"
        assert c.post("/api/logout", headers=headers).status_code == 200
        c.headers.pop("Authorization", None)
    with sqlite3.connect(tmp_path / "state.sqlite3") as db:
        assert db.execute("SELECT count(*) FROM accounts").fetchone()[0] == 1


def test_accounts_sessions_and_isolation(tmp_path):
    assert stat.S_IMODE((Path(_import_db_dir.name) / "import.sqlite3").stat().st_mode) == 0o600
    a = client(tmp_path)
    register(a, "Alice")
    db = tmp_path / "state.sqlite3"
    with sqlite3.connect(db) as con:
        hash_value = con.execute("SELECT password_hash FROM accounts").fetchone()[0]
        token_hash = con.execute("SELECT token_hash FROM sessions").fetchone()[0]
    assert hash_value.startswith("$argon2id$")
    assert "long password 123" not in hash_value
    assert token_hash != a.cookies.get(api_module.COOKIE).encode()
    assert "HttpOnly" in a.post("/api/login", json={"username": "Alice", "password": "long password 123"},
                               headers={"Origin": ORIGIN}).headers["set-cookie"]
    assert a.get("/api/me").json() == {"username": "Alice"}
    assert a.get("/api/me").headers["cache-control"] == "private, no-store"
    assert a.post("/api/register", json={"username": "alice", "password": "long password 123"},
                  headers={"Origin": ORIGIN}).status_code == 409

    created = a.put(PATH, content=b"secret", headers={"Origin": ORIGIN, "If-Match": "*"})
    assert created.status_code == 200
    etag = created.headers["etag"]
    assert a.get(PATH).content == b"secret"
    assert a.get("/api/saves").json()["files"] == [
        {"path": "warblade/profiles/profile000.acc", "size": 6, "version": created.json()["version"]}]
    b = client(tmp_path)
    register(b, "Bob")
    assert b.get("/api/saves").json() == {"files": []}
    assert b.get(PATH).status_code == 404
    assert b.put(PATH, content=b"own", headers={"Origin": ORIGIN, "If-Match": etag}).status_code == 412
    assert b.delete(PATH, headers={"Origin": ORIGIN, "If-Match": etag}).status_code == 404
    assert a.get(PATH).headers["content-type"] == "application/octet-stream"
    assert a.get(PATH).headers["x-content-type-options"] == "nosniff"
    assert a.post("/api/logout", headers={"Origin": ORIGIN}).status_code == 200
    assert a.get("/api/me").json() == {"username": None}
    assert a.get(PATH).status_code == 401
    assert a.get(PATH).headers["cache-control"] == "private, no-store"


def test_cookie_session_reload_expiry_and_logout_revocation(tmp_path):
    c = client(tmp_path)
    anonymous = c.get("/api/me")
    assert anonymous.status_code == 200
    assert anonymous.json() == {"username": None}
    assert anonymous.headers["cache-control"] == "private, no-store"
    registered = register(c, "Remembered")
    assert registered.json() == {"username": "Remembered"}
    cookie_header = registered.headers["set-cookie"]
    assert "HttpOnly" in cookie_header
    assert "SameSite=lax" in cookie_header
    assert "Max-Age=2592000" in cookie_header
    token = c.cookies.get(api_module.COOKIE)
    c.put(PATH, content=b"remembered progress", headers={"Origin": ORIGIN, "If-Match": "*"})
    reloaded = client(tmp_path)
    reloaded.cookies.set(api_module.COOKIE, token)
    assert reloaded.get("/api/me").json() == {"username": "Remembered"}
    assert reloaded.get(PATH).content == b"remembered progress"
    logout = c.post("/api/logout", headers={"Origin": ORIGIN})
    assert logout.status_code == 200
    assert "Max-Age=0" in logout.headers["set-cookie"]
    assert reloaded.get("/api/me").json() == {"username": None}
    assert reloaded.get(PATH).status_code == 401
    login = c.post("/api/login", json={"username": "remembered", "password": "long password 123"},
                   headers={"Origin": ORIGIN})
    assert login.status_code == 200
    with sqlite3.connect(tmp_path / "state.sqlite3") as db:
        db.execute("UPDATE sessions SET expires_at=0")
    assert c.get("/api/me").json() == {"username": None}
    assert c.get(PATH).status_code == 401


def test_https_remembered_cookie_is_secure(tmp_path):
    origin = "https://warblade.test"
    c = TestClient(api_module.create_app(tmp_path / "state.sqlite3", origin), base_url=origin,
                   client=("127.0.0.1", 50000))
    registered = c.post("/api/register", json={"username": "SecurePlayer", "password": "Eight123"},
                        headers={"Origin": origin})
    assert registered.status_code == 200
    assert "Secure" in registered.headers["set-cookie"]
    assert c.get("/api/me").json() == {"username": "SecurePlayer"}


def test_origin_paths_and_limits(tmp_path, monkeypatch):
    c = client(tmp_path)
    assert c.post("/api/register", json={"username": "Alice", "password": "long password 123"}).status_code == 403
    register(c, "Alice")
    assert c.put(PATH, content=b"x", headers={"If-Match": "*"}).status_code == 403
    for path in ("../passwd", "warblade/../passwd", "warblade/.hidden", "warblade/a\\b",
                 "warblade//x", "warblade/evil.svg%00"):
        res = c.put("/api/saves/" + path, content=b"x", headers={"Origin": ORIGIN, "If-Match": "*"})
        assert res.status_code in (400, 404), (path, res.text)
    assert c.put(PATH, content=b"x" * (api_module.MAX_FILE_BYTES + 1),
                 headers={"Origin": ORIGIN, "If-Match": "*"}).status_code == 413
    assert c.put(PATH, content=b"x", headers={"Origin": ORIGIN}).status_code == 428
    monkeypatch.setattr(api_module, "MAX_FILES", 1)
    created = c.put(PATH, content=b"x", headers={"Origin": ORIGIN, "If-Match": "*"})
    assert created.status_code == 200
    assert c.put("/api/saves/warblade/settings.dat", content=b"x",
                 headers={"Origin": ORIGIN, "If-Match": "*"}).status_code == 413
    assert c.delete(PATH, headers={"If-Match": created.headers["etag"]}).status_code == 403


def test_revision_race(tmp_path):
    c = client(tmp_path)
    register(c, "Alice")
    made = c.put(PATH, content=b"start", headers={"Origin": ORIGIN, "If-Match": "*"})
    assert made.status_code == 200
    old = made.headers["etag"]
    cookie = c.cookies.get(api_module.COOKIE)

    def update(value):
        peer = client(tmp_path)
        peer.cookies.set(api_module.COOKIE, cookie)
        return peer.put(PATH, content=value, headers={"Origin": ORIGIN, "If-Match": old})

    with ThreadPoolExecutor(max_workers=2) as pool:
        results = list(pool.map(update, (b"one", b"two")))
    assert sorted(r.status_code for r in results) == [200, 412]
    current = c.get(PATH)
    assert current.content in (b"one", b"two")
    assert current.headers["etag"] != old
    assert c.put(PATH, content=b"clobber", headers={"Origin": ORIGIN, "If-Match": "*"}).status_code == 412
    assert c.delete(PATH, headers={"Origin": ORIGIN, "If-Match": old}).status_code == 412
    assert c.delete(PATH, headers={"Origin": ORIGIN, "If-Match": current.headers["etag"].strip('"')}).status_code == 200
    assert c.get(PATH).status_code == 404


def test_auth_bounds_and_peer_identity(tmp_path):
    c = client(tmp_path, max_accounts=1, login_attempts=2, register_attempts=2)
    assert c.get("/healthz").text == "ok\n"
    assert c.get("/healthz").headers["cache-control"] == "private, no-store"
    register(c, "Alice")
    # A request header cannot choose a fresh rate-limit bucket.
    for value in ("203.0.113.1", "203.0.113.2"):
        r = c.post("/api/login", json={"username": "Alice", "password": "wrong password"},
                   headers={"Origin": ORIGIN, "X-Forwarded-For": value})
        assert r.status_code == 401
    assert c.post("/api/login", json={"username": "Alice", "password": "long password 123"},
                  headers={"Origin": ORIGIN, "X-Forwarded-For": "203.0.113.3"}).status_code == 429
    assert c.post("/api/register", json={"username": "Bob", "password": "long password 123"},
                  headers={"Origin": ORIGIN}).status_code == 403
    assert c.post("/api/register", json={"username": "Carl", "password": "long password 123"},
                  headers={"Origin": ORIGIN}).status_code == 429
    c.cookies.set(api_module.COOKIE, "bad-non-ascii-%CE%A9")
    assert c.get("/api/me").json() == {"username": None}


def test_trusted_proxy_header_is_strict(tmp_path):
    c = client(tmp_path, login_attempts=1, trusted_proxy_ips="127.0.0.1")
    assert c.post("/api/login", json={"username": "Alice", "password": "long password 123"},
                  headers={"Origin": ORIGIN, "X-Real-IP": "203.0.113.5"}).status_code == 401
    assert c.post("/api/login", json={"username": "Alice", "password": "long password 123"},
                  headers={"Origin": ORIGIN, "X-Real-IP": "203.0.113.5"}).status_code == 429
    assert c.post("/api/login", json={"username": "Alice", "password": "long password 123"},
                  headers={"Origin": ORIGIN, "X-Real-IP": "203.0.113.6"}).status_code == 401
    assert c.post("/api/login", json={"username": "Alice", "password": "long password 123"},
                  headers={"Origin": ORIGIN, "X-Real-IP": "203.0.113.7, 203.0.113.8"}).status_code == 400


def test_cookie_rollout_accepts_open_memory_session_but_cookie_identity_wins(tmp_path):
    old = client(tmp_path, session_mode="memory")
    token = register(old, "OldPlayer").json()["token"]
    old.headers["Authorization"] = "Bearer " + token
    assert old.put(PATH, content=b"old tab progress",
                   headers={"Origin": ORIGIN, "If-Match": "*"}).status_code == 200
    updated = client(tmp_path, session_mode="cookie")
    updated.headers["Authorization"] = "Bearer " + token
    assert updated.get("/api/me").json() == {"username": "OldPlayer"}
    assert updated.get(PATH).content == b"old tab progress"
    current = client(tmp_path, session_mode="cookie")
    cookie_login = register(current, "CurrentPlayer")
    assert cookie_login.json() == {"username": "CurrentPlayer"}
    assert current.put(PATH, content=b"current player progress",
                       headers={"Origin": ORIGIN, "If-Match": "*"}).status_code == 200
    updated.cookies.set(api_module.COOKIE, current.cookies.get(api_module.COOKIE))
    assert updated.get("/api/me").json() == {"username": "CurrentPlayer"}
    assert updated.get(PATH).content == b"current player progress"
    # A bad cookie cannot silently fall back to the old account either.
    updated.cookies.clear()
    updated.cookies.set(api_module.COOKIE, "invalid")
    assert updated.get("/api/me").json() == {"username": None}
    assert updated.get(PATH).status_code == 401
    updated.cookies.clear()
    assert updated.post("/api/logout", headers={"Origin": ORIGIN}).status_code == 200
    assert old.get("/api/me").status_code == 401
    assert old.get(PATH).status_code == 401
    assert current.get("/api/me").json() == {"username": "CurrentPlayer"}
    assert current.get(PATH).content == b"current player progress"


def test_memory_sessions_are_explicit_isolated_and_revocable(tmp_path):
    a = client(tmp_path, session_mode="memory")
    registered = register(a, "Alice")
    token = registered.json()["token"]
    assert registered.json()["username"] == "Alice"
    assert "set-cookie" not in registered.headers
    assert not a.cookies
    assert a.get("/api/me").status_code == 401
    a.cookies.set(api_module.COOKIE, token)
    rejected_cookie = a.get("/api/me")
    assert rejected_cookie.status_code == 401
    assert 'warblade_session=""' in rejected_cookie.headers["set-cookie"]
    assert "Max-Age=0" in rejected_cookie.headers["set-cookie"]
    assert token not in rejected_cookie.headers["set-cookie"]
    assert a.post("/api/logout", headers={"Origin": ORIGIN}).status_code == 401
    a.cookies.clear()
    a.headers["Authorization"] = "Bearer " + token
    assert a.get("/api/me").json() == {"username": "Alice"}
    with sqlite3.connect(tmp_path / "state.sqlite3") as con:
        digest = con.execute("SELECT token_hash FROM sessions").fetchone()[0]
    assert digest == api_module.hashlib.sha256(token.encode("ascii")).digest()
    made = a.put(PATH, content=b"alice save", headers={"Origin": ORIGIN, "If-Match": "*"})
    assert made.status_code == 200
    b = client(tmp_path, session_mode="memory")
    b.headers["Authorization"] = "Bearer " + register(b, "Bob").json()["token"]
    assert b.get("/api/saves").json() == {"files": []}
    assert b.get(PATH).status_code == 404
    assert b.delete(PATH, headers={"Origin": ORIGIN, "If-Match": made.headers["etag"]}).status_code == 404
    assert a.put(PATH, content=b"wrong origin", headers={"Origin": "http://other", "If-Match": made.headers["etag"]}).status_code == 403
    logout = a.post("/api/logout", headers={"Origin": ORIGIN})
    assert logout.status_code == 200
    assert "set-cookie" not in logout.headers
    assert a.get(PATH).status_code == 401
    assert a.get("/api/me").status_code == 401
    assert b.get("/api/me").status_code == 200
    login = a.post("/api/login", json={"username": "Alice", "password": "long password 123"},
                   headers={"Origin": ORIGIN})
    assert login.status_code == 200
    assert "set-cookie" not in login.headers
    assert login.json()["token"] != token
    a.headers["Authorization"] = "Bearer " + login.json()["token"]
    assert a.get(PATH).content == b"alice save"


def test_memory_mode_keeps_legacy_saves_but_prevents_extra_profile_writes(tmp_path, monkeypatch):
    legacy = client(tmp_path)
    register(legacy, "Alice")
    old_path = "/api/saves/warblade/profiles/profile001.acc"
    old = legacy.put(old_path, content=b"existing profile", headers={"Origin": ORIGIN, "If-Match": "*"})
    assert old.status_code == 200
    monkeypatch.setenv("WARBLADE_SESSION_MODE", "memory")
    c = client(tmp_path)
    login = c.post("/api/login", json={"username": "Alice", "password": "long password 123"},
                   headers={"Origin": ORIGIN})
    c.headers["Authorization"] = "Bearer " + login.json()["token"]
    assert c.get(old_path).content == b"existing profile"
    assert c.get("/api/saves").json()["files"][0]["path"] == "warblade/profiles/profile001.acc"
    assert c.put(old_path, content=b"overwrite", headers={"Origin": ORIGIN, "If-Match": old.headers["etag"]}).status_code == 400
    for folder in ("profiles", "backup"):
        for extension in ("acc", "wpl", "svg"):
            allowed = f"/api/saves/warblade/{folder}/profile000.{extension}"
            assert c.put(allowed, content=b"single profile", headers={"Origin": ORIGIN, "If-Match": "*"}).status_code == 200
            for slot in ("001", "002", "999"):
                denied = f"/api/saves/warblade/{folder}/profile{slot}.{extension}"
                assert c.put(denied, content=b"extra profile", headers={"Origin": ORIGIN, "If-Match": "*"}).status_code == 400
    assert c.put("/api/saves/warblade/settings.dat", content=b"settings", headers={"Origin": ORIGIN, "If-Match": "*"}).status_code == 200
    assert c.get(old_path).content == b"existing profile"


def test_memory_mode_preserves_auth_limits_and_validates_mode(tmp_path):
    c = client(tmp_path, session_mode="memory", login_attempts=1)
    register(c, "Alice")
    fields = {"username": "Alice", "password": "wrong password"}
    assert c.post("/api/login", json=fields, headers={"Origin": ORIGIN}).status_code == 401
    fields["password"] = "long password 123"
    assert c.post("/api/login", json=fields, headers={"Origin": ORIGIN}).status_code == 429
    import pytest
    with pytest.raises(ValueError, match="WARBLADE_SESSION_MODE"):
        client(tmp_path, session_mode="local-storage")


def _candidate(base, name, score):
    raw = bytearray(hiscores.unpack(base))
    offset = hiscores.TABLE_OFFSETS[0]
    raw[offset:offset + len(name)] = name.encode()
    struct.pack_into("<q", raw, offset + hiscores.SCORE_OFFSET, score)
    return zlib.compress(raw)


def test_concurrent_high_scores_merge_without_overwrite(tmp_path):
    a = client(tmp_path)
    register(a, "Alice")
    b = client(tmp_path)
    register(b, "Bob")
    base = a.get("/api/hiscores")
    assert base.status_code == 200
    assert base.headers["etag"] == '"0"'

    def submit(name, score, cookie):
        peer = client(tmp_path)
        peer.cookies.set(api_module.COOKIE, cookie)
        return peer.post("/api/hiscores", content=_candidate(base.content, name, score),
                         headers={"Origin": ORIGIN, "If-Match": base.headers["etag"]})

    with ThreadPoolExecutor(max_workers=2) as pool:
        results = list(pool.map(lambda args: submit(*args), (
            ("Alice", 1500, a.cookies.get(api_module.COOKIE)),
            ("Bob", 2000, b.cookies.get(api_module.COOKIE)),
        )))
    assert [r.status_code for r in results] == [200, 200], [r.text for r in results]
    rows = list(hiscores.entries(a.get("/api/hiscores").content))
    assert [(r[2][:5].rstrip(b"\0"), r[1]) for r in rows] == [(b"Bob", 2000), (b"Alice", 1500)]
    assert a.get("/api/saves").json()["files"] == []
    assert a.put("/api/saves/warblade/warblade_132.his", content=base.content,
                 headers={"Origin": ORIGIN, "If-Match": "*"}).status_code == 400
    # A retry of the same candidate is idempotent; the board stays at version 2.
    assert submit("Alice", 1500, a.cookies.get(api_module.COOKIE)).headers["etag"] == '"2"'


def performance_report(**changes):
    return {"version": 1, "device": "mobile", "browser": "safari", "phase": "play",
            "frames": 600, "duration_ms": 10000, "fps": 60, "mean_ms": 16.67,
            "p95_ms": 20, "max_ms": 40, "over_33_pct": 1, "target_fps": 60,
            **changes}


def test_guest_performance_logs_only_allowlist_without_database_writes(tmp_path, caplog, monkeypatch):
    import json
    import logging
    c = client(tmp_path, session_mode="memory")
    with sqlite3.connect(tmp_path / "state.sqlite3") as db:
        before = list(db.iterdump())
    # A guest report must not open the database or look up the provided token.
    monkeypatch.setattr(api_module.sqlite3, "connect", lambda *args, **kwargs:
                        pytest.fail("Performance telemetry accessed the database"))
    with caplog.at_level(logging.INFO, logger="uvicorn.error"):
        response = c.post("/api/performance", json=performance_report(),
                          headers={"Origin": ORIGIN, "Authorization": "Bearer private-token",
                                   "User-Agent": "private-device", "X-Forwarded-For": "203.0.113.4"})
    assert response.status_code == 204
    assert not response.content
    assert not response.cookies
    assert response.headers["cache-control"] == "private, no-store"
    records = [r.getMessage() for r in caplog.records if r.name == "uvicorn.error"]
    assert len(records) == 1
    assert json.loads(records[0]) == {"event": "warblade_performance", **performance_report()}
    assert all(value not in records[0] for value in ("private-token", "private-device", "203.0.113.4", "127.0.0.1"))
    monkeypatch.undo()
    with sqlite3.connect(tmp_path / "state.sqlite3") as db:
        assert list(db.iterdump()) == before


@pytest.mark.parametrize("changes", [
    {"username": "secret"}, {"version": True}, {"version": 2}, {"frames": True},
    {"frames": 0}, {"frames": 20001}, {"frames": 600.0}, {"fps": "60"},
    {"fps": False}, {"fps": float("nan")}, {"fps": float("inf")},
    {"fps": -1}, {"fps": 301}, {"fps": 59}, {"mean_ms": 17},
    {"duration_ms": 999}, {"duration_ms": 120001}, {"device": "phone model"},
    {"browser": "Chrome/123"}, {"phase": "account-name"}, {"phase": []},
    {"target_fps": 0}, {"target_fps": True}, {"max_ms": 10}, {"p95_ms": 41},
    {"over_33_pct": 101}, {"over_33_pct": -1}, {"mean_ms": 10**1000},
])
def test_performance_rejects_unbounded_or_identifying_fields(tmp_path, changes, caplog):
    import json
    c = client(tmp_path)
    # Standard JSON encoder deliberately exercises the server's NaN/Infinity guard.
    response = c.post("/api/performance", content=json.dumps(performance_report(**changes)),
                      headers={"Origin": ORIGIN, "Content-Type": "application/json"})
    assert response.status_code in (400, 413)
    assert not any(r.name == "uvicorn.error" for r in caplog.records)


def test_performance_origin_json_and_stream_body_bounds(tmp_path):
    import json
    c = client(tmp_path)
    for origin in (None, "https://different.example"):
        assert c.post("/api/performance", json=performance_report(),
                      headers={} if origin is None else {"Origin": origin}).status_code == 403
    for data in (b"[]", b"null", b"invalid", b"{}", b"\xff",
                 json.dumps(performance_report()).replace('"version": 1', '"version": 1, "version": 1').encode()):
        assert c.post("/api/performance", content=data, headers={"Origin": ORIGIN}).status_code == 400
    assert c.post("/api/performance", content=b"x" * 1025,
                  headers={"Origin": ORIGIN}).status_code == 413
    # No Content-Length: the chunked stream must be stopped at the same bound.
    assert c.post("/api/performance", content=iter((b"x" * 600, b"x" * 600)),
                  headers={"Origin": ORIGIN}).status_code == 413


def test_performance_sliding_rate_limits_proxy_identity_and_expiry(tmp_path, monkeypatch):
    c = client(tmp_path, trusted_proxy_ips="127.0.0.1")
    now = [1000.0]
    monkeypatch.setattr(api_module.time, "monotonic", lambda: now[0])
    def post(address):
        return c.post("/api/performance", json=performance_report(),
                      headers={"Origin": ORIGIN, "X-Real-IP": address})
    assert post("203.0.113.1, 203.0.113.2").status_code == 400
    for _ in range(3):
        assert post("203.0.113.1").status_code == 204
    assert post("203.0.113.1").status_code == 429
    for index in range(2, 41):
        for _ in range(3):
            assert post(f"203.0.113.{index}").status_code == 204
    assert post("203.0.113.41").status_code == 429
    now[0] += 59
    assert post("203.0.113.1").status_code == 429
    now[0] += 1
    assert post("203.0.113.1").status_code == 204
    assert post("203.0.113.41").status_code == 204


def test_performance_untrusted_forwarding_cannot_evade_peer_limit(tmp_path):
    c = client(tmp_path)
    for index in range(4):
        response = c.post("/api/performance", json=performance_report(),
                          headers={"Origin": ORIGIN, "X-Real-IP": f"203.0.113.{index}"})
        assert response.status_code == (204 if index < 3 else 429)

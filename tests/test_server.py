"""HTTP tests for the Docker image's server (docker/nginx.conf; tests/README.md).

Run by tests/run.sh against running containers; standard library only. Settings come from
the environment:
  WARBLADE_URL         a server with the real web build, the test data (tests/run.sh) at /data
                       and a legacy save folder mounted at /saves, as the old image had
  WARBLADE_LIMITS_URL  another such server, for the tests that exhaust its rate limits
  WARBLADE_FIXTURES    the folder holding that data/ and saves/ on this machine
  WARBLADE_CONTAINER   the WARBLADE_URL container's name, for the checks run inside it
  WARBLADE_IMAGE       the image, for the tests that start their own containers
"""
import hashlib
import http.client
import json
import os
import socket
import subprocess
import threading
import time
import unittest
from urllib.parse import urlsplit

URL = os.environ.get("WARBLADE_URL", "http://127.0.0.1:18081")
LIMITS_URL = os.environ.get("WARBLADE_LIMITS_URL")
FIXTURES = os.environ.get("WARBLADE_FIXTURES")
CONTAINER = os.environ.get("WARBLADE_CONTAINER")
IMAGE = os.environ.get("WARBLADE_IMAGE")

HOST = urlsplit(URL).hostname
PORT = urlsplit(URL).port

LEGACY_SAVE = "warblade/profiles/profile000.acc"   # in the fixtures' saves/ folder
WRITE_METHODS = ["PUT", "DELETE", "POST", "PATCH", "OPTIONS", "PROPFIND", "PROPPATCH", "MKCOL",
                 "COPY", "MOVE", "LOCK", "UNLOCK", "TRACE"]


def request(method, path, body=None, headers=None, url=URL):
    parts = urlsplit(url)
    conn = http.client.HTTPConnection(parts.hostname, parts.port, timeout=15)
    try:
        conn.request(method, path, body=body, headers=headers or {})
        res = conn.getresponse()
        return res.status, {k.lower(): v for k, v in res.getheaders()}, res.read()
    finally:
        conn.close()


def raw(data, host=HOST, port=PORT, wait=5):
    """Sends raw bytes and returns what the server answers before closing (or `wait` s)."""
    with socket.create_connection((host, port), timeout=wait) as s:
        s.sendall(data)
        out = b""
        try:
            while True:
                chunk = s.recv(65536)
                if not chunk:
                    break
                out += chunk
        except socket.timeout:
            pass
        return out


def status_of(response):
    try:
        return int(response.split(b" ", 2)[1])
    except (IndexError, ValueError):
        return None


def tree_digest(root):
    """A digest of every file (and link) under root, to show nothing changed."""
    h = hashlib.sha256()
    for folder, dirs, names in sorted(os.walk(root)):
        dirs.sort()
        for name in sorted(names):
            p = os.path.join(folder, name)
            h.update(os.path.relpath(p, root).encode())
            if os.path.islink(p):
                h.update(b"->" + os.readlink(p).encode())
            else:
                with open(p, "rb") as f:
                    h.update(hashlib.sha256(f.read()).digest())
    return h.hexdigest()


class Saves(unittest.TestCase):
    """An anonymous visitor can't list, read, overwrite or delete anyone's saves: the server
    has none to offer and accepts no writes. The legacy /saves volume mounted into the
    container (as the old image had it) stays unreachable and unchanged."""

    def test_no_save_listing(self):
        for path in ("/api/saves", "/api/saves/", "/saves", "/saves/", "/saves/warblade/",
                     "/saves/warblade/profiles/", "/api/data"):
            status, _, body = request("GET", path)
            self.assertEqual(status, 404, path)
            self.assertNotIn(b"profile000", body, path)

    def test_cannot_read_saves(self):
        for path in ("/saves/" + LEGACY_SAVE, "/save/" + LEGACY_SAVE, "/" + LEGACY_SAVE,
                     "/data/../saves/" + LEGACY_SAVE, "/data/%2e%2e/saves/" + LEGACY_SAVE,
                     "/data/..%2fsaves%2f" + LEGACY_SAVE.replace("/", "%2f"),
                     "/data/../../saves/" + LEGACY_SAVE, "/../saves/" + LEGACY_SAVE):
            status, _, body = request("GET", path)
            self.assertIn(status, (400, 404), path)
            self.assertNotIn(b"hunter2", body, path)

    def test_cannot_overwrite_or_delete_saves(self):
        before = tree_digest(os.path.join(FIXTURES, "saves")) if FIXTURES else None
        targets = ["/saves/" + LEGACY_SAVE, "/saves/new.svg", "/api/saves", "/data/warblade.pac",
                   "/data/new.bin", "/", "/index.html", "/warblade.wasm", "/x"]
        for path in targets:
            for method in WRITE_METHODS:
                status, headers, _ = request(method, path, body=b"pwned")
                self.assertIn(status, (405, 413), "%s %s" % (method, path))
                if status == 405:
                    self.assertIn("content-security-policy", headers)
        # Nothing appeared...
        for path in ("/saves/new.svg", "/data/new.bin", "/x"):
            self.assertEqual(request("GET", path)[0], 404, path)
        # ...and nothing changed, on the server or behind it.
        self.assertNotEqual(request("GET", "/data/warblade.pac")[2], b"pwned")
        if before:
            self.assertEqual(tree_digest(os.path.join(FIXTURES, "saves")), before)

    def test_chunked_upload_refused(self):
        resp = raw(b"PUT /saves/a HTTP/1.1\r\nHost: x\r\nTransfer-Encoding: chunked\r\n\r\n"
                   b"5\r\npwned\r\n0\r\n\r\n")
        self.assertIn(status_of(resp), (405, 413))

    @unittest.skipUnless(CONTAINER, "needs WARBLADE_CONTAINER")
    def test_server_cannot_write_its_files(self):
        # nginx's user can't change the site or the data, even with a bug in the server.
        script = ("for p in /app/web/x /app/web/index.html /data/x /data/warblade.pac /saves/x "
                  "/etc/warblade/nginx.conf; do (echo x >> $p) 2>/dev/null && echo WROTE $p; done; "
                  "id -u")
        out = subprocess.run(["docker", "exec", CONTAINER, "sh", "-c", script],
                             capture_output=True, text=True, check=True).stdout
        self.assertNotIn("WROTE", out)
        self.assertEqual(out.strip().splitlines()[-1], "101")


class Delivery(unittest.TestCase):
    """The page, the game and the data are served, read-only, with the security headers."""

    def test_page_and_game(self):
        for path, ctype in (("/", "text/html"), ("/index.html", "text/html"),
                            ("/page.js", "application/javascript"), ("/page.css", "text/css"),
                            ("/warblade.js", "application/javascript"),
                            ("/warblade.wasm", "application/wasm")):
            status, headers, body = request("GET", path)
            self.assertEqual(status, 200, path)
            self.assertTrue(headers["content-type"].startswith(ctype), (path, headers["content-type"]))
            self.assertEqual(headers.get("cache-control"), "no-store", path)
            self.assertIn("etag", headers, path)
            self.assertTrue(body, path)
        _, _, page = request("GET", "/")
        self.assertNotIn(b"<script>", page)          # no inline code: the CSP forbids it
        self.assertNotIn(b"<style>", page)
        self.assertIn(b"/api/saves", request("GET", "/page.js")[2])

    def test_revalidation(self):
        _, headers, _ = request("GET", "/warblade.js")
        status, _, body = request("GET", "/warblade.js", headers={"If-None-Match": headers["etag"]})
        self.assertEqual((status, body), (304, b""))

    def test_head(self):
        status, headers, body = request("HEAD", "/warblade.wasm")
        self.assertEqual(status, 200)
        self.assertEqual(body, b"")
        self.assertGreater(int(headers["content-length"]), 0)

    def test_static_page_preserves_remembered_session(self):
        for path in ("/", "/?startup=1", "/index.html", "/index.html?startup=1"):
            status, headers, _ = request("GET", path)
            self.assertEqual(status, 200, path)
            self.assertNotIn("set-cookie", headers, path)
        _, headers, _ = request("GET", "/page.js")
        self.assertNotIn("set-cookie", headers)

    def test_security_headers(self):
        for path in ("/", "/warblade.wasm", "/data/", "/missing", "/data/missing"):
            _, headers, _ = request("GET", path)
            csp = headers.get("content-security-policy", "")
            self.assertIn("default-src 'none'", csp, path)
            self.assertIn("script-src 'self' 'wasm-unsafe-eval'", csp, path)
            self.assertIn("frame-ancestors 'none'", csp, path)
            self.assertNotIn("unsafe-inline", csp, path)
            self.assertEqual(headers.get("x-content-type-options"), "nosniff", path)
            self.assertEqual(headers.get("x-frame-options"), "DENY", path)
            self.assertEqual(headers.get("referrer-policy"), "no-referrer", path)
            self.assertEqual(headers.get("cross-origin-opener-policy"), "same-origin", path)
            self.assertIn("camera=()", headers.get("permissions-policy", ""), path)
            self.assertNotIn("nginx/", headers.get("server", ""), path)   # no version

    def test_data_listing(self):
        status, headers, body = request("GET", "/data/")
        self.assertEqual(status, 200)
        self.assertTrue(headers["content-type"].startswith("application/json"))
        names = {e["name"]: e["type"] for e in json.loads(body)}
        self.assertEqual(names.get("warblade.pac"), "file")
        self.assertEqual(names.get("music"), "directory")
        self.assertNotIn(".secret", names)                       # no dot files
        sub = json.loads(request("GET", "/data/music/")[2])
        self.assertIn("title.xm", {e["name"] for e in sub})

    def test_data_files_are_plain_bytes(self):
        status, headers, body = request("GET", "/data/warblade.pac")
        self.assertEqual((status, body), (200, b"FAKE-PAC\n"))
        # Nothing in the data folder can run as a page on this site.
        for path in ("/data/evil.html", "/data/evil.svg", "/data/music/title.xm"):
            status, headers, _ = request("GET", path)
            self.assertEqual(status, 200, path)
            self.assertEqual(headers["content-type"], "application/octet-stream", path)

    def test_name_with_space(self):
        self.assertEqual(request("GET", "/data/music/with%20space.xm")[0], 200)

    def test_healthz(self):
        self.assertEqual(request("GET", "/healthz")[:1], (200,))


class Paths(unittest.TestCase):
    """Malformed and hostile paths get an error and never anything outside the site."""

    SECRETS = (b"root:", b"hunter2", b"nginx.conf", b"worker_processes", b"SECRET")

    def check(self, path, allowed=(400, 403, 404)):
        resp = raw(b"GET " + path + b" HTTP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n")
        self.assertIn(status_of(resp), allowed, path)
        for secret in self.SECRETS:
            self.assertNotIn(secret, resp, path)

    def test_traversal(self):
        for path in (b"/../../../../etc/passwd", b"/data/../../../../etc/passwd",
                     b"/data/../etc/passwd", b"/data/..%2f..%2f..%2fetc%2fpasswd",
                     b"/data/%2e%2e/%2e%2e/%2e%2e/etc/passwd", b"/%2e%2e/%2e%2e/etc/passwd",
                     b"/data/%252e%252e/%252e%252e/etc/passwd", b"/data//..//..//..//etc/passwd",
                     b"/data/..\\..\\..\\etc\\passwd", b"/data/..%5c..%5c..%5cetc%5cpasswd",
                     b"/data../etc/passwd", b"/data%2f..%2f..%2fetc%2fpasswd",
                     b"/etc/warblade/nginx.conf", b"/data/../../etc/warblade/nginx.conf",
                     b"/../../proc/self/environ", b"/data/../../tmp/warblade/listen.conf"):
            self.check(path)

    def test_dot_files_and_links(self):
        for path in (b"/data/.secret", b"/data/music/.hidden", b"/.secret", b"/data/%2esecret",
                     b"/data/link-to-passwd", b"/data/link-dir/passwd"):
            self.check(path)

    def test_malformed(self):
        for path in (b"/data/warblade.pac%00.html", b"/%00", b"/data/%", b"/data/%zz",
                     b"/data/\xff\xfe", b"*", b"data/warblade.pac", b"//etc/passwd"):
            self.check(path)

    def test_bad_requests(self):
        for req in (b"GARBAGE\r\n\r\n", b"GET / HTTP/9.9\r\nHost: x\r\n\r\n",
                    b"GET / HTTP/1.1\r\nHost: x\r\nHost: y\r\n\r\n",
                    b"GET / HTTP/1.1\r\nHost: x\r\nContent-Length: 5\r\nContent-Length: 6\r\n\r\nabcde",
                    b"GET / HTTP/1.1\r\nHost: x\r\nTransfer-Encoding: chunked\r\nContent-Length: 3\r\n\r\n0\r\n\r\n",
                    b"GET /\x00 HTTP/1.1\r\nHost: x\r\n\r\n"):
            resp = raw(req)
            self.assertIn(status_of(resp), (400, 405, 413, 414, 505), req)


class Limits(unittest.TestCase):
    """Request size, timeouts and request rates are bounded."""

    def test_body_too_large(self):
        status, _, _ = request("GET", "/", body=b"x" * 4096)
        self.assertEqual(status, 413)

    def test_header_too_large(self):
        status, _, _ = request("GET", "/", headers={"X-Big": "x" * 10000})
        self.assertIn(status, (400, 431, 494))

    def test_uri_too_long(self):
        self.assertIn(status_of(raw(b"GET /" + b"a" * 10000 + b" HTTP/1.1\r\nHost: x\r\n\r\n")),
                      (400, 414))

    def test_too_many_headers(self):
        headers = b"".join(b"X-H%d: %s\r\n" % (i, b"y" * 4000) for i in range(10))
        self.assertIn(status_of(raw(b"GET / HTTP/1.1\r\nHost: x\r\n" + headers + b"\r\n")),
                      (400, 431, 494))

    def test_slow_header_is_dropped(self):
        # A client that never finishes its request (slowloris) is cut off after the 10 s
        # header timeout instead of holding the connection.
        start = time.monotonic()
        with socket.create_connection((HOST, PORT), timeout=30) as s:
            s.sendall(b"GET / HTTP/1.1\r\nHost: x\r\n")
            data = s.recv(1024)          # returns when the server closes it
        elapsed = time.monotonic() - start
        self.assertLess(elapsed, 20)
        self.assertGreaterEqual(elapsed, 8)
        self.assertEqual(data, b"")      # closed without serving anything

    def test_idle_keepalive_is_dropped(self):
        start = time.monotonic()
        with socket.create_connection((HOST, PORT), timeout=60) as s:
            s.sendall(b"GET /healthz HTTP/1.1\r\nHost: x\r\n\r\n")
            buf = b""
            while b"ok\n" not in buf:
                buf += s.recv(1024)
            while s.recv(1024):
                pass
        self.assertLess(time.monotonic() - start, 45)


@unittest.skipUnless(IMAGE and FIXTURES, "needs WARBLADE_IMAGE and WARBLADE_FIXTURES")
class Entrypoint(unittest.TestCase):
    """docker/entrypoint.sh: refuses bad settings, trusts only the configured proxies and
    finds the data in a mounted install."""

    def run_image(self, *args, port=None):
        name = "warblade-entrypoint-%d-%d" % (os.getpid(), time.monotonic_ns())
        cmd = ["docker", "run", "-d", "--name", name, "--read-only", "--tmpfs", "/tmp",
               "--cap-drop", "ALL"]
        if port:
            cmd += ["-p", "127.0.0.1:%d:8080" % port]
        subprocess.run(cmd + list(args) + [IMAGE], check=True, capture_output=True)
        self.addCleanup(subprocess.run, ["docker", "rm", "-f", name], capture_output=True)
        return name

    def wait_exit(self, name):
        out = subprocess.run(["docker", "wait", name], capture_output=True, text=True, timeout=30)
        logs = subprocess.run(["docker", "logs", name], capture_output=True, text=True)
        return int(out.stdout.strip()), logs.stdout + logs.stderr

    def wait_up(self, port):
        for _ in range(100):
            try:
                if request("GET", "/healthz", url="http://127.0.0.1:%d" % port)[0] == 200:
                    return
            except OSError:
                pass
            time.sleep(0.1)
        self.fail("server did not start")

    def test_bad_settings_stop_it(self):
        for env, message in (("PORT=80; evil", "PORT must be a number"),
                             ("WARBLADE_TRUSTED_PROXIES=10.0.0.0/8 x;include", "not an address")):
            code, logs = self.wait_exit(self.run_image("-e", env))
            self.assertNotEqual(code, 0, env)
            self.assertIn(message, logs, env)

    def test_trusted_proxy(self):
        # Trusted: the client address comes from X-Forwarded-For. Not trusted: it's ignored.
        for proxies, expected in (("0.0.0.0/0", "203.0.113.9"), ("192.0.2.1", None)):
            port = 18190 if expected else 18191
            name = self.run_image("-e", "WARBLADE_TRUSTED_PROXIES=" + proxies, port=port)
            self.wait_up(port)
            request("GET", "/page.css", headers={"X-Forwarded-For": "203.0.113.9"},
                    url="http://127.0.0.1:%d" % port)
            time.sleep(0.5)
            logs = subprocess.run(["docker", "logs", name], capture_output=True, text=True).stdout
            line = [l for l in logs.splitlines() if "/page.css" in l][-1]
            if expected:
                self.assertTrue(line.startswith(expected + " "), line)
            else:
                self.assertFalse(line.startswith("203.0.113.9"), line)

    def test_whole_install_mounted(self):
        # The install folder, with its data in Data/ (Windows names): found in any case.
        install = os.path.join(FIXTURES, "install")
        os.makedirs(os.path.join(install, "Data"), exist_ok=True)
        with open(os.path.join(install, "Data", "WARBLADE.PAC"), "wb") as f:
            f.write(b"INSTALL-PAC\n")
        os.chmod(install, 0o755)
        os.chmod(os.path.join(install, "Data"), 0o755)
        os.chmod(os.path.join(install, "Data", "WARBLADE.PAC"), 0o644)
        self.run_image("-v", install + ":/data:ro", port=18192)
        self.wait_up(18192)
        names = [e["name"] for e in json.loads(request("GET", "/data/", url="http://127.0.0.1:18192")[2])]
        self.assertEqual(names, ["WARBLADE.PAC"])


@unittest.skipUnless(LIMITS_URL, "needs WARBLADE_LIMITS_URL")
class Flood(unittest.TestCase):
    """A client sending too much gets 429 once past its budget (a burst big enough to load the
    game), and a client holding too many downloads at once gets 429 for the extra ones."""

    def test_request_rate(self):
        time.sleep(3)  # refill budget after readiness probes / preceding tests
        parts = urlsplit(LIMITS_URL)
        statuses = []
        lock = threading.Lock()

        def worker():
            conn = http.client.HTTPConnection(parts.hostname, parts.port, timeout=15)
            for _ in range(150):
                conn.request("GET", "/page.css")
                res = conn.getresponse()
                res.read()
                with lock:
                    statuses.append(res.status)
                if res.will_close:
                    conn.close()
                    conn = http.client.HTTPConnection(parts.hostname, parts.port, timeout=15)
            conn.close()

        threads = [threading.Thread(target=worker) for _ in range(6)]
        for t in threads:
            t.start()
        for t in threads:
            t.join()
        ok, limited = statuses.count(200), statuses.count(429)
        self.assertEqual(ok + limited, 900)
        self.assertGreaterEqual(ok, 400)       # the burst a game load needs
        self.assertGreater(limited, 300)       # and then it stops
        time.sleep(3)                          # the budget refills at 20 requests a second
        self.assertEqual(request("GET", "/page.css", url=LIMITS_URL)[0], 200)

    def test_concurrent_downloads(self):
        parts = urlsplit(LIMITS_URL)
        time.sleep(3)
        socks = []
        try:
            for _ in range(40):
                s = socket.socket()
                s.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 4096)
                s.connect((parts.hostname, parts.port))
                s.sendall(b"GET /data/big.bin HTTP/1.1\r\nHost: x\r\n\r\n")
                socks.append(s)
                time.sleep(0.02)
            time.sleep(1)
            statuses = []
            for s in socks:
                s.settimeout(5)
                statuses.append(status_of(s.recv(64)))
        finally:
            for s in socks:
                s.close()
        self.assertEqual(statuses.count(200), 32)
        self.assertEqual(statuses.count(429), 8)


if __name__ == "__main__":
    unittest.main()

#!/bin/sh
# Tests for the Docker image (tests/README.md): ./tests/run.sh
#
# Builds the image and the stand-in game (tests/stub), makes test data and a save folder like
# the old image's /saves volume, starts the image in a few containers and runs the HTTP tests
# (tests/test_server.py) and the browser tests (tests/browser.test.cjs) against them.
#
# Needs Docker, Python 3, Node.js and Playwright with Chromium (npm ci in tests/, then
# npx playwright install chromium; a global install works too).
#   IMAGE      an image to test instead of building one from the repository
#   STUB_DIR   a folder with the stand-in game's warblade.js/.wasm instead of building it
#   KEEP=1     leave the containers running afterwards
set -eu

root=$(cd "$(dirname "$0")/.." && pwd)
work=$(mktemp -d)
fx="$work/fixtures"
name=warblade-test-$$
containers=
network=

cleanup() {
    if [ "${KEEP:-}" != 1 ]; then
        for c in $containers; do docker rm -f "$c" > /dev/null 2>&1 || true; done
        if [ -n "$network" ]; then docker network rm "$network" > /dev/null 2>&1 || true; fi
        rm -rf "$work"
    else
        echo "kept: $containers; fixtures in $fx"
    fi
}
trap cleanup EXIT INT TERM

image=${IMAGE:-}
if [ -z "$image" ]; then
    image=warblade-sr:test
    docker build -t "$image" "$root"
fi
api_image=${API_IMAGE:-warblade-sr-api:test}
if ! docker image inspect "$api_image" > /dev/null 2>&1; then
    docker build -f "$root/Dockerfile.api" -t "$api_image" "$root"
fi

if [ -z "${STUB_DIR:-}" ]; then
    STUB_DIR="$work/stub"
    docker build --output "type=local,dest=$STUB_DIR" "$root/tests/stub"
fi

# ---- fixtures ----
umask 022
mkdir -p "$fx/data/music" "$fx/localdata/music" "$fx/saves/warblade/profiles" "$fx/web-stub"
mkdir -p "$fx/api-state"
chmod 777 "$fx/api-state"
printf 'FAKE-PAC\n' > "$fx/data/warblade.pac"
printf 'music' > "$fx/data/music/title.xm"
printf 'music' > "$fx/data/music/with space.xm"
# The browser tests' data; the server tests' adds dot files, links out and pages.
cp -a "$fx/data" "$fx/data-clean"
printf 'SECRET' > "$fx/data/.secret"
printf 'SECRET' > "$fx/data/music/.hidden"
printf '<script>alert(1)</script>' > "$fx/data/evil.html"
printf '<svg xmlns="http://www.w3.org/2000/svg"><script>alert(1)</script></svg>' > "$fx/data/evil.svg"
ln -s /etc/passwd "$fx/data/link-to-passwd"
ln -s /etc "$fx/data/link-dir"
cp -a "$fx/data" "$fx/data-big"
head -c 33554432 /dev/zero > "$fx/data-big/big.bin"
printf 'LOCAL-PAC\n' > "$fx/localdata/warblade.pac"
printf 'music' > "$fx/localdata/music/title.xm"

python3 - "$fx" <<'EOF'
import io, os, sys, tarfile, zlib
fx = sys.argv[1]
# A profile as the game writes it (src/profile/account.c, SaveAccount): zlib-compressed
# struct Account, its password in plain text at +0x25 (include/types.h).
acc = bytearray(0x40d0)
acc[0:6] = b"WARBP1"
acc[7:12] = b"alice"
acc[0x25:0x2c] = b"hunter2"
packed = zlib.compress(bytes(acc))
with open(os.path.join(fx, "saves/warblade/profiles/profile000.acc"), "wb") as f:
    f.write(packed + bytes(0x40d0 - len(packed)))
with open(os.path.join(fx, "saves/warblade/WarBlade.inf"), "wb") as f:
    f.write(b"settings\n")

with tarfile.open(os.path.join(fx, "hostile.tar"), "w", format=tarfile.PAX_FORMAT) as t:
    def add(name, data=b"evil", kind=tarfile.REGTYPE, link=""):
        info = tarfile.TarInfo(name)
        info.type, info.linkname, info.size = kind, link, len(data) if kind == tarfile.REGTYPE else 0
        t.addfile(info, io.BytesIO(data) if kind == tarfile.REGTYPE else None)
    add("../../evil1")
    add("/etc/evil2")
    add("other/evil3")
    add("warblade/../../evil4")
    add("warblade\\..\\..\\evil5")
    add("warblade/" + "a" * 150 + "/../../../evil6")       # a pax path
    add("./warblade/./evil-link", kind=tarfile.SYMTYPE, link="/etc/passwd")
    add("warblade/evil-hard", kind=tarfile.LNKTYPE, link="warblade/WarBlade.inf")
    add("warblade/evil-dir", kind=tarfile.DIRTYPE)
    add("warblade/ok.txt", b"ok")
EOF
# The old image's save volume, exported as README.md says.
tar -C "$fx/saves" -cf "$fx/legacy.tar" .

# The image's page with the stand-in game.
c=$(docker create "$image")
docker cp "$c:/app/web/." "$fx/web-stub/"
docker rm "$c" > /dev/null
cp "$STUB_DIR/warblade.js" "$STUB_DIR/warblade.wasm" "$fx/web-stub/"
chmod -R a+rX "$work"

# ---- servers ----
network=$name-net
docker network create "$network" > /dev/null
# start NAME PORT ARGS...: runs the image as it should be deployed (read-only, no privileges)
start() {
    n=$name-$1; port=$2; shift 2
    docker run -d --name "$n" --network "$network" -p "127.0.0.1:$port:8080" --read-only --tmpfs /tmp \
        --cap-drop ALL --security-opt no-new-privileges "$@" "$image" > /dev/null
    containers="$containers $n"
}
start real 18081 -v "$fx/data:/data:ro" -v "$fx/saves:/saves"
start stub 18082 -v "$fx/data-clean:/data:ro" -v "$fx/web-stub:/app/web:ro"
start local 18083 -v "$fx/web-stub:/app/web:ro"
start limits 18084 -v "$fx/data-big:/data:ro"

docker run -d --name "$name-api" --network "$network" --read-only --tmpfs /tmp \
    --cap-drop ALL --security-opt no-new-privileges \
    -e WARBLADE_PUBLIC_ORIGIN=http://127.0.0.1:18085 \
    -e WARBLADE_SESSION_MODE=cookie \
    -v "$fx/api-state:/state" "$api_image" > /dev/null
containers="$containers $name-api"
cat > "$fx/Caddyfile" <<EOF
:8080 {
    @api path /api/*
    handle @api {
        reverse_proxy $name-api:8081
    }
    handle {
        reverse_proxy $name-stub:8080
    }
}
EOF
docker run -d --name "$name-proxy" --network "$network" -p 127.0.0.1:18085:8080 \
    --read-only --tmpfs /tmp --tmpfs /config --tmpfs /data \
    --cap-drop ALL --security-opt no-new-privileges \
    --cap-add NET_BIND_SERVICE \
    -v "$fx/Caddyfile:/etc/caddy/Caddyfile:ro" caddy:2.10-alpine > /dev/null
containers="$containers $name-proxy"

for port in 18081 18082 18083 18084 18085; do
    i=0
    until python3 -c "import urllib.request; urllib.request.urlopen('http://127.0.0.1:$port/healthz', timeout=2)" 2> /dev/null; do
        i=$((i + 1))
        if [ $i -gt 50 ]; then
            for c in $containers; do docker logs "$c"; done
            echo "server on port $port did not start" >&2
            exit 1
        fi
        sleep 0.2
    done
done

# The proxy can accept the home page before the API process has opened its port.
i=0
until python3 -c 'import http.client; c=http.client.HTTPConnection("127.0.0.1", 18085, timeout=2); c.request("GET", "/api/me"); assert c.getresponse().status == 200' 2> /dev/null; do
    i=$((i + 1))
    if [ "$i" -gt 50 ]; then
        docker logs "$name-api"
        echo "account API did not start" >&2
        exit 1
    fi
    sleep 0.2
done

# ---- tests ----
export WARBLADE_URL=http://127.0.0.1:18081
export WARBLADE_STUB_URL=http://127.0.0.1:18085
export WARBLADE_LOCAL_URL=http://127.0.0.1:18083
export WARBLADE_LIMITS_URL=http://127.0.0.1:18084
export WARBLADE_FIXTURES="$fx"
export WARBLADE_CONTAINER=$name-real
export WARBLADE_IMAGE=$image
export NODE_PATH="$root/tests/node_modules${NODE_PATH:+:$NODE_PATH}:$(npm root -g)"

status=0
cd "$root"
python3 -m unittest -v "$root/tests/test_server.py" || status=1
PYTHONPATH="$root" python3 -m unittest discover -s "$root/tests" -p 'test_hiscores.py' -v || status=1
node --test --test-concurrency=1 "$root/tests/browser.test.cjs" "$root/tests/auth.test.cjs" "$root/tests/startup.test.cjs" "$root/tests/mobile-login.test.cjs" "$root/tests/guest.test.cjs" "$root/tests/performance.test.cjs" "$root/tests/scaling.test.cjs" "$root/tests/remembered-login.test.cjs" || status=1
exit $status

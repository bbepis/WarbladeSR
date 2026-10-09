#!/bin/sh
# Starts the Docker image's web server (docker/nginx.conf; README.md, "Docker").
#
# Writes the per-container settings into /tmp/warblade (the only place it writes; the rest of
# the container can be read-only) and runs nginx in the foreground.
#
# Settings come from the environment:
#   PORT                       the port to listen on (default 8080)
#   WARBLADE_TRUSTED_PROXIES   space-separated addresses or CIDRs of the reverse proxies in
#                              front of this server; their X-Forwarded-For gives the client
#                              address the rate limits use. Empty: every client is the
#                              address that connects (so behind a proxy, all clients share
#                              one budget). Never list addresses clients can connect from.
set -eu

run=/tmp/warblade
mkdir -p "$run/conf.d"

port=${PORT:-8080}
case $port in
    '' | *[!0-9]*) echo "PORT must be a number, not '$port'" >&2; exit 1 ;;
esac
echo "listen $port;" > "$run/listen.conf"

# The folder holding warblade.pac: the mount itself (the install's data/ folder was mounted)
# or its data/ subfolder (the whole install was). Names match in any case, as on Windows.
rm -f "$run/gamedata"
data=
for dir in /data /data/data /data/Data /data/DATA; do
    if [ -d "$dir" ] && ls -1 "$dir" 2> /dev/null | grep -qix 'warblade\.pac'; then
        data=$dir
        ln -s "$dir" "$run/gamedata"
        break
    fi
done

# Reuse the mounted game's own title artwork for the first browser paint.
# Extract only this JPEG to ephemeral storage; never copy game assets into the image.
if [ -n "$data" ]; then
    for archive in "$data"/*; do
        [ "$(basename "$archive" | tr '[:upper:]' '[:lower:]')" = warblade.pac ] || continue
        title=$(tar -tf "$archive" 2>/dev/null | grep -i '^splashscreen\.jpg$' | head -n 1 || true)
        if [ -n "$title" ]; then
            tar -xOf "$archive" "$title" > "$run/loading-title.jpg"
        fi
        break
    done
fi

: > "$run/conf.d/realip.conf"
if [ -n "${WARBLADE_TRUSTED_PROXIES:-}" ]; then
    for proxy in $WARBLADE_TRUSTED_PROXIES; do
        case $proxy in
            *[!0-9A-Fa-f.:/]*) echo "WARBLADE_TRUSTED_PROXIES: not an address or CIDR: '$proxy'" >&2; exit 1 ;;
        esac
        echo "set_real_ip_from $proxy;" >> "$run/conf.d/realip.conf"
    done
    printf 'real_ip_header X-Forwarded-For;\nreal_ip_recursive on;\n' >> "$run/conf.d/realip.conf"
fi

echo "Warblade SR on port $port; data: ${data:-none (players choose their own folder)}; trusted proxies: ${WARBLADE_TRUSTED_PROXIES:-none}"
exec nginx -c /etc/warblade/nginx.conf -g 'daemon off;'

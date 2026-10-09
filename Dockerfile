# Warblade SR in the browser, served as a read-only static site (README.md, "Docker";
# docs/web-security.md). The image has no game data: mount your Warblade data folder at /data.
# A separate account API stores player profiles, saves and merged high scores.
#
#   docker build -t warblade-sr .
#   docker run -p 8080:8080 -v /path/to/Warblade/data:/data:ro --read-only --tmpfs /tmp warblade-sr

# ---- build: the WebAssembly game (build-web.sh) ----
FROM docker.io/emscripten/emsdk:4.0.23 AS build
# The project needs CMake 3.25+ and Ninja; the image has CMake 3.22 and no Ninja.
RUN pip3 install --no-cache-dir "cmake>=3.25" ninja
WORKDIR /src
COPY . .
RUN ./build-web.sh

# ---- run: nginx, unprivileged, serving files it can't change ----
FROM docker.io/nginxinc/nginx-unprivileged:1.30-alpine
USER root
RUN mkdir -p /app/web /data /etc/warblade
COPY --from=build /src/build/web/index.html /src/build/web/page.js /src/build/web/page.css \
                  /src/build/web/warblade.js /src/build/web/warblade.wasm /app/web/
COPY docker/nginx.conf /etc/warblade/nginx.conf
COPY --chmod=755 docker/entrypoint.sh /usr/local/bin/warblade-entrypoint
# Root owns everything and nginx (uid 101) can only read it; nginx writes only under /tmp.
RUN chmod -R a=rX,u+w /app /etc/warblade
USER 101
ENV PORT=8080
EXPOSE 8080
HEALTHCHECK --interval=30s --timeout=5s CMD wget -q -O /dev/null "http://127.0.0.1:${PORT}/healthz" || exit 1
ENTRYPOINT ["/usr/local/bin/warblade-entrypoint"]
CMD []

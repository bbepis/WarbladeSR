#!/bin/sh
# Browser build (README.md, "Browser"): ./build-web.sh
#
# Builds zlib, SDL3, SDL3_image and SDL3_mixer for WebAssembly into build/deps-web (once), then
# the game into build/web: index.html, page.js, page.css, warblade.js and warblade.wasm. Serve
# that folder over HTTP (e.g. `python3 -m http.server -d build/web`) and open it; the page asks
# for your Warblade 1.34 folder. No game data goes into the build.
#
# Needs the Emscripten SDK on PATH (emcc, emcmake: source emsdk_env.sh), cmake, ninja and git.
#   SDL_SRC, SDL_IMAGE_SRC, SDL_MIXER_SRC, ZLIB_SRC
#                      existing source trees to use instead of cloning (SDL_mixer's needs its
#                      external/libxmp submodule)
set -eu

# Each tag with the commit it must be: a tag that moved to other code stops the build.
SDL_TAG=release-3.4.18       SDL_COMMIT=829a65d769d935c4852f8159e964312c0957260a
SDL_IMAGE_TAG=release-3.4.6  SDL_IMAGE_COMMIT=f661fa1ad24ab1b81e43662532f9a6a9fcf67ea6
SDL_MIXER_TAG=release-3.2.4  SDL_MIXER_COMMIT=72a81869b45e249e8e67102db4e98dd2441f05a1
ZLIB_TAG=v1.3.1              ZLIB_COMMIT=51b7f2abdade71cd9bb0e7a373ef2610ec6f9daf

root=$(cd "$(dirname "$0")" && pwd)
deps="$root/build/deps-web"
prefix="$deps/prefix"
mkdir -p "$deps" "$prefix/lib" "$prefix/include"
command -v emcmake > /dev/null || { echo "emcmake not found: source emsdk_env.sh first" >&2; exit 1; }

# fetch NAME TAG COMMIT URL [submodules]: prints the source folder, checked to be COMMIT
fetch() {
    if [ ! -d "$deps/$1" ]; then
        git clone --quiet --depth 1 --branch "$2" ${5:+--recurse-submodules --shallow-submodules} "$4" "$deps/$1" >&2
    fi
    got=$(git -C "$deps/$1" rev-parse HEAD)
    if [ "$got" != "$3" ]; then
        echo "$1: $2 is commit $got, expected $3" >&2
        exit 1
    fi
    echo "$deps/$1"
}

# lib NAME SOURCE OPTIONS...: configures, builds and installs one library into $prefix
lib() {
    name=$1; src=$2; shift 2
    if [ ! -f "$deps/$name.done" ]; then
        emcmake cmake -S "$src" -B "$deps/build-$name" -G Ninja -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_INSTALL_PREFIX="$prefix" -DCMAKE_PREFIX_PATH="$prefix" -DCMAKE_FIND_ROOT_PATH="$prefix" \
            -DBUILD_SHARED_LIBS=OFF "$@"
        cmake --build "$deps/build-$name"
        cmake --install "$deps/build-$name"
        touch "$deps/$name.done"
    fi
}

zlib=${ZLIB_SRC:-$(fetch zlib $ZLIB_TAG $ZLIB_COMMIT https://github.com/madler/zlib.git)}
sdl=${SDL_SRC:-$(fetch SDL $SDL_TAG $SDL_COMMIT https://github.com/libsdl-org/SDL.git)}
sdl_image=${SDL_IMAGE_SRC:-$(fetch SDL_image $SDL_IMAGE_TAG $SDL_IMAGE_COMMIT https://github.com/libsdl-org/SDL_image.git)}
sdl_mixer=${SDL_MIXER_SRC:-$(fetch SDL_mixer $SDL_MIXER_TAG $SDL_MIXER_COMMIT https://github.com/libsdl-org/SDL_mixer.git submodules)}

# zlib: just its sources (the game only compresses and uncompresses buffers).
if [ ! -f "$deps/zlib.done" ]; then
    mkdir -p "$deps/build-zlib"
    for f in adler32 compress crc32 deflate gzclose gzlib gzread gzwrite infback inffast inflate \
             inftrees trees uncompr zutil; do
        emcc -O2 -DHAVE_UNISTD_H -c "$zlib/$f.c" -o "$deps/build-zlib/$f.o"
    done
    rm -f "$prefix/lib/libz.a"
    emar rcs "$prefix/lib/libz.a" "$deps/build-zlib/"*.o
    cp "$zlib/zlib.h" "$zlib/zconf.h" "$prefix/include/"
    touch "$deps/zlib.done"
fi

# The same features as the desktop builds (build-linux.sh).
lib sdl3 "$sdl" -DSDL_SHARED=OFF -DSDL_STATIC=ON -DSDL_TESTS=OFF -DSDL_TEST_LIBRARY=OFF \
    -DSDL_EXAMPLES=OFF -DSDL_GPU=OFF -DSDL_CAMERA=OFF -DSDL_HAPTIC=OFF -DSDL_SENSOR=OFF \
    -DSDL_DIALOG=OFF -DSDL_TRAY=OFF -DSDL_HIDAPI=OFF
lib sdl3_image "$sdl_image" -DSDLIMAGE_VENDORED=OFF -DSDLIMAGE_BACKEND_STB=ON -DSDLIMAGE_PNG_LIBPNG=OFF \
    -DSDLIMAGE_AVIF=OFF -DSDLIMAGE_JXL=OFF -DSDLIMAGE_TIF=OFF -DSDLIMAGE_WEBP=OFF -DSDLIMAGE_SAMPLES=OFF \
    -DSDLIMAGE_TESTS=OFF
lib sdl3_mixer "$sdl_mixer" -DSDLMIXER_VENDORED=ON -DSDLMIXER_DEPS_SHARED=OFF -DSDLMIXER_MOD_XMP=ON \
    -DSDLMIXER_MOD_XMP_SHARED=OFF -DSDLMIXER_MP3_DRMP3=ON -DSDLMIXER_MP3_MPG123=OFF -DSDLMIXER_MIDI=OFF \
    -DSDLMIXER_FLAC_LIBFLAC=OFF -DSDLMIXER_OPUS=OFF -DSDLMIXER_WAVPACK=OFF -DSDLMIXER_GME=OFF \
    -DSDLMIXER_VORBIS_VORBISFILE=OFF -DSDLMIXER_VORBIS_TREMOR=OFF -DSDLMIXER_EXAMPLES=OFF -DSDLMIXER_TESTS=OFF

out="$root/build/web"
emcmake cmake -S "$root" -B "$out" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$prefix" \
    -DCMAKE_FIND_ROOT_PATH="$prefix" -DZLIB_ROOT="$prefix"
cmake --build "$out"
echo "built $out (serve it over HTTP, e.g.: python3 -m http.server -d $out 8000)"

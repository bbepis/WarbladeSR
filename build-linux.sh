#!/bin/sh
# Linux build (README.md, "Linux"): ./build-linux.sh [release|debug]
#
# Builds SDL3, SDL3_image and SDL3_mixer from source into build/deps (static, once), then the
# game into build/linux-<config>/warblade, with a data/ link to your Warblade install's data/.
#
#   WARBLADE_GAME_DIR  your Warblade 1.34 folder (default ../game)
#   SDL_SRC, SDL_IMAGE_SRC, SDL_MIXER_SRC
#                      existing source trees to use instead of cloning (SDL_mixer's needs its
#                      external/libxmp submodule)
#
# Needs: a C compiler, cmake, ninja, git, and the X11/ALSA/PulseAudio/OpenGL development
# headers SDL uses (Debian/Ubuntu: libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev
# libxss-dev libxfixes-dev libxtst-dev libasound2-dev libpulse-dev libgl-dev libegl-dev zlib1g-dev).
set -eu

SDL_TAG=release-3.4.18
SDL_IMAGE_TAG=release-3.4.6
SDL_MIXER_TAG=release-3.2.4

config=${1:-release}
case "$config" in
    release) build_type=RelWithDebInfo ;;
    debug)   build_type=Debug ;;
    *) echo "usage: $0 [release|debug]" >&2; exit 2 ;;
esac

root=$(cd "$(dirname "$0")" && pwd)
deps="$root/build/deps"
prefix="$deps/prefix"
game_dir=${WARBLADE_GAME_DIR:-$root/../game}
mkdir -p "$deps"

# fetch NAME TAG URL [submodules]: prints the source folder
fetch() {
    if [ ! -d "$deps/$1" ]; then
        git clone --quiet --depth 1 --branch "$2" ${4:+--recurse-submodules --shallow-submodules} "$3" "$deps/$1" >&2
    fi
    echo "$deps/$1"
}

# lib NAME SOURCE OPTIONS...: configures, builds and installs one library into $prefix
lib() {
    name=$1; src=$2; shift 2
    if [ ! -f "$deps/$name.done" ]; then
        cmake -S "$src" -B "$deps/build-$name" -G Ninja -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_INSTALL_PREFIX="$prefix" -DCMAKE_PREFIX_PATH="$prefix" -DBUILD_SHARED_LIBS=OFF "$@"
        cmake --build "$deps/build-$name"
        cmake --install "$deps/build-$name"
        touch "$deps/$name.done"
    fi
}

sdl=${SDL_SRC:-$(fetch SDL $SDL_TAG https://github.com/libsdl-org/SDL.git)}
sdl_image=${SDL_IMAGE_SRC:-$(fetch SDL_image $SDL_IMAGE_TAG https://github.com/libsdl-org/SDL_image.git)}
sdl_mixer=${SDL_MIXER_SRC:-$(fetch SDL_mixer $SDL_MIXER_TAG https://github.com/libsdl-org/SDL_mixer.git submodules)}

# The same features as the Windows build's vcpkg ports (vcpkg-ports/README.md); images use
# SDL_image's built-in decoders instead of libpng/libjpeg.
lib sdl3 "$sdl" -DSDL_SHARED=OFF -DSDL_STATIC=ON -DSDL_TESTS=OFF -DSDL_TEST_LIBRARY=OFF \
    -DSDL_EXAMPLES=OFF -DSDL_GPU=OFF -DSDL_CAMERA=OFF -DSDL_HAPTIC=OFF -DSDL_SENSOR=OFF \
    -DSDL_DIALOG=OFF -DSDL_TRAY=OFF -DSDL_HIDAPI=OFF
lib sdl3_image "$sdl_image" -DSDLIMAGE_VENDORED=OFF -DSDLIMAGE_BACKEND_STB=ON -DSDLIMAGE_PNG_LIBPNG=OFF -DSDLIMAGE_AVIF=OFF \
    -DSDLIMAGE_JXL=OFF -DSDLIMAGE_TIF=OFF -DSDLIMAGE_WEBP=OFF -DSDLIMAGE_SAMPLES=OFF -DSDLIMAGE_TESTS=OFF
lib sdl3_mixer "$sdl_mixer" -DSDLMIXER_VENDORED=ON -DSDLMIXER_DEPS_SHARED=OFF -DSDLMIXER_MOD_XMP=ON \
    -DSDLMIXER_MOD_XMP_SHARED=OFF -DSDLMIXER_MP3_DRMP3=ON -DSDLMIXER_MP3_MPG123=OFF -DSDLMIXER_MIDI=OFF \
    -DSDLMIXER_FLAC_LIBFLAC=OFF -DSDLMIXER_OPUS=OFF -DSDLMIXER_WAVPACK=OFF -DSDLMIXER_GME=OFF \
    -DSDLMIXER_VORBIS_VORBISFILE=OFF -DSDLMIXER_VORBIS_TREMOR=OFF -DSDLMIXER_EXAMPLES=OFF -DSDLMIXER_TESTS=OFF

out="$root/build/linux-$config"
cmake -S "$root" -B "$out" -G Ninja -DCMAKE_BUILD_TYPE=$build_type -DCMAKE_PREFIX_PATH="$prefix"
cmake --build "$out"

if [ -d "$game_dir/data" ]; then
    ln -sfn "$(cd "$game_dir" && pwd)/data" "$out/data"
else
    echo "note: no data/ in $game_dir; link your Warblade install's data/ into $out" >&2
fi
echo "built $out/warblade (run it from that folder: cd $out && ./warblade)"

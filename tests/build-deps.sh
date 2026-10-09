#!/bin/sh
# Builds the SDL3, SDL3_image and SDL3_mixer the engine tests link (tests/README.md) into
# build/deps-test, once: static, headless (only SDL's dummy/offscreen video and dummy/disk
# audio), so they build and run without X11, ALSA or a display. The same tags as
# build-linux.sh. The game-logic tests don't need these.
#
#   WB_TEST_DEPS                         alternate build/install folder
#   SDL_SRC, SDL_IMAGE_SRC, SDL_MIXER_SRC   existing source trees to use instead of cloning
set -eu

SDL_TAG=release-3.4.18
SDL_IMAGE_TAG=release-3.4.6
SDL_MIXER_TAG=release-3.2.4

root=$(cd "$(dirname "$0")/.." && pwd)
deps=${WB_TEST_DEPS:-"$root/build/deps-test"}
prefix="$deps/prefix"
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

lib sdl3 "$sdl" -DSDL_SHARED=OFF -DSDL_STATIC=ON -DSDL_TESTS=OFF -DSDL_TEST_LIBRARY=OFF \
    -DSDL_EXAMPLES=OFF -DSDL_GPU=OFF -DSDL_CAMERA=OFF -DSDL_HAPTIC=OFF -DSDL_SENSOR=OFF \
    -DSDL_DIALOG=OFF -DSDL_TRAY=OFF -DSDL_HIDAPI=ON -DSDL_JOYSTICK=ON -DSDL_VIRTUAL_JOYSTICK=ON \
    -DSDL_X11=OFF -DSDL_WAYLAND=OFF -DSDL_KMSDRM=OFF -DSDL_OPENGL=OFF -DSDL_OPENGLES=OFF \
    -DSDL_VULKAN=OFF -DSDL_ALSA=OFF -DSDL_PULSEAUDIO=OFF -DSDL_PIPEWIRE=OFF -DSDL_JACK=OFF \
    -DSDL_SNDIO=OFF -DSDL_OSS=OFF -DSDL_DUMMYVIDEO=ON -DSDL_OFFSCREEN=ON -DSDL_DUMMYAUDIO=ON \
    -DSDL_DISKAUDIO=ON -DSDL_UNIX_CONSOLE_BUILD=ON -DSDL_DBUS=OFF -DSDL_IBUS=OFF -DSDL_LIBUDEV=OFF
lib sdl3_image "$sdl_image" -DSDLIMAGE_VENDORED=OFF -DSDLIMAGE_BACKEND_STB=ON -DSDLIMAGE_PNG_LIBPNG=OFF -DSDLIMAGE_AVIF=OFF \
    -DSDLIMAGE_JXL=OFF -DSDLIMAGE_TIF=OFF -DSDLIMAGE_WEBP=OFF -DSDLIMAGE_SAMPLES=OFF -DSDLIMAGE_TESTS=OFF
lib sdl3_mixer "$sdl_mixer" -DSDLMIXER_VENDORED=ON -DSDLMIXER_DEPS_SHARED=OFF -DSDLMIXER_MOD_XMP=ON \
    -DSDLMIXER_MOD_XMP_SHARED=OFF -DSDLMIXER_MP3_DRMP3=ON -DSDLMIXER_MP3_MPG123=OFF -DSDLMIXER_MIDI=OFF \
    -DSDLMIXER_FLAC_LIBFLAC=OFF -DSDLMIXER_OPUS=OFF -DSDLMIXER_WAVPACK=OFF -DSDLMIXER_GME=OFF \
    -DSDLMIXER_VORBIS_VORBISFILE=OFF -DSDLMIXER_VORBIS_TREMOR=OFF -DSDLMIXER_EXAMPLES=OFF -DSDLMIXER_TESTS=OFF
echo "$prefix"

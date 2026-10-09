# The tests (tests/README.md): a project of its own, so the game-logic tests need no SDL.
#   cmake -S tests -B build/tests -G Ninja && cmake --build build/tests && build/tests/wbtests
#
# wbtests: the game's code (everything in src/ but the SDL half of the engine) linked against
#   a fake engine (fake_engine.c) and the test_*.c files.
# wbengine: the real engine (src/core/sdl.c, sdl_audio.c) and engine_*.c; only when SDL3,
#   SDL3_image and SDL3_mixer are found (tests/build-deps.sh builds headless ones).
cmake_minimum_required(VERSION 3.25)
project(warblade_tests C)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

set(ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")
if(DEFINED ENV{WB_DEPS})
    list(APPEND CMAKE_PREFIX_PATH "$ENV{WB_DEPS}")
endif()
find_package(ZLIB REQUIRED)

# The game's own flags (../CMakeLists.txt): the decompiled code needs them.
set(GAME_OPTIONS -Wno-unknown-pragmas -fms-extensions -fno-strict-aliasing -fwrapv -fno-delete-null-pointer-checks -w)

file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS "${ROOT}/src/*.c")
list(FILTER GAME_SOURCES EXCLUDE REGEX "/src/core/sdl(_audio)?\\.c$")
add_library(wbgame STATIC ${GAME_SOURCES})
target_include_directories(wbgame PUBLIC "${ROOT}/include" "${ROOT}/include/posix")
target_compile_options(wbgame PRIVATE ${GAME_OPTIONS})
target_link_libraries(wbgame PUBLIC ZLIB::ZLIB m)
# main.c's plain main() would clash with the test runner's.
set_source_files_properties("${ROOT}/src/core/main.c" PROPERTIES COMPILE_DEFINITIONS "main=WarbladeProgramMain")
# The original's global order (../CMakeLists.txt).
include(CheckCCompilerFlag)
check_c_compiler_flag(-fno-toplevel-reorder HAVE_NO_TOPLEVEL_REORDER)
if(HAVE_NO_TOPLEVEL_REORDER)
    set_source_files_properties("${ROOT}/src/data/globals.c" PROPERTIES COMPILE_OPTIONS "-fno-data-sections;-fno-toplevel-reorder")
endif()

file(GLOB TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/test_*.c")
add_executable(wbtests runner.c fake_engine.c support.c ${TEST_SOURCES})
target_compile_options(wbtests PRIVATE -fms-extensions -Wall -Wno-unused-function -Wno-unknown-pragmas)
# The fake and the tests reach the game's statics and globals; link all of the library.
target_link_libraries(wbtests PRIVATE -Wl,--whole-archive wbgame -Wl,--no-whole-archive ZLIB::ZLIB m)

enable_testing()
add_test(NAME game COMMAND wbtests -j 4)

find_package(SDL3 CONFIG QUIET)
find_package(SDL3_image CONFIG QUIET)
find_package(SDL3_mixer CONFIG QUIET)
if(SDL3_FOUND AND SDL3_image_FOUND AND SDL3_mixer_FOUND)
    file(GLOB ENGINE_TESTS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/engine_*.c")
    add_executable(wbengine runner.c "${ROOT}/src/core/sdl.c" "${ROOT}/src/core/sdl_audio.c" ${ENGINE_TESTS})
    target_include_directories(wbengine PRIVATE "${ROOT}/include")
    target_compile_options(wbengine PRIVATE -fms-extensions -Wall -Wno-unused-function -Wno-unknown-pragmas)
    target_link_libraries(wbengine PRIVATE SDL3::SDL3 SDL3_image::SDL3_image SDL3_mixer::SDL3_mixer m)
    add_test(NAME engine COMMAND wbengine -j 4)
else()
    message(STATUS "SDL3/SDL3_image/SDL3_mixer not found: the engine tests (wbengine) are skipped; "
                   "tests/build-deps.sh builds them, then pass -DCMAKE_PREFIX_PATH=build/deps-test/prefix")
endif()

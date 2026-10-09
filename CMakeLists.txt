# Warblade, SDL port (SDL_PLAN.md). VS2026 (MSVC, x64 or x86) + vcpkg; run build.bat rather than
# configuring by hand. The engine underneath include/sdlhelp.h is SDL3, SDL3_image and
# SDL3_mixer (src/core/sdl.c, src/core/sdl_audio.c), linked statically (vcpkg triplet
# x64-windows-static or x86-windows-static) with the static C runtime: warblade.exe needs no DLLs but Windows'.
# Linux builds with GCC or Clang (README.md, "Linux"): SDL3, SDL3_image and SDL3_mixer come
# from CMAKE_PREFIX_PATH, and the game runs from a folder holding the install's data/.
cmake_minimum_required(VERSION 3.25)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")   # /MT, /MTd
project(warblade VERSION 2.0 LANGUAGES C)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(WARBLADE_ASAN "Build with /fsanitize=address" OFF)
if(WIN32)
    if(DEFINED ENV{WARBLADE_GAME_DIR})
        set(_game_dir "$ENV{WARBLADE_GAME_DIR}")
    else()
        set(_game_dir "${CMAKE_SOURCE_DIR}/../game")
    endif()
    set(WARBLADE_GAME_DIR "${_game_dir}" CACHE PATH
        "The original install: data/ is linked into the output folder, the icon is copied from it")
    if(NOT EXISTS "${WARBLADE_GAME_DIR}/warblade.ico")
        message(FATAL_ERROR "warblade.ico not found in ${WARBLADE_GAME_DIR}: "
                            "set WARBLADE_GAME_DIR to your Warblade folder")
    endif()
    # The exe's icon comes from the install (res/warblade.rc finds it in the build folder).
    configure_file("${WARBLADE_GAME_DIR}/warblade.ico" "${CMAKE_BINARY_DIR}/res/warblade.ico" COPYONLY)
endif()
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)   # tools/compile.py reads it

find_package(SDL3 CONFIG REQUIRED)
find_package(SDL3_image CONFIG REQUIRED)
find_package(SDL3_mixer CONFIG REQUIRED)
find_package(ZLIB REQUIRED)

file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.c")
if(WIN32)
    add_executable(warblade WIN32 ${GAME_SOURCES} res/warblade.rc)
    set_property(SOURCE res/warblade.rc APPEND PROPERTY INCLUDE_DIRECTORIES "${CMAKE_BINARY_DIR}/res")
else()
    add_executable(warblade ${GAME_SOURCES})
    # io.h and the other Microsoft C runtime names the game uses (include/posix/io.h).
    target_include_directories(warblade PRIVATE include/posix)
endif()
target_include_directories(warblade PRIVATE include)
target_link_libraries(warblade PRIVATE
    SDL3::SDL3 SDL3_image::SDL3_image SDL3_mixer::SDL3_mixer ZLIB::ZLIB)
if(MSVC)
    target_compile_definitions(warblade PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(warblade PRIVATE /W3)
    # The game's entry point is a plain main() (GameMain); SDL needs no SDL_main.
    target_link_options(warblade PRIVATE /ENTRY:mainCRTStartup)
else()
    # The decompiled code reads floats through int pointers and the like, and relies on
    # wrapping arithmetic, as MSVC allows: keep GCC/Clang from optimizing on the opposite.
    target_compile_options(warblade PRIVATE -fms-extensions -fno-strict-aliasing -fwrapv
                                            -fno-delete-null-pointer-checks)
    target_link_libraries(warblade PRIVATE m)
    # globals.c lists the globals in the original's address order, which MSVC keeps with its
    # bss_seg/data_seg pragmas; keep the same order here in case code still reaches across
    # from one global into the next, as the original binary could. GCC needs telling; Clang
    # (no such flag) emits globals in definition order anyway.
    # It's also built as one data section, so a linker that drops unused data (wasm-ld does)
    # keeps the original's padding globals, which overruns like that may land in.
    include(CheckCCompilerFlag)
    set(_globals_options -fno-data-sections)
    check_c_compiler_flag(-fno-toplevel-reorder HAVE_NO_TOPLEVEL_REORDER)
    if(HAVE_NO_TOPLEVEL_REORDER)
        list(APPEND _globals_options -fno-toplevel-reorder)
    endif()
    set_source_files_properties(src/data/globals.c PROPERTIES COMPILE_OPTIONS "${_globals_options}")
endif()

if(EMSCRIPTEN)
    # The browser build (README.md, "Browser"): warblade.js/.wasm plus the page in web/, which
    # loads the player's own data/ into the virtual file system and starts the game.
    # Asyncify lets the game keep its blocking main loop: SysFlip and AudioUpdate yield.
    set_target_properties(warblade PROPERTIES SUFFIX ".js")
    target_link_options(warblade PRIVATE
        -sASYNCIFY -sASYNCIFY_STACK_SIZE=131072 -sSTACK_SIZE=4MB
        -sALLOW_MEMORY_GROWTH -sINITIAL_MEMORY=256MB
        -sINVOKE_RUN=0 -sEXIT_RUNTIME=1 -sFORCE_FILESYSTEM
        "-sEXPORTED_RUNTIME_METHODS=['FS','callMain','ccall']")
    foreach(page index.html page.js page.css)
        configure_file(web/${page} "${CMAKE_BINARY_DIR}/${page}" COPYONLY)
    endforeach()
endif()

if(WARBLADE_ASAN AND MSVC)
    # /RTC (in CMake's Debug flags) and ASan don't mix
    foreach(v CMAKE_C_FLAGS_DEBUG)
        string(REGEX REPLACE "/RTC[1csu]*" "" ${v} "${${v}}")
    endforeach()
    target_compile_options(warblade PRIVATE /fsanitize=address /Zi)
    target_link_options(warblade PRIVATE /INCREMENTAL:NO)
    get_filename_component(_cl_dir "${CMAKE_C_COMPILER}" DIRECTORY)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        file(GLOB ASAN_DLLS "${_cl_dir}/clang_rt.asan*-x86_64.dll")
    else()
        file(GLOB ASAN_DLLS "${_cl_dir}/clang_rt.asan*-i386.dll")
    endif()
endif()

if(WIN32)
    # Next to the exe: warblade.ico from the install, the ASan runtime, and a junction to its data/
    # (cmake/postbuild.cmake).
    add_custom_command(TARGET warblade POST_BUILD
        COMMAND ${CMAKE_COMMAND} "-DGAME=${WARBLADE_GAME_DIR}"
                "-DEXTRA=${ASAN_DLLS}" "-DDST=$<TARGET_FILE_DIR:warblade>"
                -P "${CMAKE_SOURCE_DIR}/cmake/postbuild.cmake"
        VERBATIM)
endif()

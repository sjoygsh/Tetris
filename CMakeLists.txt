# ===========================================================================
# CMakeLists.txt — Build configuration for the Tetris game
# ===========================================================================
# CMake is a build system generator. This file tells CMake:
#   1. What compiler settings to use
#   2. Where to find libraries (SDL2, SDL2_ttf, SDL2_mixer)
#   3. How to compile our source code into an executable
#
# Works on both Windows (vcpkg) and Linux (system packages via pkg-config).
# ===========================================================================

# Minimum CMake version required
cmake_minimum_required(VERSION 3.15)

# Name our project "Tetris" and declare it as a C project
project(Tetris LANGUAGES C)

# Use the C11 standard (a modern version of the C language)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Find SDL2 and its extension libraries
# ---------------------------------------------------------------------------
# Strategy: try CMake config files first (works with vcpkg on Windows).
# If that fails, fall back to pkg-config (standard on Linux).
# ---------------------------------------------------------------------------

# Try to find packages via CMake config (vcpkg provides these)
find_package(SDL2 QUIET CONFIG)
find_package(SDL2_ttf QUIET CONFIG)
find_package(SDL2_mixer QUIET CONFIG)

# Check if we got imported targets from CMake config
if(TARGET SDL2::SDL2 AND TARGET SDL2_ttf::SDL2_ttf AND TARGET SDL2_mixer::SDL2_mixer)
    # Windows / vcpkg path — imported targets available
    set(SDL2_LIBS SDL2::SDL2 SDL2::SDL2main SDL2_ttf::SDL2_ttf SDL2_mixer::SDL2_mixer)
    set(SDL2_INC "")
    message(STATUS "Found SDL2 via CMake config (vcpkg)")
else()
    # Linux path — use pkg-config
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2)
    pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)
    pkg_check_modules(SDL2_MIXER REQUIRED SDL2_mixer)

    set(SDL2_LIBS
        ${SDL2_LIBRARIES}
        ${SDL2_TTF_LIBRARIES}
        ${SDL2_MIXER_LIBRARIES}
    )
    set(SDL2_INC
        ${SDL2_INCLUDE_DIRS}
        ${SDL2_TTF_INCLUDE_DIRS}
        ${SDL2_MIXER_INCLUDE_DIRS}
    )

    # pkg-config may also provide linker flags (like -L paths)
    link_directories(
        ${SDL2_LIBRARY_DIRS}
        ${SDL2_TTF_LIBRARY_DIRS}
        ${SDL2_MIXER_LIBRARY_DIRS}
    )
    message(STATUS "Found SDL2 via pkg-config")
endif()

# ---------------------------------------------------------------------------
# Define our executable
# ---------------------------------------------------------------------------
# This tells CMake: "Compile these .c files into a program called 'tetris'"
add_executable(tetris
    src/main.c
    src/game.c
    src/piece.c
    src/board.c
    src/render.c
    src/audio.c
    src/input.c
)

# ---------------------------------------------------------------------------
# Include directories — where the compiler looks for header files (.h)
# ---------------------------------------------------------------------------
target_include_directories(tetris PRIVATE
    ${SDL2_INC}
    src
)

# ---------------------------------------------------------------------------
# Link libraries — connect our program to SDL2, SDL2_ttf, SDL2_mixer
# ---------------------------------------------------------------------------
target_link_libraries(tetris PRIVATE
    ${SDL2_LIBS}
)

# Suppress math library linking on Linux (needed for sin/cos in render.c)
if(UNIX)
    target_link_libraries(tetris PRIVATE m)
endif()

# ---------------------------------------------------------------------------
# Copy assets folder next to the executable after building
# ---------------------------------------------------------------------------
# Our game needs font and sound files at runtime. This command copies
# the "assets" folder into the build directory so the game can find them.
add_custom_command(TARGET tetris POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:tetris>/assets"
    COMMENT "Copying assets to build directory..."
)

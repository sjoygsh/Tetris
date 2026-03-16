# ===========================================================================
# CMakeLists.txt — Build configuration for the Tetris game
# ===========================================================================
# CMake is a build system generator. This file tells CMake:
#   1. What compiler settings to use
#   2. Where to find libraries (SDL2, SDL2_ttf, SDL2_mixer)
#   3. How to compile our source code into an executable
#
# USAGE:
#   mkdir build && cd build
#   cmake .. -G "MinGW Makefiles"   (or your preferred generator)
#   cmake --build .
# ===========================================================================

# Minimum CMake version required (3.15+ has good FindPackage support)
cmake_minimum_required(VERSION 3.15)

# Name our project "Tetris" and declare it as a C project
project(Tetris LANGUAGES C)

# Use the C11 standard (a modern version of the C language)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Find SDL2 and its extension libraries
# ---------------------------------------------------------------------------
# CMake will search your system for these libraries.
# If they are installed via vcpkg, MSYS2, or system package manager,
# CMake should find them automatically.

# SDL2 — the core graphics/input/audio library
find_package(SDL2 REQUIRED)

# SDL2_ttf — for rendering TrueType fonts (text on screen)
find_package(SDL2_ttf REQUIRED)

# SDL2_mixer — for playing sound effects and music
find_package(SDL2_mixer REQUIRED)

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
    ${SDL2_INCLUDE_DIRS}
    src
)

# ---------------------------------------------------------------------------
# Link libraries — connect our program to SDL2, SDL2_ttf, SDL2_mixer
# ---------------------------------------------------------------------------
# "Linking" means: make the compiled SDL2 code available to our program
# so we can call functions like SDL_CreateWindow(), TTF_RenderText(), etc.
target_link_libraries(tetris PRIVATE
    SDL2::SDL2
    SDL2::SDL2main
    SDL2_ttf::SDL2_ttf
    SDL2_mixer::SDL2_mixer
)

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

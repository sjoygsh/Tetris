#!/usr/bin/env bash
# ===========================================================================
# build.sh — One-command build & run script for the Tetris game (Linux)
# ===========================================================================
# This script does everything needed to go from a fresh clone to playing:
#   1. Installs SDL2 libraries (if missing)
#   2. Generates sound effects (if missing)
#   3. Downloads fonts (if missing)
#   4. Builds the game with CMake
#   5. Runs the game
#
# USAGE:
#   chmod +x build.sh    (make it executable — only needed once)
#   ./build.sh            (build and run)
#   ./build.sh --build    (build only, don't run)
#   ./build.sh --run      (run only, skip building)
# ===========================================================================

set -e  # Exit immediately if any command fails

# --- Colors for pretty output ---
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
BOLD='\033[1m'
NC='\033[0m'  # No Color

# --- Helper functions ---
info()    { echo -e "${BLUE}[INFO]${NC} $1"; }
success() { echo -e "${GREEN}[OK]${NC} $1"; }
warn()    { echo -e "${YELLOW}[WARN]${NC} $1"; }
error()   { echo -e "${RED}[ERROR]${NC} $1"; exit 1; }

# --- Navigate to the project root (where this script lives) ---
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# ===========================================================================
# Step 1: Install dependencies
# ===========================================================================
# Detects the Linux distribution and installs SDL2, SDL2_ttf, SDL2_mixer,
# CMake, and a C compiler if they aren't already installed.
# ===========================================================================
install_deps() {
    info "Checking dependencies..."

    # Check if SDL2 headers are already available
    if pkg-config --exists sdl2 SDL2_ttf SDL2_mixer 2>/dev/null && command -v cmake &>/dev/null && command -v cc &>/dev/null; then
        success "All dependencies already installed."
        return
    fi

    info "Installing missing dependencies..."

    # Detect the package manager and install accordingly
    if command -v apt-get &>/dev/null; then
        # Debian / Ubuntu / Linux Mint / Pop!_OS
        sudo apt-get update
        sudo apt-get install -y \
            build-essential cmake pkg-config \
            libsdl2-dev libsdl2-ttf-dev libsdl2-mixer-dev

    elif command -v dnf &>/dev/null; then
        # Fedora / RHEL 8+ / CentOS Stream
        sudo dnf install -y \
            gcc cmake pkgconf-pkg-config \
            SDL2-devel SDL2_ttf-devel SDL2_mixer-devel

    elif command -v pacman &>/dev/null; then
        # Arch Linux / Manjaro
        sudo pacman -Sy --needed --noconfirm \
            base-devel cmake pkgconf \
            sdl2 sdl2_ttf sdl2_mixer

    elif command -v zypper &>/dev/null; then
        # openSUSE
        sudo zypper install -y \
            gcc cmake pkg-config \
            libSDL2-devel libSDL2_ttf-devel libSDL2_mixer-devel

    elif command -v apk &>/dev/null; then
        # Alpine Linux
        sudo apk add \
            build-base cmake pkgconf \
            sdl2-dev sdl2_ttf-dev sdl2_mixer-dev

    else
        error "Could not detect your package manager.
Please install these packages manually:
  - A C compiler (gcc or clang)
  - CMake (version 3.15+)
  - SDL2 development libraries
  - SDL2_ttf development libraries
  - SDL2_mixer development libraries
Then re-run this script."
    fi

    success "Dependencies installed."
}

# ===========================================================================
# Step 2: Generate sound effects (if missing)
# ===========================================================================
# Creates simple WAV sound effects using Python. If Python isn't available,
# the game will still work — it just won't have sound.
# ===========================================================================
generate_sounds() {
    local ASSETS_DIR="assets"
    mkdir -p "$ASSETS_DIR"

    # Check if sounds already exist
    if [ -f "$ASSETS_DIR/move.wav" ] && [ -f "$ASSETS_DIR/rotate.wav" ] && \
       [ -f "$ASSETS_DIR/drop.wav" ] && [ -f "$ASSETS_DIR/clear.wav" ] && \
       [ -f "$ASSETS_DIR/gameover.wav" ]; then
        success "Sound effects already exist."
        return
    fi

    info "Generating sound effects..."

    if command -v python3 &>/dev/null; then
        python3 -c "
import struct, math, os

def gen_wav(path, freq, dur_ms, vol=0.5, wave='sine'):
    sr = 44100
    n = int(sr * dur_ms / 1000)
    amp = int(32767 * vol)
    samples = []
    for i in range(n):
        t = i / sr
        if wave == 'sine':
            v = math.sin(2 * math.pi * freq * t)
        else:
            v = 1.0 if math.sin(2 * math.pi * freq * t) >= 0 else -1.0
        fade = 1.0 - (i / n)
        samples.append(struct.pack('<h', int(v * amp * fade)))
    data = b''.join(samples)
    with open(path, 'wb') as f:
        f.write(b'RIFF')
        f.write(struct.pack('<I', 36 + len(data)))
        f.write(b'WAVEfmt ')
        f.write(struct.pack('<IHHIIHH', 16, 1, 1, sr, sr*2, 2, 16))
        f.write(b'data')
        f.write(struct.pack('<I', len(data)))
        f.write(data)

d = '$ASSETS_DIR'
gen_wav(f'{d}/move.wav',     400, 50,  0.3, 'square')
gen_wav(f'{d}/rotate.wav',   600, 80,  0.3, 'sine')
gen_wav(f'{d}/drop.wav',     150, 150, 0.5, 'square')
gen_wav(f'{d}/clear.wav',    800, 300, 0.4, 'sine')
gen_wav(f'{d}/gameover.wav', 200, 800, 0.5, 'sine')
print('  Sound effects generated.')
"
        success "Sound effects created."
    else
        warn "Python3 not found — skipping sound generation. Game will run silently."
    fi
}

# ===========================================================================
# Step 3: Download fonts (if missing)
# ===========================================================================
# Downloads the Nunito font family from fontsource CDN.
# Three weights: Regular, Bold, Light — used for the sidebar UI.
# ===========================================================================
download_fonts() {
    local ASSETS_DIR="assets"
    mkdir -p "$ASSETS_DIR"

    # Check if fonts already exist
    if [ -f "$ASSETS_DIR/font.ttf" ] && [ -f "$ASSETS_DIR/font_bold.ttf" ] && \
       [ -f "$ASSETS_DIR/font_light.ttf" ]; then
        success "Fonts already exist."
        return
    fi

    info "Downloading Nunito fonts..."

    # Pick a download tool (curl or wget)
    local DL=""
    if command -v curl &>/dev/null; then
        DL="curl"
    elif command -v wget &>/dev/null; then
        DL="wget"
    else
        warn "Neither curl nor wget found — skipping font download."
        warn "Please place font.ttf, font_bold.ttf, font_light.ttf in the assets/ folder."
        return
    fi

    # Nunito font URLs from fontsource CDN
    local BASE="https://cdn.jsdelivr.net/fontsource/fonts/nunito@latest"
    local FONTS=(
        "latin-400-normal.ttf:font.ttf"
        "latin-700-normal.ttf:font_bold.ttf"
        "latin-300-normal.ttf:font_light.ttf"
    )

    for entry in "${FONTS[@]}"; do
        local src="${entry%%:*}"
        local dst="${entry##*:}"
        local url="$BASE/$src"
        local path="$ASSETS_DIR/$dst"

        if [ -f "$path" ]; then
            continue
        fi

        if [ "$DL" = "curl" ]; then
            curl -fsSL "$url" -o "$path" 2>/dev/null
        else
            wget -q "$url" -O "$path" 2>/dev/null
        fi

        if [ $? -ne 0 ] || [ ! -s "$path" ]; then
            rm -f "$path"
            warn "Could not download $dst — trying DejaVu Sans as fallback..."

            # Fallback: use DejaVu Sans (widely available on Linux)
            local DEJAVU="/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
            local DEJAVU2="/usr/share/fonts/TTF/DejaVuSans.ttf"
            local DEJAVU3="/usr/share/fonts/dejavu-sans-fonts/DejaVuSans.ttf"

            for fallback in "$DEJAVU" "$DEJAVU2" "$DEJAVU3"; do
                if [ -f "$fallback" ]; then
                    cp "$fallback" "$path"
                    info "Using system DejaVu Sans for $dst"
                    break
                fi
            done
        fi
    done

    if [ -f "$ASSETS_DIR/font.ttf" ]; then
        success "Fonts ready."
    else
        warn "Fonts not found. The game needs .ttf fonts in assets/ to run."
    fi
}

# ===========================================================================
# Step 4: Build the game
# ===========================================================================
# Creates a "build" folder, runs CMake to generate makefiles, then compiles.
# ===========================================================================
build_game() {
    info "Building Tetris..."

    mkdir -p build
    cd build

    # Run CMake to generate build files
    cmake .. -DCMAKE_BUILD_TYPE=Release 2>&1 | tail -5

    # Compile (use all available CPU cores for speed)
    local CORES
    CORES=$(nproc 2>/dev/null || echo 2)
    cmake --build . --config Release -j "$CORES"

    cd ..

    # Verify the executable was created
    if [ -f "build/tetris" ]; then
        success "Build complete! Executable: build/tetris"
    else
        error "Build failed — executable not found."
    fi
}

# ===========================================================================
# Step 5: Run the game
# ===========================================================================
run_game() {
    local EXE="build/tetris"

    if [ ! -f "$EXE" ]; then
        error "Executable not found at $EXE — run './build.sh' to build first."
    fi

    echo ""
    echo -e "${BOLD}========================================${NC}"
    echo -e "${BOLD}          Starting Tetris!              ${NC}"
    echo -e "${BOLD}========================================${NC}"
    echo -e "  Arrow keys  = Move & Rotate"
    echo -e "  Space       = Hard drop"
    echo -e "  P           = Pause"
    echo -e "  R           = Restart"
    echo -e "  Esc         = Menu"
    echo -e "${BOLD}========================================${NC}"
    echo ""

    cd build
    ./tetris
}

# ===========================================================================
# Main — parse arguments and run the appropriate steps
# ===========================================================================
echo ""
echo -e "${BOLD}Tetris — Build Script${NC}"
echo "=============================="
echo ""

case "${1:-}" in
    --run)
        # Just run, don't build
        run_game
        ;;
    --build)
        # Build only, don't run
        install_deps
        generate_sounds
        download_fonts
        build_game
        success "Done! Run './build.sh --run' or 'cd build && ./tetris' to play."
        ;;
    *)
        # Default: build and run
        install_deps
        generate_sounds
        download_fonts
        build_game
        run_game
        ;;
esac

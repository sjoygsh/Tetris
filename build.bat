@echo off
REM ===========================================================================
REM build.bat — One-command build & run script for the Tetris game (Windows)
REM ===========================================================================
REM This script does everything needed to go from a fresh clone to playing:
REM   1. Installs vcpkg (if missing)
REM   2. Installs SDL2 libraries via vcpkg (if missing)
REM   3. Generates sound effects (if missing)
REM   4. Downloads fonts (if missing)
REM   5. Builds the game with CMake
REM   6. Runs the game
REM
REM USAGE:
REM   build.bat            (build and run)
REM   build.bat --build    (build only, don't run)
REM   build.bat --run      (run only, skip building)
REM
REM REQUIREMENTS:
REM   - Git (for cloning vcpkg)
REM   - CMake (3.15+)
REM   - Visual Studio or Build Tools with C/C++ workload
REM ===========================================================================

setlocal enabledelayedexpansion

REM --- Navigate to the folder where this script lives ---
cd /d "%~dp0"

echo.
echo ==============================
echo  Tetris — Build Script
echo ==============================
echo.

REM --- Parse arguments ---
if "%~1"=="--run" goto :run_game
if "%~1"=="--build" (
    call :all_build_steps
    echo.
    echo [OK] Done! Run "build.bat --run" or "build\Release\tetris.exe" to play.
    goto :eof
)

REM Default: build and run
call :all_build_steps
goto :run_game

REM ===========================================================================
REM all_build_steps — runs install, assets, and build in order
REM ===========================================================================
:all_build_steps
    call :check_tools
    call :install_vcpkg
    call :install_sdl2
    call :generate_sounds
    call :download_fonts
    call :build_game
    goto :eof

REM ===========================================================================
REM Step 0: Check that Git and CMake are available
REM ===========================================================================
:check_tools
    echo [INFO] Checking for required tools...

    where git >nul 2>&1
    if errorlevel 1 (
        echo [ERROR] Git is not installed or not in PATH.
        echo         Download from: https://git-scm.com/download/win
        exit /b 1
    )

    where cmake >nul 2>&1
    if errorlevel 1 (
        echo [ERROR] CMake is not installed or not in PATH.
        echo         Download from: https://cmake.org/download/
        exit /b 1
    )

    echo [OK] Git and CMake found.
    goto :eof

REM ===========================================================================
REM Step 1: Install vcpkg (C/C++ package manager)
REM ===========================================================================
REM vcpkg makes it easy to install SDL2 and friends on Windows.
REM We clone it into a "vcpkg" folder inside the project if not present.
REM ===========================================================================
:install_vcpkg
    if exist "vcpkg\vcpkg.exe" (
        echo [OK] vcpkg already installed.
        goto :eof
    )

    REM Check for a system-wide vcpkg installation
    if defined VCPKG_ROOT (
        if exist "%VCPKG_ROOT%\vcpkg.exe" (
            echo [OK] Using system vcpkg at %VCPKG_ROOT%
            goto :eof
        )
    )

    echo [INFO] Installing vcpkg...
    git clone https://github.com/microsoft/vcpkg.git vcpkg
    if errorlevel 1 (
        echo [ERROR] Failed to clone vcpkg.
        exit /b 1
    )

    call vcpkg\bootstrap-vcpkg.bat -disableMetrics
    if errorlevel 1 (
        echo [ERROR] Failed to bootstrap vcpkg.
        exit /b 1
    )

    echo [OK] vcpkg installed.
    goto :eof

REM ===========================================================================
REM Step 2: Install SDL2, SDL2_ttf, SDL2_mixer via vcpkg
REM ===========================================================================
:install_sdl2
    REM Determine which vcpkg to use
    set "VCPKG_EXE="
    if exist "vcpkg\vcpkg.exe" set "VCPKG_EXE=vcpkg\vcpkg.exe"
    if defined VCPKG_ROOT (
        if exist "%VCPKG_ROOT%\vcpkg.exe" set "VCPKG_EXE=%VCPKG_ROOT%\vcpkg.exe"
    )

    if "!VCPKG_EXE!"=="" (
        echo [ERROR] vcpkg not found. Run the full build first.
        exit /b 1
    )

    REM Check if SDL2 is already installed (look for the triplet)
    "!VCPKG_EXE!" list sdl2 2>nul | findstr /i "sdl2:" >nul 2>&1
    if not errorlevel 1 (
        echo [OK] SDL2 libraries already installed.
        goto :eof
    )

    echo [INFO] Installing SDL2 libraries (this may take a few minutes)...
    "!VCPKG_EXE!" install sdl2 sdl2-ttf sdl2-mixer --triplet x64-windows
    if errorlevel 1 (
        echo [ERROR] Failed to install SDL2 packages.
        exit /b 1
    )

    echo [OK] SDL2 libraries installed.
    goto :eof

REM ===========================================================================
REM Step 3: Generate sound effects (if missing)
REM ===========================================================================
:generate_sounds
    if not exist "assets" mkdir assets

    if exist "assets\move.wav" if exist "assets\rotate.wav" if exist "assets\drop.wav" if exist "assets\clear.wav" if exist "assets\gameover.wav" (
        echo [OK] Sound effects already exist.
        goto :eof
    )

    echo [INFO] Generating sound effects...

    where python >nul 2>&1
    if errorlevel 1 (
        where python3 >nul 2>&1
        if errorlevel 1 (
            echo [WARN] Python not found — skipping sound generation. Game will run silently.
            goto :eof
        )
        set "PY=python3"
    ) else (
        set "PY=python"
    )

    !PY! -c "import struct,math,os;d='assets';os.makedirs(d,exist_ok=True);exec(\"def g(p,f,dur,v=0.5,w='sine'):\n sr=44100;n=int(sr*dur/1000);a=int(32767*v);s=[]\n for i in range(n):\n  t=i/sr\n  if w=='sine':val=math.sin(2*math.pi*f*t)\n  else:val=1.0 if math.sin(2*math.pi*f*t)>=0 else -1.0\n  s.append(struct.pack('<h',int(val*a*(1.0-i/n))))\n data=b''.join(s)\n with open(p,'wb') as o:\n  o.write(b'RIFF'+struct.pack('<I',36+len(data))+b'WAVEfmt '+struct.pack('<IHHIIHH',16,1,1,sr,sr*2,2,16)+b'data'+struct.pack('<I',len(data))+data)\");g(f'{d}/move.wav',400,50,.3,'sq');g(f'{d}/rotate.wav',600,80,.3,'sine');g(f'{d}/drop.wav',150,150,.5,'sq');g(f'{d}/clear.wav',800,300,.4,'sine');g(f'{d}/gameover.wav',200,800,.5,'sine');print('  Sound effects generated.')"

    if errorlevel 1 (
        echo [WARN] Sound generation failed — game will run without sound.
    ) else (
        echo [OK] Sound effects created.
    )
    goto :eof

REM ===========================================================================
REM Step 4: Download fonts (if missing)
REM ===========================================================================
:download_fonts
    if not exist "assets" mkdir assets

    if exist "assets\font.ttf" if exist "assets\font_bold.ttf" if exist "assets\font_light.ttf" (
        echo [OK] Fonts already exist.
        goto :eof
    )

    echo [INFO] Downloading Nunito fonts...

    REM Use PowerShell to download (available on all modern Windows)
    set "BASE=https://cdn.jsdelivr.net/fontsource/fonts/nunito@latest"

    if not exist "assets\font.ttf" (
        powershell -Command "try { [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri '%BASE%/latin-400-normal.ttf' -OutFile 'assets\font.ttf' -UseBasicParsing } catch { Write-Host '[WARN] Could not download font.ttf' }" 2>nul
    )
    if not exist "assets\font_bold.ttf" (
        powershell -Command "try { [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri '%BASE%/latin-700-normal.ttf' -OutFile 'assets\font_bold.ttf' -UseBasicParsing } catch { Write-Host '[WARN] Could not download font_bold.ttf' }" 2>nul
    )
    if not exist "assets\font_light.ttf" (
        powershell -Command "try { [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri '%BASE%/latin-300-normal.ttf' -OutFile 'assets\font_light.ttf' -UseBasicParsing } catch { Write-Host '[WARN] Could not download font_light.ttf' }" 2>nul
    )

    if exist "assets\font.ttf" (
        echo [OK] Fonts ready.
    ) else (
        echo [WARN] Font download failed. Place font.ttf, font_bold.ttf, font_light.ttf in assets\
    )
    goto :eof

REM ===========================================================================
REM Step 5: Build the game with CMake
REM ===========================================================================
:build_game
    echo [INFO] Building Tetris...

    if not exist "build" mkdir build

    REM Determine vcpkg toolchain path
    set "TOOLCHAIN="
    if exist "vcpkg\scripts\buildsystems\vcpkg.cmake" (
        set "TOOLCHAIN=-DCMAKE_TOOLCHAIN_FILE=vcpkg\scripts\buildsystems\vcpkg.cmake"
    )
    if defined VCPKG_ROOT (
        if exist "%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" (
            set "TOOLCHAIN=-DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake"
        )
    )

    REM Run CMake configure
    cmake -S . -B build !TOOLCHAIN! -DCMAKE_BUILD_TYPE=Release
    if errorlevel 1 (
        echo [ERROR] CMake configuration failed.
        exit /b 1
    )

    REM Build
    cmake --build build --config Release
    if errorlevel 1 (
        echo [ERROR] Build failed.
        exit /b 1
    )

    REM Find the executable
    if exist "build\Release\tetris.exe" (
        echo [OK] Build complete! Executable: build\Release\tetris.exe
    ) else if exist "build\tetris.exe" (
        echo [OK] Build complete! Executable: build\tetris.exe
    ) else (
        echo [ERROR] Build seemed to succeed but executable not found.
        exit /b 1
    )
    goto :eof

REM ===========================================================================
REM Step 6: Run the game
REM ===========================================================================
:run_game
    echo.
    echo ========================================
    echo           Starting Tetris!
    echo ========================================
    echo   Arrow keys  = Move ^& Rotate
    echo   Space       = Hard drop
    echo   P           = Pause
    echo   R           = Restart
    echo   Esc         = Menu
    echo ========================================
    echo.

    if exist "build\Release\tetris.exe" (
        cd build\Release
        start "" tetris.exe
    ) else if exist "build\tetris.exe" (
        cd build
        start "" tetris.exe
    ) else (
        echo [ERROR] Executable not found. Run "build.bat" to build first.
        exit /b 1
    )
    goto :eof

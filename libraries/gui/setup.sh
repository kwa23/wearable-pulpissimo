#!/bin/bash
# =============================================================================
# NusaCore GUI Framework — Setup Script
# ICDeC RISC-V Wearable Project
# =============================================================================

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

CHROOT_ARCHIVE="${1:-$SCRIPT_DIR/chroot.tar.xz}"
PULP_REPO="${2:-}"

echo "================================================"
echo "           GUI Framework Setup"
echo "      ICDeC RISC-V Wearable Project"
echo "================================================"
echo ""

# ---------------------------------------------------------------------------
# 1. Check Linux build prerequisites
# ---------------------------------------------------------------------------
echo "[1/4] Checking Linux build prerequisites..."
ALL_OK=1

# Check standard command-line tools
for cmd in gcc git cmake pkg-config python3 pip; do
    if command -v "$cmd" &>/dev/null; then
        echo "  OK : $cmd found"
    else
        # Friendly mapping for pip if it's missing via python3-pip
        if [ "$cmd" = "pip" ]; then
            echo "  MISSING: python3-pip"
        else
            echo "  MISSING: $cmd"
        fi
        ALL_OK=0
    fi
done

# Check Ubuntu/Debian specific packages (Libraries, headers, and venv)
if [ -f /etc/debian_version ]; then
    echo "Checking Ubuntu/Debian development packages..."

    # List of libraries/packages to verify via dpkg
    DEB_PKGS=(
        "build-essential"
        "python3.12-venv"
        "libwayland-dev"
        "libxkbcommon-dev"
        "libwayland-bin"
        "wayland-protocols"
    )

    for pkg in "${DEB_PKGS[@]}"; do
        if dpkg-query -W -f='${Status}' "$pkg" 2>/dev/null | grep -q "ok installed"; then
            echo "  OK : $pkg is installed"
        else
            echo "  MISSING: $pkg"
            ALL_OK=0
        fi
    done
fi

# Check SDL2 configuration tool
if command -v sdl2-config &>/dev/null; then
    echo "  OK : SDL2 found ($(sdl2-config --version))"
else
    echo "  MISSING: SDL2 development library"
    echo "    Ubuntu/Debian : sudo apt install libsdl2-dev"
    echo "    CachyOS/Arch  : sudo pacman -S sdl2"
    ALL_OK=0
fi

# Final warning output
if [ "$ALL_OK" -eq 0 ]; then
    echo ""
    echo "  WARN: Install missing packages before running 'make PLATFORM=linux'"
    echo "  To fix missing Ubuntu packages, run:"
    echo "  sudo apt update && sudo apt install -y build-essential cmake pkg-config python3-pip python3.12-venv libwayland-dev libxkbcommon-dev libwayland-bin wayland-protocols"
fi

# ---------------------------------------------------------------------------
# 2. Check RISC-V toolchain PATH
# ---------------------------------------------------------------------------
echo ""
echo "[2/4] Verifying RISC-V toolchain path..."

# Name of the cross-compiler binary to look for
RISCV_GCC="riscv32-unknown-elf-gcc"

if command -v "$RISCV_GCC" &>/dev/null; then
    # Automatically resolve the absolute bin directory
    GCC_PATH=$(command -v "$RISCV_GCC")
    TOOLCHAIN_BIN=$(dirname "$GCC_PATH")

    echo "  OK: Toolchain found in PATH"
    echo "      Binary: $GCC_PATH"
    echo "      Directory: $TOOLCHAIN_BIN"
else
    # Calculate the expected fallback path for the instruction text
    EXPECTED_BIN_DIR="$SCRIPT_DIR/chroot/bin"

    echo "  ERROR: $RISCV_GCC not found in your PATH."
    echo "  Please add the chroot toolchain to your environment variables."
    echo ""
    echo "  To fix this for your current terminal session, run:"
    echo "    export PATH=\"\$PATH:$EXPECTED_BIN_DIR\""
    echo ""
    echo "  To make this permanent, add that line to your ~/.bashrc or ~/.zshrc"

    ALL_OK=0
fi

# ---------------------------------------------------------------------------
# 3. Verify pulp-runtime Environment (PULPRT_HOME)
# ---------------------------------------------------------------------------
echo ""
echo "[3/4] Verifying pulp-runtime configuration..."

if [ -n "$PULPRT_HOME" ]; then
    # Check if the directory pointed to by PULPRT_HOME actually exists
    if [ -d "$PULPRT_HOME" ]; then
        echo "  OK : PULPRT_HOME is active"
        echo "       Path: $PULPRT_HOME"
    else
        echo "  ERROR: PULPRT_HOME is set, but the directory does not exist!"
        echo "         Current value: $PULPRT_HOME"
        echo "         Please check your path or re-run the configuration scripts."
        ALL_OK=0
    fi
else
    echo "  ERROR: PULPRT_HOME environment variable is not set."
    echo "         You need to activate the pulpissimo configuration first."
    echo ""
    echo "  Please run your mentor's configuration commands:"
    echo "    . <path_to_pulp-runtime>/configs/pulpissimo.sh"
    echo "    . <path_to_pulp-runtime>/configs/fpgas/pulpissimo/genesys2.sh"

    ALL_OK=0
fi

# ---------------------------------------------------------------------------
# 4. Check and Setup LVGL Library
# ---------------------------------------------------------------------------
echo ""
echo "[4/4] Setting up LVGL library..."

# Updated destination path as requested
LVGL_DIR="$SCRIPT_DIR/libs/lvgl"

if [ -d "$LVGL_DIR" ] && [ -f "$LVGL_DIR/lvgl.h" ]; then
    echo "  SKIP: LVGL library already present at $LVGL_DIR"
elif [ -f "$SCRIPT_DIR/.gitmodules" ]; then
    echo "  LVGL missing, but .gitmodules found. Initialising submodule..."
    # Ensure the parent directory structure exists first
    mkdir -p "$(dirname "$LVGL_DIR")"
    git -C "$SCRIPT_DIR" submodule update --init --depth 1
    echo "  OK: LVGL submodule initialised"
else
    echo "  LVGL missing and .gitmodules not found — cloning LVGL directly..."
    # Create the directory path if it doesn't exist yet
    mkdir -p "$(dirname "$LVGL_DIR")"

    git clone --depth=1 --branch release/v9.2 \
        https://github.com/lvgl/lvgl.git "$LVGL_DIR"
    echo "  OK: LVGL cloned to $LVGL_DIR"
fi

# ---------------------------------------------------------------------------
# Setup Verification Result
# ---------------------------------------------------------------------------
echo ""
echo "================================================================"
if [ "$ALL_OK" -eq 1 ]; then
    echo "  Setup Verification: SUCCESS!"
    echo "  All prerequisites, toolchains, and libraries are ready."
    echo "================================================================"
    echo ""
    echo "  To build for Linux (SDL2 window):"
    echo "    make PLATFORM=linux"
    echo "    ./gui_app"
    echo ""
    echo "  To run unit tests (no hardware):"
    echo "    cd tests && make"
    echo ""
    echo "  To build for RISC-V (NusaCore board):"
    echo "    make PLATFORM=riscv"
else
    echo "  Setup Verification: FAILED!"
    echo "  Please resolve the missing dependencies listed above."
    echo "================================================================"
    exit 1
fi

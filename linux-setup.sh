#!/bin/bash

# © XeyyzuV2

echo "Gurotopia installer 2025-9-23 sha-48701e5f8a9082e3f2b6c42cd29fecdc3c488f95"
echo

# Check if the script is run with root privileges (sudo)
if [ "$(id -u)" -ne 0 ]; then
  echo "Error: This script must be run with root privileges." >&2
  echo "Please try again using: sudo ./installer.sh" >&2
  exit 1
fi

# Check if the OS is Debian-based
if [ -f /etc/debian_version ]; then
    echo "Debian-based system detected. Proceeding with installation using apt-get..."
    echo

    # 1. Initial package list update
    echo "[1/4] Running initial apt-get update..."
    apt-get update -y
    if [ $? -ne 0 ]; then
        echo "Failed to update package list. Please check your internet connection or try again later." >&2
        exit 1
    fi
    echo "Initial update finished."
    echo

    # 2. Install software-properties-common to manage PPAs
    echo "[2/4] Installing software-properties-common..."
    apt-get install -y software-properties-common
    if [ $? -ne 0 ]; then
        echo "Failed to install software-properties-common." >&2
        exit 1
    fi
    echo "software-properties-common installed."
    echo

    # 3. Add PPA for modern GCC and update again
    echo "[3/4] Adding PPA for modern GCC (ppa:ubuntu-toolchain-r/test)..."
    add-apt-repository ppa:ubuntu-toolchain-r/test -y
    if [ $? -ne 0 ]; then
        echo "Failed to add PPA. Please check the output above for details." >&2
        exit 1
    fi
    echo "PPA added successfully. Running apt-get update again..."
    apt-get update -y
    echo "Update finished."
    echo

    # 4. Install all dependencies including g++-13
    echo "[4/4] Installing dependencies: build-essential libssl-dev openssl sqlite3 libsqlite3-dev g++-13..."
    apt-get install -y build-essential libssl-dev openssl sqlite3 libsqlite3-dev g++-13
    if [ $? -ne 0 ]; then
        echo "Failed to install dependencies. Please check the output above for error details." >&2
        exit 1
    fi
    echo "Dependency installation finished."
    echo

    # 5. Set g++-13 as default gcc/g++ using update-alternatives
    echo "[5/5] Setting g++-13 as default compiler..."
    if [ -f /usr/bin/gcc-13 ] && [ -f /usr/bin/g++-13 ]; then
        update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 100
        update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 100
        update-alternatives --set gcc /usr/bin/gcc-13
        update-alternatives --set g++ /usr/bin/g++-13
        echo "g++-13 set as default compiler."
    else
        echo "Warning: gcc-13 or g++-13 not found. Please check if g++-13 was installed correctly."
    fi
    echo

    echo "======================================"
    echo "Installation Successful!"
    echo "You can now compile the server by running the command: make -j$(nproc)"
    echo "======================================"

# Check if the OS is Arch-based
elif [ -f /etc/arch-release ]; then
    echo "Arch-based system detected. Proceeding with installation using pacman..."
    echo

    # 1. Update package database
    echo "[1/4] Updating package database..."
    pacman -Sy --noconfirm
    if [ $? -ne 0 ]; then
        echo "Failed to update package database. Please check your internet connection or try again later." >&2
        exit 1
    fi
    echo "Package database updated."
    echo

    # 2. Install base development tools
    echo "[2/4] Installing base-devel..."
    pacman -S --noconfirm --needed base-devel
    if [ $? -ne 0 ]; then
        echo "Failed to install base-devel." >&2
        exit 1
    fi
    echo "base-devel installed."
    echo

    # 3. Install additional dependencies
    echo "[3/4] Installing additional dependencies: openssl sqlite..."
    pacman -S --noconfirm --needed openssl sqlite
    if [ $? -ne 0 ]; then
        echo "Failed to install additional dependencies." >&2
        exit 1
    fi
    echo "Additional dependencies installed."
    echo

    echo "======================================"
    echo "Installation Successful!"
    echo "You can now compile the server by running the command: make -j$(nproc)"
    echo "======================================"
else
    echo "Error: This script supports Debian-based (Ubuntu, Debian, Mint) and Arch-based (Arch, Manjaro) systems." >&2
    echo "Please install the following dependencies manually for your distribution:" >&2
    echo "  - A modern C++ compiler (g++ version 13+)" >&2
    echo "  - build-essential (or base-devel on Arch)" >&2
    echo "  - libssl-dev" >&2
    echo "  - openssl" >&2
    echo "  - sqlite3 (or sqlite on Arch)" >&2
    exit 1
fi

exit 0

#!/usr/bin/env bash
set -euo pipefail

echo "Installing basic RH56DFX development dependencies..."

sudo apt update
sudo apt install -y \
    build-essential \
    cmake \
    git \
    libboost-all-dev \
    libspdlog-dev

mkdir -p "$HOME/workspace"

echo
echo "Basic environment ready."
echo
echo "Next:"
echo "  1. clone/build unitree_sdk2"
echo "  2. clone/build dfx_inspire_service"
echo "  3. run scripts/check_hardware.sh"
echo
echo "This script intentionally does not clone or pin upstream repositories."
echo "Record known-good commits after validation."

#!/usr/bin/env bash
set -u

echo "========================================"
echo " G1 + RH56DFX Robot-Side Check"
echo "========================================"

echo
echo "[1] System"
uname -a
echo
if command -v lsb_release >/dev/null 2>&1; then
    lsb_release -a 2>/dev/null || true
fi

echo
echo "[2] Stable RH56 aliases"
if compgen -G "/dev/inspire_*" > /dev/null; then
    ls -l /dev/inspire_*
else
    echo "No /dev/inspire_* aliases found."
    echo "Install verified udev rules after identifying both hands."
fi

echo
echo "[3] ttyUSB devices"
if compgen -G "/dev/ttyUSB*" > /dev/null; then
    ls -l /dev/ttyUSB*
else
    echo "No /dev/ttyUSB* devices found."
fi

echo
echo "[4] Processes using serial ports"
for dev in /dev/ttyUSB*; do
    [ -e "$dev" ] || continue
    echo "--- $dev"
    if command -v fuser >/dev/null 2>&1; then
        fuser "$dev" 2>/dev/null || echo "not currently opened by a process"
    fi
done

echo
echo "[5] Network interfaces"
ip -br addr 2>/dev/null || ip addr

echo
echo "[6] Routes"
ip route || true

echo
echo "[7] Unitree-related libraries"
ldconfig -p 2>/dev/null | grep -i unitree || echo "No Unitree library found through ldconfig."

echo
echo "[8] Workspace"
for d in "$HOME/workspace/unitree_sdk2" "$HOME/workspace/dfx_inspire_service"; do
    if [ -d "$d" ]; then
        echo "[OK] $d"
    else
        echo "[MISSING] $d"
    fi
done

echo
echo "[9] Next useful commands"
echo "udevadm info -a -n /dev/ttyUSBX"
echo "git -C ~/workspace/unitree_sdk2 rev-parse HEAD"
echo "git -C ~/workspace/dfx_inspire_service rev-parse HEAD"

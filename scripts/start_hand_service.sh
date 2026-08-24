#!/usr/bin/env bash
set -euo pipefail

SERVICE_DIR="${DFX_INSPIRE_SERVICE_DIR:-$HOME/workspace/dfx_inspire_service/build}"

if [ ! -d "$SERVICE_DIR" ]; then
    echo "Missing build directory:"
    echo "  $SERVICE_DIR"
    exit 1
fi

if [ ! -x "$SERVICE_DIR/inspire_g1" ]; then
    echo "Missing executable:"
    echo "  $SERVICE_DIR/inspire_g1"
    exit 1
fi

echo "Starting G1 RH56DFX service."
echo
echo "Before continuing, verify:"
echo "  - serial device paths"
echo "  - right/left mapping"
echo "  - udev aliases if configured"
echo

cd "$SERVICE_DIR"
exec sudo ./inspire_g1

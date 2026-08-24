#!/usr/bin/env bash
set -u

RIGHT="/dev/serial/by-id/usb-FTDI_USB__-__Serial_Converter_FTABQDTD-if01-port0"
LEFT="/dev/serial/by-id/usb-FTDI_USB__-__Serial_Converter_FTABQDTD-if02-port0"

echo "G1 RH56DFX hardware check"
for item in "right:$RIGHT" "left:$LEFT"; do
    name="${item%%:*}"
    device="${item#*:}"
    if [[ -e "$device" ]]; then
        echo "$name: $device -> $(readlink -f "$device")"
        [[ -r "$device" && -w "$device" ]] \
            && echo "  access: rw" \
            || echo "  access: denied"
        if command -v fuser >/dev/null 2>&1; then
            users="$(fuser "$device" 2>/dev/null || true)"
            [[ -z "$users" ]] || echo "  in use by PID:$users"
        fi
    else
        echo "$name: missing ($device)"
    fi
done

echo "DDS topics: rt/inspire/cmd, rt/inspire/state"

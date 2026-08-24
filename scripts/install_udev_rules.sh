#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
RULE_FILE="$ROOT_DIR/udev/99-inspire-hands.rules"

echo "WARNING"
echo "-------"
echo "The udev file is only a template."
echo "Do not install it before replacing the TODO identifiers with attributes"
echo "measured on the actual G1."
echo
echo "Inspect devices with:"
echo "  udevadm info -a -n /dev/ttyUSB1"
echo "  udevadm info -a -n /dev/ttyUSB2"
echo

read -r -p "Install the edited rule now? [y/N] " answer

case "$answer" in
    y|Y)
        if grep -q "TODO_" "$RULE_FILE"; then
            echo "Refusing installation: TODO identifiers still exist."
            exit 1
        fi

        sudo cp "$RULE_FILE" /etc/udev/rules.d/99-inspire-hands.rules
        sudo udevadm control --reload-rules
        sudo udevadm trigger

        echo "Installed."
        echo "Replug the adapters if aliases do not appear."
        ;;
    *)
        echo "Cancelled."
        ;;
esac

#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SELECTION="${1:-both}"
HOLD_BIN="$ROOT_DIR/build/src/hand_hold_position"
PID_FILE="$ROOT_DIR/run/hand_controller.pid"

case "$SELECTION" in
    left|right|both) ;;
    *)
        echo "Usage: $0 [left|right|both]" >&2
        exit 2
        ;;
esac

if [[ -f "$PID_FILE" ]]; then
    pid="$(<"$PID_FILE")"
    if [[ "$pid" =~ ^[0-9]+$ ]] && [[ -d "/proc/$pid" ]]; then
        executable="$(readlink -f "/proc/$pid/exe" 2>/dev/null || true)"
        case "$executable" in
            "$ROOT_DIR"/build/*)
                kill -TERM "$pid" 2>/dev/null || true
                for _ in 1 2 3 4 5; do
                    [[ ! -d "/proc/$pid" ]] && break
                    sleep 0.05
                done
                [[ -d "/proc/$pid" ]] && kill -KILL "$pid" 2>/dev/null || true
                ;;
            *)
                echo "Refusing to kill unrecognized PID $pid ($executable)" >&2
                ;;
        esac
    fi
    rm -f "$PID_FILE"
fi

if [[ ! -x "$HOLD_BIN" ]]; then
    echo "Missing $HOLD_BIN" >&2
    echo "Build with: cmake --build $ROOT_DIR/build -j2" >&2
    exit 1
fi

exec "$HOLD_BIN" "$SELECTION"

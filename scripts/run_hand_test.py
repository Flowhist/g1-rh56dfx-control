#!/usr/bin/env python3

import argparse
import math
import subprocess
import sys
from pathlib import Path

import yaml


JOINTS = {
    "pinky",
    "ring",
    "middle",
    "index",
    "thumb-bend",
    "thumb-rotation",
}
CONTROL_PERIOD_MS = 250


def load_config(path: Path) -> dict:
    with path.open("r", encoding="utf-8") as stream:
        data = yaml.safe_load(stream)
    if not isinstance(data, dict):
        raise ValueError("configuration must be a YAML mapping")

    required = {"hand", "joint", "delta", "move_duration_ms", "hold_ms"}
    missing = required - data.keys()
    unknown = data.keys() - required
    if missing:
        raise ValueError(f"missing keys: {', '.join(sorted(missing))}")
    if unknown:
        raise ValueError(f"unknown keys: {', '.join(sorted(unknown))}")

    hand = data["hand"]
    joint = data["joint"]
    delta = float(data["delta"])
    move_duration_ms = int(data["move_duration_ms"])
    hold_ms = int(data["hold_ms"])

    if hand not in {"left", "right"}:
        raise ValueError("hand must be left or right")
    if joint not in JOINTS:
        raise ValueError(f"joint must be one of: {', '.join(sorted(JOINTS))}")
    if not math.isfinite(delta) or delta == 0:
        raise ValueError("delta must be finite and non-zero")
    if move_duration_ms < 0:
        raise ValueError("move_duration_ms must be nonnegative")
    if hold_ms < 0:
        raise ValueError("hold_ms must be nonnegative")

    steps = max(1, round(move_duration_ms / CONTROL_PERIOD_MS))
    step = abs(delta) / steps
    step_delay_ms = round(move_duration_ms / steps)

    return {
        "hand": hand,
        "joint": joint,
        "delta": delta,
        "hold_ms": hold_ms,
        "step": step,
        "step_delay_ms": step_delay_ms,
    }


def main() -> int:
    root = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser(description="Run a configured RH56 joint test")
    parser.add_argument(
        "--config",
        type=Path,
        default=root / "config" / "hand_test.yaml",
    )
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument(
        "--execute",
        action="store_true",
        help="send hardware commands (kept for compatibility; this is the default)",
    )
    mode.add_argument(
        "--dry-run",
        action="store_true",
        help="validate and print the command without moving the hand",
    )
    args = parser.parse_args()

    try:
        config = load_config(args.config)
    except (OSError, ValueError, yaml.YAMLError) as error:
        print(f"Invalid configuration: {error}", file=sys.stderr)
        return 2

    binary = root / "build" / "examples" / "hand_single_joint_safe"
    if not binary.is_file():
        print(f"Missing executable: {binary}", file=sys.stderr)
        return 1

    command = [
        str(binary),
        "--hand",
        config["hand"],
        "--joint",
        config["joint"],
        "--delta",
        str(config["delta"]),
        "--hold-ms",
        str(config["hold_ms"]),
        "--step",
        str(config["step"]),
        "--step-delay-ms",
        str(config["step_delay_ms"]),
    ]
    execute = not args.dry_run
    if execute:
        command.append("--execute")

    mode = "EXECUTE" if execute else "DRY RUN"
    print(
        f"{mode}: hand={config['hand']} joint={config['joint']} "
        f"delta={config['delta']:+.3f} "
        f"step={config['step']:.3f} delay={config['step_delay_ms']}ms",
        flush=True,
    )
    return subprocess.run(command, cwd=root, check=False).returncode


if __name__ == "__main__":
    raise SystemExit(main())

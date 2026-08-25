# Unitree G1 · Inspire RH56DFX

Minimal dual-hand compliant-teaching controller for the RH56DFX.

## Hardware

| Hand | Serial interface | RH56 ID |
|---|---|---:|
| Right | FTDI `if01` | 1 |
| Left | FTDI `if02` | 1 |

Position convention: `q=0` closed, `q=1` open. Per-hand joint order:
`pinky, ring, middle, index, thumb-bend, thumb-rotation`.

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target hand_compliant_teach hand_hold_position -j2
```

Requires Eigen and yaml-cpp.

## Compliant teaching

All controller settings are in [`config/compliant_teach.yaml`](config/compliant_teach.yaml).

Dry run:

```bash
./build/src/hand_compliant_teach --hand both --joint all --profile slow
```

Execute:

```bash
./build/src/hand_compliant_teach \
  --hand both --joint all --profile slow --duration 60 --execute
```

Available profiles: `slow`, `medium`, `fast`. Use `Ctrl+C` to stop.

Best-effort software stop:

```bash
./scripts/hand_estop.sh both
```

This cannot replace power removal or the physical emergency stop.

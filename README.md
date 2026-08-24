# Unitree G1 + Inspire RH56DFX — Jetson-First Onboarding

This package is the recommended starting point for developing two Inspire RH56DFX dexterous hands on a Unitree G1.

## Default development model

**Develop and run the hand-control stack directly on the G1 onboard Jetson.**

Your Windows/macOS/Linux development machine is only used as a development terminal/IDE through SSH or VS Code Remote SSH.

```text
Developer PC
    |
    | SSH / Remote IDE
    v
G1 Jetson
├── source code
├── unitree_sdk2
├── dfx_inspire_service
├── hand wrapper / safety layer
└── RH56DFX runtime
        |
        | RS485
        v
   Left + Right RH56DFX
```

No WSL or local Unitree SDK installation is required for the basic hand-development workflow.

Later, if a VLA or other heavy model runs on an external GPU machine:

```text
External GPU PC
VLA / Teleoperation / Planner
        |
        | Ethernet + DDS
        v
G1 Jetson
Hand Service / Safety / Hardware
        |
        v
RH56DFX
```

SSH is for development, deployment, startup and debugging.

DDS is the runtime robot-control communication path.

---

## First-day workflow

1. Read:
   - `docs/01_ARCHITECTURE.md`
   - `docs/02_JETSON_DEVELOPMENT.md`
   - `docs/03_RH56DFX_DEVELOPER_GUIDE.md`
2. SSH into the G1 Jetson.
3. Run:
   ```bash
   bash scripts/check_hardware.sh
   ```
4. Verify both RH56DFX serial devices.
5. Install/verify `unitree_sdk2`.
6. Build/verify `dfx_inspire_service`.
7. Run Unitree's official hand example.
8. Run the minimal examples in `examples/`.
9. Confirm the canonical 12-D hand interface in `interface/HAND_INTERFACE.md`.
10. Only then integrate ROS 2, teleoperation or VLA.

---

## Project philosophy

The RH56DFX stack is treated as a small, independent robot-side subsystem.

The upper-level application should only see:

```text
command: q[12]
state:   q[12]
```

and should not need to know:

```text
/dev/ttyUSB*
RS485 registers
baud rate
USB enumeration
RH56 low-level packet format
```

---

## Canonical hand ordering

```text
0   right_pinky
1   right_ring
2   right_middle
3   right_index
4   right_thumb_bend
5   right_thumb_rotation

6   left_pinky
7   left_ring
8   left_middle
9   left_index
10  left_thumb_bend
11  left_thumb_rotation
```

Current Unitree example convention:

```text
0.0 = close
1.0 = open
```

---

## Important

`udev/99-inspire-hands.rules` is a template and MUST be customized on the actual G1 before installation.

Do not permanently depend on `/dev/ttyUSB1` and `/dev/ttyUSB2`.

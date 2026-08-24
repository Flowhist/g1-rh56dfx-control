# Project Core

- Unitree G1 dexterous-hand development on the robot's Jetson; hardware is Inspire Robots RH56DFX dual hands.
- Project root: `/home/unitree/rh56dfx/workspace`. Ask before global environment changes or writes outside this root.
- Main implementation is vendored from Unitree's official `unitreerobotics/dfx_inspire_service` at commit `d6c4eae9bc959990bf87c262f29f62142519e923` (BSD-3-Clause). Local changes only add stable configurable G1 serial paths and zero initialization.
- Official Unitree contract: command/state `q[12]` in `[0,1]`, with 0=closed and 1=open; RH56 raw ANGLE uses the same direction, so encode as `q*1000` and decode as `raw/1000`.
- Canonical order: right_pinky, right_ring, right_middle, right_index, right_thumb_bend, right_thumb_rotation, left_pinky, left_ring, left_middle, left_index, left_thumb_bend, left_thumb_rotation.
- Hardware mapping: FTDI FTABQDTD interface 01 (`/dev/ttyUSB1`) is right ID 1; interface 02 (`/dev/ttyUSB2`) is left ID 1. Project defaults use their stable `/dev/serial/by-id` paths.
- DDS boundary: `rt/inspire/cmd` with `unitree_go::msg::dds_::MotorCmds_`; `rt/inspire/state` with `MotorStates_`. Namespace is configurable.
- Critical physical semantics confirmed on the left thumb: `ANGLE_SET`, `ANGLE_ACT`, and `POS_ACT` describe actuator travel/allowed finger range, not the externally observed anatomical pose. At q=1 the actuator reaches the open endpoint and releases the passive mechanism; a downward unloaded finger can remain lowered under gravity and be moved manually within the allowed range. External pose therefore depends on gravity, orientation, contact and passive mechanics.
- Keep only the official service plus compact isolated single-axis protocol/safety tools. Do not reintroduce duplicate DDS services or speculative architecture layers.
- The RH56 manual exposes no verified emergency-stop/disable register. `scripts/hand_estop.sh` is best-effort software stop only and cannot replace power removal or the robot physical E-stop.
- Respect one-hand-only testing. Never open or command the other serial port while its arm is working.
- Existing trees `/home/unitree/unitree_sdk2-main` and `/home/unitree/h1_inspire_service` remain read-only references.
- Architecture/environment details: `mem:tech_stack`. Rules: `mem:conventions`. Commands: `mem:suggested_commands`. Completion gates: `mem:task_completion`.

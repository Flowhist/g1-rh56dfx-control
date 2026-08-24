# Project Core

- Unitree G1 dexterous-hand development on the robot's Jetson; hardware is Inspire Robots RH56DFX dual hands.
- Project root: `/home/unitree/rh56dfx/workspace`. Keep project implementations and generated project artifacts inside this root.
- Stable upper-layer contract: command `q[12]`, state `q[12]`, each normalized to `[0,1]`, with 0=closed and 1=open. RH56 raw ANGLE values use the inverse direction, so encode as `(1-q)*1000` and decode as `1-raw/1000`; never expose raw direction at the DDS boundary.
- Canonical order: right_pinky, right_ring, right_middle, right_index, right_thumb_bend, right_thumb_rotation, left_pinky, left_ring, left_middle, left_index, left_thumb_bend, left_thumb_rotation.
- DDS boundary: command `rt/inspire/cmd` using `unitree_go::msg::dds_::MotorCmds_`; state `rt/inspire/state` using `MotorStates_`. The project now has a verified read-only dual-port state publisher at `examples/hand_readonly_state_service.cpp`; it contains no command subscriber or motion-register path.
- Upper layers must not depend on ttyUSB enumeration, RS485 frames/registers, baud rate, or USB topology.
- Safety invariant: validate shape/finite values -> clamp range -> rate-limit -> watchdog -> DDS/hardware.
- Never move hardware until device mapping, service health, observed state, command semantics, workspace clearance, and emergency-stop method are verified.
- Existing sources outside project are reference/runtime assets, not safe edit targets: `/home/unitree/unitree_sdk2-main`, `/home/unitree/h1_inspire_service`.
- Ask user before any global environment change or any write outside project root, including apt/pip installs, `/usr/local`, `/etc`, udev, groups, services, or edits to existing external repositories.
- Architecture and hardware background: `mem:tech_stack`. Development rules: `mem:conventions`. Commands: `mem:suggested_commands`. Completion gates: `mem:task_completion`.
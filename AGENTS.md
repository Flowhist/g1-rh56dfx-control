# Unitree G1 RH56DFX Project Guidance

## Scope

- This repository develops dual Inspire RH56DFX hand control for a Unitree G1 on its Jetson.
- Keep project code and generated project artifacts inside this repository.
- Before changing the global environment or any file outside this repository, explain the exact change and obtain the user's approval. This includes package installation, `/usr/local`, `/etc`, udev rules, groups, services, network settings, and external repositories.
- Treat existing external trees such as `/home/unitree/unitree_sdk2-main` and `/home/unitree/h1_inspire_service` as read-only unless the user explicitly approves changes.
- Never discard or overwrite existing user changes.

## Architecture

- Expose a stable normalized hand boundary: command `q[12]`, state `q[12]`, values in `[0, 1]`.
- Canonical order:
  1. right_pinky
  2. right_ring
  3. right_middle
  4. right_index
  5. right_thumb_bend
  6. right_thumb_rotation
  7. left_pinky
  8. left_ring
  9. left_middle
  10. left_index
  11. left_thumb_bend
  12. left_thumb_rotation
- DDS topics are `rt/inspire/cmd` and `rt/inspire/state`.
- Keep ttyUSB enumeration, RS485 framing/registers, baud rate, and USB topology behind the robot-side backend.

## Safety

- Develop in this order: compile-only checks, read-only state monitoring, one-joint low-amplitude motion, one-hand motion, then coordinated dual-hand control.
- Before publishing motion commands, verify physical left/right mapping, serial access, service health, state feedback, joint semantics, clear workspace, robot stability, and an immediate stop method.
- Command path: validate size and finite values, clamp range, rate-limit changes, apply watchdog, then publish.
- Never run installation, udev, service-start, or motion scripts merely as a diagnostic.
- Never claim hardware validation when only compilation or static inspection was performed.

## Implementation Style

- Prefer concise, efficient, auditable C++17 using the installed Unitree SDK2 and standard library.
- Avoid unnecessary dependencies, allocations in control loops, hidden global state, and premature abstraction.
- Separate safety/validation logic from DDS transport and hardware-service lifecycle.
- Use fixed-size 12-element representations at project boundaries.
- Do not hard-code `/dev/ttyUSB*` paths in new upper-layer code.
- Keep diagnostic output bounded; avoid logging at control-loop frequency by default.

## Local Baseline

- Target: Jetson Orin NX, Ubuntu 20.04, aarch64, JetPack/L4T 35.3.1.
- Toolchain: GCC 9.4, CMake 3.16, GNU Make, C++17.
- Unitree SDK2 is installed under `/usr/local`.
- ROS 2 Foxy is available after sourcing `/opt/ros/foxy/setup.bash`; use CycloneDDS unless a task requires otherwise.
- Confirmed FT4232H mapping: right RH56DFX is interface 01 (`ttyUSB1`), left RH56DFX is interface 02 (`ttyUSB2`); both use device ID 1. Prefer their interface-qualified `/dev/serial/by-id` paths until project aliases are installed.

## Verification

- Configure examples with `cmake -S examples -B build/examples`.
- Build with `cmake --build build/examples -j$(nproc)`.
- Run tests with `(cd build/examples && ctest --output-on-failure)`; the installed CTest 3.16 does not reliably support `--test-dir`.
- Run hardware programs only when their corresponding safety gate above is satisfied.
- At handoff, report changed files, verification commands, and hardware behavior that remains untested.

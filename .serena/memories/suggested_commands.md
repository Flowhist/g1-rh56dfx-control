# Suggested Commands

Read-only environment checks:
- `bash scripts/check_hardware.sh`
- `ls -l /dev/ttyUSB* /dev/serial/by-id/* /dev/serial/by-path/*`
- `udevadm info --query=property --name=/dev/ttyUSBX`
- `fuser /dev/ttyUSBX`
- `ip -brief addr; ip route`

ROS shell:
- `source /opt/ros/foxy/setup.bash`
- `export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp`

Build supplied examples inside project:
- `cmake -S examples -B build/examples`
- `cmake --build build/examples -j$(nproc)`

Safe read-only dual-hand DDS check after serial access is available:
- Terminal 1: `./build/examples/hand_readonly_state_service eth0`
- Terminal 2: `./build/examples/hand_state_monitor eth0`
- The read-only service uses confirmed FTDI interface-qualified paths for right interface 01 and left interface 02; it publishes state only.

Do not run without explicit preparation/approval:
- `scripts/bootstrap_jetson.sh` (apt/global changes)
- `scripts/install_udev_rules.sh` (writes /etc and reloads udev)
- `scripts/start_hand_service.sh` (sudo + hardware service; current expected path/binary does not match actual installation)
- Motion examples `hand_single_joint` and `hand_open_close`.

Memory integrity:
- `serena memories check` from project root.

# Technical Stack

- Target: NVIDIA Orin NX Developer Kit, aarch64, Ubuntu 20.04.6, JetPack/L4T 35.3.1, kernel 5.10.104-tegra.
- Native C++17, GCC/G++ 9.4, CMake 3.16, GNU Make. Prefer direct native builds on Jetson.
- Unitree SDK2 installed system-wide: headers under `/usr/local/include/unitree`, library `/usr/local/lib/libunitree_sdk2.a`, CMake config `/usr/local/lib/cmake/unitree_sdk2`.
- ROS 2 Foxy is installed; source `/opt/ros/foxy/setup.bash`. Known working middleware: `rmw_cyclonedds_cpp`.
- DDS topics and message types are documented in `mem:core`.
- USB transport: FTDI FT4232H (0403:6011), serial FTABQDTD, four interfaces 00..03 mapped to ttyUSB0..3. Read-only before/after unplug probing confirms: right RH56DFX = ttyUSB1/interface 01/ID 1; left RH56DFX = ttyUSB2/interface 02/ID 1; ttyUSB0/3 do not respond as hand ID 1/2. The FT4232H remains enumerated when a downstream hand is unplugged, so identify connection by protocol response, not tty node presence alone.
- Existing legacy hand service: `/home/unitree/h1_inspire_service`, binary `build/inspire_hand`; dirty worktree with local changes. It defaults to `/dev/ttyUSB0` and B115200. Treat as evidence/reference until validated for G1 dual-hand use.
- No persistent `/dev/inspire_left` or `/dev/inspire_right` aliases are installed. Current user is not in `dialout`; direct serial access is unavailable without an approved permission change or sudo.
- No CAN interface is active; RH56 path is USB serial/RS485, not SocketCAN.
- Detailed development constraints: `mem:conventions`.
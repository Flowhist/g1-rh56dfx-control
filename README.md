# Unitree G1 · Inspire RH56DFX

Minimal G1 hand-control workspace based on Unitree's official
[`dfx_inspire_service`](https://github.com/unitreerobotics/dfx_inspire_service).

## Hardware mapping

| Hand | Stable serial path | RH56 ID |
|---|---|---:|
| Right | `/dev/serial/by-id/usb-FTDI_USB__-__Serial_Converter_FTABQDTD-if01-port0` | 1 |
| Left | `/dev/serial/by-id/usb-FTDI_USB__-__Serial_Converter_FTABQDTD-if02-port0` | 1 |

Official Unitree position convention: `q=0` closed, `q=1` open. The value is
actuator position/allowed finger travel, not a measured anatomical joint pose.
Opening releases the passive mechanism, so gravity and contact can determine the
visible finger pose.

## Build

The Jetson already has the required Unitree SDK2, Boost, Eigen and spdlog.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j2
(cd build && ctest --output-on-failure)
```

No system installation is required.

## Official dual-hand service

```bash
./build/third_party/dfx_inspire_service/inspire_g1
```

Optional arguments:

```bash
./build/third_party/dfx_inspire_service/inspire_g1 \
  --right-serial /dev/serial/by-id/...if01-port0 \
  --left-serial /dev/serial/by-id/...if02-port0 \
  --network eth0 \
  --namespace inspire
```

DDS interface:

- command: `rt/inspire/cmd`, `unitree_go::msg::dds_::MotorCmds_`;
- state: `rt/inspire/state`, `unitree_go::msg::dds_::MotorStates_`;
- order: right six axes, then left six axes;
- per-hand order: pinky, ring, middle, index, thumb bend, thumb rotation.

Only run the dual-hand service when both arms and hands are safe to command.

## Manual one-axis test

Edit [`config/hand_test.yaml`](config/hand_test.yaml), then run:

```bash
./scripts/run_hand_test.py
```

The tool controls only the selected serial hand and returns to its starting
actuator position. Validate without motion using:

```bash
./scripts/run_hand_test.py --dry-run
```

Best-effort software stop:

```bash
./scripts/hand_estop.sh left
```

This is not a hardware emergency stop. Use robot power removal or the physical
E-stop for a real emergency.

## Sources retained

- `third_party/dfx_inspire_service/`: official Unitree service, with only the
  serial-path and initialization adaptations recorded in `UPSTREAM.md`;
- `examples/`: compact protocol tests and isolated one-axis safety tools;
- `因时机器人仿人五指灵巧手--RH56用户手册V1.09cn.pdf`: vendor register manual.

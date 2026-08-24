# RH56DFX Developer Guide

## 1. Scope

Target:

```text
Unitree G1
+ two Inspire RH56DFX hands
+ non-tactile version
```

Recommended lower-level integration:

```text
Application
   |
   | DDS
   v
dfx_inspire_service
   |
   | RS485
   v
RH56DFX
```

For ordinary application development, do not start from raw RS485.

---

## 2. Hand representation

Each hand has six commanded dimensions:

```text
0  pinky
1  ring
2  middle
3  index
4  thumb_bend
5  thumb_rotation
```

Dual-hand ordering:

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

Treat this as the canonical project representation.

---

## 3. Unitree DDS interface

Command:

```text
rt/inspire/cmd
```

State:

```text
rt/inspire/state
```

Unitree's current official RH56DFX service uses the `q` field for position control.

The official example uses:

```text
0.0 = close
1.0 = open
```

Do not assume other motor command fields are functional for this hand unless verified in the current service implementation.

---

## 4. Required software

Required:

```text
unitreerobotics/unitree_sdk2
unitreerobotics/dfx_inspire_service
```

Optional:

```text
unitreerobotics/unitree_sdk2_python
unitreerobotics/xr_teleoperate
```

Recommended reading:

1. `dfx_inspire_service/README.md`
2. `dfx_inspire_service/example/hand_example.cpp`
3. `dfx_inspire_service/inspire_g1.cpp`
4. Unitree SDK2 examples
5. `xr_teleoperate`
6. Inspire low-level documentation only if additional hardware capabilities are required

---

## 5. Basic dependencies

```bash
sudo apt update

sudo apt install -y \
    build-essential \
    cmake \
    git \
    libboost-all-dev \
    libspdlog-dev
```

---

## 6. Build Unitree SDK2

```bash
cd ~/workspace

git clone https://github.com/unitreerobotics/unitree_sdk2.git
cd unitree_sdk2

mkdir -p build
cd build

cmake ..
make -j$(nproc)
sudo make install
```

Record a known-good commit after validation:

```bash
git rev-parse HEAD
```

---

## 7. Build the official hand service

```bash
cd ~/workspace

git clone https://github.com/unitreerobotics/dfx_inspire_service.git
cd dfx_inspire_service

mkdir -p build
cd build

cmake ..
make -j$(nproc)
```

---

## 8. Serial devices

Check:

```bash
ls -l /dev/ttyUSB*
```

Verify which physical hand corresponds to each device.

The official G1 service may contain hard-coded serial paths.

Do not assume USB enumeration is stable.

Use persistent aliases:

```text
/dev/inspire_right
/dev/inspire_left
```

after creating verified udev rules.

---

## 9. First hardware bring-up

Run:

```bash
bash scripts/check_hardware.sh
```

Then start the official service.

Typical location:

```bash
cd ~/workspace/dfx_inspire_service/build
sudo ./inspire_g1
```

The serial paths in the source must match the real system.

---

## 10. First acceptance test

Do not proceed until all items pass:

- both serial devices are present;
- left/right mapping is known;
- service starts without serial errors;
- both hands can open and close;
- right hand can move independently;
- left hand can move independently;
- all six dimensions respond;
- `rt/inspire/state` updates;
- reboot does not silently reverse device mapping.

---

## 11. Safety layer

Do not pass arbitrary upper-level commands directly to hardware.

Recommended path:

```text
input command
    |
    v
finite / shape validation
    |
    v
range clamp
    |
    v
rate limiter
    |
    v
watchdog
    |
    v
DDS
```

Minimum clamp:

```text
q[i] in [0, 1]
```

A rate limiter should prevent large instantaneous changes.

---

## 12. Watchdog

Define command timeout behavior.

Suggested default policy:

```text
normal:
    accept valid commands

timeout:
    hold current target

invalid input:
    reject command

emergency:
    stop generating new motion commands
```

Tune exact timing for the final application.

---

## 13. Force vs tactile

This package targets the non-tactile RH56DFX configuration.

Do not assume availability of:

- tactile images;
- taxel arrays;
- dense fingertip pressure maps.

The hand may expose lower-level force-related information through the manufacturer protocol, but that is separate from tactile-array sensing and is not part of the minimal DDS hand abstraction.

---

## 14. When to use raw RS485

Only go below the Unitree DDS abstraction when there is a concrete requirement such as:

- extra diagnostics;
- additional force information;
- current;
- temperature;
- fault/status registers;
- device configuration.

If such functionality is added, keep it inside the robot-side hand service rather than exposing serial operations to the VLA.

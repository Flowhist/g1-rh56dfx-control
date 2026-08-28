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
cmake --build build --target hand_compliant_teach hand_hold_position hand_clear_fault hand_web_control -j2
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

## Browser joint control

Build and start the local preview (the preview never reads or moves hardware):

```bash
cmake --build build --target hand_web_control -j2
./build/src/hand_web_control
```

Open <http://127.0.0.1:8080>. The page provides six independent controls for
each hand, presets, a shared pose library with custom pose names, live actuator
feedback, and a simple pose visualization. Poses are automatically persisted to
`data/hand_poses.tsv` after every save, rename, or delete and reloaded at service startup.
Use `--poses-file PATH` to select another local file. Each hand also has a
collapsed register monitor;
opening it continuously reads position (`0x060A`), force (`0x062E`), current
(`0x063A`), error (`0x0646`), status (`0x064C`), and temperature (`0x0652`).
Closing the panel stops the extra serial polling.

For contact gripping, each hand has a separate **Apply grip and hold target**
control. It writes the firmware force threshold (`0x05DA`), applies a current
limit (`0x03FC`), and resends the current position target after checking current,
temperature, and error telemetry. The web controller hard-limits this operation
to 300 mA, matching the existing project safety threshold; it does not expose
the firmware's 1500 mA maximum. Start with a low force value and increase it
gradually. Position percentage is not measured finger closure, and grip success
still depends on object geometry, friction, thumb opposition, and force-sensor
calibration.

Faulted register rows expose a **Clear** button while the register monitor is
open. Clearing requires hardware control to be unlocked and a separate safety
confirmation. The controller first targets each faulted actuator's current
feedback position, then writes `CLEAR_ERROR(0x03EC)=1`, and finally verifies the
error and status registers. This register applies to every clearable fault on the
selected hand; overtemperature faults are rejected and must cool to auto-clear.

The same guarded operation is available from the command line after stopping all
hand controllers:

```bash
./build/src/hand_clear_fault --hand right --joint middle --execute
```

After confirming the workspace is clear, the robot is stable, no other hand
controller is active, and the physical E-stop is immediately reachable, enable
hardware access explicitly:

```bash
./build/src/hand_web_control --execute --hand both
```

The server binds to all IPv4 interfaces by default and prints each available LAN
URL at startup. Only run it on a trusted network. Use `--bind 127.0.0.1` for
local-only access. The page starts locked after every load and requires a
separate safety confirmation before sending motion commands.
`scripts/hand_estop.sh both` stops the service and commands the current positions
as a best-effort software hold.

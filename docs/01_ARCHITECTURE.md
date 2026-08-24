# Architecture

## 1. Recommended system boundary

Treat the RH56DFX subsystem as a robot-side service.

```text
                    ROBOT SIDE — G1 Jetson

┌──────────────────────────────────────────────────┐
│ Hand API / HandController                        │
│                                                  │
│ - normalized command interface                   │
│ - joint ordering                                 │
│ - clamp / rate limit                             │
│ - watchdog                                       │
│ - state validity                                 │
└──────────────────────┬───────────────────────────┘
                       │
                       v
┌──────────────────────────────────────────────────┐
│ Unitree dfx_inspire_service                      │
│ DDS <-> RH56DFX protocol                         │
└──────────────────────┬───────────────────────────┘
                       │ RS485
                 ┌─────┴─────┐
                 v           v
           Right RH56DFX  Left RH56DFX
```

The application boundary should stay above the low-level hardware protocol.

---

## 2. Runtime architecture

For standalone development:

```text
G1 Jetson
    |
    +-- test / control application
    |
    +-- DDS
    |
    +-- dfx_inspire_service
    |
    +-- RS485
    |
    +-- RH56DFX
```

For future VLA use:

```text
External GPU Computer
┌───────────────────────────────┐
│ VLA / policy / teleoperation  │
└───────────────┬───────────────┘
                │
                │ Ethernet
                │ DDS
                v
G1 Jetson
┌───────────────────────────────┐
│ Hand command interface        │
│ Safety / watchdog             │
│ Hardware service              │
└───────────────┬───────────────┘
                v
             RH56DFX
```

The external VLA does not directly access the hand serial devices.

---

## 3. Development architecture

Default:

```text
Windows / macOS / Linux developer computer
                |
                | SSH
                v
             G1 Jetson
```

The Jetson is the execution environment.

The developer computer provides:

- IDE
- terminal UI
- Git UI
- Codex / code assistant
- SSH client

The Jetson provides:

- compiler
- runtime
- Unitree SDK
- DDS
- serial devices
- robot hardware access

Therefore a Windows developer does not need WSL for the basic workflow.

---

## 4. Why this architecture

The hand subsystem is:

- lightweight;
- hardware-specific;
- latency-sensitive enough to benefit from robot-side execution;
- directly connected to the Jetson through serial/USB;
- largely independent from VLA compute.

This makes it an appropriate component to finish, stabilize and encapsulate on the Jetson.

Large models can later run elsewhere without changing the hand hardware layer.

---

## 5. Stable abstraction

Higher-level software should ideally depend on a stable interface:

```text
set_hand_target(q[12])
get_hand_state() -> q[12], valid, timestamp
```

Do not expose low-level serial concepts to the VLA or task-level application.

---

## 6. Layer ownership

### Jetson owns

- serial devices;
- RH56 protocol;
- left/right mapping;
- DDS hand bridge;
- command clamping;
- rate limiting;
- watchdog;
- fault handling;
- hand state validity.

### External compute may own

- VLA inference;
- vision models;
- task planning;
- XR/teleoperation frontend;
- large-scale logging;
- training.

---

## 7. Rule of thumb

If code must know `/dev/ttyUSB*`, it belongs on the robot side.

If code only needs a normalized hand action/state vector, it can run anywhere.

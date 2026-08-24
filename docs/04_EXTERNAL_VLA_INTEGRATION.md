# External VLA Integration

## 1. Goal

Allow a VLA running on a separate GPU computer to control the G1 hands without knowing anything about RH56 serial hardware.

---

## 2. Desired boundary

The VLA should produce a hand action:

```text
a_hand in R^12
```

with:

```text
a_hand[i] in [0, 1]
```

Canonical ordering:

```text
[
  right_pinky,
  right_ring,
  right_middle,
  right_index,
  right_thumb_bend,
  right_thumb_rotation,
  left_pinky,
  left_ring,
  left_middle,
  left_index,
  left_thumb_bend,
  left_thumb_rotation
]
```

---

## 3. Runtime path

```text
Camera / Language / Robot State
            |
            v
           VLA
            |
            | arm + hand action
            v
External GPU PC
            |
            | Ethernet + DDS
            v
G1 Jetson
            |
            v
Hand safety / wrapper
            |
            v
dfx_inspire_service
            |
            v
RH56DFX
```

The external process should never depend on:

```text
/dev/ttyUSB*
RH56 register addresses
serial baud rate
USB topology
```

---

## 4. Integration contract

Recommended minimum command:

```text
q[12]
timestamp
```

Recommended minimum state:

```text
q[12]
timestamp
valid
```

Optional future fields:

```text
sequence_id
communication health
fault status
force-related data
```

Do not add fields until there is an actual use case.

---

## 5. Arm + hand action

A higher-level policy may represent the robot action as:

```text
action
├── arm
└── hand[12]
```

or:

```text
action
├── left_arm
├── right_arm
├── left_hand[6]
└── right_hand[6]
```

Convert into the canonical hand representation at the robot/application adapter boundary.

---

## 6. Network rule

The external VLA and G1 must communicate over the robot network.

Prefer wired Ethernet for stable latency and jitter.

SSH remains only an administrative channel.

---

## 7. Keep the Jetson service stable

Once the robot-side hand interface has been validated, avoid changing it for every new VLA experiment.

Prefer adapting new policies to the stable hand interface.

This allows:

```text
new VLA
new teleoperation system
new ROS node
new planner
```

to reuse the same robot-side RH56 stack.

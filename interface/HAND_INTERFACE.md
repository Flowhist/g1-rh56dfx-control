# Canonical Hand Interface

This file defines the project-level contract between upper-level software and the robot-side RH56DFX subsystem.

## Command

```text
HandCommand
    q: float[12]
    timestamp: optional
```

Requirements:

```text
shape = 12
all values finite
q[i] in [0, 1]
```

Canonical ordering:

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

## State

```text
HandState
    q: float[12]
    timestamp
    valid
```

Recommended optional diagnostics:

```text
lost_count
last_command_age
right_connected
left_connected
```

## Semantics

Current normalized command convention:

```text
0.0 = close
1.0 = open
```

## Stability rule

Upper-level software must not depend on:

```text
USB serial names
RS485 packet layout
manufacturer register addresses
device baud rate
```

Those are implementation details of the robot-side backend.

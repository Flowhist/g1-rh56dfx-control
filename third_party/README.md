# Third-Party Dependencies

Do not copy upstream Unitree repositories into this onboarding package.

Clone them separately on the G1 Jetson.

## Required

### unitree_sdk2

```text
https://github.com/unitreerobotics/unitree_sdk2
```

Known-good commit:

```text
TODO: record after validation on the actual G1
```

### dfx_inspire_service

```text
https://github.com/unitreerobotics/dfx_inspire_service
```

Purpose:

```text
Unitree DDS <-> RH56DFX hardware bridge
```

Key topics:

```text
rt/inspire/cmd
rt/inspire/state
```

Known-good commit:

```text
TODO: record after validation on the actual G1
```

---

## Optional

### unitree_sdk2_python

```text
https://github.com/unitreerobotics/unitree_sdk2_python
```

Useful for:

- Python prototypes;
- external VLA integration;
- data collection;
- teleoperation.

Known-good commit:

```text
TODO
```

### xr_teleoperate

```text
https://github.com/unitreerobotics/xr_teleoperate
```

Useful as a reference for:

- G1 teleoperation;
- hand retargeting;
- data collection;
- external host / robot communication.

Known-good commit:

```text
TODO if used
```

---

## Version policy

After validating the stack:

```bash
git rev-parse HEAD
```

Record the commit hashes here.

Prefer:

```text
known-good versions
```

instead of blindly tracking upstream `HEAD`.

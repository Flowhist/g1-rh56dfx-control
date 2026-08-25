# Upstream

- Repository: https://github.com/unitreerobotics/dfx_inspire_service
- Commit: `d6c4eae9bc959990bf87c262f29f62142519e923`
- License: BSD-3-Clause (`LICENSE.txt`)

Local G1 adaptations are intentionally small:

- configurable `--right-serial` and `--left-serial` options;
- stable FTDI `by-id` paths as this robot's defaults;
- zero-initialized command and state vectors;
- type-safe six-axis APIs for raw force, current, error, status and temperature reads;
- validated six-axis velocity, force-limit and current-limit writes.

The additional RH56 register addresses and value ranges were cross-checked
against the retained RH56 V1.09 vendor manual and the Correll Lab
`rh56_controller` implementation. They do not change the official Unitree DDS
topics or this robot's two-port, hand-ID-1 mapping.

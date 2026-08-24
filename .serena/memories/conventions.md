# Project Conventions

- Keep implementations minimal, efficient, explicit, and easy to audit on embedded hardware.
- C++17; avoid unnecessary frameworks, allocations in control loops, hidden global state, and abstraction without a demonstrated need.
- Use fixed-size 12-element hand representations at project boundaries; define the canonical order once and reuse it.
- Separate pure command validation/safety logic from DDS transport and hardware service lifecycle.
- Validate vector size and finite values; clamp to [0,1]; apply per-cycle rate limit; implement stale-command watchdog and a known safe fallback.
- Read-only state monitoring precedes any command publication. Single-joint, low-amplitude motion precedes full-hand or dual-hand motion.
- Do not hard-code ttyUSB names in new upper-layer code. Hardware backend may use verified persistent aliases after left/right mapping is established.
- Avoid editing or resetting dirty external repositories. Never overwrite user changes.
- New dependencies require justification; prefer installed Unitree SDK2/CMake/standard library.
- Do not install packages, change groups, udev, services, network, or files outside project root without user approval.
- Keep logs bounded and avoid printing at control-loop frequency unless diagnostics explicitly require it.
- Relevant architecture invariants: `mem:core`. Completion gates: `mem:task_completion`.
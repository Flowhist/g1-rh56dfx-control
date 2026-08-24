# Task Completion

For ordinary changes:
1. Configure with `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`.
2. Build with `cmake --build build -j2`.
3. Run `(cd build && ctest --output-on-failure)`.
4. Run `git diff --check`.
5. Confirm no writes occurred outside the project unless separately approved.
6. Report compiled, read-only hardware-verified and motion-tested behavior separately.

For hand-control changes:
- Verify stable left/right mapping and current serial permissions.
- Ensure no competing serial owner exists.
- Confirm official q semantics: 0 closed, 1 open.
- Treat q/POS_ACT/ANGLE_ACT as actuator state, not anatomical pose.
- Warn before motion and respect one-hand-only scope.
- Keep physical E-stop/power removal available.

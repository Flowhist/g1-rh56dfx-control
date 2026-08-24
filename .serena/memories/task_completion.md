# Task Completion

For ordinary C++ changes:
1. Configure: `cmake -S examples -B build/examples` (or the task's project source/build directories).
2. Build: `cmake --build build/examples -j$(nproc)`.
3. Run non-hardware tests with `(cd build/examples && ctest --output-on-failure)`; installed CTest 3.16 does not reliably support `--test-dir`.
4. Verify no writes occurred outside project root unless explicitly approved.
5. Report files changed, commands run, and any untested hardware behavior.

Additional gates for DDS state monitoring:
- Confirm SDK target resolves.
- Confirm intended network interface.
- Confirm subscriber waits safely and does not publish commands.
- Bound output rate and handle missing/malformed state.

Additional gates before any motion test:
- Left/right physical mapping verified.
- Serial access method and service executable/path verified.
- No conflicting serial process.
- State feedback observed.
- Joint index and open/close semantics verified.
- Workspace clear; robot stable; operator present; stop method ready.
- Start with one joint, small delta, conservative dwell/rate; return to safe pose.
- Full-hand/dual-hand tests only after single-joint acceptance.

Never claim hardware behavior was validated if only compilation or simulation was performed.

# Suggested Commands

Build and test:
- `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`
- `cmake --build build -j2`
- `(cd build && ctest --output-on-failure)`

Read-only checks:
- `./scripts/check_hardware.sh`
- `./scripts/run_hand_test.py --dry-run` (opens only the hand selected in YAML and sends no position command)
- `./build/third_party/dfx_inspire_service/inspire_g1 --help` (does not open serial ports)

Motion, only after warning and physical clearance:
- `./scripts/run_hand_test.py`
- `./scripts/hand_estop.sh left` or `right` (best-effort, not hardware E-stop)

Official dual-hand service, only when both hands are safe:
- `./build/third_party/dfx_inspire_service/inspire_g1 --network <iface> --namespace inspire`

Repository checks:
- `git status --short`
- `git diff --check`

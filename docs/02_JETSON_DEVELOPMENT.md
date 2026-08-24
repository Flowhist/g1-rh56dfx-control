# Jetson-First Development Workflow

## 1. Recommended workflow

Use SSH or VS Code Remote SSH to work directly in a project directory on the G1 Jetson.

Example:

```text
Developer PC
   |
   | VS Code Remote SSH
   v
G1 Jetson
~/workspace/g1_hand/
```

The files shown in the editor are stored on the Jetson and compiled on the Jetson.

---

## 2. SSH connection

From the development computer:

```bash
ssh <user>@<g1-jetson-ip>
```

For VS Code:

1. Install `Remote - SSH`.
2. Add the G1 Jetson SSH target.
3. Open the remote directory.
4. Use the integrated terminal.

No local Unitree SDK is required.

---

## 3. Recommended workspace

```text
~/workspace/
├── unitree_sdk2/
├── dfx_inspire_service/
└── g1_hand_project/
```

Use Git for source control.

The remote Jetson working tree is the active development copy.

GitHub/GitLab remains the source of truth and backup.

---

## 4. Keep the Jetson clean

Avoid installing unrelated experimental packages globally.

Suggested rule:

```text
system packages:
    compiler / CMake / robot dependencies

Python:
    separate virtualenv / conda environment per project

source:
    ~/workspace/

logs:
    ~/logs/ or project/logs/
```

Do not turn the Jetson into an untracked collection of temporary repositories and environments.

---

## 5. Build on Jetson

Example:

```bash
cd ~/workspace/dfx_inspire_service
mkdir -p build
cd build
cmake ..
make -j$(nproc)
```

The resulting ARM64 executable runs directly against the robot-side environment.

---

## 6. Deployment model

During early development there is effectively no separate deployment step:

```text
edit remotely -> build remotely -> run remotely
```

Later, when the hand stack is stable, it can be packaged as a service and updated through Git or a deployment script.

---

## 7. Future external VLA

Do not move the hand driver to the external GPU machine.

Instead:

```text
External VLA
     |
     | DDS command/state
     v
Jetson Hand Service
     |
     v
RH56DFX
```

The hand stack stays unchanged.

---

## 8. SSH is not the control protocol

Do not implement runtime robot control using:

```text
ssh command execution
remote shell calls
scp-triggered actions
```

Use SSH for:

- editing;
- starting/stopping programs;
- logs;
- debugging;
- deployment.

Use DDS for runtime commands and state.

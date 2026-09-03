# Unitree G1 · Inspire RH56DFX 手部控制

本项目提供 RH56DFX 双手控制服务、网页控制、柔顺示教、位置保持和故障清除工具。
所有上层程序统一通过 `hand_service` 访问手部硬件，只有该服务直接打开串口。

## 硬件约定

| 手 | 串口接口 | RH56 ID |
|---|---|---:|
| 右手 | FTDI `if01` | 1 |
| 左手 | FTDI `if02` | 1 |

位置约定：`q=0` 表示闭合，`q=1` 表示张开。单手关节顺序为：
`小指、无名指、中指、食指、拇指弯曲、拇指旋转`。
寄存器定义可查阅 [`docs/因时机器人仿人五指灵巧手--RH56用户手册V1.09cn.pdf`](docs/因时机器人仿人五指灵巧手--RH56用户手册V1.09cn.pdf)。

## 编译

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target hand_service hand_compliant_teach hand_hold_position hand_clear_fault hand_web_control -j2
```

依赖 yaml-cpp 和 Unitree SDK2。

## 快速启动

连接真实硬件前，请先确认手部活动范围内无人、无障碍物，并确保物理急停触手可及。

终端 1：启动统一手部服务。只有该进程直接访问串口。

```bash
./build/src/hand_service --execute --hand both --network eth0
```

终端 2：启动网页控制服务。网页进程通过统一 RPC 接口访问 `hand_service`。

```bash
./build/src/hand_web_control --hand both --network eth0
```

本机浏览器访问：<http://127.0.0.1:8080>。

局域网设备访问：`http://机器人IP:8080`。可用以下命令查看机器人网卡地址：

```bash
ip -br addr
```

网页默认监听 `0.0.0.0:8080`，因此同一局域网内可访问，但程序不会主动向局域网广播或进行服务发现。
启动时会打印可用的局域网 URL。请只在可信网络中使用；若只允许本机访问，使用：

```bash
./build/src/hand_web_control \
  --hand both --network eth0 --bind 127.0.0.1
```

页面每次加载后都处于锁定状态，必须单独确认安全提示后才会发送运动命令。

## `--network` 参数

`--network` 用于选择 Unitree DDS/RPC 通信所使用的网卡，例如 `eth0`。它负责
`hand_service` 与网页、示教程序及命令行工具之间的通信，不控制 HTTP 网页监听地址。

- `--network eth0`：选择 DDS/RPC 网卡。
- `--bind 0.0.0.0`：允许网页从所有 IPv4 接口访问，这是默认值。
- `--bind 127.0.0.1`：网页仅允许本机访问。
- `--port 8080`：设置网页服务端口。

`hand_service` 和所有控制客户端应使用相同的 `--network`。如果 Unitree SDK 的默认网卡
已经正确，可以省略该参数；多网卡设备建议明确指定，避免 DDS 连接到错误网络。

## 统一上层接口

`hand_service` 是唯一的硬件所有者，对外提供：

- Unitree 兼容的 DDS 位置指令与状态主题；
- 支持关节掩码的位置控制 RPC；
- 完整状态读取 RPC；
- 夹持、当前位置保持和故障清除 RPC。

接口主题、RPC 数据结构、结果码和 C++ 客户端示例见
[`docs/hand_service_api.md`](docs/hand_service_api.md)。服务运行时不要再启动其他直接访问手部串口的程序。

## 网页控制

网页提供双手各 6 个关节的独立控制、预设动作、姿态库、实时执行器反馈和姿态示意图。
自定义姿态在保存、重命名或删除后会自动写入 `data/hand_poses.tsv`，并在下次启动时恢复。
使用 `--poses-file PATH` 可以指定其他姿态文件。

每只手都有默认折叠的寄存器监视器。展开后会持续读取位置 (`0x060A`)、力 (`0x062E`)、
电流 (`0x063A`)、错误 (`0x0646`)、状态 (`0x064C`) 和温度 (`0x0652`)；关闭面板后停止额外轮询。

“应用夹持并保持目标”会写入力阈值 (`0x05DA`)，应用电流限制 (`0x03FC`)，并在检查
电流、温度和错误状态后重新发送当前位置目标。网页将电流硬限制为 300 mA，不开放固件的
1500 mA 上限。建议从较小的力值开始逐步增加。位置百分比并不等于实测闭合程度，夹持效果
还会受到物体形状、摩擦、拇指对置和力传感器标定的影响。

## 故障清除

寄存器监视器展开后，发生故障的行会显示“清除”按钮。执行前必须解锁硬件控制并再次确认安全。
服务会先把故障执行器的目标设为当前反馈位置，再写入 `CLEAR_ERROR(0x03EC)=1`，最后验证错误
和状态寄存器。该寄存器会清除所选手上所有可清除故障；过温故障会被拒绝，必须等待降温后自动恢复。

也可以通过统一服务的命令行客户端清除故障：

```bash
./build/src/hand_clear_fault \
  --hand right --joint middle --network eth0 --execute
```

## 柔顺示教

配置位于 [`config/compliant_teach.yaml`](config/compliant_teach.yaml)。先启动 `hand_service`；
示教程序通过统一的状态和位置 RPC 工作，不直接打开串口。

模拟运行，不发送真实运动命令：

```bash
./build/src/hand_compliant_teach \
  --hand both --joint all --profile slow --network eth0
```

真实运行 60 秒：

```bash
./build/src/hand_compliant_teach \
  --hand both --joint all --profile slow --duration 60 \
  --network eth0 --execute
```

可用档位：`slow`、`medium`、`fast`。按 `Ctrl+C` 停止。

## 位置保持与停止

通过统一服务保持当前位置：

```bash
./build/src/hand_hold_position both --network eth0
```

使用 `Ctrl+C` 或 `SIGTERM` 正常停止 `hand_service`。服务退出前会请求已启用的手保持
当前位置。软件保持不能替代断电或物理急停。

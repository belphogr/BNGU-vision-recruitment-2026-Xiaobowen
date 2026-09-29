# Task 3：视觉结果串口报文发送

## 1. 任务目标

本任务把任务二得到的 AprilTag 检测和位姿结果，按照规定的 ASCII 报文格式连续发送到串口。由于本次在 WSL 环境下完成，没有连接真实下位机，因此使用一对虚拟串口模拟发送端和接收端。发送端是 C++ 程序，接收端独立运行在另一个终端中，并把收到的内容保存为日志文件。

完成后，接收端能够看到三种情况：

- Tag 0 出现在画面中时，发送 `valid=1` 以及实际计算出的位姿；
- Tag 移出画面时，发送 `valid=0`、`id=-1` 和全零位姿；
- Tag 再次出现时，重新发送新的有效位姿。

## 2. 环境与文件

- Ubuntu 26.04（WSL2）
- C++17
- OpenCV 4.10.0
- AprilTag 库（`tag36h11` 家族）
- `socat`：创建一对虚拟串口

本任务的主要文件如下：

```text
task3/
├── src/
│   ├── serial_sender.cpp          # 固定内容报文发送测试
│   └── vision_serial_sender.cpp   # 摄像头检测并发送实际位姿
├── output/
│   └── serial_receive.log         # 接收端保存的报文日志
└── docs/
    └── task3.md
```

`vision_serial_sender.cpp` 会读取任务二生成的标定文件：

```text
../task2/output/camera_calibration.yml
```

其中包含相机内参和畸变系数，因此任务三的坐标结果与任务二使用的是同一套相机参数。

## 3. 虚拟串口连接方式

真实串口通信中，发送端和接收端通过一根串口线相连。本次使用 `socat` 在 Linux 中创建两个伪终端，作用相当于一根虚拟串口线的两端：

```text
vision_serial_sender.cpp
        ↓ 写入
/tmp/vision_serial
        ↕ socat 转发
/tmp/receiver_serial
        ↓ 读取
接收终端与 serial_receive.log
```

启动虚拟串口的命令如下：

```bash
socat -d -d pty,raw,echo=0,link=/tmp/vision_serial pty,raw,echo=0,link=/tmp/receiver_serial
```

其中 `/tmp/vision_serial` 被发送程序以只写方式打开；另一个终端从 `/tmp/receiver_serial` 读取。虚拟端口不会真实限制波特率，但报文设计仍按任务要求使用串口常见的 **115200、8N1** 约定。之后接入真实串口时，只需要把这两个 `/tmp/...` 路径换成实际设备路径，并设置对应串口参数即可。

接收端使用以下命令显示并同时保存日志：

```bash
cd ~/BNGU-vision-recruitment-2026-Xiaobowen/task3
cat /tmp/receiver_serial | tee output/serial_receive.log
```

`cat` 负责读取接收端；`tee` 一边把内容显示在终端，一边写入 `output/serial_receive.log`。这样保存的是接收端实际读到的数据，而不是发送程序自己的打印内容。

## 4. 报文格式

每条报文使用如下格式：

```text
$CV1,seq,t_ms,valid,id,x_mm,y_mm,z_mm,rx,ry,rz*HH\r\n
```

字段含义如下：

| 字段 | 含义 |
| --- | --- |
| `CV1` | 协议版本标识 |
| `seq` | 从 0 开始递增的报文序号 |
| `t_ms` | 从程序开始运行起经过的单调时间，单位为 ms |
| `valid` | `1` 表示检测到指定 Tag 且姿态有效；`0` 表示当前无有效目标 |
| `id` | 有效目标的 Tag ID；无效时固定为 `-1` |
| `x_mm, y_mm, z_mm` | Tag 中心相对于相机光学坐标系的平移，单位 mm |
| `rx, ry, rz` | Rodrigues 旋转向量，单位为弧度 |
| `HH` | 对正文逐字节异或得到的两位大写十六进制校验和 |

例如日志中的一条有效报文为：

```text
$CV1,94,10843,1,0,-38.0,-4.5,221.4,-0.070035,0.031501,1.621035*31
```

这表示第 94 条报文在程序启动约 10.843 秒后生成，检测到 ID 为 0 的 Tag；其中心在相机坐标系中约为 `(-38.0, -4.5, 221.4)` mm。

当 Tag 不在画面中时，报文例如：

```text
$CV1,123,14148,0,-1,0.0,0.0,0.0,0.000000,0.000000,0.000000*30
```

此时不沿用上一帧的旧坐标，而是明确发送无效标志、`id=-1` 和全零位姿，避免接收方把旧数据误认为当前结果。

## 5. 校验和计算

校验和不包含 `$`、`*` 和行结束符，而是对 `CV1` 到最后一个 `rz` 字符之间的所有 ASCII 字节逐个异或。例如正文：

```text
CV1,42,12345,1,0,100.0,-50.0,800.0,0.000000,0.000000,0.000000
```

程序从 0 开始累积异或，最终得到一个 8 位数值，再以两位大写十六进制写在 `*` 后。对应代码如下：

```cpp
unsigned char checksum = 0;
for (unsigned char character : body) {
    checksum ^= character;
}
```

这样接收方也可以对正文重新计算一次校验和，并和报文中的 `HH` 比较，从而发现传输中的错误。

## 6. 程序实现

### 6.1 固定内容发送测试

`src/serial_sender.cpp` 是先完成的通信测试程序。它不依赖摄像头和 AprilTag，而是持续发送一组固定的示例位姿，用来先确认：虚拟串口能建立、报文格式正确、校验和能够生成、接收端能够读到完整内容。

程序每约 100 ms 发送一次，因此频率约为 10 Hz。每次发送前会增加 `seq`，并用 `std::chrono::steady_clock` 计算 `t_ms`。`steady_clock` 不会受系统时间调整影响，适合记录程序运行时长。

### 6.2 实时视觉发送

`src/vision_serial_sender.cpp` 复用了任务二的工作：读取任务二相机标定参数，打开 `/dev/video0`，识别 `tag36h11` 家族中 ID 为 0 的 AprilTag，并调用 `estimate_tag_pose` 解算三维位姿。

每一帧开始时，程序会先把报文内容初始化为无效状态：

```cpp
bool valid = false;
int selectedId = -1;
double targetX = 0.0;
double targetY = 0.0;
double targetZ = 0.0;
```

只有在本帧检测到 ID 为 0 的 Tag、并完成姿态计算后，才把 `valid` 设为 `true`，同时填入平移和旋转向量。平移结果由米转换为毫米后写入报文，保留 1 位小数；旋转向量保留 6 位小数。没有检测到目标时，初始化的无效状态会直接被发送。

程序在每次发送后暂停约 100 ms。实际运行还包含摄像头读帧、Tag 检测和位姿计算，所以日志中相邻报文间隔大约为 110～120 ms，属于约 9 Hz 左右的实时更新。

## 7. 编译与运行

在 `task3` 文件夹中编译并运行固定内容测试：

```bash
g++ -std=c++17 src/serial_sender.cpp -o serial_sender
./serial_sender
```

编译并运行实时视觉发送程序：

```bash
g++ -std=c++17 src/vision_serial_sender.cpp -o vision_serial_sender \
  $(pkg-config --cflags --libs opencv4) \
  $(pkg-config --cflags --libs apriltag) -lapriltag-utils
./vision_serial_sender
```

在 VS Code 中也配置了快捷任务：`F5` 用于固定内容发送测试，`F4` 用于实时视觉发送。运行实时程序前，需要先启动虚拟串口和接收端，再确认摄像头已通过 USBIP 连接到 WSL。

## 8. 接收结果验证

本次测试中，先让 Tag 0 出现在摄像头画面内，再将其移出，最后重新放回画面。`output/serial_receive.log` 中保存了接收端实际收到的报文。

日志中能找到 55 条 `valid=1、id=0` 的有效报文，坐标和旋转向量会随 Tag 的位置、角度变化而变化。例如：

```text
$CV1,262,29909,1,0,123.8,-60.9,225.6,-0.368562,0.121151,1.670366*24
$CV1,268,30595,1,0,75.3,13.3,227.2,-0.163025,0.202873,1.498556*36
```

日志末尾连续出现无效报文，例如：

```text
$CV1,340,38781,0,-1,0.0,0.0,0.0,0.000000,0.000000,0.000000*3A
$CV1,342,39013,0,-1,0.0,0.0,0.0,0.000000,0.000000,0.000000*35
```

这说明 Tag 移出画面后，发送端没有停止，也没有保留上一次有效检测的旧数据，而是持续向接收端报告“当前无有效目标”。重新放回 Tag 后，又能继续收到 `valid=1` 的实际位姿报文。

## 9. 目前的限制

- 这次使用 `socat` 模拟串口链路，验证的是程序和协议收发流程；未连接真实下位机。
- 虚拟串口不模拟真实串口线路的电气特性、丢包和波特率限制，因此接入真实硬件后仍应在目标设备上做一次 115200、8N1 的联调。
- 当前只选择 ID 为 0 的 Tag 发送。若之后需要支持多个目标，可以增加目标选择策略，并让 `id` 字段反映被选中的目标。

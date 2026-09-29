# BNGU-vision-recruitment-2026-Xiaobowen

BNGU RoboMaster 视觉组招新考核仓库。本仓库完成了视频中的蓝色灯条检测、相机标定与 AprilTag 位姿估计，以及将实时视觉结果按自定义协议发送到虚拟串口的三个任务。

## 环境

- Windows + WSL2 Ubuntu 26.04
- VS Code Remote - WSL
- C++17
- OpenCV 4.10.0
- AprilTag（`tag36h11`）
- USB 摄像头通过 USBIP 连接至 WSL
- `socat` 用于任务三的虚拟串口收发测试

## 仓库结构

```text
.
├── task1/     # 蓝色灯条检测
├── task2/     # 相机标定与 AprilTag 位姿估计
├── task3/     # 视觉结果串口报文发送
├── .vscode/   # VS Code 编译、运行快捷任务
└── README.md
```

每个任务文件夹都按 `src/`、`output/`、`docs/` 分类：`src/` 放源代码，`output/` 放结果或日志，`docs/` 放该任务的完整过程说明。

## 任务总览

| 任务 | 完成内容 | 主要结果 | 详细过程和结果 |
| --- | --- | --- | --- |
| Task 1 | 对输入视频逐帧进行蓝色灯条检测 | 共处理 634 帧，生成带旋转矩形框、帧号、灯条数和耗时的结果视频 | [查看任务一详细过程和结果](task1/docs/task1.md) |
| Task 2 | 采集棋盘格图像完成相机标定，并检测 AprilTag 0 的二维位置与三维位姿 | 19 张有效标定图，平均重投影误差 1.67574 px；输出内参、畸变参数和位姿可视化结果 | [查看任务二详细过程和结果](task2/docs/task2.md) |
| Task 3 | 将任务二的实时 AprilTag 检测结果封装为自定义协议，并经虚拟串口发送和接收 | 接收日志包含目标出现、移开、再次出现时的有效与无效报文 | [查看任务三详细过程和结果](task3/docs/task3.md) |

上表仅用于快速了解各任务。颜色分割参数、形态学处理比较、相机标定数据采集、位姿解算、报文字段和异或校验的具体解释，都在对应任务的 Markdown 文档中，可以直接点击上面的链接查看。

## 运行方法

### Task 1：蓝色灯条检测

```bash
cd task1
g++ -std=c++17 src/main.cpp -o task1 $(pkg-config --cflags --libs opencv4)
./task1
```

输入视频为 `task1/data/test_video2.webm`，结果视频为 [task1_result.mp4](task1/output/task1_result.mp4)。详细处理流程和中间图片见 [task1/docs/task1.md](task1/docs/task1.md)。

### Task 2：相机标定与 AprilTag 位姿估计

任务二需要先将 USB 摄像头连接到 WSL，并保证 `/dev/video0` 可用。采集标定图、计算标定参数、执行 AprilTag 位姿估计分别对应不同源文件，完整命令、棋盘规格和结果解释见 [task2/docs/task2.md](task2/docs/task2.md)。

主要标定输出为 [camera_calibration.yml](task2/output/camera_calibration.yml)，AprilTag 可视化结果为 [apriltag_pose_result.jpg](task2/output/apriltag_pose_result.jpg)。

### Task 3：视觉结果串口发送

任务三使用任务二保存的相机标定参数。运行实时发送前，需要先在一个终端启动 `socat` 创建虚拟串口，在另一个终端启动接收端记录日志；然后运行视觉发送程序。

```bash
cd task3
g++ -std=c++17 src/vision_serial_sender.cpp -o vision_serial_sender \
  $(pkg-config --cflags --libs opencv4) \
  $(pkg-config --cflags --libs apriltag) -lapriltag-utils
./vision_serial_sender
```

虚拟串口命令、报文格式、校验规则、有效/无效状态处理，以及接收日志的验证过程见 [task3/docs/task3.md](task3/docs/task3.md)。本次接收结果保存在 [serial_receive.log](task3/output/serial_receive.log)。

## 结果入口

- [任务一结果视频](task1/output/task1_result.mp4)
- [任务二相机标定参数](task2/output/camera_calibration.yml)
- [任务二 AprilTag 位姿结果图](task2/output/apriltag_pose_result.jpg)
- [任务三串口接收日志](task3/output/serial_receive.log)

## 说明

任务一到任务三的代码、参数选择和测试记录均保留在仓库中。根目录 README 负责说明整体结构、环境和入口；需要查看某一步为什么这样实现时，请进入对应的 `task?/docs/task?.md` 文档阅读完整过程。

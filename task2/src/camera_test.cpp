#include <opencv2/opencv.hpp>
#include <iostream>

int main() {
    // 明确打开 Ubuntu 中已确认可用的摄像头节点
    cv::VideoCapture camera("/dev/video0", cv::CAP_V4L2);

    // 按刚才直接测试成功的格式请求画面
    camera.set(cv::CAP_PROP_FOURCC,
               cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));
    camera.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    camera.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    camera.set(cv::CAP_PROP_FPS, 15);

    // 检查摄像头是否真的打开
    if (!camera.isOpened()) {
        std::cerr << "无法打开摄像头 /dev/video0。" << std::endl;
        return 1;
    }

    cv::Mat frame;

    // 读取此刻摄像头拍到的一帧画面
    if (!camera.read(frame) || frame.empty()) {
        std::cerr << "无法读取摄像头画面。" << std::endl;
        return 1;
    }

    // 保存这帧照片，之后在 VS Code 的 output 文件夹中查看
    if (!cv::imwrite("output/camera_test.jpg", frame)) {
        std::cerr << "图片保存失败。" << std::endl;
        return 1;
    }

    std::cout << "摄像头测试成功，已保存到 output/camera_test.jpg"
              << std::endl;
    return 0;
}

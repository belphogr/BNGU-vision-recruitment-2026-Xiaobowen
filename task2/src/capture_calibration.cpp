#include <opencv2/opencv.hpp>

#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

int main() {
    // 使用已经验证正常的摄像头节点和画面格式。
    cv::VideoCapture camera("/dev/video0", cv::CAP_V4L2);
    camera.set(cv::CAP_PROP_FOURCC,
               cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));
    camera.set(cv::CAP_PROP_FRAME_WIDTH, 1280);
    camera.set(cv::CAP_PROP_FRAME_HEIGHT, 720);
    camera.set(cv::CAP_PROP_FPS, 15);

    if (!camera.isOpened()) {
        std::cerr << "无法打开摄像头 /dev/video0。" << std::endl;
        return 1;
    }

    cv::Mat frame;

    // 刚打开摄像头时，自动曝光可能还不稳定；先丢掉前 10 帧。
    for (int i = 0; i < 10; ++i) {
        if (!camera.read(frame) || frame.empty()) {
            std::cerr << "无法读取摄像头画面。" << std::endl;
            return 1;
        }
    }

    std::cout << "4 秒后拍摄，请摆好棋盘格……" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(4));

    // 等待期间可能积累了旧画面，连续读取几帧，最后一帧才是当前姿势。
    for (int i = 0; i < 5; ++i) {
        if (!camera.read(frame) || frame.empty()) {
            std::cerr << "无法读取摄像头画面。" << std::endl;
            return 1;
        }
    }

    const std::filesystem::path outputDir = "data/calibration_images";
    std::filesystem::create_directories(outputDir);

    // 找到还没有被使用的编号，避免覆盖以前拍好的标定图。
    std::filesystem::path outputPath;
    for (int index = 1; index <= 999; ++index) {
        std::ostringstream filename;
        filename << "calibration_" << std::setw(3) << std::setfill('0')
                 << index << ".jpg";
        outputPath = outputDir / filename.str();

        if (!std::filesystem::exists(outputPath)) {
            break;
        }
    }

    if (std::filesystem::exists(outputPath)) {
        std::cerr << "标定图片已达到 999 张，请整理 data/calibration_images。"
                  << std::endl;
        return 1;
    }

    if (!cv::imwrite(outputPath.string(), frame)) {
        std::cerr << "图片保存失败。" << std::endl;
        return 1;
    }

    std::cout << "已保存：" << outputPath.string() << std::endl;
    return 0;
}

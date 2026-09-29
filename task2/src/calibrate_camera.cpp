#include <opencv2/opencv.hpp>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <vector>

int main() {
    const cv::Size boardSize(9, 6);//行有9个内交叉点，列有6个内交叉点

    const float squareSize = 0.024f;// 一个小格的真实边长为24mm 也就是0.024m

    const std::filesystem::path imageDir = "data/calibration_images";//找到当前存有图的文件夹
    const std::filesystem::path cornerDir = "output/corners";//画出棋盘格角点的图片输出到corners文件夹

    std::vector<std::filesystem::path> imagePaths;//存放发现图片的数组

    for (const auto& entry : std::filesystem::directory_iterator(imageDir)) {//在imageDir对应路径中读取文件
        if (entry.is_regular_file() &&
            entry.path().extension() == ".jpg") {//如果不是子文件且是jpg格式
            imagePaths.push_back(entry.path());//将该读取到的文件存入imagePaths数组
        }
    }

    std::sort(imagePaths.begin(), imagePaths.end());//把图片按顺序排序好

    std::vector<std::vector<cv::Point2f>> imagePoints;//std::vector<cv::Point2f>表示一张图内所有角点，总式子表示所有的图的角点列表
    
     cv::Size imageSize;//创立一个imageSize记录标定图片的分辨率（宽和高）

    std::filesystem::create_directories(cornerDir);//创建cornerDir指向的文件夹

    for (const auto& imagePath : imagePaths) {//每次从imagePaths里取元素
        cv::Mat image = cv::imread(imagePath.string());

        if (image.empty()) {
            std::cerr << "无法读取：" << imagePath.string() << std::endl;
            continue;
        }

      
if (imageSize.empty()) {
    imageSize = image.size();
}//只记录第一张成功读到的图的尺寸及分辨率

else if (image.size() != imageSize) {
    std::cerr << "图片分辨率不一致，跳过："
              << imagePath.filename().string() << std::endl;
    continue;
}//确保后续所有图片分辨率一致

       
        cv::Mat gray;
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);//将棋盘转成灰度图

        std::vector<cv::Point2f> corners;//创立一个数组存放当前棋盘角点

        bool found = cv::findChessboardCorners(
            gray,//当前图
            boardSize,//开头已经定义每行9个，每列6个
            corners,
            cv::CALIB_CB_ADAPTIVE_THRESH |
            cv::CALIB_CB_NORMALIZE_IMAGE
        );//查找当前棋盘灰度图所有内角点并输出到corners中

        if (!found) {
            std::cout << "未找到角点：" << imagePath.filename().string()
                      << std::endl;
            continue;
        }//一个没找到或者没找全都会输出未找到角点并且跳过

        cv::cornerSubPix(
            gray,
            corners,//即作为输入，即已经得到的初步位置，也作为输出存放更精细位置
            cv::Size(11, 11),//每个角点周围取一个11x11像素窗口分析
            cv::Size(-1, -1),//不排除窗口中心的任何区域
            cv::TermCriteria(
                cv::TermCriteria::EPS + cv::TermCriteria::MAX_ITER,
                30,//最多微调30次
                0.001//下一次调整小于0.001的时候停止
            )
        );//在之前找到的大致角点位置进一步精细化，即corners里坐标会被更新

        imagePoints.push_back(corners);//记录这张图的有效角点到所有图角点列表

        cv::drawChessboardCorners(image, boardSize, corners, found);//在当前原图上画出角点

        std::filesystem::path outputPath =
            cornerDir / ("corners_" + imagePath.filename().string());//生成这张画好角点的图片要保存的完整路径，即取出原本文件名并加上前缀corners

        cv::imwrite(outputPath.string(), image);//保存该图

        std::cout << "识别成功：" << imagePath.filename().string()
                  << "，共 " << corners.size() << " 个角点。"
                  << std::endl;
    }




if (imagePoints.size() < 10) {
    std::cerr << "有效标定图少于 10 张，无法可靠标定。"
              << std::endl;
    return 1;
}


std::vector<cv::Point3f> oneBoardPoints;//创建一个真实棋盘，里面包含所有角点三维坐标

for (int row = 0; row < boardSize.height; ++row) {
    for (int col = 0; col < boardSize.width; ++col) {
        oneBoardPoints.emplace_back(
            col * squareSize,//之前定义的小格子边长，col为列
            row * squareSize,//row为行
            0.0f//深度为0
        );//左上角的角点默认原点(0,0,0),创立的每个角点加入棋盘数组
    }
}

// 每张有效图片面对的都是同一块真实棋盘格
std::vector<std::vector<cv::Point3f>> objectPoints(
    imagePoints.size(),//成功找到所有角点的图片个数
    oneBoardPoints
);//即创造包含imagePoints.size()个真实棋盘坐标数据副本的objecPoints合集，来和每个图对比


cv::Mat cameraMatrix;//存放标定后相机成像参数的矩阵
cv::Mat distortionCoefficients;//存放镜头畸变系数的矩阵

std::vector<cv::Mat> rotationVectors;//旋转向量列表，存所有照片向哪个方向倾斜
std::vector<cv::Mat> translationVectors;//平移向量列表，记录所有照片棋盘格相对相机的位置，如上下左右前后
//两者存每张棋盘格照片相对的棋盘格相对摄像头姿态

double reprojectionError = cv::calibrateCamera(
    objectPoints,//真实棋盘格角点位置，格式必须和imagepoints一致
    imagePoints,//角点在照片中的位置
    imageSize,//照片分辨率
    cameraMatrix,//输出到相机内参矩阵
    distortionCoefficients,//输出到镜头畸变系数矩阵
    rotationVectors,//输出到每张照片旋转度
    translationVectors//输出到每张照片位置
);//由现实棋盘格和照片棋盘格反推摄像头参数，并得到一个误差值，表示程序算出的相机模型和真实照片相差了多少像素

    std::cout << "共 " << imagePoints.size()
              << " 张图片成功找到棋盘格角点。" << std::endl;

std::cout << "平均重投影误差："<< reprojectionError << " 像素" << std::endl;

std::cout << "相机内参矩阵：" << std::endl;
std::cout << cameraMatrix << std::endl;

std::cout << "畸变系数：" << std::endl;
std::cout << distortionCoefficients << std::endl;

// 将标定结果保存和记录
cv::FileStorage file(
    "output/camera_calibration.yml",//生成的文件路径和名字，yml是普通文本配置文件，以名字:内容方式保存数据
    cv::FileStorage::WRITE//写入模式
);

if (!file.isOpened()) {
    std::cerr << "无法创建标定结果文件。" << std::endl;
    return 1;
}

file << "image_width" << imageSize.width;//imageSize.width为内容。image_width为名字
file << "image_height" << imageSize.height;

file << "board_inner_corners_width" << boardSize.width;
file << "board_inner_corners_height" << boardSize.height;
file << "square_size_m" << squareSize;

file << "valid_image_count"
     << static_cast<int>(imagePoints.size());

file << "rms_reprojection_error_px" << reprojectionError;
file << "camera_matrix" << cameraMatrix;
file << "distortion_coefficients" << distortionCoefficients;

file.release();//写完后关闭文件

std::cout << "标定数据已保存到 "
          << "output/camera_calibration.yml" << std::endl;

// 将标定结果保存和记录

    return 0;
}
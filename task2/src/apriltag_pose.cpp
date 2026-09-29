#include <opencv2/opencv.hpp>
#include <apriltag/apriltag.h>
#include <apriltag/tag36h11.h>//AprilTag检测器
#include <iostream>
#include <apriltag/apriltag_pose.h>
#include <cmath>

int main() {


    const double tagSize = 0.076;//Tag最外层的黑框为76mm，即0.076m
   
    cv::Mat cameraMatrix;
    cv::Mat distortionCoefficients;//定义这两个量分别接收测得的相机内参和畸变系数

   
    cv::FileStorage file(
        "output/camera_calibration.yml",
        cv::FileStorage::READ
    );//已读取模式打开之前保存的记录

    if (!file.isOpened()) {
        std::cerr << "无法打开 output/camera_calibration.yml。"
                  << std::endl;
        return 1;
    }//无法打开的报错

  
    file["camera_matrix"] >> cameraMatrix;
    file["distortion_coefficients"] >> distortionCoefficients;//从记录中取出相应数据存放到本代码的变量里

    file.release();//关闭文件

   
apriltag_family_t* tagFamily = tag36h11_create();//创建tag36h11家族

// 创建检测器，并告诉它只识别 tag36h11 家族
apriltag_detector_t* detector = apriltag_detector_create();//创建一个空检测器指针
apriltag_detector_add_family(detector, tagFamily);//将tagFamily加入检测器，即tag36h11

    if (cameraMatrix.empty() || distortionCoefficients.empty()) {
        std::cerr << "标定文件中缺少相机参数。" << std::endl;
        return 1;
    }//文件缺少相关数据时的报错

    
cv::VideoCapture camera("/dev/video0", cv::CAP_V4L2);//打开确认可用的摄像头节点

camera.set(cv::CAP_PROP_FOURCC,
           cv::VideoWriter::fourcc('M', 'J', 'P', 'G'));//请求摄像头输出 MJPG 格式
camera.set(cv::CAP_PROP_FRAME_WIDTH, 1280);
camera.set(cv::CAP_PROP_FRAME_HEIGHT, 720);//请求画面分辨率为1280x720
camera.set(cv::CAP_PROP_FPS, 15);//请求每秒15帧

if (!camera.isOpened()) {
    std::cerr << "无法打开摄像头 /dev/video0。" << std::endl;
    return 1;
}//摄像头打开失败则报错

cv::Mat frame;

for (int i = 0; i < 10; ++i) {
    if (!camera.read(frame) || frame.empty()) {
        std::cerr << "无法读取摄像头画面。" << std::endl;
        return 1;
    }
}//读不到画面或者画面为空报错

std::cout << "已成功读取一帧摄像头画面。" << std::endl;

cv::Mat gray;
cv::Mat result = frame.clone();//克隆一份原彩图，用作标记中心、角点等数据的最终图
cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);//将彩色照片转为灰白图存进gray

image_u8_t grayImage = {
    static_cast<int32_t>(gray.cols),//图片宽度
    static_cast<int32_t>(gray.rows),//图片高度
    static_cast<int32_t>(gray.step),//每行图像占的字节（步长）
    gray.data//指向opencv中gray的数据
};//把 OpenCV的灰度图 gray包装成 AprilTag能接受的int32_t图像格式


zarray_t* detections =
    apriltag_detector_detect(detector, &grayImage);//在图片里寻找符合 tag36h11 规则的 AprilTag，即把灰度图grayimage传给已经存入tag36h11的指示器detector，得到一个结果列表detections

std::cout << "共检测到 "
          << zarray_size(detections)
          << " 个 AprilTag。" << std::endl;


          for (int index = 0; index < zarray_size(detections); ++index) {
    apriltag_detection_t* detection;//创建一个变量等下来指向当前取出的tag的检测结果
    zarray_get(detections, index, &detection);//从detections结果列表中取出第index个结果写进detection
    std::cout << "检测到 AprilTag，ID = "
              << detection->id << std::endl; 
            
        
std::cout << "中心像素坐标：("
          << detection->c[0] << ", "
          << detection->c[1] << ")"
          << std::endl;//显示中心点在图中像素二维坐标

// Tag 四个角点在图片中的像素坐标
for (int cornerIndex = 0; cornerIndex < 4; ++cornerIndex) {
    std::cout << "角点 " << cornerIndex << "：("
              << detection->p[cornerIndex][0] << ", "
              << detection->p[cornerIndex][1] << ")"
              << std::endl;//显示Tag四个角点在图片中的像素二维坐标
               

}

         cv::Point center(
    static_cast<int>(detection->c[0]),
    static_cast<int>(detection->c[1])//把中心坐标的小数化为整数方便画图
);

cv::circle(result, center, 6, cv::Scalar(0, 255, 255), -1);//在result上把中心以半径为6像素、绿色红色叠加的颜色的实心圆的形式标记出来

cv::putText(result, "ID: " + std::to_string(detection->id),
            center + cv::Point(10, -10),
            cv::FONT_HERSHEY_SIMPLEX, 0.7,
            cv::Scalar(0, 255, 255), 2);//在tag中心点旁边加上编号，在中心往右 10 像素、往上 10 像素，字体大小为0.7，颜色为黄色，粗细2像素


    for (int cornerIndex = 0; cornerIndex < 4; ++cornerIndex) {

    cv::Point corner(
        static_cast<int>(detection->p[cornerIndex][0]),
        static_cast<int>(detection->p[cornerIndex][1])
    );//把角点转化为整数方便画图

    cv::circle(result, corner, 5, cv::Scalar(0, 0, 255), -1);//在result上把角点以5像素、红色的颜色的实心圆的形式标记出来
    cv::putText(result, std::to_string(cornerIndex),
                corner + cv::Point(8, -8),
                cv::FONT_HERSHEY_SIMPLEX, 0.5,
                cv::Scalar(0, 0, 255), 1);//在角点旁边加上编号，在中心往右8像素，往上8像素，字大小为0.5，颜色为红色，粗细1像素



}

apriltag_detection_info_t info;//创建一个叫 info 的信息包
info.det = detection;//放入当前检测到的结果，包括ID，图片的四个角点
info.tagsize = tagSize;//记录tag的实际边长，开头已定义为0.076
info.fx = cameraMatrix.at<double>(0, 0);//横向焦距参数
info.fy = cameraMatrix.at<double>(1, 1);//纵向焦距参数
info.cx = cameraMatrix.at<double>(0, 2);//中心横向位置
info.cy = cameraMatrix.at<double>(1, 2);//中心纵向位置
       
         //相机内参矩阵[ fx   0   cx ]
                   //[  0  fy   cy ]
                   //[  0   0    1 ]

apriltag_pose_t pose;//创建一个 pose 变量，准备存 Tag 的姿态结果
double poseError = estimate_tag_pose(&info, &pose);//调用姿态估计函数，把信息包info进行分析运算，算入的三维姿态放入pose//poseError接收函数返回的姿态误差，即三维投影到图有多少个角点吻合，越小可信度越高

double x = pose.t->data[0];//tag在镜头左右多少
double y = pose.t->data[1];//tag在镜头上下多少
double z = pose.t->data[2];//tag在镜头前后多少

double distance = std::sqrt(x * x + y * y + z * z);//AprilTag 到摄像头光学中心的直线距离

std::cout << "位移 t（米）：("
          << x << ", " << y << ", " << z << ")"
          << std::endl;

std::cout << "距离：" << distance << " 米" << std::endl;
std::cout << "姿态解算误差：" << poseError << std::endl;

cv::Mat rotationMatrix(3, 3, CV_64F, pose.R->data);//把 AprilTag 库算出的旋转结果，包装成 OpenCV 能方便使用的 3×3 矩阵

cv::Mat rvec;//用于将旋转矩阵转化为旋转向量，rvec的方向表示旋转轴朝哪里rvec的长度表示一共转了多少弧度，rvec=(rx, ry, rz)
cv::Rodrigues(rotationMatrix, rvec);//调用 OpenCV 的 Rodrigues 转换

cv::Mat tvec = (cv::Mat_<double>(3, 1) << x, y, z);//按顺序把 x、y、z 填进cv::Mat这个三行一列的double矩阵得到平移向量，再把平移向量保存到tvec

cv::drawFrameAxes(
    result,//画图的地方
    cameraMatrix,//相机内参
    distortionCoefficients,//镜头畸变参数
    rvec,//旋转向量
    tvec,//平移向量
    tagSize * 0.5,//每根坐标轴长度
    2//坐标轴线宽2像素
);//绘画三维坐标轴

std::cout << "旋转矩阵 R：" << std::endl;
std::cout << rotationMatrix << std::endl;

            
 }

 cv::imwrite("output/apriltag_pose_result.jpg", result);//把画图结果保存

    apriltag_detections_destroy(detections);//释放指针内存

    std::cout << "已读取相机内参：" << std::endl;
    std::cout << cameraMatrix << std::endl;

    std::cout << "已读取畸变系数：" << std::endl;
    std::cout << distortionCoefficients << std::endl;

apriltag_detector_destroy(detector);
tag36h11_destroy(tagFamily);//释放内存

    return 0;
}

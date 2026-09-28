#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <chrono>

int main(){

  const std::string videoPath="data/test_video2.webm";

  cv::VideoCapture capture(videoPath);//打开视频

  if (!capture.isOpened()) {
    std::cerr << "无法打开视频：" << videoPath << std::endl;
    return 1;
}//视频打不开则报错


  cv::Mat frame;
  int frameIndex = 0;
  
 cv::VideoWriter writer;//创立视频写入器

 double fps = 14.4;//根据原视频算得的帧率
 while (capture.read(frame)) {
   //每次自动读取下一帧，frame更新
    frameIndex++;
    bool saveDebug = (frameIndex == 1); //只给第一帧保存过程图
    auto startTime = std::chrono::steady_clock::now();//记下当前时刻，作为这一帧开始处理的时间

 if (saveDebug)cv::imwrite("output/sample_frame.png", frame);//保存第一帧图

std::vector<cv::Mat> bgrChannels;
cv::split(frame, bgrChannels);

if (saveDebug) cv::imwrite("output/blue_channel.png", bgrChannels[0]);
if (saveDebug) cv::imwrite("output/green_channel.png", bgrChannels[1]);
if (saveDebug) cv::imwrite("output/red_channel.png", bgrChannels[2]);//保存BGR分通道图

cv::Mat gray;
cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
if (saveDebug) cv::imwrite("output/gray.png", gray);//保存灰度图


cv::Mat blueMinusRed;
cv::subtract(bgrChannels[0], bgrChannels[2], blueMinusRed);//蓝减红

cv::Mat blueMask;
cv::threshold(blueMinusRed, blueMask, 60, 255, cv::THRESH_BINARY);//阈值为60，大于60显白，小于60显黑

if (saveDebug) cv::imwrite("output/blue_mask_raw.png", blueMask);//保存掩膜图


cv::Mat kernel3 = cv::getStructuringElement(
    cv::MORPH_RECT, cv::Size(3, 3)
);//得到一个3×3大小的矩形核

cv::Mat maskClose3;
cv::morphologyEx(blueMask, maskClose3, cv::MORPH_CLOSE, kernel3);//用前面的矩形核对之前的蓝色掩膜图进行闭运算，结果保存到maskClose3

if (saveDebug) cv::imwrite("output/blue_mask_close_3x3.png", maskClose3);//保存调整后的闭运算图
//第一组形态学操作

cv::Mat kernel5 = cv::getStructuringElement(
    cv::MORPH_RECT,
    cv::Size(5, 5)
);

cv::Mat maskClose5;
cv::morphologyEx(
    blueMask,
    maskClose5,
    cv::MORPH_CLOSE,
    kernel5
);

if (saveDebug) cv::imwrite(
    "output/blue_mask_close_5x5.png",
    maskClose5
);//同样步骤用5×5的核进行对比，后续保留3×3的核

cv::Mat maskOpen3;
cv::morphologyEx(blueMask, maskOpen3, cv::MORPH_OPEN, kernel3);
if (saveDebug) cv::imwrite("output/blue_mask_open_3x3.png", maskOpen3);//相同操作，保存开运算图
//第二组形态学操作

cv::Mat contourInput = maskClose3.clone();//闭运算的掩膜图副本
std::vector<std::vector<cv::Point>> contours;//存白色区域的数组

cv::findContours(
    contourInput,
    contours,
    cv::RETR_EXTERNAL,//只找最外层边界
    cv::CHAIN_APPROX_SIMPLE//保留关键点
);

cv::Mat contourView = frame.clone();//原始彩色图的副本

cv::drawContours(
    contourView,
    contours,
    -1,//画出全部轮廓
    cv::Scalar(0, 255, 0),//绿色
    2//线宽为2
);//在原来彩色图副本上根据找到的白色边界用绿色描出来
if (saveDebug) cv::imwrite("output/contours_all.png", contourView);//保存轮廓图

std::vector<std::vector<cv::Point>> polygons;//存放近似多边形的数组

for (const auto& contour : contours) {
    //遍历，每次从contours中取一条
    double perimeter = cv::arcLength(contour, true);//计算该轮廓完整一圈周长

    std::vector<cv::Point> polygon;//局部变量，存少量关键点的数组

    cv::approxPolyDP(contour, polygon, 0.02 * perimeter, true);//多边形近似，将原始很多点的contour以0.02 * perimeter的简化差别简化为polygon，并闭合

    polygons.push_back(polygon);//将该条轮廓存入多边形数组
}

cv::Mat polygonView = frame.clone();//复制一份原彩色图

cv::drawContours(
    polygonView,
    polygons,
    -1,//画出全部轮廓
    cv::Scalar(255, 0, 255),//留蓝色和红色(紫色)
    2//线宽为2
);

if (saveDebug) cv::imwrite("output/polygons_all.png", polygonView);//保存多边形近似结果

cv::Mat rectView = frame.clone();//再次创立一个原彩图副本
int lightBarCount = 0; // 本帧通过几何筛选的灯条数量

for (const auto& polygon : polygons) {
    //遍历，每次选择一条灯条多边形
    if (polygon.size() < 3) {
        continue;
    }//少于三个点无法形成正常必和区域则跳过

    cv::RotatedRect rect = cv::minAreaRect(polygon);//根据当前多边形得到一个刚好包住它的、面积最小的可旋转倾斜矩形 rect

    float width = rect.size.width;
    float height = rect.size.height;//旋转矩形的两条边长

    float longSide = std::max(width, height);
    float shortSide = std::min(width, height);//定义长边和短边

    float area = width * height;//矩形面积
    float ratio = longSide / shortSide;//长边短边之比


    if (area < 30 || ratio < 1.5) {
        continue;
    }//灯条面积过小，或者长宽比不够大，及不够细长，则不能被算成灯条
    //几何筛选
    lightBarCount++;//当前灯管数加1

    cv::Point2f corners[4];//装矩形四个顶点的二维顶点数组
    rect.points(corners);//将rect顶点数据写入corners

    for (int i = 0; i < 4; i++) {
        cv::line(
            rectView,//在副本上画线
            corners[i],//线的起点
            corners[(i + 1) % 4],//线的终点
            cv::Scalar(0, 255, 255),//绿色加红色(黄色)
            2//线宽两像素
        );
    }
}
auto endTime = std::chrono::steady_clock::now();

double processingMs =
    std::chrono::duration<double, std::milli>(
        endTime - startTime
    ).count();//得到经过的时间

cv::putText(
    rectView,//将字画到rectView上
    "Frame: " + std::to_string(frameIndex),//显示第几帧
    cv::Point(20, 70),//文字位置
    cv::FONT_HERSHEY_SIMPLEX, //文字字体
    0.8,//字体缩放大小
    cv::Scalar(255, 255, 255),//文字颜色(白色)
    2//粗细
);

cv::putText(
    rectView,
    "Light bars: " + std::to_string(lightBarCount),//通过面积、长宽比筛选的灯条数量
    cv::Point(20, 105),
    cv::FONT_HERSHEY_SIMPLEX,
    0.8,
    cv::Scalar(255, 255, 255),//白色
    2
);

cv::putText(
    rectView,
    "Time: " + std::to_string(static_cast<int>(processingMs)) + " ms",
    cv::Point(20, 140),
    cv::FONT_HERSHEY_SIMPLEX,
    0.8,
    cv::Scalar(255, 255, 255),
    2
);



if (!writer.isOpened()) {
    writer.open(
        "output/task1_result.mp4",
        cv::VideoWriter::fourcc('m', 'p', '4', 'v'),//视频的编码方式
        fps,//使用最开始读取的帧率
        frame.size()//保证输出尺寸一样大
    );

    if (!writer.isOpened()) {
        std::cerr << "无法创建输出视频。" << std::endl;
        return 1;
    }
}

writer.write(rectView);//把当前循环得到的一帧存入视频

if (saveDebug) cv::imwrite("output/light_bars_filtered.png", rectView);//保存旋转矩形贴灯管图


}
writer.release();//释放内存

std::cout << "共处理了 " << frameIndex << " 帧视频。" << std::endl;












    return 0;
}
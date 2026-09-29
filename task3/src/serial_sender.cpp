#include <fcntl.h>    
#include <unistd.h>   
#include <iostream>
#include <string>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <cstdint>
#include <thread>

unsigned char calculateChecksum(const std::string& body) {
    unsigned char checksum = 0;

    for (unsigned char character : body) {
        checksum ^= character;
    }

    return checksum;
}//计算一条字符串的异或校验和的函数，以便在串口通信时检查数据有没有传错



std::string createPacket(std::uint32_t seq, long long tMs) {
    std::ostringstream bodyStream;//正文拼接器

    bodyStream << "CV1,"
               << seq << ","
               << tMs << ","
               << "1,0,"
               << std::fixed << std::setprecision(1)
               << 100.0 << ","
               << -50.0 << ","
               << 800.0 << ","
               << std::setprecision(6)
               << 0.0 << ","
               << 0.0 << ","
               << 0.0;

    std::string body = bodyStream.str();//将结果从正文拼接器取出得到正文字符串
    unsigned char checksum = calculateChecksum(body);//计算校验码

    std::ostringstream messageStream;//报文拼接器

    messageStream << '$' << body << '*'
                  << std::uppercase << std::hex
                  << std::setw(2) << std::setfill('0')
                  << static_cast<int>(checksum)
                  << "\r\n";//将开始结束符号、正文、校验码、回车换行符拼接为报文

    return messageStream.str();
}//生成报文的函数




int main(){


    const char* portPath = "/tmp/vision_serial";//发送端使用的虚拟串口路径

    int serialFd = open(portPath, O_WRONLY | O_NOCTTY);//用只写的方式打开该端口

    if (serialFd == -1) {
        std::cerr << "无法打开虚拟串口：" << portPath << std::endl;
        return 1;
    }//无法打开端口时报错



auto startTime = std::chrono::steady_clock::now();//记录程序开始发送报告时刻
std::uint32_t sequence = 0;//创建从0开始的报文序号，每发送一帧加1

while (true) {
    auto currentTime = std::chrono::steady_clock::now();//记录当前循环程序开始时间

    long long elapsedMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            currentTime - startTime
        ).count();//计算程序从启动到现在经过多少毫秒保存到elapsedMs

    std::string message = createPacket(sequence, elapsedMs);//调用写好的 createPacket 函数，将序号和毫秒数填进正文，并进行校验值、拼接一些列操作得到message报文

    ssize_t written = write(
        serialFd,//写入的发送端口
        message.c_str(),//发送字符串起始位置
        message.size()//报文字符
    );//准备发送数据

    if (written != static_cast<ssize_t>(message.size())) {
        std::cerr << "发送不完整。" << std::endl;
        break;
    }//检查是否完整发送

    std::cout << message;//方便观察当前发送的报文内容

    sequence++;//序号加1

    std::this_thread::sleep_for(
        std::chrono::milliseconds(100)
    );//让程序暂停约 100 毫秒，然后再进行下一轮
}

      close(serialFd);//用完端口关闭

      

    return 0;
}
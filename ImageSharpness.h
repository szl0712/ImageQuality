#ifndef IMAGESHARPNESS_H
#define IMAGESHARPNESS_H
#include <QImage>
#include <QPair>
#include <mutex>
#include <opencv2/opencv.hpp>

class ImageSharpness
{
public:
    // 清晰度分数
    static double calcUniversalSharpness(const QImage &img);
    // 平均亮度
    static double calcAverageBrightness(const QImage &img);
    static QPair<double, double> getBrightRawAndScore(const QImage &img);
    QPair<QString, int> getBrightEvalTextAndScore(const QImage &img);

    // 对比度计算
    static double calcContrast(const QImage &img);
    // 返回 pair：<欠曝像素占比0~1, 过曝像素占比0~1>
    static QPair<double,double> calcOverUnderExposureRatio(const QImage &img);

private:
    // 工具：QImage 转为单通道灰度 cv::Mat
    static cv::Mat qimageToGrayMat(const QImage &srcImg);

    static std::mutex mtx;

};

#endif // IMAGESHARPNESS_H

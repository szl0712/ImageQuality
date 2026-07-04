#include "ImageSharpness.h"
#include <opencv2/opencv.hpp>
#include <QImage>
#include <QDebug>
#include <algorithm>

// ============================================================
// 工具函数：QImage -> cv::Mat
// ============================================================
cv::Mat ImageSharpness::qimageToGrayMat(const QImage &img)
{
    // 步骤1：将QImage转换为OpenCV的Mat格式（BGR彩色）
    cv::Mat bgr;
    switch (img.format()) {
    case QImage::Format_RGB888:
        // RGB888格式：直接转换
        bgr = cv::Mat(img.height(), img.width(), CV_8UC3,
                      const_cast<uchar*>(img.bits()), img.bytesPerLine()).clone();
        cv::cvtColor(bgr, bgr, cv::COLOR_RGB2BGR);
        break;
    case QImage::Format_ARGB32:
    case QImage::Format_RGBA8888:
        // RGBA格式：去掉Alpha通道
        bgr = cv::Mat(img.height(), img.width(), CV_8UC4,
                      const_cast<uchar*>(img.bits()), img.bytesPerLine()).clone();
        cv::cvtColor(bgr, bgr, cv::COLOR_RGBA2BGR);
        break;
    default:
        // 其他格式：先转为RGB888
        QImage rgb = img.convertToFormat(QImage::Format_RGB888);
        bgr = cv::Mat(rgb.height(), rgb.width(), CV_8UC3,
                      const_cast<uchar*>(rgb.bits()), rgb.bytesPerLine()).clone();
        cv::cvtColor(bgr, bgr, cv::COLOR_RGB2BGR);
        break;
    }

    // 步骤2：彩色转灰度
    cv::Mat gray;
    cv::cvtColor(bgr, gray, cv::COLOR_BGR2GRAY);
    return gray;
}

// =============================
// 1. Laplacian energy
// =============================
static double calcLaplacian(const cv::Mat &gray)
{
    cv::Mat f;
    gray.convertTo(f, CV_64F);

    cv::Mat lap;
    cv::Laplacian(f, lap, CV_64F, 3);

    cv::Scalar meanAbs = cv::mean(cv::abs(lap));
    return meanAbs[0];
}

// =============================
// 2. Tenengrad (Sobel energy)
// =============================
static double calcTenengrad(const cv::Mat &gray)
{
    cv::Mat gx, gy;
    cv::Sobel(gray, gx, CV_64F, 1, 0, 3);
    cv::Sobel(gray, gy, CV_64F, 0, 1, 3);

    cv::Mat g2 = gx.mul(gx) + gy.mul(gy);
    return cv::mean(g2)[0];
}

// =============================
// 3. Edge Ratio (structure coverage)
// =============================
static double calcEdgeRatio(const cv::Mat &gray)
{
    cv::Mat edges;
    cv::Canny(gray, edges, 50, 150);

    double edgePixels = cv::countNonZero(edges);
    return edgePixels / (double)(gray.rows * gray.cols);
}

// ============================================================
// 主接口
// ============================================================
double ImageSharpness::calcUniversalSharpness(const QImage &img)
{
    if (img.isNull()) return 0.0;

    cv::Mat gray = qimageToGrayMat(img);

    int w = gray.cols;
    int h = gray.rows;

    int minEdge = std::min(w, h);
    int blockSize = std::max((int)(minEdge * 0.25), 150);

    int cx = w / 2;
    int cy = h / 2;
    int dx = blockSize / 3;

    std::vector<cv::Rect> rois = {
        {cx - blockSize/2, cy - blockSize/2, blockSize, blockSize},
        {cx - blockSize/2, cy - blockSize/2 - dx, blockSize, blockSize},
        {cx - blockSize/2, cy - blockSize/2 + dx, blockSize, blockSize},
        {cx - blockSize/2 - dx, cy - blockSize/2, blockSize, blockSize},
        {cx - blockSize/2 + dx, cy - blockSize/2, blockSize, blockSize}
    };

    cv::Rect imgRect(0, 0, w, h);

    std::vector<double> Ls, Ts, Rs;

    for (auto r : rois)
    {
        r = r & imgRect;
        if (r.width < 10 || r.height < 10) continue;

        cv::Mat patch = gray(r);

        Ls.push_back(calcLaplacian(patch));
        Ts.push_back(calcTenengrad(patch));
        Rs.push_back(calcEdgeRatio(patch));
    }

    if (Ls.size() < 3) return 0.0;

    // =============================
    // 1. 排序 + trimmed mean
    // =============================
    auto trimmedMean = [](std::vector<double> v)
    {
        std::sort(v.begin(), v.end());
        int trim = v.size() / 5;

        double sum = 0;
        int cnt = 0;

        for (size_t i = trim; i < v.size() - trim; i++)
        {
            sum += v[i];
            cnt++;
        }
        return cnt > 0 ? sum / cnt : 0.0;
    };

    double L = trimmedMean(Ls);
    double T = trimmedMean(Ts);
    double R = trimmedMean(Rs);

    // =============================
    // 2. 归一化（关键稳定性）
    // =============================

    L = log(1.0 + L);
    T = log(1.0 + T);

    // EdgeRatio本身是0~1，做轻微增强
    R = std::sqrt(R);

    // =============================
    // 3. 融合（核心）
    // =============================
    double raw = 0.45 * L + 0.35 * T + 0.20 * R;

    // =============================
    // 4. 映射到 0~100（稳定 sigmoid）
    // =============================
    double score = 1.0 / (1.0 + exp(-(raw - 3.0) * 1.8)) * 100.0;

    return score;
}

QPair<double, double> ImageSharpness::getBrightRawAndScore(const QImage &img)
{
    if (img.isNull())
        return {0.0, 0.0};

    cv::Mat gray = qimageToGrayMat(img);
    if (gray.empty())
        return {0.0, 0.0};

    cv::Scalar meanVal;
    cv::meanStdDev(gray, meanVal, cv::noArray());
    double grayMean = meanVal[0];
    double normMean = grayMean / 255.0;

    int totalPixel = gray.rows * gray.cols;

    // 直方图
    cv::Mat hist;
    int histSize = 256;
    float range[] = {0, 256};
    const float* histRange = {range};
    cv::calcHist(&gray, 1, nullptr, cv::Mat(), hist, 1, &histSize, &histRange);

    // 更精细的区间划分
    double darkCount = 0.0;      // 0-30 严重欠曝
    double shadowCount = 0.0;    // 31-70 暗部
    double midCount = 0.0;       // 71-184 中间调
    double highlightCount = 0.0; // 185-229 高光
    double overCount = 0.0;      // 230-255 严重过曝

    for (int i = 0; i < 256; i++) {
        double v = hist.at<float>(i);
        if (i <= 30) darkCount += v;
        else if (i <= 70) shadowCount += v;
        else if (i <= 184) midCount += v;
        else if (i <= 229) highlightCount += v;
        else overCount += v;
    }

    double darkRatio = darkCount / totalPixel;
    double shadowRatio = shadowCount / totalPixel;
    double midRatio = midCount / totalPixel;
    double highlightRatio = highlightCount / totalPixel;
    double overRatio = overCount / totalPixel;

    // 有效亮度范围得分（中间调 + 轻微高光/暗部都算有效）
    double validRangeScore = (midRatio + shadowRatio * 0.8 + highlightRatio * 0.8) * 100.0;

    // 非线性惩罚：少量过曝/欠曝轻罚，大量重罚
    // 使用平方根压缩惩罚力度，让轻微过曝不会导致分数暴跌
    double penalty = (std::sqrt(darkRatio) * 30.0 + std::sqrt(overRatio) * 35.0);

    double exposureScore = validRangeScore - penalty;
    exposureScore = std::clamp(exposureScore, 0.0, 100.0);

    // 均值评分：动态基准
    // 根据过曝/欠曝比例判断照片风格，自适应基准
    double targetMean;
    if (overRatio > 0.15 && darkRatio < 0.05) {
        targetMean = 160.0; // 高-key 照片，基准偏亮
    } else if (darkRatio > 0.15 && overRatio < 0.05) {
        targetMean = 90.0;  // 低-key 照片，基准偏暗
    } else {
        targetMean = 128.0; // 正常
    }

    double meanDiff = std::abs(grayMean - targetMean);
    double meanScore = 100.0 - std::min(meanDiff / 80.0, 1.0) * 40.0; // 最大扣 40 分
    meanScore = std::clamp(meanScore, 0.0, 100.0);

    // 最终加权：提高 meanScore 权重，降低 exposureScore 权重
    // 因为直方图区间划分有主观性，均值更稳定
    double finalScore = exposureScore * 0.55 + meanScore * 0.45;
    finalScore = std::clamp(finalScore, 0.0, 100.0);

    return {finalScore, normMean};
}

double ImageSharpness::calcAverageBrightness(const QImage &img)
{
    return getBrightRawAndScore(img).second;
}

double ImageSharpness::calcContrast(const QImage &img)
{
    if(img.isNull())
        return 0.0;

    cv::Mat gray = qimageToGrayMat(img);

    int blockCount = 0;
    double totalContrast = 0.0;
    const int blockSize = 8;
    const double flatThreshold = 0.02;

    int rows = gray.rows;
    int cols = gray.cols;

    for (int y = 0; y < rows; y += blockSize)
    {
        int bh = std::min(blockSize, rows - y);
        for (int x = 0; x < cols; x += blockSize)
        {
            int bw = std::min(blockSize, cols - x);
            cv::Rect roi(x, y, bw, bh);
            cv::Mat patch = gray(roi);

            double minV, maxV;
            cv::minMaxLoc(patch, &minV, &maxV);

            if ((maxV + minV) < 1.0)
                continue;

            double michelson = (maxV - minV) / (maxV + minV);
            if (michelson < flatThreshold)
                continue;

            totalContrast += michelson;
            blockCount++;
        }
    }

    if (blockCount <= 0)
    {
        return 0.0;
    }

    double avgRaw = totalContrast / blockCount;
    double stretched = std::min(avgRaw / 0.35, 1.0);
    return std::clamp(stretched, 0.0, 1.0);
}

QPair<double, double> ImageSharpness::calcOverUnderExposureRatio(const QImage &img)
{
    if (img.isNull())
        return {0.0, 0.0};

    cv::Mat gray = qimageToGrayMat(img);

    int totalPixel = gray.rows * gray.cols;

    if(totalPixel <= 0)
        return {0.0, 0.0};

    cv::Mat hist;

    int histSize = 256;

    float range[] = {0,256};

    const float* histRange = {range};

    cv::calcHist(&gray, 1, nullptr, cv::Mat(), hist, 1, &histSize, &histRange);

    double darkCount = 0.0;
    double brightCount = 0.0;

    for(int i=0; i<=50; i++)
        darkCount += hist.at<float>(i);

    for(int i=206; i<256; i++)
        brightCount += hist.at<float>(i);

    double darkRatio = darkCount / totalPixel;

    double brightRatio = brightCount / totalPixel;

    return {darkRatio, brightRatio};
}
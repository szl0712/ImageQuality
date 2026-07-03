#ifndef IMAGELOADER_H
#define IMAGELOADER_H
#include <QString>
#include <QPixmap>
#include <QImage>
#include <QImageReader>
#include <QSize>

class ImageLoader
{
public:
    bool loadImageQt(const QString& filePath, QImage& originImg, QPixmap& previewPix, QString& errMsg, QByteArray& outRawData);

    // 限制长边最大像素，超大图提前缩小，防止OOM
    static QImage downSampleImage(const QImage &src, int maxLongSide = 2000);

    // 通过文件二进制头部识别图片格式，Windows/Android双端通用
    static QString getImageFormatByHeader(const QString& filePath);

    static QString getImageFormatByHeader(const QByteArray& fileData);

private:
    static QImage downSampleForPreview(const QImage& srcImg, int maxEdge = 1200);

    // 手写WebP解码核心函数
    static QImage SimpleWebPDecode(const QByteArray& data, QString& err);
};
#endif
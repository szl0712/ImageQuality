#include "ImageLoader.h"
#include <QFileInfo>
#include <QFile>
#include <algorithm>
#include <QtMath>
#include <QImageReader>
#include <QBuffer>
#include <QSize>
#include <QByteArray>

bool ImageLoader::loadImageQt(const QString &filePath, QImage &originImg, QPixmap &previewPix, QString &errMsg, QByteArray &outRawData)
{
    originImg = QImage();
    previewPix = QPixmap();
    errMsg.clear();
    outRawData.clear(); // 清空输出字节数组

#ifdef Q_OS_ANDROID
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        errMsg = "文件打开失败：" + file.errorString();
        return false;
    }
    QByteArray imgData = file.readAll();
    outRawData = imgData; // 把读到的原图二进制输出出去
    file.close();

    QBuffer buf(&imgData);
    buf.open(QIODevice::ReadOnly);
    QImageReader reader(&buf);
    reader.setAutoTransform(true);
#else
    // Windows 读取完整二进制存入outRawData
    QImageReader reader(filePath);
    reader.setAutoTransform(true);
    // 读取原图二进制
    QFile f(filePath);
    if(f.open(QIODevice::ReadOnly))
    {
        outRawData = f.readAll();
        f.close();
    }
#endif

    originImg = reader.read();

    if (originImg.isNull())
    {
        errMsg = QString("图片解码失败：%1").arg(reader.errorString());
        return false;
    }

    QImage previewImg = downSampleForPreview(originImg, 1200);
    previewPix = QPixmap::fromImage(previewImg);
    return true;
}

QImage ImageLoader::downSampleImage(const QImage &src, int maxLongSide)
{
    if (src.isNull())
        return QImage();

    int w = src.width();
    int h = src.height();

    // 长边本身小于限制，无需缩放直接返回原图
    if (w <= maxLongSide && h <= maxLongSide)
        return src;

    // 计算缩放比例
    qreal scale;
    if (w > h)
        scale = qreal(maxLongSide) / w;
    else
        scale = qreal(maxLongSide) / h;

    int newW = qRound(w * scale);
    int newH = qRound(h * scale);

    // SmoothTransformation 平滑降采样，保留梯度、明暗特征，不影响画质打分
    return src.scaled(newW, newH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

QImage ImageLoader::downSampleForPreview(const QImage &srcImg, int maxEdge)
{
    int w = srcImg.width();
    int h = srcImg.height();
    if (w <= maxEdge && h <= maxEdge)
        return srcImg;
    double scale = static_cast<double>(maxEdge) / std::max(w, h);
    return srcImg.scaled(w * scale, h * scale, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

// 内存版：直接从字节数组取头部，无文件IO
QString ImageLoader::getImageFormatByHeader(const QByteArray &fileData)
{
    if(fileData.size() < 12)
        return "未知";
    QByteArray header = fileData.left(12);

    // JPG FF D8 FF
    if (header.startsWith("\xFF\xD8\xFF"))
        return "JPG";
    // PNG 89 50 4E 47
    if (header.startsWith("\x89PNG"))
        return "PNG";
    // WebP RIFF + WEBP
    if (header.size() >= 12 && header.mid(0,4) == "RIFF" && header.mid(8,4) == "WEBP")
        return "WebP";
    // HEIC
    if (header.size() >= 12 && header.mid(4,4) == "ftyp")
    {
        QString subType = header.mid(8,4);
        if (subType == "heic" || subType == "heix")
            return "HEIC";
    }
    return "未知";
}

QString ImageLoader::getImageFormatByHeader(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return "未知";
    QByteArray headerBuf = file.read(12);
    file.close();
    return getImageFormatByHeader(headerBuf);
}
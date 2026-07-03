#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QFile>
#include <QElapsedTimer>
#include <QMessageBox>
#include <QFileInfo>
#include <QResizeEvent>
#include <QApplication>
#include <QStandardPaths>
#include <QStringList>
#include "ImageSharpness.h"
#include "ImageLoader.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_btnOpenImg_clicked()
{
    QString path = QFileDialog::getOpenFileName(
        this, "选择图片", "", "*.jpg *.jpeg *.png *.webp *.heic"
        );
    if (path.isEmpty())
        return;

    ImageLoader loader;
    QImage srcImg;
    QPixmap preview;
    QString err;
    QByteArray rawData;
    QElapsedTimer t;
    t.start();
    bool ok = false;
    try {
        ok = loader.loadImageQt(path, srcImg, preview, err, rawData);
    } catch (...) {
        err = "图片解码内存异常，文件损坏或格式不支持";
        ok = false;
    }
    int ms = t.elapsed();

    if (ok) {
        ui->textLog->clear();

        m_originImg = srcImg;
        QPixmap fitPix = QPixmap::fromImage(m_originImg).scaled(
            ui->lblPreview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        ui->lblPreview->setPixmap(fitPix);

        QFileInfo info(path);
        QString log;
        QImage analyseImg = ImageLoader::downSampleImage(srcImg);

        double sharpScore = ImageSharpness::calcUniversalSharpness(srcImg);

        QString sharpLevel;

        if (sharpScore < 25)
            sharpLevel = "严重模糊";
        else if (sharpScore < 50)
            sharpLevel = "模糊";
        else if (sharpScore < 75)
            sharpLevel = "清晰";
        else
            sharpLevel = "非常清晰";

        int sharpSubInt = qRound(sharpScore);

        auto brightInfo = ImageSharpness::getBrightRawAndScore(analyseImg);
        double finalScore = brightInfo.first;
        auto expRatio = ImageSharpness::calcOverUnderExposureRatio(analyseImg);
        double darkRatio = expRatio.first;
        double brightRatio = expRatio.second;
        QString brightnessLevel;

        bool severeOver = (brightRatio > 0.30) || (brightRatio > 0.20 && finalScore < 55);
        bool moderateOver = (brightRatio > 0.15) || (brightRatio > 0.08 && finalScore < 70);
        bool mildOver = (brightRatio > 0.05);

        bool severeUnder = (darkRatio > 0.30) || (darkRatio > 0.20 && finalScore < 55);
        bool moderateUnder = (darkRatio > 0.15) || (darkRatio > 0.08 && finalScore < 70);
        bool mildUnder = (darkRatio > 0.05);

        if (severeOver)
        {
            brightnessLevel = "亮度严重偏高";
        }
        else if (severeUnder)
        {
            brightnessLevel = "亮度严重偏低";
        }
        else if (moderateOver)
        {
            brightnessLevel = "亮度偏高";
        }
        else if (moderateUnder)
        {
            brightnessLevel = "亮度偏低";
        }
        else if (mildOver)
        {
            brightnessLevel = "亮度略高";
        }
        else if (mildUnder)
        {
            brightnessLevel = "亮度略低";
        }
        else
        {
            brightnessLevel = "亮度适中";
        }

        int brightSubInt = static_cast<int>(std::round(finalScore));

        double contVal = ImageSharpness::calcContrast(analyseImg);

        // 0~1 → 线性拉伸为0~100，对应日志打印数值
        double contNorm = contVal * 100.0;

        // 仅做轻微人眼Gamma矫正，无断崖压缩，和亮度保持统一
        double contSub = std::pow(contNorm / 100.0, 0.9) * 100.0;
        contSub = std::clamp(contSub, 0.0, 100.0);

        // 文字评级只做展示，不干预分数！
        QString contLevel;
        if (contNorm < 20)
        {
            contLevel = "对比度偏低，画面发灰";
        }
        else if (contNorm <= 50)
        {
            contLevel = "对比度适中";
        }
        else
        {
            contLevel = "对比度充足，层次丰富";
        }

        int contSubInt = qRound(contSub);

        int totalScore = qRound(sharpSubInt * 0.4 + brightSubInt * 0.35 + contSubInt * 0.25);
        QString imgFormat = ImageLoader::getImageFormatByHeader(rawData);

        log = QString(
                  " 文件:%1\n"
                  " 格式:%2 | 尺寸:%3×%4 | 处理耗时:%5 ms\n"
                  "【清晰度】原始分数:%6  评级:%7  分项分:%8\n"
                  "【曝光亮度】均值:%9  欠曝占比:%10%  过曝占比:%11%  评级:%12  分项分:%13\n"
                  "【对比度】标准差:%14  评级:%15  分项分:%16\n"
                  "==== 综合画质总分:%17 ===="
                  )
                  .arg(info.fileName())
                  .arg(imgFormat)
                  .arg(srcImg.width())
                  .arg(srcImg.height())
                  .arg(ms)
                  .arg(sharpScore, 0, 'f', 2)
                  .arg(sharpLevel)
                  .arg(sharpSubInt)
                  .arg(finalScore, 0, 'f', 2)        // 0~100亮度分值
                  .arg(darkRatio * 100.0, 0, 'f', 1)       // 暗部百分比
                  .arg(brightRatio * 100.0, 0, 'f', 1)     // 高亮百分比
                  .arg(brightnessLevel)
                  .arg(brightSubInt)
                  .arg(contNorm, 0, 'f', 2)
                  .arg(contLevel)
                  .arg(contSubInt)
                  .arg(totalScore);

        ui->textLog->append(log);
    } else {
        ui->textLog->append("错误：" + err);
        QMessageBox::warning(this, "加载失败", err);
    }
}

void MainWindow::on_btnExportReport_clicked()
{
    QString savePath;
#if defined(Q_OS_ANDROID)
    QString docPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    savePath = QFileDialog::getSaveFileName(
        this,
        "保存画质分析报告",
        docPath + "/图像画质检测报告.txt",
        "文本文件 (*.txt)"
        );
#else
    savePath = QFileDialog::getSaveFileName(
        this,
        "保存画质分析报告",
        "图像画质检测报告.txt",
        "文本文件 (*.txt)"
        );
#endif

    if (savePath.isEmpty())
        return;
    QFile outFile(savePath);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::warning(this, "导出失败", "文件无法写入，请更换保存路径！");
        return;
    }
    QString logContent = ui->textLog->toPlainText();
    outFile.write(logContent.toUtf8());
    outFile.close();
    QMessageBox::information(this, "导出完成",
                             QString("报告已保存至：\n%1").arg(savePath));
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    if (!m_originImg.isNull())
    {
        QPixmap fitPix = QPixmap::fromImage(m_originImg).scaled(
            ui->lblPreview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        ui->lblPreview->setPixmap(fitPix);
    }
}
#include "ui/mainwindow.h"

#include <QApplication>
#include <QMetaType>
#include <QImage>
#include <QDebug>

#include <opencv2/core.hpp>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    qRegisterMetaType<QImage>("QImage");

    // ========================================================
    // 【必须显式注册 SourceConfig】
    // MainWindow 通过 QMetaObject::invokeMethod(..., Q_ARG(SourceConfig, cfg))
    // 以队列方式把参数投递给子线程的 DetectionWorker::start()。
    // 队列连接要求参数类型对 Qt 元对象系统可见，
    // Q_DECLARE_METATYPE 只是编译期声明，这里补上运行期注册更保险，
    // 否则可能报 "Unable to handle unregistered datatype 'SourceConfig'"。
    // ========================================================
    qRegisterMetaType<SourceConfig>("SourceConfig");

    // ---- OpenCV 环境自检 ----
    qDebug() << "OpenCV version:" << CV_VERSION;
    cv::Mat probe(2, 2, CV_8UC1, cv::Scalar(0));
    qDebug() << "OpenCV link check OK, Mat =" << probe.rows << "x" << probe.cols;

    MainWindow w;
    w.resize(1100, 750);
    w.show();
    return a.exec();
}

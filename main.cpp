#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    qputenv("QT_IM_MODULE",QByteArray("Qt5Input")); // 有这条语句，就可以触发虚拟键盘
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}

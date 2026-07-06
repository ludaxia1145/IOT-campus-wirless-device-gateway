#include "mainwindow.h"
#include <QApplication>
#include <QProcessEnvironment>

int main(int argc, char *argv[])
{
    // 启用Qt虚拟键盘（必须在QApplication创建之前设置）
    qputenv("QT_IM_MODULE", QByteArray("qtvirtualkeyboard"));

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}

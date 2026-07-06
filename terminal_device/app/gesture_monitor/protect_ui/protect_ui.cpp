/*
 * 文件名: protect_ui.cpp
 * 说明: Qt 保护页面主窗口实现
 * 功能: 显示蓝色背景的故障提示页面
 * 编译: qmake -project
 *       qmake
 *       make
 */

#include <QApplication>
#include <QMainWindow>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>
#include <QFont>
#include <QPalette>
#include <QColor>
#include <QScreen>

#include "protect_ui.h"

/*
 * 构造函数: ProtectUI
 * 说明: 初始化保护页面UI
 */
ProtectUI::ProtectUI(QWidget *parent)
    : QMainWindow(parent)
{
    setupUI();
}

/*
 * 析构函数: ~ProtectUI
 * 说明: 析构函数
 */
ProtectUI::~ProtectUI()
{
}

/*
 * 函数: setupUI
 * 说明: 设置UI界面
 *       - 蓝色背景（RGB: 30, 144, 255 - 道奇蓝）
 *       - 显示故障提示文本
 *       - 全屏显示
 */
void ProtectUI::setupUI()
{
    /* 设置窗口属性 */
    setWindowTitle("保护页面 - 设备故障");
    
    /* 获取屏幕尺寸，设置全屏 */
    QScreen *screen = QGuiApplication::primaryScreen();
    setGeometry(screen->geometry());
    
    /* 创建中央Widget和布局 */
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);
    
    /* 设置蓝色背景 */
    QPalette palette;
    palette.setColor(QPalette::Window, QColor(30, 144, 255));  /* 道奇蓝 */
    centralWidget->setPalette(palette);
    centralWidget->setAutoFillBackground(true);
    
    /* 创建故障提示标签 */
    QLabel *titleLabel = new QLabel("此设备出现故障，不可使用", this);
    
    /* 设置标签字体 */
    QFont font;
    font.setPointSize(48);          /* 大字体 */
    font.setBold(true);             /* 加粗 */
    titleLabel->setFont(font);
    
    /* 设置标签文字颜色为白色 */
    QPalette labelPalette;
    labelPalette.setColor(QPalette::WindowText, QColor(255, 255, 255));  /* 白色 */
    titleLabel->setPalette(labelPalette);
    
    /* 标签对齐到中心 */
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(
        "QLabel {"
        "   color: white;"
        "   background-color: rgba(30, 144, 255, 255);"
        "}"
    );
    
    /* 将标签添加到布局 */
    layout->addStretch(1);
    layout->addWidget(titleLabel);
    layout->addStretch(1);
    
    /* 设置中央Widget */
    setCentralWidget(centralWidget);
    
    /* 设置窗口样式表（蓝色背景） */
    setStyleSheet(
        "QMainWindow {"
        "   background-color: rgb(30, 144, 255);"
        "}"
    );
    
    /* 显示窗口 */
    showMaximized();
}

/*
 * 主函数
 */
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    ProtectUI window;
    window.show();
    
    return app.exec();
}

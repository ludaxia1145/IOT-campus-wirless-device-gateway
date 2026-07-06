/*
 * 文件名: protect_ui.h
 * 说明: Qt 保护页面主窗口头文件
 */

#ifndef PROTECT_UI_H
#define PROTECT_UI_H

#include <QMainWindow>

class ProtectUI : public QMainWindow
{
    Q_OBJECT

public:
    ProtectUI(QWidget *parent = nullptr);
    ~ProtectUI();

private:
    void setupUI();
};

#endif // PROTECT_UI_H

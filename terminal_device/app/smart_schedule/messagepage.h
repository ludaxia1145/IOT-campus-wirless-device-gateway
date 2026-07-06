#ifndef MESSAGEPAGE_H
#define MESSAGEPAGE_H

#include <QWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QLineEdit>
#include "networkclient.h"
#include "audioplayer.h"
class MessagePage : public QWidget
{
    Q_OBJECT
public:
    explicit MessagePage(NetworkClient *client, QWidget *parent = nullptr);

signals:
    void backToMain();

private slots:
    void sendMessage();
    void onResponseReceived(const QString &type, const QJsonObject &data);

private:
    NetworkClient *networkClient;
    QLineEdit *studentNameEdit;  // 添加学生姓名输入
    QLineEdit *studentIdEdit;    // 添加学号输入
    QTextEdit *messageEdit;
    QPushButton *btnSend;
    QPushButton *btnBack;
};

#endif

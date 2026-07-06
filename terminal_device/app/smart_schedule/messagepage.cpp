#include "messagepage.h"
#include "audioplayer.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>
#include <QDateTime>

MessagePage::MessagePage(NetworkClient *client, QWidget *parent)
    : QWidget(parent), networkClient(client)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);

    QLabel *title = new QLabel("教学办留言");
    title->setStyleSheet("font-size: 24px; font-weight: bold;");
    layout->addWidget(title);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->setSpacing(10);

    studentNameEdit = new QLineEdit();
    studentNameEdit->setPlaceholderText("请输入姓名");
    studentNameEdit->setStyleSheet("font-size: 16px; padding: 8px;");
    formLayout->addRow("学生姓名:", studentNameEdit);

    studentIdEdit = new QLineEdit();
    studentIdEdit->setPlaceholderText("请输入学号");
    studentIdEdit->setStyleSheet("font-size: 16px; padding: 8px;");
    formLayout->addRow("学号:", studentIdEdit);

    layout->addLayout(formLayout);
    layout->addSpacing(10);

    QLabel *hint = new QLabel("留言内容:");
    layout->addWidget(hint);

    messageEdit = new QTextEdit();
    messageEdit->setPlaceholderText("请输入留言内容...");
    messageEdit->setStyleSheet("font-size: 16px; padding: 10px;");
    layout->addWidget(messageEdit);

    QHBoxLayout *buttonLayout = new QHBoxLayout();

    btnSend = new QPushButton("发送留言");
    btnSend->setStyleSheet("font-size: 18px; padding: 10px 30px; background-color: #4CAF50; color: white;");

    btnBack = new QPushButton("返回");
    btnBack->setStyleSheet("font-size: 18px; padding: 10px 30px; background-color: #2196F3; color: white;");

    buttonLayout->addWidget(btnSend);
    buttonLayout->addWidget(btnBack);
    layout->addLayout(buttonLayout);

    connect(btnSend, &QPushButton::clicked, this, &MessagePage::sendMessage);
    connect(btnBack, &QPushButton::clicked, this, &MessagePage::backToMain);
    connect(networkClient, &NetworkClient::responseReceived,
            this, &MessagePage::onResponseReceived);
}

void MessagePage::sendMessage()
{
    QString studentName = studentNameEdit->text().trimmed();
    QString studentId = studentIdEdit->text().trimmed();
    QString message = messageEdit->toPlainText().trimmed();

    if (studentName.isEmpty() || studentId.isEmpty() || message.isEmpty()) {
        QMessageBox::warning(this, "提示", "请填写完整信息！");
        return;
    }

    QJsonObject data;
    data["student_name"] = studentName;
    data["student_id"] = studentId;
    data["message"] = message;
    data["timestamp"] = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

    networkClient->sendMessage("send_message", data);
}

void MessagePage::onResponseReceived(const QString &type, const QJsonObject &data)
{
    if (type == "message_response") {
        if (data["status"].toString() == "success") {
            AudioPlayer::instance()->playSuccess();  // 播放提交成功音频
            QMessageBox::information(this, "成功", "留言已发送！");
            studentNameEdit->clear();
            studentIdEdit->clear();
            messageEdit->clear();
        } else {
            QMessageBox::warning(this, "失败", "留言发送失败：" + data["error"].toString());
        }
    }
}

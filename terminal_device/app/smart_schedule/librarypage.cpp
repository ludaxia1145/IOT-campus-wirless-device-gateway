#include "librarypage.h"
#include "audioplayer.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QMessageBox>
#include <QScrollArea>
#include <QComboBox>
#include <QJsonArray>
#include <QDateTime>

LibraryPage::LibraryPage(NetworkClient *client, QWidget *parent)
    : QWidget(parent), networkClient(client), selectedSeat(-1)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);

    QLabel *title = new QLabel("图书馆座位预约");
    title->setStyleSheet("font-size: 24px; font-weight: bold;");
    layout->addWidget(title);

    QHBoxLayout *controlLayout = new QHBoxLayout();

    QLabel *timeLabel = new QLabel("时间段:");
    controlLayout->addWidget(timeLabel);

    timeSlotCombo = new QComboBox();
    timeSlotCombo->addItem("上午");
    timeSlotCombo->addItem("下午");
    timeSlotCombo->addItem("晚上");
    timeSlotCombo->setStyleSheet("font-size: 16px; padding: 5px;");
    controlLayout->addWidget(timeSlotCombo);

    controlLayout->addSpacing(20);

    QLabel *idLabel = new QLabel("学号:");
    controlLayout->addWidget(idLabel);

    studentIdEdit = new QLineEdit();
    studentIdEdit->setPlaceholderText("请输入学号");
    studentIdEdit->setStyleSheet("font-size: 16px; padding: 5px;");
    studentIdEdit->setMaximumWidth(150);
    controlLayout->addWidget(studentIdEdit);

    controlLayout->addStretch();

    layout->addLayout(controlLayout);
    layout->addSpacing(10);

    QLabel *legend = new QLabel("图例: 蓝色=可预约  红色=已预约  绿色=已选择");
    legend->setStyleSheet("font-size: 14px; color: #666;");
    layout->addWidget(legend);

    scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("QScrollArea { border: 2px solid #ccc; background-color: #f5f5f5; }");

    seatContainer = new QWidget();
    seatLayout = new QGridLayout(seatContainer);
    seatLayout->setSpacing(5);

    // 创建200个座位按钮 (20行 x 10列)
    for (int i = 1; i <= 200; i++) {
        QPushButton *btn = new QPushButton(QString::number(i));
        btn->setFixedSize(50, 50);
        btn->setProperty("seatNumber", i);
        btn->setStyleSheet(
            "QPushButton { background-color: #2196F3; color: white; font-size: 14px; "
            "border-radius: 5px; font-weight: bold; }"
            "QPushButton:hover { background-color: #1976D2; }"
        );

        int row = (i - 1) / 10;
        int col = (i - 1) % 10;
        seatLayout->addWidget(btn, row, col);

        seatButtons[i] = btn;
        connect(btn, &QPushButton::clicked, this, &LibraryPage::onSeatClicked);
    }

    seatContainer->setLayout(seatLayout);
    scrollArea->setWidget(seatContainer);
    layout->addWidget(scrollArea);

    // 按钮区域
    QHBoxLayout *buttonLayout = new QHBoxLayout();

    btnBack = new QPushButton("返回");
    btnBack->setStyleSheet("font-size: 18px; padding: 10px 30px; background-color: #2196F3; color: white;");

    buttonLayout->addStretch();
    buttonLayout->addWidget(btnBack);
    layout->addLayout(buttonLayout);

    connect(btnBack, &QPushButton::clicked, this, &LibraryPage::backToMain);
    connect(timeSlotCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(loadSeats()));
    connect(networkClient, &NetworkClient::librarySeatsReceived,
            this, &LibraryPage::onLibrarySeatsReceived);
    connect(networkClient, &NetworkClient::responseReceived,
            this, &LibraryPage::onResponseReceived);
}

void LibraryPage::loadSeats()
{
    currentTimeSlot = timeSlotCombo->currentText();
    selectedSeat = -1;

    QJsonObject data;
    networkClient->sendMessage("get_seat_status", data);
}

void LibraryPage::onLibrarySeatsReceived(const QJsonObject &data)
{
    QJsonArray seats = data["seats"].toArray();
    seatOwners.clear();

    for (const QJsonValue &val : seats) {
        QJsonObject seat = val.toObject();
        int seatNumber = seat["seat_number"].toInt();
        bool isReserved = seat["is_reserved"].toBool();
        QString studentId = seat["student_id"].toString();

        if (seatButtons.contains(seatNumber)) {
            QPushButton *btn = seatButtons[seatNumber];
            if (isReserved) {
                btn->setStyleSheet(
                    "QPushButton { background-color: #f44336; color: white; font-size: 14px; "
                    "border-radius: 5px; font-weight: bold; }"
                    "QPushButton:hover { background-color: #d32f2f; }"
                );
                seatOwners[seatNumber] = studentId;
            } else {
                btn->setStyleSheet(
                    "QPushButton { background-color: #2196F3; color: white; font-size: 14px; "
                    "border-radius: 5px; font-weight: bold; }"
                    "QPushButton:hover { background-color: #1976D2; }"
                );
            }
        }
    }
}

void LibraryPage::onSeatClicked()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    int seatNumber = btn->property("seatNumber").toInt();
    QString studentId = studentIdEdit->text().trimmed();

    if (studentId.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先输入学号！");
        return;
    }

    QString timeSlotEn = "morning";
    if (currentTimeSlot == "下午") timeSlotEn = "afternoon";
    else if (currentTimeSlot == "晚上") timeSlotEn = "evening";

    if (seatOwners.contains(seatNumber)) {
        if (seatOwners[seatNumber] == studentId) {
            QMessageBox::StandardButton reply = QMessageBox::question(
                this, "取消预约", "确定要取消座位" + QString::number(seatNumber) + "的预约吗？",
                QMessageBox::Yes | QMessageBox::No
            );

            if (reply == QMessageBox::Yes) {
                QJsonObject data;
                data["seat_number"] = seatNumber;
                data["student_id"] = studentId;
                networkClient->sendMessage("cancel_reservation", data);
            }
        } else {
            QMessageBox::warning(this, "提示", "该座位已被其他同学预约！");
        }
    } else {
        selectedSeat = seatNumber;
        btn->setStyleSheet(
            "QPushButton { background-color: #4CAF50; color: white; font-size: 14px; "
            "border-radius: 5px; font-weight: bold; }"
        );

        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "预约座位", "确定预约座位" + QString::number(seatNumber) + "（" + currentTimeSlot + "）吗？",
            QMessageBox::Yes | QMessageBox::No
        );

        if (reply == QMessageBox::Yes) {
            QJsonObject data;
            data["seat_number"] = seatNumber;
            data["student_id"] = studentId;
            data["time_slot"] = timeSlotEn;
            data["timestamp"] = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
            networkClient->sendMessage("reserve_seat", data);
        } else {
            btn->setStyleSheet(
                "QPushButton { background-color: #2196F3; color: white; font-size: 14px; "
                "border-radius: 5px; font-weight: bold; }"
                "QPushButton:hover { background-color: #1976D2; }"
            );
        }
    }
}

void LibraryPage::onResponseReceived(const QString &type, const QJsonObject &data)
{
    if (type == "reservation_response") {
        if (data["status"].toString() == "success") {
            AudioPlayer::instance()->playSuccess();  // 播放提交成功音频
            QMessageBox::information(this, "成功",
                "预约成功！\n座位号：" + QString::number(data["seat_number"].toInt()));
            loadSeats();
        } else {
            QMessageBox::warning(this, "失败", "预约失败：" + data["error"].toString());
            loadSeats();
        }
    }
}

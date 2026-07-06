#include "noticepage.h"
#include "audioplayer.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QJsonArray>

NoticePage::NoticePage(NetworkClient *client, QWidget *parent)
    : QWidget(parent), networkClient(client)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    stackedWidget = new QStackedWidget();

    listPage = new QWidget();
    QVBoxLayout *listLayout = new QVBoxLayout(listPage);
    listLayout->setContentsMargins(20, 20, 20, 20);

    QLabel *title = new QLabel("校园公告");
    title->setStyleSheet("font-size: 24px; font-weight: bold;");
    listLayout->addWidget(title);

    noticeList = new QListWidget();
    noticeList->setStyleSheet("QListWidget { font-size: 16px; } QListWidget::item { padding: 10px; border-bottom: 1px solid #ddd; }");
    listLayout->addWidget(noticeList);

    QHBoxLayout *buttonLayout = new QHBoxLayout();

    btnRefresh = new QPushButton("刷新");
    btnRefresh->setStyleSheet("font-size: 18px; padding: 10px 30px; background-color: #4CAF50; color: white;");

    btnBack = new QPushButton("返回");
    btnBack->setStyleSheet("font-size: 18px; padding: 10px 30px; background-color: #2196F3; color: white;");

    buttonLayout->addWidget(btnRefresh);
    buttonLayout->addWidget(btnBack);
    listLayout->addLayout(buttonLayout);

    stackedWidget->addWidget(listPage);

    detailPage = new QWidget();
    QVBoxLayout *detailLayout = new QVBoxLayout(detailPage);
    detailLayout->setContentsMargins(20, 20, 20, 20);

    detailTitle = new QLabel();
    detailTitle->setStyleSheet("font-size: 22px; font-weight: bold; color: #333;");
    detailTitle->setWordWrap(true);
    detailLayout->addWidget(detailTitle);

    detailTime = new QLabel();
    detailTime->setStyleSheet("font-size: 14px; color: #666; margin-bottom: 10px;");
    detailLayout->addWidget(detailTime);

    detailContent = new QTextEdit();
    detailContent->setReadOnly(true);
    detailContent->setStyleSheet("font-size: 16px; border: 1px solid #ddd; padding: 10px;");
    detailLayout->addWidget(detailContent);

    btnBackToList = new QPushButton("返回列表");
    btnBackToList->setStyleSheet("font-size: 18px; padding: 10px 30px; background-color: #2196F3; color: white;");
    detailLayout->addWidget(btnBackToList);

    stackedWidget->addWidget(detailPage);

    mainLayout->addWidget(stackedWidget);

    connect(btnRefresh, &QPushButton::clicked, this, &NoticePage::refreshNotices);
    connect(btnBack, &QPushButton::clicked, this, &NoticePage::backToMain);
    connect(btnBackToList, &QPushButton::clicked, this, &NoticePage::showNoticeList);
    connect(noticeList, &QListWidget::itemClicked, this, &NoticePage::onNoticeItemClicked);
    connect(networkClient, &NetworkClient::noticeListReceived,
            this, &NoticePage::onNoticeListReceived);
    connect(networkClient, &NetworkClient::noticeDetailReceived,
            this, &NoticePage::onNoticeDetailReceived);
}

void NoticePage::refreshNotices()
{
    QJsonObject data;
    networkClient->sendMessage("get_notices", data);
}

void NoticePage::onNoticeListReceived(const QJsonObject &data)
{
    QJsonArray notices = data["notices"].toArray();

    // 如果有新公告，播放提示音
    if (notices.size() > 0 && noticeList->count() < notices.size()) {
        AudioPlayer::instance()->playNewMessage();  // 播放新消息音频
    }

    noticeList->clear();

    for (const QJsonValue &val : notices) {
        QJsonObject obj = val.toObject();
        QString itemText = "[" + obj["type"].toString() + "] " + obj["title"].toString() + "\n" +
                          "时间：" + obj["time"].toString();

        QListWidgetItem *item = new QListWidgetItem(itemText, noticeList);
        item->setData(Qt::UserRole, obj["notice_id"].toInt());
    }
}

void NoticePage::onNoticeItemClicked(QListWidgetItem *item)
{
    int noticeId = item->data(Qt::UserRole).toInt();

    QJsonObject data;
    data["notice_id"] = noticeId;
    networkClient->sendMessage("get_notice_detail", data);
}

void NoticePage::onNoticeDetailReceived(const QJsonObject &data)
{
    detailTitle->setText(data["title"].toString());
    detailTime->setText("发布时间：" + data["time"].toString());

    QString content = data["content"].toString();
    content.replace("\\n", "\n");
    detailContent->setText(content);

    stackedWidget->setCurrentWidget(detailPage);
}

void NoticePage::showNoticeList()
{
    stackedWidget->setCurrentWidget(listPage);
}

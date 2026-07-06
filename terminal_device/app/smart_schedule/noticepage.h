#ifndef NOTICEPAGE_H
#define NOTICEPAGE_H

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QTextEdit>
#include <QStackedWidget>
#include <QLabel>
#include "networkclient.h"
#include "audioplayer.h"
class NoticePage : public QWidget
{
    Q_OBJECT
public:
    explicit NoticePage(NetworkClient *client, QWidget *parent = nullptr);
    void refreshNotices();

signals:
    void backToMain();

private slots:
    void onNoticeListReceived(const QJsonObject &data);
    void onNoticeDetailReceived(const QJsonObject &data);
    void onNoticeItemClicked(QListWidgetItem *item);
    void showNoticeList();

private:
    NetworkClient *networkClient;
    QStackedWidget *stackedWidget;

    // 列表页面
    QWidget *listPage;
    QListWidget *noticeList;
    QPushButton *btnRefresh;
    QPushButton *btnBack;

    // 详情页面
    QWidget *detailPage;
    QLabel *detailTitle;
    QTextEdit *detailContent;
    QLabel *detailTime;
    QPushButton *btnBackToList;
};

#endif

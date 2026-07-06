#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLabel>
#include <QStackedWidget>
#include <QTimer>
#include "networkclient.h"

class MessagePage;
class LibraryPage;
class NoticePage;
class AttendancePage;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onCourseInfoReceived(const QJsonObject &info);
    void onConnectionStatusChanged(bool connected);
    void showMainPage();
    void showMessagePage();
    void showLibraryPage();
    void showNoticePage();
    void showAttendancePage();
    void updateTime();
    void refreshCourseInfo();
    void scheduleNextRefresh();

private:
    void setupUI();
    void setupConnections();

    QWidget *mainPage;
    QLabel *courseNameLabel;
    QLabel *teacherLabel;
    QLabel *classLabel;
    QLabel *timeLabel;
    QLabel *roomLabel;
    QLabel *statusLabel;
    QLabel *currentTimeLabel;

    QPushButton *btnMessage;
    QPushButton *btnLibrary;
    QPushButton *btnNotice;
    QPushButton *btnAttendance;

    QStackedWidget *stackedWidget;

    MessagePage *messagePage;
    LibraryPage *libraryPage;
    NoticePage *noticePage;
    AttendancePage *attendancePage;

    NetworkClient *networkClient;

    QTimer *timeTimer;
    QTimer *courseRefreshTimer;
};

#endif

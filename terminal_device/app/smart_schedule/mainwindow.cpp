#include "mainwindow.h"
#include "messagepage.h"
#include "librarypage.h"
#include "noticepage.h"
#include "attendancepage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QDateTime>
#include <QFont>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    networkClient = new NetworkClient(this);

    setupUI();

    setupConnections();

    networkClient->connectToServer("192.168.72.249", 8888);

    timeTimer = new QTimer(this);
    connect(timeTimer, &QTimer::timeout, this, &MainWindow::updateTime);
    timeTimer->start(1000);
    updateTime();

    courseRefreshTimer = new QTimer(this);
    connect(courseRefreshTimer, &QTimer::timeout, this, &MainWindow::scheduleNextRefresh);
    courseRefreshTimer->start(60000); // 每分钟检查一次是否到达7:00

    // 初始化时先查询一次课程信息
    QTimer::singleShot(1000, this, &MainWindow::refreshCourseInfo);

    // 立即检查是否需要调度刷新
    scheduleNextRefresh();
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI()
{
    setWindowTitle("xx大学");
    resize(800, 480);

    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    stackedWidget = new QStackedWidget(centralWidget);

    mainPage = new QWidget();
    mainPage->setStyleSheet("background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
                            "stop:0 #26d0ce, stop:1 #1a2980);");

    QVBoxLayout *mainLayout = new QVBoxLayout(mainPage);
    mainLayout->setContentsMargins(30, 30, 30, 30);

    QHBoxLayout *topLayout = new QHBoxLayout();

    QLabel *schoolLabel = new QLabel("狗头大学 | 4号教学楼4-106");
    schoolLabel->setStyleSheet("color: white; font-size: 16px;");
    topLayout->addWidget(schoolLabel);

    topLayout->addStretch();

    currentTimeLabel = new QLabel();
    currentTimeLabel->setStyleSheet("color: white; font-size: 16px;");
    topLayout->addWidget(currentTimeLabel);

    mainLayout->addLayout(topLayout);

    mainLayout->addSpacing(40);

    courseNameLabel = new QLabel("");
    courseNameLabel->setStyleSheet("color: white; font-size: 48px; font-weight: bold;");
    mainLayout->addWidget(courseNameLabel);

    mainLayout->addSpacing(30);

    QVBoxLayout *infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(15);

    teacherLabel = new QLabel("授课教师：?");
    teacherLabel->setStyleSheet("color: white; font-size: 20px;");
    infoLayout->addWidget(teacherLabel);

    classLabel = new QLabel("授课班级：?");
    classLabel->setStyleSheet("color: white; font-size: 18px;");
    infoLayout->addWidget(classLabel);

    timeLabel = new QLabel("授课时间：?");
    timeLabel->setStyleSheet("color: white; font-size: 20px;");
    infoLayout->addWidget(timeLabel);

    roomLabel = new QLabel("教室类型：?");
    roomLabel->setStyleSheet("color: white; font-size: 18px;");
    infoLayout->addWidget(roomLabel);

    mainLayout->addLayout(infoLayout);

    mainLayout->addStretch();

    QGridLayout *buttonLayout = new QGridLayout();
    buttonLayout->setSpacing(15);

    btnMessage = new QPushButton("教学办留言");
    btnLibrary = new QPushButton("图书馆预约");
    btnNotice = new QPushButton("校园公告");
    btnAttendance = new QPushButton("课堂考勤");

    QString buttonStyle = "QPushButton {"
                         "background-color: rgba(255, 255, 255, 0.2);"
                         "color: white;"
                         "border: 2px solid white;"
                         "border-radius: 10px;"
                         "font-size: 18px;"
                         "padding: 15px;"
                         "min-width: 150px;"
                         "min-height: 60px;"
                         "}"
                         "QPushButton:pressed {"
                         "background-color: rgba(255, 255, 255, 0.4);"
                         "}";

    btnMessage->setStyleSheet(buttonStyle);
    btnLibrary->setStyleSheet(buttonStyle);
    btnNotice->setStyleSheet(buttonStyle);
    btnAttendance->setStyleSheet(buttonStyle);

    buttonLayout->addWidget(btnMessage, 0, 0);
    buttonLayout->addWidget(btnLibrary, 0, 1);
    buttonLayout->addWidget(btnNotice, 1, 0);
    buttonLayout->addWidget(btnAttendance, 1, 1);

    mainLayout->addLayout(buttonLayout);

    statusLabel = new QLabel("连接中...");
    statusLabel->setStyleSheet("color: yellow; font-size: 14px;");
    mainLayout->addWidget(statusLabel);

    stackedWidget->addWidget(mainPage);

    messagePage = new MessagePage(networkClient, this);
    libraryPage = new LibraryPage(networkClient, this);
    noticePage = new NoticePage(networkClient, this);
    attendancePage = new AttendancePage(networkClient, this);

    stackedWidget->addWidget(messagePage);
    stackedWidget->addWidget(libraryPage);
    stackedWidget->addWidget(noticePage);
    stackedWidget->addWidget(attendancePage);

    QVBoxLayout *centralLayout = new QVBoxLayout(centralWidget);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->addWidget(stackedWidget);

    connect(btnMessage, &QPushButton::clicked, this, &MainWindow::showMessagePage);
    connect(btnLibrary, &QPushButton::clicked, this, &MainWindow::showLibraryPage);
    connect(btnNotice, &QPushButton::clicked, this, &MainWindow::showNoticePage);
    connect(btnAttendance, &QPushButton::clicked, this, &MainWindow::showAttendancePage);

    connect(messagePage, SIGNAL(backToMain()), this, SLOT(showMainPage()));
    connect(libraryPage, SIGNAL(backToMain()), this, SLOT(showMainPage()));
    connect(noticePage, SIGNAL(backToMain()), this, SLOT(showMainPage()));
    connect(attendancePage, SIGNAL(backToMain()), this, SLOT(showMainPage()));
}

void MainWindow::setupConnections()
{
    connect(networkClient, &NetworkClient::courseInfoReceived,
            this, &MainWindow::onCourseInfoReceived);
    connect(networkClient, &NetworkClient::connectionStatusChanged,
            this, &MainWindow::onConnectionStatusChanged);
}

void MainWindow::refreshCourseInfo()
{
    if (networkClient->isConnected()) {
        QJsonObject data;
        data["classroom"] = "4-106";
        networkClient->sendMessage("get_course", data);
        qDebug() << "[v0] 刷新课程信息...";
    }
}

void MainWindow::onCourseInfoReceived(const QJsonObject &info)
{
    QString courseName = info["course_name"].toString();
    QString teacher = info["teacher"].toString();
    QString classInfo = info["class"].toString();
    QString time = info["time"].toString();
    QString roomType = info["room_type"].toString();

    if (courseName == "暂无课程") {
        courseNameLabel->setText("暂无课程");
        teacherLabel->setText("");
        classLabel->setText("");
        timeLabel->setText("当前时间段无课程安排");
        roomLabel->setText("");
    } else {
        courseNameLabel->setText(courseName);
        teacherLabel->setText("授课教师：" + teacher);
        classLabel->setText("授课班级：" + classInfo);
        timeLabel->setText("授课时间：" + time);
        roomLabel->setText("教室类型：" + roomType);
    }

    qDebug() << "[v0] 课程信息已更新：" << courseName;
}

void MainWindow::onConnectionStatusChanged(bool connected)
{
    if (connected) {
        statusLabel->setText("✓ 已连接");
        statusLabel->setStyleSheet("color: #00ff00; font-size: 14px;");
        qDebug() << "[v0] 服务器连接成功";
    } else {
        statusLabel->setText("✗ 未连接");
        statusLabel->setStyleSheet("color: red; font-size: 14px;");
        qDebug() << "[v0] 服务器连接断开";
    }
}

void MainWindow::showMainPage()
{
    stackedWidget->setCurrentWidget(mainPage);
}

void MainWindow::showMessagePage()
{
    stackedWidget->setCurrentWidget(messagePage);
}

void MainWindow::showLibraryPage()
{
    stackedWidget->setCurrentWidget(libraryPage);
    libraryPage->loadSeats();
}

void MainWindow::showNoticePage()
{
    stackedWidget->setCurrentWidget(noticePage);
    noticePage->refreshNotices();
}

void MainWindow::showAttendancePage()
{
    stackedWidget->setCurrentWidget(attendancePage);
    attendancePage->loadAttendanceList();
}

void MainWindow::updateTime()
{
    QString currentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    currentTimeLabel->setText(currentTime);
}

void MainWindow::scheduleNextRefresh()
{
    QDateTime now = QDateTime::currentDateTime();
    QDateTime nextRefresh = now;

    // 设置下次刷新时间为明天的7:00
    nextRefresh.setTime(QTime(7, 0, 0));

    // 如果现在的时间还没到今天的7:00，则设置为今天的7:00
    if (now.time() < QTime(7, 0, 0)) {
        nextRefresh = now;
        nextRefresh.setTime(QTime(7, 0, 0));
    } else {
        // 如果已经过了今天的7:00，则设置为明天的7:00
        nextRefresh = now.addDays(1);
        nextRefresh.setTime(QTime(7, 0, 0));
    }

    // 计算等待时间（毫秒）
    qint64 msecUntilRefresh = now.msecsTo(nextRefresh);

    qDebug() << "[v0] 下次课程查询时间：" << nextRefresh.toString("yyyy-MM-dd hh:mm:ss");
    qDebug() << "[v0] 等待时间（秒）：" << msecUntilRefresh / 1000;

    // 设置单次定时器在指定时间触发刷新
    QTimer::singleShot(msecUntilRefresh, this, &MainWindow::refreshCourseInfo);

    // 递归调用自己，为下一天的7:00做准备
    // 这样每当这个函数被调用时，它都会为下一个7:00设置一个定时器
    QTimer::singleShot(msecUntilRefresh + 1000, this, &MainWindow::scheduleNextRefresh);
}

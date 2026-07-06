#ifndef ATTENDANCEPAGE_H
#define ATTENDANCEPAGE_H

#include <QWidget>
#include <QPushButton>
#include <QScrollArea>
#include <QGridLayout>
#include <QSet>
#include <QLabel>
#include <QLineEdit>
#include <QDialog>
#include "networkclient.h"
#include "audioplayer.h"

class TeacherLoginDialog : public QDialog {
    Q_OBJECT
public:
    explicit TeacherLoginDialog(NetworkClient *client, const QString &classroom, QWidget *parent = nullptr);
    bool isAuthenticated() const { return authenticated; }

private slots:
    void onVerifyClicked();
    void onResponseReceived(const QString &type, const QJsonObject &data);

private:
    NetworkClient *networkClient;
    QString classroomId;
    QLineEdit *teacherIdEdit;
    QPushButton *btnVerify;
    QPushButton *btnCancel;
    QLabel *statusLabel;
    bool authenticated;
};

class AttendancePage : public QWidget
{
    Q_OBJECT
public:
    explicit AttendancePage(NetworkClient *client, QWidget *parent = nullptr);
    void loadAttendanceList();
    void setCourseId(int id) { currentCourseId = id; }
    bool showTeacherLoginDialog();

signals:
    void backToMain();

private slots:
    void onAttendanceListReceived(const QJsonObject &list);
    void submitAttendance();
    void onResponseReceived(const QString &type, const QJsonObject &data);
    void onStudentButtonClicked();

private:
    NetworkClient *networkClient;
    QScrollArea *scrollArea;
    QWidget *studentContainer;
    QGridLayout *studentLayout;
    QLabel *infoLabel;
    QPushButton *btnSubmit;
    QPushButton *btnBack;

    QMap<QString, QPushButton*> studentButtons;  // 学号 -> 按钮
    QSet<QString> absentStudents;
    int currentCourseId;
    bool teacherAuthenticated;
};

#endif

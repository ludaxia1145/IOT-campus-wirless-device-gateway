#include "attendancepage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QJsonArray>
#include <QDateTime>
#include <QDebug>
#include <QRegularExpressionValidator>

TeacherLoginDialog::TeacherLoginDialog(NetworkClient *client, const QString &classroom, QWidget *parent)
    : QDialog(parent), networkClient(client), classroomId(classroom), authenticated(false)
{
    setWindowTitle("教师身份验证");
    setModal(true);
    setFixedSize(400, 280);

    QVBoxLayout *layout = new QVBoxLayout(this);

    QLabel *classroomLabel = new QLabel("教室: " + classroom);
    classroomLabel->setStyleSheet("font-size: 14px; color: #333;");
    layout->addWidget(classroomLabel);

    QLabel *title = new QLabel("请输入教师工号");
    title->setStyleSheet("font-size: 18px; font-weight: bold;");
    layout->addWidget(title);

    QLabel *tipLabel = new QLabel("(10位数字)");
    tipLabel->setStyleSheet("font-size: 12px; color: #666;");
    layout->addWidget(tipLabel);

    teacherIdEdit = new QLineEdit();
    teacherIdEdit->setPlaceholderText("请输入工号，例如：1001000001");
    teacherIdEdit->setStyleSheet("font-size: 16px; padding: 8px;");
    QRegularExpressionValidator *validator = new QRegularExpressionValidator(QRegularExpression("^\\d{0,10}$"), this);
    teacherIdEdit->setValidator(validator);
    layout->addWidget(teacherIdEdit);

    statusLabel = new QLabel("");
    statusLabel->setStyleSheet("font-size: 12px; color: red;");
    layout->addWidget(statusLabel);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnVerify = new QPushButton("验证");
    btnVerify->setStyleSheet("font-size: 14px; padding: 8px 20px; background-color: #4CAF50; color: white;");
    btnCancel = new QPushButton("取消");
    btnCancel->setStyleSheet("font-size: 14px; padding: 8px 20px; background-color: #f44336; color: white;");

    btnLayout->addWidget(btnVerify);
    btnLayout->addWidget(btnCancel);
    layout->addLayout(btnLayout);

    connect(btnVerify, &QPushButton::clicked, this, &TeacherLoginDialog::onVerifyClicked);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(networkClient, &NetworkClient::responseReceived,
            this, &TeacherLoginDialog::onResponseReceived);
}

void TeacherLoginDialog::onVerifyClicked()
{
    QString teacherId = teacherIdEdit->text().trimmed();
    if (teacherId.length() != 10) {
        statusLabel->setText("工号必须是10位数字");
        return;
    }

    QJsonObject data;
    data["teacher_id"] = teacherId;
    data["classroom"] = classroomId;
    networkClient->sendMessage("verify_teacher", data);
    statusLabel->setText("验证中...");
}

void TeacherLoginDialog::onResponseReceived(const QString &type, const QJsonObject &data)
{
    if (type != "verify_teacher") return;

    int status = data.value("status").toInt(-1);
    QString message = data.value("message").toString();

    if (status == 0) {
        authenticated = true;
        accept();
    } else {
        statusLabel->setText("验证失败：" + message);
    }
}

AttendancePage::AttendancePage(NetworkClient *client, QWidget *parent)
    : QWidget(parent), networkClient(client), currentCourseId(0), teacherAuthenticated(false)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);

    QLabel *title = new QLabel("课堂考勤");
    title->setStyleSheet("font-size: 24px; font-weight: bold;");
    layout->addWidget(title);

    infoLabel = new QLabel("点击按钮标记缺席学生（红色=缺席，绿色=出席）");
    infoLabel->setStyleSheet("font-size: 16px; color: #666; margin-bottom: 10px;");
    layout->addWidget(infoLabel);

    scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("QScrollArea { border: 2px solid #ccc; background-color: #f5f5f5; }");

    studentContainer = new QWidget();
    studentLayout = new QGridLayout(studentContainer);
    studentLayout->setSpacing(10);

    studentContainer->setLayout(studentLayout);
    scrollArea->setWidget(studentContainer);
    layout->addWidget(scrollArea);

    QHBoxLayout *buttonLayout = new QHBoxLayout();

    btnSubmit = new QPushButton("提交考勤");
    btnSubmit->setStyleSheet("font-size: 18px; padding: 10px 30px; background-color: #4CAF50; color: white;");

    btnBack = new QPushButton("返回");
    btnBack->setStyleSheet("font-size: 18px; padding: 10px 30px; background-color: #2196F3; color: white;");

    buttonLayout->addWidget(btnSubmit);
    buttonLayout->addWidget(btnBack);
    layout->addLayout(buttonLayout);

    loadAttendanceList();

    connect(btnSubmit, &QPushButton::clicked, this, &AttendancePage::submitAttendance);
    connect(btnBack, &QPushButton::clicked, this, &AttendancePage::backToMain);
    connect(networkClient, &NetworkClient::attendanceListReceived,
            this, &AttendancePage::onAttendanceListReceived);
    connect(networkClient, &NetworkClient::responseReceived,
            this, &AttendancePage::onResponseReceived);
}

void AttendancePage::loadAttendanceList()
{
    absentStudents.clear();

    currentCourseId = 1;

    QJsonObject data;
    data["classroom"] = "4-106";  // 添加教室参数
    qDebug() << "[v0] 发送考勤请求，教室：4-106";
    qDebug() << "[v0] data内容：" << data;
    networkClient->sendMessage("get_attendance_list", data);
}

void AttendancePage::onAttendanceListReceived(const QJsonObject &list)
{
    qDebug() << "[v0] 收到考勤列表：" << list;

    for (QPushButton *btn : studentButtons.values()) {
        studentLayout->removeWidget(btn);
        delete btn;
    }
    studentButtons.clear();
    absentStudents.clear();

    if (list.contains("course_id")) {
        currentCourseId = list["course_id"].toInt();
        qDebug() << "[v0] 课程ID：" << currentCourseId;
    }

    QJsonArray students = list["students"].toArray();
    qDebug() << "[v0] 学生数量：" << students.size();
    int row = 0, col = 0;

    for (const QJsonValue &val : students) {
        QJsonObject student = val.toObject();
        QString studentId = student["student_id"].toString();
        QString studentName = student["name"].toString();
        QString buttonText = studentName + "\n" + studentId;

        QPushButton *btn = new QPushButton(buttonText);
        btn->setFixedSize(120, 80);
        btn->setProperty("studentId", studentId);
        btn->setStyleSheet(
            "QPushButton { background-color: #4CAF50; color: white; font-size: 14px; "
            "border-radius: 5px; font-weight: bold; }"
            "QPushButton:hover { background-color: #45a049; }"
        );

        connect(btn, &QPushButton::clicked, this, &AttendancePage::onStudentButtonClicked);

        studentLayout->addWidget(btn, row, col);
        studentButtons[studentId] = btn;

        col++;
        if (col >= 5) {
            col = 0;
            row++;
        }
    }

    infoLabel->setText(QString("共%1名学生，点击按钮标记缺席（红色=缺席，绿色=出席）").arg(students.size()));
}

void AttendancePage::onStudentButtonClicked()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    QString studentId = btn->property("studentId").toString();

    if (absentStudents.contains(studentId)) {
        // 标记为出席
        absentStudents.remove(studentId);
        btn->setStyleSheet(
            "QPushButton { background-color: #4CAF50; color: white; font-size: 14px; "
            "border-radius: 5px; font-weight: bold; }"
            "QPushButton:hover { background-color: #45a049; }"
        );
    } else {
        // 标记为缺席
        absentStudents.insert(studentId);
        btn->setStyleSheet(
            "QPushButton { background-color: #f44336; color: white; font-size: 14px; "
            "border-radius: 5px; font-weight: bold; }"
            "QPushButton:hover { background-color: #da190b; }"
        );
    }
}

void AttendancePage::submitAttendance()
{
    if (!teacherAuthenticated) {
        if (!showTeacherLoginDialog()) {
            QMessageBox::warning(this, "验证失败", "未通过教师身份验证，无法提交考勤");
            return;
        }
    }

    // 构造absent_students数组，包含完整的学生信息
    QJsonArray absentArray;
    for (const QString &studentId : absentStudents) {
        if (studentButtons.contains(studentId)) {
            QPushButton *btn = studentButtons[studentId];
            QString buttonText = btn->text();
            // 按钮文本格式为 "姓名\n学号"，需要提取姓名
            QStringList parts = buttonText.split("\n");
            QString studentName = parts.size() > 0 ? parts[0] : "未知";

            QJsonObject student;
            student["student_id"] = studentId;
            student["name"] = studentName;
            student["class"] = "工商管理2201班";
            absentArray.append(student);
        }
    }

    qDebug() << "[v0] 提交考勤，缺勤人数：" << absentArray.size();

    QJsonObject data;
    data["course_id"] = currentCourseId;
    data["absent_students"] = absentArray;  // ← 关键：使用QJsonArray而不是字符串
    data["timestamp"] = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

    qDebug() << "[v0] 提交的数据：" << data;
    networkClient->sendMessage("submit_attendance", data);

    QMessageBox::information(this, "提示", QString("已提交 %1 名缺席学生").arg(absentArray.size()));
}

bool AttendancePage::showTeacherLoginDialog()
{
    TeacherLoginDialog dialog(networkClient, "4-106", this);
    if (dialog.exec() == QDialog::Accepted) {
        teacherAuthenticated = dialog.isAuthenticated();
        if (teacherAuthenticated) {
            qDebug() << "[v0] 教师验证成功";
            return true;
        }
    }
    return false;
}

void AttendancePage::onResponseReceived(const QString &type, const QJsonObject &data)
{
    if (type == "attendance_response") {
        if (data["status"].toString() == "success") {
            AudioPlayer::instance()->playSuccess();  // 播放提交成功音频
            QMessageBox::information(this, "成功",
                "考勤记录已提交！\n缺席人数：" + QString::number(absentStudents.size()));
            // 重置状态
            for (QPushButton *btn : studentButtons.values()) {
                btn->setStyleSheet(
                    "QPushButton { background-color: #4CAF50; color: white; font-size: 14px; "
                    "border-radius: 5px; font-weight: bold; }"
                    "QPushButton:hover { background-color: #45a049; }"
                );
            }
            absentStudents.clear();
        } else {
            QMessageBox::warning(this, "失败", "提交失败：" + data["error"].toString());
        }
    }
}

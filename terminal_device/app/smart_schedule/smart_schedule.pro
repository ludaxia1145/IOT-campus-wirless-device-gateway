QT += core gui network widgets multimedia

TARGET = smart_schedule
TEMPLATE = app

CONFIG += c++11

# 中文输入法支持（使用fcitx，不需要额外配置）
# 如果使用Qt Virtual Keyboard，取消注释下面这行
CONFIG += qtvirtualkeyboard

SOURCES += main.cpp \
           audioplayer.cpp \
           mainwindow.cpp \
           messagepage.cpp \
           librarypage.cpp \
           noticepage.cpp \
           attendancepage.cpp \
           networkclient.cpp


HEADERS += mainwindow.h \
           audioplayer.h \
           messagepage.h \
           librarypage.h \
           noticepage.h \
           attendancepage.h \
           networkclient.h

DISTFILES += \
    README.md

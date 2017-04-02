#-------------------------------------------------
#
# Project created by QtCreator 2013-04-30T12:10:31
#
#-------------------------------------------------

QT       += core gui network dbus

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = recovery
TEMPLATE = app
LIBS += -lqjson

SOURCES += main.cpp \
    libs/WebServer.cpp \
    Utility_Json.cpp \
    Utility_Sys.cpp \
    Utility_Debug.cpp \
    MultiImageWrite.cpp \
    OSInfo.cpp \
    PartitionInfo.cpp \
    PreSetup.cpp \
    BootManager.cpp \
    InstallManager.cpp

HEADERS  += \
    libs/WebServer.h \
    libs/easylogging++.h \
    Utility.h \
    MultiImageWrite.h \
    OSInfo.h \
    PartitionInfo.h \
    PreSetup.h \
    BootManager.h \
    InstallManager.h

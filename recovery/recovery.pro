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

QMAKE_CXXFLAGS += -std=c++11
CONFIG += c++11

SOURCES += main.cpp \
    libs/QRCode/BitBuffer.cpp \
    libs/QRCode/QrCode.cpp \
    libs/QRCode/QrSegment.cpp \
    libs/Web/Web.cpp \
    libs/Web/WebServer.cpp \
    libs/Web/WebClient.cpp \
    Utility.cpp \
    Utility_Json.cpp \
    Utility_Sys.cpp \
    Utility_Debug.cpp \
    OSInfo.cpp \
    PartitionInfo.cpp \
    PreSetup.cpp \
    BootManager.cpp \
    InstallManager.cpp \
    InstallManager_Utility.cpp

HEADERS  += \
    libs/easylogging++.h \
    libs/QRCode/BitBuffer.hpp \
    libs/QRCode/QrCode.hpp \
    libs/QRCode/QrSegment.hpp \
    libs/Web/Web.h \
    libs/Web/WebServer.h \
    libs/Web/WebClient.h \
    Utility.h \
    OSInfo.h \
    PartitionInfo.h \
    PreSetup.h \
    BootManager.h \
    InstallManager.h

//
// Created by Frank Steiler on 9/14/16 as part of NOOBS4IoT (https://github.com/steilerDev/NOOBS4IoT)
//
// main.cpp:
//      This file is the entry point for the recovery application.. It defines the logging parameters and starts the
//      BootManager.
//      For more information see https://github.com/steilerDev/NOOBS4IoT/wiki.
//
// This class is based on several files from the NOOBS project (c) 2013, Raspberry Pi All rights reserved.
// See https://github.com/raspberrypi/noobs for more information.
//
// This file is licensed under a GNU General Public License v3.0 (c) Frank Steiler.
// See https://raw.githubusercontent.com/steilerDev/NOOBS4IoT/master/LICENSE for more information.
//

#include <QCoreApplication>
#include <QTimer>
#include "libs/easylogging++.h"
#include "BootManager.h"

_INITIALIZE_EASYLOGGINGPP

void setup_logger() {
    easyloggingpp::Configurations defaultConf;
    defaultConf.setToDefault();
    defaultConf.set(easyloggingpp::Level::All, easyloggingpp::ConfigurationType::Format, "%datetime %level %log");
    defaultConf.set(easyloggingpp::Level::All, easyloggingpp::ConfigurationType::ToStandardOutput, "true");
    easyloggingpp::Loggers::reconfigureAllLoggers(defaultConf);
}

int main(int argc, char *argv[])
{
    setup_logger();
    LINFO << "Welcome to NOOBS4IoT";

    // Creating and using QCoreApplication, since QNetworking requires an active event loop.
    QCoreApplication a(argc, argv);

    // The Boot Manager is our entry point
    BootManager *bootManager = new BootManager();

    // The application only waits for the BootManager to finish, in order to quit
    QObject::connect(bootManager, SIGNAL(finished()), &a, SLOT(quit()));

    // Start the BootManager immediately
    QTimer::singleShot(0, bootManager, SLOT(run()));

    return a.exec();
}
#include <iostream>
#include <QCoreApplication>
#include <QTimer>
#include "libs/easylogging++.h"
#include "BootManager.h"

_INITIALIZE_EASYLOGGINGPP
/*
 *
 * Initial author: Floris Bos
 * Reworking author: Frank Steiler
 * Maintained by Raspberry Pi
 *
 * See LICENSE.txt for license details
 *
 */

void setup_logger() {
    easyloggingpp::Configurations defaultConf;
    defaultConf.setToDefault();
    defaultConf.set(easyloggingpp::Level::All, easyloggingpp::ConfigurationType::Format, "%datetime %level %log"); // Values are always std::string
    defaultConf.set(easyloggingpp::Level::All, easyloggingpp::ConfigurationType::ToStandardOutput, "true");
    easyloggingpp::Loggers::reconfigureAllLoggers(defaultConf);
}

int main(int argc, char *argv[])
{
    setup_logger();
    LINFO << "Welcome to NOOBS for TOSCA4IoT";

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
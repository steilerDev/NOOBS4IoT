//
// Created by Frank Steiler on 9/14/16.
// Copyright (c) 2016 Hewlett-Packard. All rights reserved.
//
// BootManager.cpp: [...]
//

#include <QDir>
#include <QProcess>
#include <QSettings>
#include <sys/reboot.h>
#include "BootManager.h"
#include "libs/easylogging++.h"
#include <QCoreApplication>
#include <QApplication>
#include <QTime>
#include <QTextStream>
#include "Utility.h"
#include "PreSetup.h"
#include "InstallManager.h"

BootManager::BootManager(): QObject(), webserver(true) {}


void BootManager::run() {

    if(Utility::Sys::mountSettingsPartition()) {
        LFATAL << "Unable to mount settings partition";
        emit finished();
    }

    if(bootCheck()) {
        LINFO << "Booting into OS specified in " << DEFAULT_BOOT_PARTITION_FILE << " this can be changed through setup mode and by modifying the file directly lying on " << SETTINGS_PARTITION;
        bootIntoPartition();
    } else {
        PreSetup preSetup;
        if(!preSetup.checkAndPrepareSDCard()) {
            LFATAL << "Unable to check and prepare SDCard, aborting";
            emit finished();
        }
        if(!preSetup.clearCMDline()) {
            LERROR << "Unable to remove 'runinstaller' from commandline arguments!";
        }
        preSetup.startNetworking();

        std::cout << "Recovery mode started!" << std::endl << std::endl;
        if(webserver) {
            LINFO << "Creating and starting webserver...";
            // Todo: Insert webserver stuff here
        } else {
            LINFO << "'no-webserver' argument found, starting local-mode...";
            int selection;
            bool stayInMenu = true;
            while (stayInMenu) {
                std::cout << "You have the following options to choose from:" << std::endl;
                std::cout << "  1) Reboot to default partition" << std::endl;
                std::cout << "  2) Reboot into specific partition" << std::endl;
                std::cout << "  3) Set default partition" << std::endl;
                std::cout << "  4) Install Raspbian" << std::endl;
                std::cout << "  5) Exit to recovery shell" << std::endl << std::endl;
                std::cout << "Enter a number: ";
                std::cin >> selection;
                switch (selection) {
                    case 1:
                        bootIntoPartition();
                        break;
                    case 2: {
                        std::cout << "Please give the partition device /dev/mmcblk0pX" << std::endl;
                        QTextStream qtin(stdin);
                        QString line = qtin.readLine();
                        bootIntoPartition(line);
                        break;
                    }
                    case 3: {
                        std::cout << "Please give the partition device /dev/mmcblk0pX" << std::endl;
                        QTextStream qtin(stdin);
                        QString line = qtin.readLine();
                        setDefaultBootPartition(line);
                        break;
                    }
                    case 4: {
                        InstallManager *installManager = new InstallManager();
                        installManager->installOS(*Utility::Debug::getRaspbianJSON());
                        break;
                    }
                    case 5:
                        stayInMenu = false;
                        break;
                    default:
                        break;
                }
            }

        }
    }
    emit finished();
}

// This function checks the hardware capabilities and decides whether to boot to a partition or to enter the setup mode.
// The function returns true, if the system should boot into a partition, false if it should enter setup mode
bool BootManager::bootCheck() {

    QStringList args = QCoreApplication::arguments();

    bool runinstaller = false;
    // Process command-line arguments
    for (int i = 0; i < args.size(); i++) {
        LDEBUG << "Found argument: " << args[i].toUtf8().constData();

        // Flag to indicate first boot
        if (args[i].compare("-runinstaller", Qt::CaseInsensitive) == 0) {
            //If runinstaller is specified, start installation right away
            LDEBUG << "Runinstaller specified, entering setup mode";
            runinstaller = true;
        } else if (args[i].compare("-partition", Qt::CaseInsensitive) == 0) {
            if (args.size() > i + 1) {
                QString defaultPartition = args[i + 1];
                LINFO << " Found default partition in args: " << defaultPartition.toUtf8().constData();
                setDefaultBootPartition(defaultPartition);
            }
        } else if (args[i].compare("-no-webserver", Qt::CaseInsensitive) == 0) {
            LDEBUG << "Found no-webserver, setting flag to start local-mode";
            webserver = false;
        }
    }

    if(runinstaller) {
        return false;
    }

    if (!hasInstalledOS()) {
        LINFO << "No OS installation detected, entering setup mode";
        return false;
    }
    LINFO << "Nothing matched, returning true";
    return true;
}
/*
 * bootCheck helper function
 */

bool BootManager::hasInstalledOS() {
    return getInstalledOS().size() > 0;
}

QVariantList BootManager::getInstalledOS() {
    if(QFile::exists("/settings/installed_os.json")) {
        LINFO << "Getting installed os file";
        QVariantList installedOS = Utility::Json::loadFromFile("/settings/installed_os.json").toList();
        return installedOS;
    } else {
        LERROR << "Unable to get installed os, because the installed_os.json does not exist";
        return QVariantList();
    }
}

void BootManager::setDefaultBootPartition(OSInfo &osInfo) {
    LDEBUG << "Setting " << osInfo.name().toUtf8().constData() << " as default os";
    if(osInfo.partitions()->isEmpty()) {
        LFATAL << "Unable to set OS as boot partition, because there are no partition information available!";
        return;
    } else {
        BootManager::setDefaultBootPartition(*osInfo.partitions()->first());
    }
}

void BootManager::setDefaultBootPartition(PartitionInfo &partition) {
    QString partDevice = QString(partition.partitionDevice());
    if(partDevice.isEmpty()) {
        LFATAL << "Unable to set partition as boot partition, because there is no partition device available!";
        return;
    } else {
        BootManager::setDefaultBootPartition(partDevice);
    }
}

void BootManager::setDefaultBootPartition(const QString &partitionDevice) {
    LINFO << "Setting boot partition to " << partitionDevice.toUtf8().constData();
    if(!partitionDevice.startsWith("/dev/mmcblk0p")) {
        LFATAL << "Unable to set boot partition to " << partitionDevice.toUtf8().constData() << " because it does not look like a SD Card partition device";
        return;
    }

    QFile f(DEFAULT_BOOT_PARTITION_FILE);
    if(f.exists()) {
        LDEBUG << "Removing existing default partition information";
        f.remove();
    }
    if(!Utility::Sys::putFileContents(DEFAULT_BOOT_PARTITION_FILE, partitionDevice.toAscii())) {
        LERROR << "Unable to set default boot partition";
    }
}

void BootManager::bootIntoPartition(const QString &partitionDevice) {
    QString partDevice;
    if(partitionDevice.isEmpty()) {
        LDEBUG << "Getting current default boot partition";
        partDevice = QString(Utility::Sys::getFileContents(DEFAULT_BOOT_PARTITION_FILE));
    } else {
        partDevice = partitionDevice;
    }

    LDEBUG << "Trying to boot into partition device " << partDevice.toUtf8().constData();

    if(!partDevice.startsWith("/dev/mmcblk0p")) {
        LFATAL << "Stated partition does not have the format of a partition on the SD Card, can't boot into it";
        return;
    }

    // Getting partition number
    partDevice.replace("/dev/mmcblk0p", "");
    bool ok;
    QVariant partDeviceVariant = QVariant(partDevice.toInt(&ok));
    if(!ok) {
        LFATAL << "Detected partition device number does not seems to be an integer!";
        return;
    }

    QString rebootDev;
    if (QFileInfo("/sys/module/bcm2708/parameters/reboot_part").exists()) {
        rebootDev = QString("/sys/module/bcm2708/parameters/reboot_part");
    } else if (QFileInfo("/sys/module/bcm2709/parameters/reboot_part").exists()) {
        rebootDev = QString("/sys/module/bcm2708/parameters/reboot_part");
    } else {
        LFATAL << "Unable to determine where to write reboot partition in (are you having a supported Raspberry Pi?)";
        return;
    }

    LDEBUG << "Setting reboot partition (" << partDeviceVariant.toString().toUtf8().constData() << ") into " << rebootDev.toUtf8().constData();

    QFile f(rebootDev);
    f.open(f.WriteOnly);
    f.write(partDeviceVariant.toByteArray() + "\n");
    f.close();

    // Shut down networking
    QProcess::execute("ifdown -a");
    // Unmount file systems
    QProcess::execute("umount -ar");
    ::sync();
    // Reboot
    ::reboot(RB_AUTOBOOT);
}

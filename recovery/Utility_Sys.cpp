//
// Created by Frank Steiler on 9/28/16 as part of NOOBS4IoT (https://github.com/steilerDev/NOOBS4IoT)
//
// Utility_Sys.cpp:
//      This file contains utility functions for interacting with the system: Mounting partitions and writing files.
//      For more information see https://github.com/steilerDev/NOOBS4IoT/wiki.
//
// This file is based on several files from the NOOBS project (c) 2013, Raspberry Pi All rights reserved.
// See https://github.com/raspberrypi/noobs for more information.
//
// This file is licensed under a GNU General Public License v3.0 (c) Frank Steiler.
// See https://raw.githubusercontent.com/steilerDev/NOOBS4IoT/master/LICENSE for more information.
//

#include <QFile>
#include <QProcess>
#include <QDir>
#include "Utility.h"

QByteArray Utility::Sys::getFileContents(const QString &filename) {
    QByteArray r;
    QFile f(filename);
    if(!f.open(f.ReadOnly)) {
        LFATAL << "Unable to open file " << filename.toUtf8().constData();
    } else {
        r = f.readAll();
        f.close();
    }
    return r;
}

bool Utility::Sys::putFileContents(const QString &filename, const QByteArray &data) {
    QFile f(filename);
    if(f.exists() || !f.open(f.WriteOnly)) {
        LFATAL << "Unable to open file " << filename.toUtf8().constData() << " for writing";
        return false;
    } else if(f.write(data) == -1) {
        LFATAL << "Error writing file " << filename.toUtf8().constData();
        f.close();
        return false;
    } else {
        f.close();
        return true;
    }
}

bool Utility::Sys::mountSystemsPartition() {
    return Utility::Sys::mountPartition(SYSTEMS_PARTITION, SYSTEMS_DIR, "-t vfat");
}

bool Utility::Sys::unmountSystemsPartition() {
    return Utility::Sys::unmountPartition(SYSTEMS_DIR);
}

bool Utility::Sys::mountSettingsPartition() {
    return Utility::Sys::mountPartition(SETTINGS_PARTITION, SETTINGS_DIR, "-t ext4");
}

bool Utility::Sys::unmountSettingsPartition() {
    return Utility::Sys::unmountPartition(SETTINGS_DIR);
}

bool Utility::Sys::mountPartition(const QString &partition, const QString &dir, const QString &args) {
    QDir settingsDir;
    if(!settingsDir.exists(dir)) {
        LDEBUG << "Creating directory " << dir.toUtf8().constData();
        settingsDir.mkdir(dir);
    }

    if(partitionIsMounted(partition, dir)) {
        LINFO << "Partition " << partition.toUtf8().constData() << " is allready mounted on " << dir.toUtf8().constData() << " not proceeding";
        return true;
    } else {
        QDir d;
        LDEBUG << "Mounting partition " << partition.toUtf8().constData() << " into " << dir.toUtf8().constData()
               << (!args.isEmpty()? " (with arguments: " + args + ")": "").toUtf8().constData();
        if (d.exists(partition) && d.exists(dir)) {
            return QProcess::execute("mount " + args + " " + partition + " " + dir) == 0;
        } else {
            LERROR << "Partition " << partition.toUtf8().constData() << ", or mounting point "
                   << dir.toUtf8().constData() << " does not exist!";
            return false;
        }
    }
}


bool Utility::Sys::remountPartition(const QString &dir, const QString &args) {
    if(!partitionIsMounted(dir)) {
        LERROR << "There is no partition mounted on this mountpoint, can't remount";
        return false;
    } else {
        QDir d;
        LDEBUG << "Remounting directory " << dir.toUtf8().constData();
        if (d.exists(dir)) {
            if (!args.isEmpty()) {
                return QProcess::execute("mount -o remount," + args + " " + dir) == 0;
            } else {
                return QProcess::execute("mount -o remount " + dir) == 0;
            }
        } else {
            LERROR << "Unable to remount directory " << dir.toUtf8().constData() << " because it does not exist";
            return false;
        }
    }
}

bool Utility::Sys::unmountPartition(const QString &dir) {
    LDEBUG << "Unmounting directory " << dir.toUtf8().constData();
    return QProcess::execute("umount " + dir) == 0;
}

bool Utility::Sys::partitionIsMounted(const QString &partition, const QString &dir) {
    LDEBUG << "Testing if " << partition.toUtf8().constData() << " is mounted" << (!dir.isEmpty()? " on " + dir : "").toUtf8().constData();
    return QProcess::execute("mount | grep " + partition + (!dir.isEmpty()? " on " + dir : "")) == 0;
}
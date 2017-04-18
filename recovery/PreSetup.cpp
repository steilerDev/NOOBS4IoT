//
// Created by Frank Steiler on 9/17/16 as part of NOOBS4IoT (https://github.com/steilerDev/NOOBS4IoT)
//
// PreSetup.cpp:
//      This class handles all necessary steps, preceding the install of Operating Systems. This will format the SD
//      Card to match the necessary schema and reduce the size of the FAT partition.
//      For more information see https://github.com/steilerDev/NOOBS4IoT/wiki.
//
// This file is based on several files from the NOOBS project (c) 2013, Raspberry Pi All rights reserved.
// See https://github.com/raspberrypi/noobs for more information.
//
// This file is licensed under a GNU General Public License v3.0 (c) Frank Steiler.
// See https://raw.githubusercontent.com/steilerDev/NOOBS4IoT/master/LICENSE for more information.
//

#include "PreSetup.h"

#include "libs/easylogging++.h"
#include "BootManager.h"
#include <QProcess>
#include <QFile>
#include <QDir>
#include <QDebug>
#include <unistd.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <QtNetwork/QNetworkInterface>
#include <QCoreApplication>

// This function checks if the SD Card is properly formatted, if this is not the case it will be formatted.
bool PreSetup::checkAndPrepareSDCard() {
    QDir dir;

    LINFO << "Waiting for SD card to be ready";
    while (!QFile::exists("/dev/mmcblk0"))
    {
        usleep(100);
    }

    LINFO << "Checking if this SD Card has already been formatted";
    // Checking if the data partition (/dev/mmcblk0p2) or the settings partition (/dev/mmcblk0p5) exist
    if (QFile::exists("/dev/mmcblk0p2") || QFile::exists(SETTINGS_PARTITION)) {
        LWARNING << "The SD Card has already been formatted, no need for that...";
        return true;
    }
    LINFO << "Needing to prepare SD Card...";

    LDEBUG << "Mounting systems partition";
    if (!Utility::Sys::mountSystemsPartition()) {
        LFATAL << "Unable to mount systems partition!";
        return false;
    }

    // Try to resize existing partitions
    if (!resizePartitions()) {
        LFATAL << "Unable to resize existing partitions";
        return false;
    }

    LDEBUG << "Formatting settings partition";
    if (!formatSettingsPartition()) {
        LFATAL << "Error formatting settings partition";
        return false;
    }

#ifdef RISCOS_BLOB_FILENAME
    if (QFile::exists(RISCOS_BLOB_FILENAME))
    {
        LINFO << "Writing RiscOS blob";
        if (!writeRiscOSblob())
        {
            LFATAL << "Error writing RiscOS blob";
            return false;
        }
    }
#endif

    /* Finish writing */
    LDEBUG << "Unmounting system partition";
    Utility::Sys::unmountSystemsPartition();

    LDEBUG << "Finish writing to disk (sync)";
    sync();

    LDEBUG << "Checking changes...";
    /* Perform a quick test to verify our changes were written
     * Drop page cache to make sure we are reading from card, and not from cache */
    QFile dc("/proc/sys/vm/drop_caches");
    dc.open(dc.WriteOnly);
    dc.write("3\n");
    dc.close();

    LDEBUG << "Mounting boot partition again";
    if (!Utility::Sys::mountSystemsPartition()) {
        LFATAL << "Unable to mount systems partition!";
        return false;
    }

    Utility::Sys::unmountPartition(SYSTEMS_DIR);
    LINFO << "Successfully repartitioned the SD Card";
    return true;
}



/*
 * checkAndPrepareSDCard helper functions
 */
bool PreSetup::resizePartitions()
{
    int newStartOfRescuePartition = Utility::Sys::getFileContents("/sys/class/block/mmcblk0p1/start").trimmed().toInt();
    int newSizeOfRescuePartition  = sizeofBootFilesInKB()*1.024/1000 + 100;

    if (!Utility::Sys::unmountSystemsPartition()) {
        LFATAL << "Error unmounting system partition.";
        return false;
    }

    if (!QFile::exists("/dev/mmcblk0p1")) {
        // SD card does not have a MBR.
        LFATAL << " No MBR record present on SD Card, recreating it and wiping all the data";
        return false;
    }

    LDEBUG << "Removing partitions 2,3,4";

    QFile f("/dev/mmcblk0");
    f.open(f.ReadWrite);
    // Seek to partition entry 2
    f.seek(462);
    // Zero out partition 2,3,4 to prevent parted complaining about invalid constraints
    f.write(QByteArray(16*3, '\0'));
    f.flush();
    // Tell Linux to re-read the partition table
    ioctl(f.handle(), BLKRRPART);
    f.close();
    usleep(500000);

    LDEBUG << "Resizing FAT partition";

    /* Relocating the start of the FAT partition is a write intensive operation
     * only move it when it is not aligned on a MiB boundary already */
    if (newStartOfRescuePartition < 2048 || newStartOfRescuePartition % 2048 != 0)
    {
        newStartOfRescuePartition = PARTITION_ALIGNMENT; /* 4 MiB */
    }

    QString cmd = "/usr/sbin/parted --script /dev/mmcblk0 resize 1 "+QString::number(newStartOfRescuePartition)+"s "+QString::number(newSizeOfRescuePartition)+"M";
    LDEBUG << "Executing" << cmd.toUtf8().constData();
    QProcess p;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    /* Suppress parted's big fat warning about its file system manipulation code not being robust.
       It distracts from any real error messages that may follow it. */
    env.insert("PARTED_SUPPRESS_FILE_SYSTEM_MANIPULATION_WARNING", "1");
    p.setProcessEnvironment(env);
    p.setProcessChannelMode(p.MergedChannels);
    p.start(cmd);
    p.closeWriteChannel();
    p.waitForFinished(-1);

    if (p.exitCode() != 0) {
        LFATAL << "Error resizing existing FAT partition: " << p.readAll().constData();
        return false;
    }
    LDEBUG << "parted done, output:" << p.readAll().constData();
    usleep(500000);

    LINFO << "Creating extended partition";

    QByteArray partitionTable;
    int startOfOurPartition = Utility::Sys::getFileContents("/sys/class/block/mmcblk0p1/start").trimmed().toInt();
    int sizeOfOurPartition  = Utility::Sys::getFileContents("/sys/class/block/mmcblk0p1/size").trimmed().toInt();
    int startOfExtended = startOfOurPartition+sizeOfOurPartition;

    // Align start of settings partition on 4 MiB boundary
    int startOfSettings = startOfExtended + PARTITION_GAP;
    if (startOfSettings % PARTITION_ALIGNMENT != 0)
        startOfSettings += PARTITION_ALIGNMENT-(startOfSettings % PARTITION_ALIGNMENT);

    // Primary partitions
    partitionTable  = QByteArray::number(startOfOurPartition)+","+QByteArray::number(sizeOfOurPartition)+",0E\n"; /* FAT partition */
    partitionTable += QByteArray::number(startOfExtended)+",,E\n"; /* Extended partition with all remaining space */
    partitionTable += "0,0\n";
    partitionTable += "0,0\n";
    // Logical partitions
    partitionTable += QByteArray::number(startOfSettings)+","+QByteArray::number(SETTINGS_PARTITION_SIZE)+",L\n"; /* Settings partition */
    LDEBUG << "Writing partition table" << partitionTable.constData();

    /* Let sfdisk write a proper partition table */
    cmd = QString("/sbin/sfdisk -uS /dev/mmcblk0");
    QProcess proc;
    proc.setProcessChannelMode(proc.MergedChannels);
    proc.start(cmd);
    proc.write(partitionTable);
    proc.closeWriteChannel();
    proc.waitForFinished(-1);
    if (proc.exitCode() != 0)
    {
        LFATAL << "Error creating extended partition: " << proc.readAll().constData();
        return false;
    }
    LDEBUG << "sfdisk done, output:" << proc.readAll().constData();
    usleep(500000);

    /* For reasons unknown Linux sometimes
     * only finds /dev/mmcblk0p2 and /dev/mmcblk0p1 goes missing */
    if (!QFile::exists("/dev/mmcblk0p1"))
    {
        /* Probe again */
        QProcess::execute("/usr/sbin/partprobe");
        usleep(500000);
    }

    QProcess::execute("/sbin/mlabel p:RECOVERY");

    LDEBUG << "Mounting systems partition";
    if(!Utility::Sys::mountSystemsPartition()) {
        LFATAL << "Unable to mount systems partition!";
        return false;
    }

    return true;
}

bool PreSetup::clearCMDline() {
    Utility::Sys::mountSystemsPartition();
    LDEBUG << "Editing cmdline.txt";

    QString cmdlinefilename = "/mnt/recovery.cmdline";
    if (!QFile::exists(cmdlinefilename)) {
        cmdlinefilename = "/mnt/cmdline.txt";
    }

    /* Remove "runinstaller" from cmdline.txt */
    QFile f(cmdlinefilename);
    if (!f.open(f.ReadOnly)) {
        LFATAL << "Error opening " << cmdlinefilename.toUtf8().constData();
        return false;
    }
    QByteArray line = f.readAll().trimmed();
    line = line.replace("runinstaller", "").trimmed();
    f.close();
    if(!f.open(f.WriteOnly)) {
        LFATAL << "Error opening " << cmdlinefilename.toUtf8().constData();
        return false;
    }
    f.write(line);
    f.close();

    /* Verify that cmdline.txt was written correctly */
    f.open(f.ReadOnly);
    QByteArray cmdlineread = f.readAll();
    f.close();

    if (cmdlineread != line) {
        LFATAL << "SD card broken (writes do not persist)";
        return false;
    } else {
        return true;
    }
}

int PreSetup::sizeofBootFilesInKB()
{
    QProcess proc;
    proc.start("du -s /mnt");
    proc.waitForFinished();
    return proc.readAll().split('\t').first().toInt();
}

bool PreSetup::formatSettingsPartition() {
    return QProcess::execute("/usr/sbin/mkfs.ext4 -L SETTINGS " SETTINGS_PARTITION) == 0;
}

#ifdef RISCOS_BLOB_FILENAME
bool PreSetup::writeRiscOSblob() {
    qDebug() << "writing RiscOS blob";
    return QProcess::execute("/bin/dd conv=fsync bs=512 if=" RISCOS_BLOB_FILENAME " of=/dev/mmcblk0 seek="+QString::number(RISCOS_BLOB_SECTOR_OFFSET)) == 0;
}
#endif

void PreSetup::startNetworking() {
    LINFO << "Starting network";

    /* Enable dbus so that we can use it to talk to wpa_supplicant later */
    LDEBUG << "Starting dbus";
    QProcess::execute("/etc/init.d/S30dbus start");

    /* Run dhcpcd in background */
    QProcess *proc = new QProcess();
    LDEBUG << "Starting dhcpcd";
    proc->start("/sbin/dhcpcd --noarp -e wpa_supplicant_conf=/settings/wpa_supplicant.conf --denyinterfaces \"*_ap\"");

    while(!isOnline()) {
        LINFO << "Waiting for network...";
        usleep(5000000);
    }
}

bool PreSetup::isOnline() {
    /* Check if we have an IP-address other than localhost */
    QList<QHostAddress> addresses = QNetworkInterface::allAddresses();

    foreach (QHostAddress a, addresses) {
        if (a != QHostAddress::LocalHost && a != QHostAddress::LocalHostIPv6) {
            return true;
        }
    }

    return false;
}

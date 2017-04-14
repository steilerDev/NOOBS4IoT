//
// Created by Frank Steiler on 4/12/17.
// Copyright (c) 2017 Hewlett-Packard. All rights reserved.
//
// InstallManager_Utility.cpp: [...]
//

#include <QProcess>
#include "InstallManager.h"
#include "Utility.h"
#include "libs/easylogging++.h"
#include "BootManager.h"
#include "Utility.h"
#include <QDir>
#include <QDebug>
#include <QProcess>
#include <QSettings>
#include <QTime>

bool InstallManager::writePartitionTable() {
    /* Write partition table using sfdisk */

    /* Fixed NOOBS partition */
    int startP1 = Utility::Sys::getFileContents("/sys/class/block/mmcblk0p1/start").trimmed().toInt();
    int sizeP1  = Utility::Sys::getFileContents("/sys/class/block/mmcblk0p1/size").trimmed().toInt();
    /* Fixed start of extended partition. End is not fixed, as it depends on primary partition 3 & 4 */
    int startExtended = startP1+sizeP1;
    /* Fixed settings partition */
    int startP5 = Utility::Sys::getFileContents("/sys/class/block/mmcblk0p5/start").trimmed().toInt();
    int sizeP5  = Utility::Sys::getFileContents("/sys/class/block/mmcblk0p5/size").trimmed().toInt();

    if (!startP1 || !sizeP1 || !startP5 || !sizeP5) {
        LFATAL << "Error reading existing partition table";
        return false;
    }

    /* Clone partition map, and add our system partitions to it */
    QMap<int, PartitionInfo *> partitionMap(_partitionMap);

    partitionMap.insert(1, new PartitionInfo(1, startP1, sizeP1, "0E")); /* FAT boot partition */
    partitionMap.insert(5, new PartitionInfo(5, startP5, sizeP5, "L")); /* Ext4 settings partition */

    int sizeExtended = partitionMap.values().last()->endSector() - startExtended;
    if (!partitionMap.contains(2)) {
        partitionMap.insert(2, new PartitionInfo(2, startExtended, sizeExtended, "E"));
    } else {
        /* If an OS already claimed primary partition 2, use out-of-order partitions, and store extended at partition 4 */
        partitionMap.insert(4, new PartitionInfo(4, startExtended, sizeExtended, "E"));
    }

    /* Add partitions */

    LDEBUG << "Printing partition map (size " << partitionMap.size() << ")";
    foreach(PartitionInfo* partition, partitionMap.values()) {

            LDEBUG << "Found partition: " << partition->label().constData();
            LDEBUG << "    Offset: " << partition->offset();
            LDEBUG << "    Size sector: " << partition->partitionSizeSectors();
            LDEBUG << "    Type: " << partition->partitionType().constData();
            LDEBUG << "    Number: " << partitionMap.key(partition);
        }

    QByteArray partitionTable;
    for (int i=1; i <= partitionMap.keys().last(); i++) {
        if (partitionMap.contains(i)) {
            PartitionInfo *p = partitionMap.value(i);

            partitionTable += QByteArray::number(p->offset())+","+QByteArray::number(p->partitionSizeSectors())+","+p->partitionType();
            if (p->active()) {
                partitionTable += " *";
            }
            partitionTable += "\n";
        } else {
            partitionTable += "0,0\n";
        }
    }

    LDEBUG << "Partition map information gathered";

    LDEBUG << "Unmounting all partitions";
    /* Unmount everything before modifying partition table */
    Utility::Sys::unmountPartition(SYSTEMS_DIR);
    Utility::Sys::unmountPartition(SETTINGS_DIR);

    LDEBUG << "Writing partition table using sfdisk";
    /* Let sfdisk write a proper partition table */
    QProcess proc;
    proc.setProcessChannelMode(proc.MergedChannels);
    proc.start("/sbin/sfdisk -uS /dev/mmcblk0");
    proc.write(partitionTable);
    proc.closeWriteChannel();
    proc.waitForFinished(-1);

    LDEBUG << "sfdisk done, output " << proc.readAll().constData();
    ::sync();
    usleep(500000);

    LDEBUG << "Doing partprobe";
    QProcess::execute("/usr/sbin/partprobe");
    usleep(500000);

    /* Remount */
    Utility::Sys::mountSystemsPartition();
    Utility::Sys::mountSettingsPartition();

    if (proc.exitCode() != 0) {
        LFATAL << "Error creating partition table (exit code: " << proc.exitCode() << "): " << proc.readAll().constData();
        return false;
    } else {
        return true;
    }
}


bool InstallManager::mkfs(const QByteArray &device, const QByteArray &fstype, const QByteArray &label, const QByteArray &mkfsopt) {
    QString cmd;

    if (fstype == "fat" || fstype == "FAT") {
        cmd = "/sbin/mkfs.fat ";
        if (!label.isEmpty()) {
            cmd += "-n "+label+" ";
        }
    } else if (fstype == "ext4") {
        cmd = "/usr/sbin/mkfs.ext4 ";
        if (!label.isEmpty()) {
            cmd += "-L "+label+" ";
        }
    } else if (fstype == "ntfs") {
        cmd = "/sbin/mkfs.ntfs --fast ";
        if (!label.isEmpty()) {
            cmd += "-L "+label+" ";
        }
    }

    if (!mkfsopt.isEmpty()) {
        cmd += mkfsopt+" ";
    }

    cmd += device;

    LDEBUG << "Executing:" << cmd.toUtf8().constData();
    QProcess p;
    p.setProcessChannelMode(p.MergedChannels);
    p.start(cmd);
    p.closeWriteChannel();
    p.waitForFinished(-1);

    if (p.exitCode() != 0) {
        LFATAL << "Error creating file system: " << p.readAll().constData();
        return false;
    } else {
        return true;
    }
}

bool InstallManager::isLabelAvailable(const QByteArray &label) {
    return (QProcess::execute("/sbin/findfs LABEL="+label) != 0);
}

bool InstallManager::untar(const QString &tarball) {
    QString cmd = "sh -o pipefail -c \"";

    if (isURL(tarball)) {
        cmd += "wget --no-verbose --tries=inf -O- "+tarball+" | ";
    }

    if (tarball.endsWith(".gz")) {
        cmd += "gzip -dc";
    } else if (tarball.endsWith(".xz")) {
        cmd += "xz -dc";
    } else if (tarball.endsWith(".bz2")) {
        cmd += "bzip2 -dc";
    } else if (tarball.endsWith(".lzo")) {
        cmd += "lzop -dc";
    } else if (tarball.endsWith(".zip")) {
        /* Note: the image must be the only file inside the .zip */
        cmd += "unzip -p";
    } else {
        LFATAL << "Unknown compression format file extension. Expecting .lzo, .gz, .xz, .bz2 or .zip";
        return false;
    }

    if (!isURL(tarball)) {
        cmd += " "+tarball;
    }
    cmd += " | tar x -C /mnt2 ";
    cmd += "\"";

    QTime t1;
    t1.start();
    LDEBUG << "Executing:" << cmd.toUtf8().constData();

    QProcess p;
    p.setProcessChannelMode(p.MergedChannels);
    p.start(cmd);
    p.closeWriteChannel();
    p.waitForFinished(-1);

    if (p.exitCode() != 0) {
        LFATAL << "Error downloading or extracting tarball: " << p.readAll().constData();
        return false;
    } else {
        LDEBUG << "Finished writing filesystem in " << (t1.elapsed()/1000.0) << " seconds";
        return true;
    }
}

bool InstallManager::dd(const QString &imagePath, const QString &device) {
    QString cmd = "sh -o pipefail -c \"";

    if (isURL(imagePath)) {
        cmd += "wget --no-verbose --tries=inf -O- "+imagePath+" | ";
    }

    if (imagePath.endsWith(".gz")) {
        cmd += "gzip -dc";
    } else if (imagePath.endsWith(".xz")) {
        cmd += "xz -dc";
    } else if (imagePath.endsWith(".bz2")) {
        cmd += "bzip2 -dc";
    } else if (imagePath.endsWith(".lzo")) {
        cmd += "lzop -dc";
    } else if (imagePath.endsWith(".zip")) {
        /* Note: the image must be the only file inside the .zip */
        cmd += "unzip -p";
    } else {
        LFATAL << "Unknown compression format file extension. Expecting .lzo, .gz, .xz, .bz2 or .zip";
        return false;
    }

    if (!isURL(imagePath)) {
        cmd += " "+imagePath;
    }

    cmd += " | dd of="+device+" conv=fsync obs=4M\"";

    QTime t1;
    t1.start();
    LDEBUG << "Executing:" << cmd.toUtf8().constData();

    QProcess p;
    p.setProcessChannelMode(p.MergedChannels);
    p.start(cmd);
    p.closeWriteChannel();
    p.waitForFinished(-1);

    if (p.exitCode() != 0) {
        LFATAL << "Error downloading or writing OS to SD card: " << p.readAll().constData();
        return false;
    } else {
        LDEBUG << "Finished writing filesystem in " << (t1.elapsed() / 1000.0) << " seconds";
        return true;
    }
}

bool InstallManager::partclone_restore(const QString &imagePath, const QString &device) {
    QString cmd = "sh -o pipefail -c \"";

    if (isURL(imagePath)) {
        cmd += "wget --no-verbose --tries=inf -O- "+imagePath+" | ";
    }

    if (imagePath.endsWith(".gz")) {
        cmd += "gzip -dc";
    } else if (imagePath.endsWith(".xz")) {
        cmd += "xz -dc";
    } else if (imagePath.endsWith(".bz2")) {
        cmd += "bzip2 -dc";
    } else if (imagePath.endsWith(".lzo")) {
        cmd += "lzop -dc";
    } else if (imagePath.endsWith(".zip")) {
        /* Note: the image must be the only file inside the .zip */
        cmd += "unzip -p";
    } else {
        LFATAL << "Unknown compression format file extension. Expecting .lzo, .gz, .xz, .bz2 or .zip";
        return false;
    }

    if (!isURL(imagePath)) {
        cmd += " "+imagePath;
    }

    cmd += " | partclone.restore -q -s - -o "+device+" \"";

    QTime t1;
    t1.start();
    LDEBUG << "Executing:" << cmd.toUtf8().constData();

    QProcess p;
    p.setProcessChannelMode(p.MergedChannels);
    p.start(cmd);
    p.closeWriteChannel();
    p.waitForFinished(-1);

    if (p.exitCode() != 0) {
        LFATAL << "Error downloading or writing OS to SD card" << p.readAll().constData();
        return false;
    } else {
        LDEBUG << "Finished writing filesystem in " << (t1.elapsed()/1000.0) << " seconds";
    }

    return true;
}

void InstallManager::patchConfigTxt() {
    QSettings settings("/settings/noobs.conf", QSettings::IniFormat);
    int videomode = settings.value("display_mode", 0).toInt();

    QByteArray dispOptions;

    switch (videomode) {
        case 0: /* HDMI PREFERRED */
            dispOptions = "hdmi_force_hotplug=1\r\n";
            break;
        case 1: /* HDMI VGA */
            dispOptions = "hdmi_ignore_edid=0xa5000080\r\nhdmi_force_hotplug=1\r\nhdmi_group=2\r\nhdmi_mode=4\r\n";
            break;
        case 2: /* PAL */
            dispOptions = "hdmi_ignore_hotplug=1\r\nsdtv_mode=2\r\n";
            break;
        case 3: /* NTSC */
            dispOptions = "hdmi_ignore_hotplug=1\r\nsdtv_mode=0\r\n";
            break;
    }

    QFile f("/mnt2/config.txt");
    f.open(f.Append);
    f.write("\r\n# NOOBS Auto-generated Settings:\r\n"+dispOptions);
    f.close();
}

QByteArray InstallManager::getLabel(const QString part) {
    QByteArray result;
    QProcess p;
    p.start("/sbin/blkid -s LABEL -o value "+part);
    p.waitForFinished();

    if (p.exitCode() == 0) {
        result = p.readAll().trimmed();
    }

    return result;
}

QByteArray InstallManager::getUUID(const QString part) {
    QByteArray result;
    QProcess p;
    p.start("/sbin/blkid -s UUID -o value "+part);
    p.waitForFinished();

    if (p.exitCode() == 0) {
        result = p.readAll().trimmed();
    }

    return result;
}

bool InstallManager::isURL(const QString &s) {
    return s.startsWith("http:") || s.startsWith("https:");
}

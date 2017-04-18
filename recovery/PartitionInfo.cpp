//
// Created by Frank Steiler on 10/25/16 as part of NOOBS4IoT (https://github.com/steilerDev/NOOBS4IoT)
//
// PartitionInfo.cpp
//      This class holds all necessary information about the installation of an Operating System's partition. It can be
//      created through the QMap representation of a Json file based on the "partitions_info" ressource within the default
//      NOOBS specification: http://downloads.raspberrypi.org/os_list_v3.json.
//      Required entries: 'filesystem_type'
//      For more information see https://github.com/steilerDev/NOOBS4IoT/wiki.
//
// This file is based on several files from the NOOBS project (c) 2013, Raspberry Pi All rights reserved.
// See https://github.com/raspberrypi/noobs for more information.
//
// This file is licensed under a GNU General Public License v3.0 (c) Frank Steiler.
// See https://raw.githubusercontent.com/steilerDev/NOOBS4IoT/master/LICENSE for more information.
//

#include <QProcess>
#include <QDir>
#include "PartitionInfo.h"
#include "libs/easylogging++.h"
#include "Utility.h"

/*
 * Keys of partition info map
 */
#define PI_FS_TYPE "filesystem_type"
#define PI_MKFS_OP "mkfs_options"
#define PI_LABEL "label"
#define PI_WANT_MAXIMISED "want_maximised"
#define PI_EMPTY_FS "empty_fs"
#define PI_OFFSET "offset_in_sectors"
#define PI_PART_SIZE_NOM "partition_size_nominal"
#define PI_REQ_PART_NO "requires_partition_number"
#define PI_UNCOMPRESSED_TAR_SIZE "uncompressed_tarball_size"
#define PI_ACTIVE "active"
#define PI_PART_TYPE "partition_type"

PartitionInfo::PartitionInfo(const QMap<QString, QVariant> &partInfo,
                             const QString &tarball) : _tarball(tarball),
                                                       _mountedDir(""),
                                                       _partitionSizeNominal(0),
                                                       _requiresPartitionNumber(0),
                                                       _offset(0),
                                                       _uncompressedTarballSize(0),
                                                       _emptyFS(false),
                                                       _wantMaximised(false),
                                                       _active(false) {
    LDEBUG << "Creating PartitionInfo object from JSON";

    _valid = parsePartitionInfo(partInfo);
}

bool PartitionInfo::parsePartitionInfo(QMap<QString, QVariant> partitionInfo) {

    if(!Utility::Json::parseEntry<QByteArray>(partitionInfo, PI_FS_TYPE, &_fstype, false, "filesystem type")) {
        return false;
    } else {
        LDEBUG << "Found " << _fstype.constData() << " as filestystem type";
    }

    if(!Utility::Json::parseEntry<QByteArray>(partitionInfo, PI_PART_TYPE, &_partitionType, false, "partition type")) {
        LWARNING << "No partition type specified, concluding partition type from file system type.";
        QString fsString = QString(_fstype);
        if(fsString.contains("fat", Qt::CaseInsensitive)) {
            //_partitionType = "0E";
            _partitionType = "0c"; /* FAT32 LBA */
        } else if (fsString.contains("swap", Qt::CaseInsensitive)) {
            _partitionType = "82";
        } else if (fsString.contains("ntfs", Qt::CaseInsensitive)) {
            _partitionType = "07";
        } else {
            //_partitionType = "L";
            _partitionType = "83"; /* Linux native */
        }
    }
    LDEBUG << "Using " << _partitionType.constData() << " as partition type";

    if(Utility::Json::parseEntry<QByteArray>(partitionInfo, PI_MKFS_OP, &_mkfsOptions, true, "MKFS options")) {
        LDEBUG << "Found " << _mkfsOptions.constData() << " as MKFS options";
    }

    if(Utility::Json::parseEntry<QByteArray>(partitionInfo, PI_LABEL, &_label, true, "label")){
        LDEBUG << "Found " << _label.constData() << " as label";
    }

    if(Utility::Json::parseEntry<int>(partitionInfo, PI_OFFSET, &_offset, true, "offset")) {
        LDEBUG << "Found offset of " << _offset;
    }

    if(Utility::Json::parseEntry<int>(partitionInfo, PI_PART_SIZE_NOM, &_partitionSizeNominal, true, "nominal partition size")) {
        LDEBUG << "Found nominal size of " << _partitionSizeNominal;
    }

    if(Utility::Json::parseEntry<int>(partitionInfo, PI_REQ_PART_NO, &_requiresPartitionNumber, true, "required partition number")) {
        LDEBUG << "Found required number of partitions: " << _requiresPartitionNumber;
    }

    if(Utility::Json::parseEntry<int>(partitionInfo, PI_UNCOMPRESSED_TAR_SIZE, &_uncompressedTarballSize, true, "uncompressed tarball size")) {
        LDEBUG << "Found uncompressed tarball size: " << _uncompressedTarballSize;
    }

    Utility::Json::parseEntry<bool>(partitionInfo, PI_WANT_MAXIMISED, &_wantMaximised, true, "want maximised");
    Utility::Json::parseEntry<bool>(partitionInfo, PI_EMPTY_FS, &_emptyFS, true, "empty FS");
    Utility::Json::parseEntry<bool>(partitionInfo, PI_ACTIVE, &_active, true, "active partition");

    return true;
}

PartitionInfo::PartitionInfo(int partitionNr, int offset, int sectors, const QByteArray &partType) : _partitionType(partType),
                                                                                                     _mountedDir(""),
                                                                                                     _requiresPartitionNumber(partitionNr),
                                                                                                     _offset(offset),
                                                                                                     _partitionSizeSectors(sectors),
                                                                                                     _active(false) {}

bool PartitionInfo::mountPartition(const QString &dir, const char* args) {
    if(!_mountedDir.isEmpty()) {
        LFATAL << "Can't mount partition, because it is still mounted";
        return false;
    } else if(_partitionDevice.isEmpty()) {
        LFATAL << "Can't mount partition because there is no partition device assigned";
        return false;
    }

    QString mountCMD;
    if(_fstype == "ntfs") {
        LDEBUG << "Mounting ntfs partition";
        mountCMD = "/sbin/mount.ntfs-3g ";
    } else {
        LDEBUG << "Mounting non-ntfs partition";
        mountCMD = "mount ";
    }

    QDir d;
    if(d.exists(dir) && d.exists(_partitionDevice)) {
        LDEBUG << "Mounting partition " << _partitionDevice.constData() << "with file system " << _fstype.constData() << " into " << dir.toUtf8().constData() << " (with arguments: " << args << ")";
        if(QProcess::execute(mountCMD + " " + args + " " + _partitionDevice + " " + dir) == 0) {
            LDEBUG << "Mount successful!";
            _mountedDir = dir;
            return true;
        } else {
            LDEBUG << "Mounting partition " << _partitionDevice.constData() << " with file system " << _fstype.constData() << " into " << dir.toUtf8().constData() << " (with arguments: " << args << ") failed!";
            return false;
        }
    } else {
        LERROR << "Partition " << _partitionDevice.constData() << ", or mounting point " << dir.toUtf8().constData() << " does not exist!";
        return false;
    }
}

bool PartitionInfo::unmountPartition() {
    if(!_mountedDir.isEmpty()) {
        LDEBUG << "Unmounting partition " << _partitionDevice.constData() << " from mount point " << _mountedDir.toUtf8().constData();
        if(QProcess::execute("umount " + _mountedDir) == 0) {
            LINFO << "Successfully unmounted partition " << _partitionDevice.constData() << " from mount point " << _mountedDir.toUtf8().constData();
            _mountedDir.clear();
            return true;
        } else {
            LERROR << "Unable to unmount partition " << _partitionDevice.constData() << " from mount point " << _mountedDir.toUtf8().constData();
            return false;
        }
    } else {
        LWARNING << "Partition " << _partitionDevice.constData() << " is currently not mounted";
        return true;
    }
}

void PartitionInfo::printPartitionInfo() {
    LDEBUG << "    PartitionInfo: " << _label.constData();
    LDEBUG << "        Partition Device: " << _partitionDevice.constData();
    LDEBUG << "        FS Type: " << _fstype.constData();
    LDEBUG << "        Partition Type: " << _partitionType.constData();
    LDEBUG << "        MKFS Options: " << _mkfsOptions.constData();
    LDEBUG << "        Tarball: " << _tarball.toUtf8().constData();
    LDEBUG << "        Mounted dir: " << _tarball.toUtf8().constData();
}
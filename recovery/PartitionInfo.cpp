#include <QProcess>
#include <QDir>
#include "PartitionInfo.h"
#include "easylogging++.h"

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

PartitionInfo::PartitionInfo(const QVariantMap &partInfo, const QString &tarball) : _tarball(tarball),
                                                                                    _mountedDir("") ,
                                                                                    _valid(true) {

    if(partInfo.contains(PI_FS_TYPE)) {
        _fstype = partInfo.value(PI_FS_TYPE).toByteArray().toLower();
    } else {
        LFATAL << "No filesystem type specified!";
        _valid = false;
    }

    if(partInfo.contains(PI_PART_TYPE)) {
        _partitionType = partInfo.value(PI_PART_TYPE).toByteArray();
    } else if (_valid) {
        LWARNING << "No partition type specified, concluding partition type from file system type.";
        if (_fstype.contains("fat")) {
            _partitionType = "0c"; /* FAT32 LBA */
        } else if (_fstype == "swap") {
            _partitionType = "82";
        } else if (_fstype.contains("ntfs")) {
            _partitionType = "07";
        } else {
            _partitionType = "83"; /* Linux native */
        }
        LDEBUG << "Using " << _partitionType.constData() << " as partition type";
    } else {
        LFATAL << "Unable to detect partition type";
        _valid = false;
    }

    if(partInfo.contains(PI_MKFS_OP)) {
        _mkfsOptions = partInfo.value(PI_MKFS_OP).toByteArray();
    } else {
        LWARNING << "No MKFS options specified";
    }

    if(partInfo.contains(PI_LABEL)) {
        _label = partInfo.value(PI_LABEL).toByteArray();
    } else {
        LWARNING << "No label specified";
    }

    if(partInfo.contains(PI_OFFSET)) {
        _offset = partInfo.value(PI_OFFSET).toInt();
    } else {
        _offset = 0;
        LWARNING << "No offset specified";
    }

    if(partInfo.contains(PI_PART_SIZE_NOM)) {
        _partitionSizeNominal = partInfo.value(PI_PART_SIZE_NOM).toInt();
    } else {
        _partitionSizeNominal = 0;
        LWARNING << "No nominal partition size specified";
    }

    if(partInfo.contains(PI_REQ_PART_NO)) {
        _requiresPartitionNumber = partInfo.value(PI_REQ_PART_NO).toInt();
    } else {
        _requiresPartitionNumber = 0;
        LWARNING << "No required partition number specified";
    }

    if(partInfo.contains(PI_UNCOMPRESSED_TAR_SIZE)) {
        _uncompressedTarballSize = partInfo.value(PI_UNCOMPRESSED_TAR_SIZE).toInt();
    } else {
        _uncompressedTarballSize = 0;
        LWARNING << "No uncompressed tarball size specified";
    }

    _wantMaximised = partInfo.value(PI_WANT_MAXIMISED, false).toBool();
    _emptyFS       = partInfo.value(PI_EMPTY_FS, false).toBool();
    _active        = partInfo.value(PI_ACTIVE, false).toBool();
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


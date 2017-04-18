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

#ifndef RECOVERY_PARTITIONINFO_H
#define RECOVERY_PARTITIONINFO_H

#include <QObject>
#include <QVariantMap>

class PartitionInfo {
public:
    /* Constructor. Gets called from OsInfo with info from json file */
    explicit PartitionInfo(const QMap<QString, QVariant> &m, const QString &tarball);
    explicit PartitionInfo(int partitionNr, int offset, int sectors, const QByteArray &partType);

    bool mountPartition(const QString &dir, const char* args = "");
    bool unmountPartition();

    void printPartitionInfo();

    /*
     * Setter
     */
    inline void setPartitionDevice(const QByteArray &partdevice) { _partitionDevice = partdevice; }
    inline void setRequiresPartitionNumber(int nr) { _requiresPartitionNumber = nr; }
    inline void setPartitionSizeSectors(int size) { _partitionSizeSectors = size; }
    inline void setOffset(int offset) { _offset = offset; }

    /*
     * Getter
     */
    inline QByteArray partitionDevice() { return _partitionDevice; }
    inline QByteArray fsType() { return _fstype; }
    inline QByteArray mkfsOptions() { return _mkfsOptions; }
    inline QByteArray label() { return _label; }
    inline QByteArray partitionType() { return _partitionType; }
    inline QString tarball() { return _tarball; }
    inline int partitionSizeNominal() { return _partitionSizeNominal; }
    inline int requiresPartitionNumber() { return _requiresPartitionNumber; }
    inline int uncompressedTarballSize() { return _uncompressedTarballSize; }
    inline int offset() { return _offset; }
    inline int partitionSizeSectors() { return _partitionSizeSectors; }
    inline int endSector() { return _offset + _partitionSizeSectors; }
    inline bool emptyFS() { return _emptyFS; }
    inline bool wantMaximised() { return _wantMaximised; }
    inline bool active() { return _active; }
    inline bool isValid() { return _valid; } // Indicates if config is valid

protected:
    QByteArray _fstype,
               _mkfsOptions,
               _label,
               _partitionDevice,
               _partitionType;
    QString _tarball,
            _mountedDir;
    int _partitionSizeNominal,
        _requiresPartitionNumber,
        _offset,
        _uncompressedTarballSize,
        _partitionSizeSectors;
    bool _emptyFS,
         _wantMaximised,
         _active,
         _valid;

private:
    bool parsePartitionInfo(QMap<QString, QVariant> partitionInfo);
};

#endif // RECOVERY_PARTITIONINFO_H

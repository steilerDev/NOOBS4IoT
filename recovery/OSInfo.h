//
// Created by Frank Steiler on 10/24/16 as part of NOOBS4IoT (https://github.com/steilerDev/NOOBS4IoT)
//
// OSInfo.h:
//      This class holds all necessary information about the installation of an Operating System. It can be created
//      through the QMap representation of a Json file based on the default NOOBS layout: http://downloads.raspberrypi.org/os_list_v3.json
//      For more information see https://github.com/steilerDev/NOOBS4IoT/wiki.
//
// This file is based on several files from the NOOBS project (c) 2013, Raspberry Pi All rights reserved.
// See https://github.com/raspberrypi/noobs for more information.
//
// This file is licensed under a GNU General Public License v3.0 (c) Frank Steiler.
// See https://raw.githubusercontent.com/steilerDev/NOOBS4IoT/master/LICENSE for more information.
//

#ifndef RECOVERY_OSINFO_H
#define RECOVERY_OSINFO_H

#include <QObject>
#include <QList>
#include <QStringList>
#include "PartitionInfo.h"

// Keys of the OS map
#define OS_DESC "description"
#define OS_INFO "os_info"
#define OS_PARTITION_INFO "partitions_info"
#define OS_PARTITIONS "partitions"
#define OS_PARTITION_SETUP "partition_setup"
#define OS_RELEASE_DATE "release_date"
#define OS_SUP_MODELS "supported_models"
#define OS_TARBALLS "tarballs"
#define OS_BOOTABLE "bootable"
#define OS_VERSION "version"
#define OS_RISCOS_OFFSET "riscos_offset" // Formerly known as RISCOS_OFFSET_KEY

#define OS_PARTITION_SETUP_PATH "/tmp/partition_setup.sh"

class OSInfo {

public:
    // The constructor takes the JSON, formatted as Map, downloads all necessary metadata and stores it in this variable
    explicit OSInfo(const QVariantMap &os);
    ~OSInfo();

    /*
     * Virtual getter
     */
    QVariantList vPartitionList() const;

    void printOSInfo();

    /*
     * Getter methods
     */
    inline QString folder() const { return _folder; }
    inline QString flavour() const { return _flavour; }
    inline QString name() const { return _name; }
    inline QString description() const { return _description; }
    inline QString releaseDate() const { return _releaseDate; }
    inline QList<PartitionInfo *> *partitions() { return &_partitions; }
    inline bool bootable() const { return _bootable; }
    inline bool isValid() const { return _valid; } // Shows if config looks valid
    inline QByteArray* partitionSetupScript() { return &_partitionSetupScript; }

protected:
    QString _folder,
            _flavour,
            _name,
            _description,
            _version,
            _releaseDate;

    QStringList _tarballs;

    bool _bootable,
         _valid, // Check for this variable to see if the object is usable
         _supported;

    int _riscosOffset;

    QList<PartitionInfo *> _partitions;
    QByteArray _partitionSetupScript;


private:
    bool parseOS(const QMap<QString, QVariant> &os);

    /*
     * This function retrieves the os_info.json file, parses its information into the object and stores the file persistently
     */
    bool parseOSInfo(QString &url);

    /*
     * This function retrieves the partitions.json file, parses its information into the object and stores the file persistently
     */
    bool parsePartitionInfo(QString &url);


    QByteArray downloadRessource(QString &url);
};

#endif // RECOVERY_OSINFO_H

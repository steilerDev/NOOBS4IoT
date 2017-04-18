//
// Created by Frank Steiler on 9/14/16 as part of NOOBS4IoT (https://github.com/steilerDev/NOOBS4IoT)
//
// InstallManager.h:
//      This class handles the install procedure of a single or multiple Operating Systems from a list of a single entity
//      of OSInfo objects.
//      For more information see https://github.com/steilerDev/NOOBS4IoT/wiki.
//
// This file is based on several files from the NOOBS project (c) 2013, Raspberry Pi All rights reserved.
// See https://github.com/raspberrypi/noobs for more information.
//
// This file is licensed under a GNU General Public License v3.0 (c) Frank Steiler.
// See https://raw.githubusercontent.com/steilerDev/NOOBS4IoT/master/LICENSE for more information.
//

#ifndef RECOVERY_INSTALLMANAGER_H
#define RECOVERY_INSTALLMANAGER_H

#include <qsettings.h>
#include <QtNetwork/QNetworkAccessManager>
#include "OSInfo.h"

class InstallManager {
public:
    InstallManager();
    ~InstallManager();
    bool installOS(QList<OSInfo> &oses);
    bool installOS(OSInfo &os);

private:
    // Must be done for all images before installing the OSes
    bool prepareImage(QList<OSInfo> &os);
    bool prepareImage(OSInfo &os);

    bool checkImage(OSInfo &os);
    bool checkPartition(PartitionInfo *partitionInfo);
    bool calculateSpaceRequirements();

    bool partitionSDCard();

    bool writeImage(OSInfo &os);
    bool writeImage(QList<OSInfo> &os);

    // Keeping track of next start sector, etc.
    int _extraSpacePerPartition,
        _part,
        _totalnominalsize ,
        _totaluncompressedsize,
        _numparts,
        _numexpandparts,
        _startSector,
        _totalSectors,
        _availableMB;

    /* key: partition number, value: partition information */
    QMap<int, PartitionInfo *> _partitionMap;
    QList<OSInfo*> *_osList;
    QVariantList _installed_os;

    /*
     * Utility functions defined in InstallManager_Utility.cpp
     */
    bool mkfs(const QByteArray &device, const QByteArray &fstype = "ext4", const QByteArray &label = "", const QByteArray &mkfsopt = "");
    bool dd(const QString &imagePath, const QString &device);
    bool partclone_restore(const QString &imagePath, const QString &device);
    bool untar(const QString &tarball);
    bool isLabelAvailable(const QByteArray &label);
    QByteArray getLabel(const QString part);
    QByteArray getUUID(const QString part);
    void patchConfigTxt();
    bool writePartitionTable();
    bool isURL(const QString &s);
};

#endif //RECOVERY_INSTALLMANAGER_H

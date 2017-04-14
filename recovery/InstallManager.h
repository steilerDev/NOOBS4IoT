//
// Created by Frank Steiler on 9/14/16.
// Copyright (c) 2016 Hewlett-Packard. All rights reserved.
//
// InstallManager.h: [...]
//

#ifndef RECOVERY_INSTALLMANAGER_H
#define RECOVERY_INSTALLMANAGER_H


// Directory for all metadata
#define METADATA_DIR "/settings/os/"

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

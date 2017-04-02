//
// Created by Frank Steiler on 9/28/16.
// Copyright (c) 2016 Hewlett-Packard. All rights reserved.
//
// Utlility.h: [...]
//

#ifndef RECOVERY_UTLILITY_H
#define RECOVERY_UTLILITY_H

#include <QString>
#include <QVariant>
#include "easylogging++.h"

/* Location to download the list of available distributions from
 * Multiple lists can be specified by space separating the URLs */
#define DEFAULT_REPO_SERVER  "http://downloads.raspberrypi.org/os_list_v3.json"

/* Partitioning settings */
#define PARTITION_ALIGNMENT  8192
#define PARTITION_GAP  2
/* Allow partitions to be shrinked PARTITION_GAP sectors
   if that prevents having a 4 MiB gap between the next one */
#define SHRINK_PARTITIONS_TO_MINIMIZE_GAPS

#define SETTINGS_PARTITION  "/dev/mmcblk0p5"
#define SETTINGS_DIR "/settings"
#define SYSTEMS_PARTITION "/dev/mmcblk0p1"
#define SYSTEMS_DIR "/mnt"
#define SETTINGS_PARTITION_SIZE  (32 * 2048 - PARTITION_GAP)

// This file contains the partition device that the system will automatically boot into
#define DEFAULT_BOOT_PARTITION_FILE "/settings/default_boot_partition.txt"

/* RiscOS magic */
#define RISCOS_OFFSET (1760)
#define RISCOS_SECTOR_OFFSET (RISCOS_OFFSET * 2048)

#define RISCOS_BLOB_FILENAME  "/mnt/riscos-boot.bin"
#define RISCOS_BLOB_SECTOR_OFFSET  (1)

/* Maximum number of partitions */
#define MAXIMUM_PARTITIONS  32

namespace Utility {
    namespace Json {
        QVariant parse(const QByteArray &json);
        QVariant loadFromFile(const QString &filename);
        bool saveToFile(const QString &filename, const QVariant &json);
    }

    namespace Sys {
        bool mountSystemsPartition();
        bool unmountSystemsPartition();
        bool mountSettingsPartition();
        bool unmountSettingsPartition();
        bool mountPartition(const QString &partition, const QString &dir, const QString &args = QString());
        bool unmountPartition(const QString &dir);
        bool remountPartition(const QString &partition, const QString &args = QString());
        bool partitionIsMounted(const QString &partition, const QString &dir = QString());
        QByteArray getFileContents(const QString &filename);
        bool putFileContents(const QString &filename, const QByteArray &data);
    }

    namespace Debug {
        QVariantList* getRaspbianJSON();
    }
}

#endif //RECOVERY_UTLILITY_H

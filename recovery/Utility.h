//
// Created by Frank Steiler on 9/28/16 as part of NOOBS4IoT (https://github.com/steilerDev/NOOBS4IoT)
//
// Utility.h:
//      This file contains general utility functions required by the application.
//      For more information see https://github.com/steilerDev/NOOBS4IoT/wiki.
//
// This file is based on several files from the NOOBS project (c) 2013, Raspberry Pi All rights reserved.
// See https://github.com/raspberrypi/noobs for more information.
//
// This file is licensed under a GNU General Public License v3.0 (c) Frank Steiler.
// See https://raw.githubusercontent.com/steilerDev/NOOBS4IoT/master/LICENSE for more information.
//

#ifndef RECOVERY_UTLILITY_H
#define RECOVERY_UTLILITY_H

#include <QString>
#include <QVariant>
#include "libs/easylogging++.h"
#include "libs/QRCode/QrCode.hpp"

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
#define DEFAULT_BOOT_PARTITION_FILE "/settings/default_boot_partition"

/* RiscOS magic */
#define RISCOS_OFFSET (1760)
#define RISCOS_SECTOR_OFFSET (RISCOS_OFFSET * 2048)

#define RISCOS_BLOB_FILENAME  "/mnt/riscos-boot.bin"
#define RISCOS_BLOB_SECTOR_OFFSET  (1)

/* Maximum number of partitions */
#define MAXIMUM_PARTITIONS 32

namespace Utility {
    namespace Json {
        QMap<QString, QVariant> parseJson(const QString &jsonString);
        QMap<QString, QVariant> parseJson(const QByteArray &json);

        QVariant loadFromFile(const QString &filename);
        bool saveToFile(const QString &filename, const QVariant &json);
        void printJson(QMap<QString, QVariant> &json);
        void printJsonArray(QList<QVariant> &json);

        // This function attemps to parse an entry to the given format. It returns true on success and writes the value into target.
        // If optional is set to true, the function will return true without writing to the target if the value was not found in the provided map.
        // description is used for logging reasons.
        template <typename T>
        static bool parseEntry(const QMap<QString, QVariant> source, const QString &key, T* target, const bool optional, const std::string &description) {
            LDEBUG << "Trying to parse " << description;
            if(source.contains(key)) {
                if(source.value(key).canConvert<T>()) {
                    *target = source.value(key).value<T>();
                    return true;
                } else {
                    LFATAL << "Unable to parse " << description << " (wrong format)";
                    return false;
                }
            } else {
                if(optional) {
                    LWARNING << "Unable to find " << description;
                    return true;
                } else {
                    LFATAL << "Unable to find " << description;
                    return false;
                }
            }
        }
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
        QMap<QString, QVariant>* getRaspbianJSON();
    }

    void printQrCode(const qrcodegen::QrCode &qrCode);
}

#endif //RECOVERY_UTLILITY_H

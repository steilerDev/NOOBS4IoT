//
// Created by Frank Steiler on 9/14/16.
// Copyright (c) 2016 Hewlett-Packard. All rights reserved.
//
// InstallManager.cpp: [...]
//

#include <QFile>
#include <QProcess>
#include <QSettings>
#include "InstallManager.h"
#include "Utility.h"
#include "MultiImageWrite.h"

InstallManager::InstallManager() {
    LDEBUG << "Mounting systems partition";
    Utility::Sys::mountSystemsPartition();

    _osList = new QList<OSInfo*>();
}

InstallManager::~InstallManager() {
    Utility::Sys::unmountSystemsPartition();
    qDeleteAll(*_osList);
    delete(_osList);
}


/*
 * OS should have the layout as obtained by http://downloads.raspberrypi.org/os_list_v3.json (Plus-sign indicates a mandatory entry)
 * A list of the following entries is expected.
 *      {
 *          "description": "LibreELEC is a fast and user-friendly Kodi Entertainment Center distribution.",
 *          "icon": "http://releases.libreelec.tv/noobs/LibreELEC_RPi/LibreELEC_RPi.png",
 *          "marketing_info": "http://releases.libreelec.tv/noobs/LibreELEC_RPi/marketing.tar",
 *          "nominal_size": 1024,
 *    +     "os_info": "http://releases.libreelec.tv/noobs/LibreELEC_RPi/os.json",
 *    +     "os_name": "LibreELEC_RPi",
 *          "partition_setup": "http://releases.libreelec.tv/noobs/LibreELEC_RPi/partition_setup.sh",
 *    +     "partitions_info": "http://releases.libreelec.tv/noobs/LibreELEC_RPi/partitions.json",
 *          "release_date": "2016-05-17",
 *          "supported_hex_revisions": "2,3,4,5,6,7,8,9,d,e,f,10,11,12,14,19,0092",
 *          "supported_models": [
 *              "Pi Model",
 *              "Pi Compute Module",
 *              "Pi Zero"
 *          ],
 *    +     "tarballs": [
 *    +         "http://releases.libreelec.tv/noobs/LibreELEC_RPi/LibreELEC_RPi_System.tar.xz",
 *    +         "http://releases.libreelec.tv/noobs/LibreELEC_RPi/LibreELEC_RPi_Storage.tar.xz"
 *          ]
 *      },
 */

bool InstallManager::installOS(const QMap<QString, QVariant> &os) {
    QVariantList osList;
    osList.append(os);
    return installOS(osList);
}

bool InstallManager::installOS(const QVariantList &oses) {
    OSInfo *osInfo;
    foreach(QVariant os, oses) {
        LDEBUG << "Creating OSInfo from os map";
        osInfo = new OSInfo(os.toMap());
        if(osInfo->isValid()) {
            _osList->append(osInfo);
        } else {
            LERROR << "OS info does not seem to be valid, not including os " << osInfo->name().toUtf8().constData();
        }
    }

    if(_osList->empty()) {
        LFATAL << "No OS left to install, aborting";
        return false;
    } else {
        /* All meta files downloaded, extract slides tarball, and launch image writer thread */
        MultiImageWrite imageWriteThread(*_osList);
        if(!imageWriteThread.writeImages()) {
            LFATAL << "Unable to write images!";
            return false;
        } else {
            LINFO << "Successfully installed specified OSes";
            return true;
        }
    }
}
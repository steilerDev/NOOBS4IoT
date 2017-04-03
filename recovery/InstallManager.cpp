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
}

InstallManager::~InstallManager() {
    Utility::Sys::unmountSystemsPartition();
}

bool InstallManager::installOS(OSInfo *os) {
    QList<OSInfo*> osInfoList;
    osInfoList.append(os);
    return installOS(osInfoList);
}

bool InstallManager::installOS(QList<OSInfo *> &osInfoList) {
    if(osInfoList.empty()) {
        LFATAL << "No OS to install, aborting";
        return false;
    } else {
        /* All meta files downloaded, extract slides tarball, and launch image writer thread */
        MultiImageWrite imageWriteThread(osInfoList);
        if(!imageWriteThread.writeImages()) {
            LFATAL << "Unable to write images!";
            return false;
        } else {
            LINFO << "Successfully installed specified OSes";
            return true;
        }
    }
}

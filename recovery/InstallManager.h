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
    void installOS(const QVariantList &oses);

private:
    QList<OSInfo*> *_osList;
};


#endif //RECOVERY_INSTALLMANAGER_H

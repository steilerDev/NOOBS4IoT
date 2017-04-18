//
// Created by Frank Steiler on 9/17/16 as part of NOOBS4IoT (https://github.com/steilerDev/NOOBS4IoT)
//
// PreSetup.h:
//      This class handles all necessary steps, preceding the install of Operating Systems. This will format the SD
//      Card to match the necessary schema and reduce the size of the FAT partition.
//      For more information see https://github.com/steilerDev/NOOBS4IoT/wiki.
//
// This file is based on several files from the NOOBS project (c) 2013, Raspberry Pi All rights reserved.
// See https://github.com/raspberrypi/noobs for more information.
//
// This file is licensed under a GNU General Public License v3.0 (c) Frank Steiler.
// See https://raw.githubusercontent.com/steilerDev/NOOBS4IoT/master/LICENSE for more information.
//

#ifndef RECOVERY_PRESETUP_H
#define RECOVERY_PRESETUP_H

#include <QString>
#include "Utility.h"

class PreSetup {

public:
    bool checkAndPrepareSDCard();
    void startNetworking();
    bool clearCMDline();

private:
    /*
     * bootCheck() helper functions
     */
    bool resizePartitions();
    int sizeofBootFilesInKB();
    bool formatSettingsPartition();
#ifdef RISCOS_BLOB_FILENAME
    bool writeRiscOSblob();
#endif

    bool isOnline();
};

#endif // RECOVERY_PRESETUP_H
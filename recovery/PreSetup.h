/*
 *
 * Initial author: Frank Steiler
 * Based on work of: Floris Bos
 *
 * See LICENSE.txt for license details
 *
 */

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
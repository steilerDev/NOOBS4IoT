//
// Created by Frank Steiler on 9/14/16.
//
// BootManager.h: [...]
//

#ifndef RECOVERY_BOOTMANAGER_H
#define RECOVERY_BOOTMANAGER_H


#include <QVariant>
#include "PartitionInfo.h"
#include "OSInfo.h"
#include "libs/WebServer.h"

#define PORT 80

class BootManager: public QObject {
    Q_OBJECT

public:
    BootManager();

    static void installOSREST(Request* request, Response* response);
    static void setDefaultBootPartitionREST(Request* request, Response* response);

    /*
     * The following function save the default partition's number to
     */
    static bool setDefaultBootPartition(OSInfo &osInfo);
    static bool setDefaultBootPartition(PartitionInfo &partition);
    static bool setDefaultBootPartition(const QString &partitionDevice);
    /*
     * This function actually boots into the default partition, if no argument is given. If an argument is given, this will NOT become the default partition!
     */
    void bootIntoPartition(const QString &partitionDevice = QString());

public slots:
    void run();

private:
    // This function returns true, if the boot manager should continue booting or false, if the setup should be started
    bool bootCheck();
    bool hasInstalledOS();
    QVariantList getInstalledOS();
    // Should the webserver be started
    bool webserver;

signals:
    void finished();
};


#endif //RECOVERY_BOOTMANAGER_H

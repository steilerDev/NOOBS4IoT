//
// Created by Frank Steiler on 9/14/16 as part of NOOBS4IoT (https://github.com/steilerDev/NOOBS4IoT)
//
// MultiImageWrite.cpp:
//      This class takes the paramaters provided by the InstallManager and writes
//      For more information see https://github.com/steilerDev/NOOBS4IoT/wiki.
//
// This class is based on several files from the NOOBS project (c) 2013, Raspberry Pi All rights reserved.
// See https://github.com/raspberrypi/noobs for more information.
//
// This file is licensed under a GNU General Public License v3.0 (c) Frank Steiler.
// See https://raw.githubusercontent.com/steilerDev/NOOBS4IoT/master/LICENSE for more information.
//
#include "MultiImageWrite.h"
#include "libs/easylogging++.h"
#include "BootManager.h"
#include "Utility.h"
#include <QDir>
#include <QDebug>
#include <QProcess>
#include <QSettings>
#include <QTime>

MultiImageWrite::MultiImageWrite(const QList<OSInfo*> &oses) : _extraSpacePerPartition(0),
                                                               _part(5),
                                                               _images(oses) {
    QDir dir;

    if (!dir.exists("/mnt2")) {
        dir.mkdir("/mnt2");
    }
}

bool MultiImageWrite::writeImages() {

    /* Calculate space requirements, and check special requirements */
    int totalnominalsize = 0, totaluncompressedsize = 0, numparts = 0, numexpandparts = 0;
    int startSector = Utility::Sys::getFileContents("/sys/class/block/mmcblk0p5/start").trimmed().toULongLong()
                    + Utility::Sys::getFileContents("/sys/class/block/mmcblk0p5/size").trimmed().toULongLong();
    int totalSectors = Utility::Sys::getFileContents("/sys/class/block/mmcblk0/size").trimmed().toULongLong();
    int availableMB = (totalSectors-startSector)/2048;

    /* key: partition number, value: partition information */
    QMap<int, PartitionInfo *> partitionMap;

    foreach (OSInfo *image, _images) {
        LDEBUG << "Checking image " << image->name().toUtf8().constData();
        QList<PartitionInfo *> *partitions = image->partitions();
        if (partitions->isEmpty()) {
            LFATAL << "No partitions specified!";
            return false;
        }

        if (image->name().contains("risc", Qt::CaseInsensitive)) {
            LDEBUG << "Detected a Risc OS Image, checking requirements";
            if (startSector > RISCOS_SECTOR_OFFSET-2048) {
                LFATAL << "RISCOS cannot be installed. Size of recovery partition too large.";
                return false;
            }

            totalnominalsize += (RISCOS_SECTOR_OFFSET - startSector)/2048;

            partitions->first()->setRequiresPartitionNumber(6);
            partitions->first()->setOffset(RISCOS_SECTOR_OFFSET);
            partitions->last()->setRequiresPartitionNumber(7);
        }

        foreach (PartitionInfo *partition, *partitions) {
            LDEBUG << "Checking partition "  << partition->label().constData();
            numparts++;
            if (partition->wantMaximised()) {
                numexpandparts++;
            }

            totalnominalsize += partition->partitionSizeNominal();
            totaluncompressedsize += partition->uncompressedTarballSize();

            if (partition->fsType() == "ext4") {
                totaluncompressedsize += /*0.035*/ 0.01 * totalnominalsize; /* overhead for file system meta data */
            }
            LDEBUG << "Checking required partition number";
            int reqPart = partition->requiresPartitionNumber();
            if (reqPart) {
                if (partitionMap.contains(reqPart)) {
                    LFATAL << "More than one operating system requires partition number " << reqPart;
                    return false;
                }
                if (reqPart == 1 || reqPart == 5) {
                    LFATAL << "Operating system cannot require a system partition (1,5)";
                    return false;
                }
                if ((reqPart == 2 && partitionMap.contains(4)) || (reqPart == 4 && partitionMap.contains(2))) {
                    LFATAL << "Operating system cannot claim both primary partitions 2 and 4";
                    return false;
                }

                partition->setPartitionDevice("/dev/mmcblk0p"+QByteArray::number(reqPart));
                partitionMap.insert(reqPart, partition);
            }

            /* Maximum overhead per partition for alignment */
#ifdef SHRINK_PARTITIONS_TO_MINIMIZE_GAPS
            if (partition->wantMaximised() || (partition->partitionSizeNominal()*2048) % PARTITION_ALIGNMENT != 0) {
                totalnominalsize += PARTITION_ALIGNMENT / 2048;
            }
#else
            totalnominalsize += PARTITION_ALIGNMENT/2048;
#endif
        }
        LDEBUG << "Finished partition checks for " << image->name().toUtf8().constData();
    }
    LDEBUG << "Finished image checks";

    if (numexpandparts) {
        /* Extra spare space available for partitions that want to be expanded */
        _extraSpacePerPartition = (availableMB-totalnominalsize)/numexpandparts;
        LDEBUG << "Extra space of " << _extraSpacePerPartition << " MB per partition";
    }

    LINFO << "Parsed Image Size: " << qint64(totaluncompressedsize)*1024*1024;

    if (totalnominalsize > availableMB) {
        LFATAL << "Not enough disk space. Need " << totalnominalsize << " MB, got " << availableMB << " MB";
        return false;
    } else {
        LDEBUG << "Enough space available!";
    }

    /* Assign logical partition numbers to partitions that did not reserve a special number */
    LDEBUG << "Assigning remaining partition numbers";
    int pnr;
    if (partitionMap.isEmpty()) {
        pnr = 6;
    } else {
        pnr = qMax(partitionMap.keys().last(), 5)+1;
    }

    foreach (OSInfo *image, _images)
    {
        foreach (PartitionInfo *partition, *(image->partitions()))
        {
            if (!partition->requiresPartitionNumber())
            {
                partitionMap.insert(pnr, partition);
                partition->setPartitionDevice("/dev/mmcblk0p"+QByteArray::number(pnr));
                pnr++;
            }
        }
    }

    /* Set partition starting sectors and sizes.
     * First allocate space to all logical partitions, then to primary partitions */
    QList<PartitionInfo *> log_before_prim = partitionMap.values();
    if (!log_before_prim.isEmpty() && log_before_prim.first()->requiresPartitionNumber() == 2) {
        log_before_prim.push_back(log_before_prim.takeFirst());
    }
    if (!log_before_prim.isEmpty() && log_before_prim.first()->requiresPartitionNumber() == 3) {
        log_before_prim.push_back(log_before_prim.takeFirst());
    }
    if (!log_before_prim.isEmpty() && log_before_prim.first()->requiresPartitionNumber() == 4) {
        log_before_prim.push_back(log_before_prim.takeFirst());
    }

    int offset = startSector;

    foreach (PartitionInfo *p, log_before_prim)
    {
        LDEBUG << "Getting start sector for partition";
        if (p->offset()) /* OS wants its partition at a fixed offset */
        {
            if (p->offset() <= offset) {
                LFATAL << "Fixed partition offset too low";
                return false;
            } else {
                offset = p->offset();
            }
        } else {
            offset += PARTITION_GAP;
            /* Align at 4 MiB offset */
            if (offset % PARTITION_ALIGNMENT != 0) {
                    offset += PARTITION_ALIGNMENT-(offset % PARTITION_ALIGNMENT);
            }

            p->setOffset(offset);
        }
        LDEBUG << "Offset is " << p->offset();

        int partsizeMB = p->partitionSizeNominal();
        if ( p->wantMaximised() ) {
            partsizeMB += _extraSpacePerPartition;
        }
        int partsizeSectors = partsizeMB * 2048;

        if (p == log_before_prim.last()) {
            /* Let last partition have any remaining space that we couldn't divide evenly */
            int spaceleft = totalSectors - offset - partsizeSectors;

            if (spaceleft > 0 && p->wantMaximised()) {
                partsizeSectors += spaceleft;
            }
        } else {
#ifdef SHRINK_PARTITIONS_TO_MINIMIZE_GAPS
            if (partsizeSectors % PARTITION_ALIGNMENT == 0 && p->fsType() != "raw") {
                /* Partition size is dividable by 4 MiB
                   Take off a couple sectors of the end of our partition to make room
                   for the EBR of the next partition, so the next partition can
                   align nicely without having a 4 MiB gap */
                partsizeSectors -= PARTITION_GAP;
            }
#endif
            if (p->wantMaximised() && (partsizeSectors+PARTITION_GAP) % PARTITION_ALIGNMENT != 0) {
                /* Enlarge partition to close gap to next partition */
                partsizeSectors += PARTITION_ALIGNMENT-((partsizeSectors+PARTITION_GAP) % PARTITION_ALIGNMENT);
            }
        }

        p->setPartitionSizeSectors(partsizeSectors);
        offset += partsizeSectors;
    }

    /* Delete information about previously installed operating systems */
    QFile f(SETTINGS_DIR "/installed_os.json");
    if (f.exists()) {
        f.remove();
    }

    LINFO << "Writing partition table";
    if (!writePartitionTable(partitionMap)) {
        LFATAL << "Unable to write partition table";
        return false;
    } else {
        LINFO << "Partition table successfully written";
    }

    /* Zero out first sector of partitions, to make sure to get rid of previous file system (label) */
    LINFO << "Zero'ing start of each partition";
    foreach (PartitionInfo *p, partitionMap.values()) {
        if (p->partitionSizeSectors()) {
            QProcess::execute("/bin/dd count=1 bs=512 if=/dev/zero of="+p->partitionDevice());
        }
    }

    /* Install each operating system */
    foreach (OSInfo *image, _images) {
        if (!processImage(image)) {
            LFATAL << "Unable to process image " << image->name().toUtf8().constData();
            return false;
        } else {
            LINFO << "Successfully processed " << image->name().toUtf8().constData();
        }
    }

    BootManager::setDefaultBootPartition(*_images.first());

    LINFO << "Finish writing (sync)";
    sync();
    return true;
}

bool MultiImageWrite::writePartitionTable(const QMap<int, PartitionInfo *> &pmap) {
    /* Write partition table using sfdisk */

    /* Fixed NOOBS partition */
    int startP1 = Utility::Sys::getFileContents("/sys/class/block/mmcblk0p1/start").trimmed().toInt();
    int sizeP1  = Utility::Sys::getFileContents("/sys/class/block/mmcblk0p1/size").trimmed().toInt();
    /* Fixed start of extended partition. End is not fixed, as it depends on primary partition 3 & 4 */
    int startExtended = startP1+sizeP1;
    /* Fixed settings partition */
    int startP5 = Utility::Sys::getFileContents("/sys/class/block/mmcblk0p5/start").trimmed().toInt();
    int sizeP5  = Utility::Sys::getFileContents("/sys/class/block/mmcblk0p5/size").trimmed().toInt();

    if (!startP1 || !sizeP1 || !startP5 || !sizeP5) {
        LFATAL << "Error reading existing partition table";
        return false;
    }

    /* Clone partition map, and add our system partitions to it */
    QMap<int, PartitionInfo *> partitionMap(pmap);

    partitionMap.insert(1, new PartitionInfo(1, startP1, sizeP1, "0E")); /* FAT boot partition */
    partitionMap.insert(5, new PartitionInfo(5, startP5, sizeP5, "L")); /* Ext4 settings partition */

    int sizeExtended = partitionMap.values().last()->endSector() - startExtended;
    if (!partitionMap.contains(2)) {
        partitionMap.insert(2, new PartitionInfo(2, startExtended, sizeExtended, "E"));
    } else {
        /* If an OS already claimed primary partition 2, use out-of-order partitions, and store extended at partition 4 */
        partitionMap.insert(4, new PartitionInfo(4, startExtended, sizeExtended, "E"));
    }

    /* Add partitions */

    QByteArray partitionTable;
    for (int i=1; i <= partitionMap.keys().last(); i++) {
        if (partitionMap.contains(i)) {
            PartitionInfo *p = partitionMap.value(i);

            partitionTable += QByteArray::number(p->offset())+","+QByteArray::number(p->partitionSizeSectors())+","+p->partitionType();
            if (p->active()) {
                partitionTable += " *";
            }
            partitionTable += "\n";
        } else {
            partitionTable += "0,0\n";
        }
    }

    LDEBUG << "Partition map information gathered";

    LDEBUG << "Unmounting all partitions";
    /* Unmount everything before modifying partition table */
    Utility::Sys::unmountPartition(SYSTEMS_DIR);
    Utility::Sys::unmountPartition(SETTINGS_DIR);

    LDEBUG << "Writing partition table using sfdisk";
    /* Let sfdisk write a proper partition table */
    QProcess proc;
    proc.setProcessChannelMode(proc.MergedChannels);
    proc.start("/sbin/sfdisk -uS /dev/mmcblk0");
    proc.write(partitionTable);
    proc.closeWriteChannel();
    proc.waitForFinished(-1);

    LDEBUG << "sfdisk done, output " << proc.readAll().constData();
    ::sync();
    usleep(500000);

    LDEBUG << "Doing partprobe";
    QProcess::execute("/usr/sbin/partprobe");
    usleep(500000);

    /* Remount */
    Utility::Sys::mountSystemsPartition();
    Utility::Sys::mountSettingsPartition();

    if (proc.exitCode() != 0) {
        LFATAL << "Error creating partition table: " << proc.readAll().constData();
        return false;
    } else {
        return true;
    }
}

bool MultiImageWrite::processImage(OSInfo *image) {
    QString os_name = image->name();
    LDEBUG << "Processing OS:" << os_name.toUtf8().constData();

    foreach (PartitionInfo *curPartition, *image->partitions()) {
        LDEBUG << "Checking partition label";
        if (curPartition->label().size() > 15) {
            curPartition->label().clear();
        } else if (!isLabelAvailable(curPartition->label())) {
            for (int i=0; i<10; i++) {
                if (isLabelAvailable(curPartition->label() + QByteArray::number(i))) {
                    curPartition->label() = curPartition->label() + QByteArray::number(i);
                    break;
                }
            }
        }
        LDEBUG << "Using label " << curPartition->label().constData();

        if (curPartition->fsType() == "raw") {
            LINFO << os_name.toUtf8().constData() << ": Writing raw OS image to " << curPartition->partitionDevice().constData();
            if (!dd(curPartition->tarball(), curPartition->partitionDevice())) {
                LFATAL << "Write failed!";
                return false;
            }
        } else if (curPartition->fsType().startsWith("partclone")) {
            LINFO << os_name.toUtf8().constData() << ": Writing cloned OS image to " << curPartition->partitionDevice().constData();
            if (!partclone_restore(curPartition->tarball(), curPartition->partitionDevice())) {
                LFATAL << "Write failed!";
                return false;
            }
        } else if (curPartition->fsType() != "unformatted") {
            LINFO << os_name.toUtf8().constData() << ": Creating filesystem " << curPartition->fsType().constData() << " on " << curPartition->partitionDevice().constData();
            if (!mkfs(curPartition->partitionDevice(), curPartition->fsType(), curPartition->label(), curPartition->mkfsOptions())) {
                LFATAL << "Unable to make file system";
                return false;
            } else {
                LINFO << "File system successfully created";
            }

            if (!curPartition->emptyFS()) {
                LDEBUG << os_name.toUtf8().constData() << ": Mounting file system";

                if(!curPartition->mountPartition("/mnt2")) {
                    LFATAL << os_name.toUtf8().constData() << ": Error mounting file system";
                    return false;
                }

                LINFO << os_name.toUtf8().constData() << ": Downloading and extracting filesystem";

                if (!untar(curPartition->tarball())) {
                    LFATAL << "Download and extracting file system failed!";
                    curPartition->unmountPartition();
                    return false;
                } else {
                    LINFO << "Download and extracting file system successfull!";
                    curPartition->unmountPartition();
                }
            }
        }
        //_part++;
    }

    LINFO << "Finished processing all partitions for " << os_name.toUtf8().constData();

    LINFO << os_name.toUtf8().constData() << ": Mounting first partition";
    if(!image->partitions()->first()->mountPartition("/mnt2")) {
        LFATAL << os_name.toUtf8().constData() << ": Error mounting file system on partition " << image->partitions()->first()->partitionDevice().constData();
        return false;
    }

    LINFO << os_name.toUtf8().constData() << ": Creating os_config.json";


    QSettings settings("/settings/noobs.conf", QSettings::IniFormat);

    QVariantMap qm;
    qm.insert("flavour", image->flavour());
    qm.insert("release_date", image->releaseDate());
    qm.insert("imagefolder", image->folder());
    qm.insert("description", image->description());
    qm.insert("videomode", settings.value("display_mode", 0).toInt());
    qm.insert("partitions", image->vPartitionList());
    qm.insert("language", settings.value("language", "en").toString());
    qm.insert("keyboard", settings.value("keyboard_layout", "gb").toString());

    if(!Utility::Json::saveToFile("/mnt2/os_config.json", qm)) {
        LFATAL << "Unable to save os_config.json to /mnt2/os_config.json";
        return false;
    } else {
        LDEBUG << "Successfully saved os_config.json";
    }

    LINFO << os_name.toUtf8().constData() << ": Saving display mode to config.txt";
    patchConfigTxt();

    /* Partition setup script can either reside in the image folder
     * or inside the boot partition tarball */
    QString postInstallScript;
    if (QFile::exists(image->folder() + OS_PARTITION_SETUP_MF)) {
        postInstallScript = image->folder() + OS_PARTITION_SETUP_MF;
    } else if(QFile::exists("/mnt2/partition_setup.sh")) {
        postInstallScript = "/mnt2/partition_setup.sh";
    }

    if (!postInstallScript.isEmpty()) {
        LINFO << os_name.toUtf8().constData() << ": Running partition setup script from " << postInstallScript.toUtf8().constData();
        QProcess proc;
        QProcessEnvironment env;
        QStringList args(postInstallScript);
        env.insert("PATH", "/bin:/usr/bin:/sbin:/usr/sbin");

        /* - Parameters to the partition-setup script are supplied both as
         *   command line parameters and set as environement variables
         * - Boot partition is mounted, working directory is set to its mnt folder
         *
         *  partition_setup.sh part1=/dev/mmcblk0p3 id1=LABEL=BOOT part2=/dev/mmcblk0p4
         *  id2=UUID=550e8400-e29b-41d4-a716-446655440000
         */
        int pnr = 1;
        foreach (PartitionInfo *p, *image->partitions()) {
            QString part  = p->partitionDevice();
            QString nr    = QString::number(pnr);
            QString uuid  = getUUID(part);
            QString label = getLabel(part);
            QString id;
            if (!label.isEmpty()) {
                id = "LABEL="+label;
            } else {
                id = "UUID=" + uuid;
            }

            LDEBUG << "part" << part.toUtf8().constData() << uuid.toUtf8().constData() << label.toUtf8().constData();

            args << "part"+nr+"="+part << "id"+nr+"="+id;
            env.insert("part"+nr, part);
            env.insert("id"+nr, id);
            pnr++;
        }

        LDEBUG << "Executing: sh " << args.join(" ").toUtf8().constData();
        LDEBUG << "Env: " << env.toStringList().join(" ").toUtf8().constData();
        proc.setProcessChannelMode(proc.MergedChannels);
        proc.setProcessEnvironment(env);
        proc.setWorkingDirectory("/mnt2");
        proc.start("/bin/sh", args);
        proc.waitForFinished(-1);

        if (proc.exitCode() != 0) {
            LFATAL << os_name.toUtf8().constData() << ": Error executing partition setup script: " << proc.readAll().constData();
            return false;
        } else {
            LINFO << "Successfully ran post-install scripts";
        }
    } else {
        LDEBUG << "No post-install script available";
    }

    LINFO << os_name.toUtf8().constData() << ": Unmounting FAT partition";
    if(!Utility::Sys::unmountPartition("/mnt2")) {
       LWARNING << os_name.toUtf8().constData() << ": Error unmounting";
    }

    /* Save information about installed operating systems in installed_os.json */
    LDEBUG << "Modifying installed_os.json";
    QVariantMap ventry;
    ventry["name"]        = image->flavour();
    ventry["description"] = image->description();
    ventry["folder"]      = image->folder();
    ventry["release_date"]= image->releaseDate();
    ventry["partitions"]  = image->vPartitionList();
    ventry["bootable"]    = image->bootable();
    QString iconfilename  = image->folder()+"/"+image->flavour()+".png";
    iconfilename.replace(" ", "_");
    if (QFile::exists(iconfilename)) {
        ventry["icon"] = iconfilename;
    } else if (QFile::exists(image->folder()+"/icon.png")) {
        ventry["icon"] = image->folder()+"/icon.png";
    }
    installed_os.append(ventry);

    if(!Utility::Json::saveToFile("/settings/installed_os.json", installed_os)) {
        LFATAL << "Unable to save installed_os.json to /settings/installed_os.json";
        return false;
    } else {
        LDEBUG << "Successfully saved installed_os.json";
        return true;
    }
}

bool MultiImageWrite::mkfs(const QByteArray &device, const QByteArray &fstype, const QByteArray &label, const QByteArray &mkfsopt) {
    QString cmd;

    if (fstype == "fat" || fstype == "FAT") {
        cmd = "/sbin/mkfs.fat ";
        if (!label.isEmpty()) {
            cmd += "-n "+label+" ";
        }
    } else if (fstype == "ext4") {
        cmd = "/usr/sbin/mkfs.ext4 ";
        if (!label.isEmpty()) {
            cmd += "-L "+label+" ";
        }
    } else if (fstype == "ntfs") {
        cmd = "/sbin/mkfs.ntfs --fast ";
        if (!label.isEmpty()) {
            cmd += "-L "+label+" ";
        }
    }

    if (!mkfsopt.isEmpty()) {
        cmd += mkfsopt+" ";
    }

    cmd += device;

    LDEBUG << "Executing:" << cmd.toUtf8().constData();
    QProcess p;
    p.setProcessChannelMode(p.MergedChannels);
    p.start(cmd);
    p.closeWriteChannel();
    p.waitForFinished(-1);

    if (p.exitCode() != 0) {
        LFATAL << "Error creating file system: " << p.readAll().constData();
        return false;
    } else {
        return true;
    }
}

bool MultiImageWrite::isLabelAvailable(const QByteArray &label) {
    return (QProcess::execute("/sbin/findfs LABEL="+label) != 0);
}

bool MultiImageWrite::untar(const QString &tarball) {
    QString cmd = "sh -o pipefail -c \"";

    if (isURL(tarball)) {
        cmd += "wget --no-verbose --tries=inf -O- "+tarball+" | ";
    }

    if (tarball.endsWith(".gz")) {
        cmd += "gzip -dc";
    } else if (tarball.endsWith(".xz")) {
        cmd += "xz -dc";
    } else if (tarball.endsWith(".bz2")) {
        cmd += "bzip2 -dc";
    } else if (tarball.endsWith(".lzo")) {
        cmd += "lzop -dc";
    } else if (tarball.endsWith(".zip")) {
        /* Note: the image must be the only file inside the .zip */
        cmd += "unzip -p";
    } else {
        LFATAL << "Unknown compression format file extension. Expecting .lzo, .gz, .xz, .bz2 or .zip";
        return false;
    }

    if (!isURL(tarball)) {
        cmd += " "+tarball;
    }
    cmd += " | tar x -C /mnt2 ";
    cmd += "\"";

    QTime t1;
    t1.start();
    LDEBUG << "Executing:" << cmd.toUtf8().constData();

    QProcess p;
    p.setProcessChannelMode(p.MergedChannels);
    p.start(cmd);
    p.closeWriteChannel();
    p.waitForFinished(-1);

    if (p.exitCode() != 0) {
        LFATAL << "Error downloading or extracting tarball: " << p.readAll().constData();
        return false;
    } else {
        LDEBUG << "Finished writing filesystem in " << (t1.elapsed()/1000.0) << " seconds";
        return true;
    }
}

bool MultiImageWrite::dd(const QString &imagePath, const QString &device)
{
    QString cmd = "sh -o pipefail -c \"";

    if (isURL(imagePath)) {
        cmd += "wget --no-verbose --tries=inf -O- "+imagePath+" | ";
    }

    if (imagePath.endsWith(".gz")) {
        cmd += "gzip -dc";
    } else if (imagePath.endsWith(".xz")) {
        cmd += "xz -dc";
    } else if (imagePath.endsWith(".bz2")) {
        cmd += "bzip2 -dc";
    } else if (imagePath.endsWith(".lzo")) {
        cmd += "lzop -dc";
    } else if (imagePath.endsWith(".zip")) {
        /* Note: the image must be the only file inside the .zip */
        cmd += "unzip -p";
    } else {
        LFATAL << "Unknown compression format file extension. Expecting .lzo, .gz, .xz, .bz2 or .zip";
        return false;
    }

    if (!isURL(imagePath)) {
        cmd += " "+imagePath;
    }

    cmd += " | dd of="+device+" conv=fsync obs=4M\"";

    QTime t1;
    t1.start();
    LDEBUG << "Executing:" << cmd.toUtf8().constData();

    QProcess p;
    p.setProcessChannelMode(p.MergedChannels);
    p.start(cmd);
    p.closeWriteChannel();
    p.waitForFinished(-1);

    if (p.exitCode() != 0) {
        LFATAL << "Error downloading or writing OS to SD card: " << p.readAll().constData();
        return false;
    } else {
        LDEBUG << "Finished writing filesystem in " << (t1.elapsed() / 1000.0) << " seconds";
        return true;
    }
}

bool MultiImageWrite::partclone_restore(const QString &imagePath, const QString &device) {
    QString cmd = "sh -o pipefail -c \"";

    if (isURL(imagePath)) {
        cmd += "wget --no-verbose --tries=inf -O- "+imagePath+" | ";
    }

    if (imagePath.endsWith(".gz")) {
        cmd += "gzip -dc";
    } else if (imagePath.endsWith(".xz")) {
        cmd += "xz -dc";
    } else if (imagePath.endsWith(".bz2")) {
        cmd += "bzip2 -dc";
    } else if (imagePath.endsWith(".lzo")) {
        cmd += "lzop -dc";
    } else if (imagePath.endsWith(".zip")) {
        /* Note: the image must be the only file inside the .zip */
        cmd += "unzip -p";
    } else {
        LFATAL << "Unknown compression format file extension. Expecting .lzo, .gz, .xz, .bz2 or .zip";
        return false;
    }

    if (!isURL(imagePath)) {
        cmd += " "+imagePath;
    }

    cmd += " | partclone.restore -q -s - -o "+device+" \"";

    QTime t1;
    t1.start();
    LDEBUG << "Executing:" << cmd.toUtf8().constData();

    QProcess p;
    p.setProcessChannelMode(p.MergedChannels);
    p.start(cmd);
    p.closeWriteChannel();
    p.waitForFinished(-1);

    if (p.exitCode() != 0) {
        LFATAL << "Error downloading or writing OS to SD card" << p.readAll().constData();
        return false;
    } else {
        LDEBUG << "Finished writing filesystem in " << (t1.elapsed()/1000.0) << " seconds";
    }

    return true;
}

void MultiImageWrite::patchConfigTxt() {
        QSettings settings("/settings/noobs.conf", QSettings::IniFormat);
        int videomode = settings.value("display_mode", 0).toInt();

        QByteArray dispOptions;

        switch (videomode) {
        case 0: /* HDMI PREFERRED */
            dispOptions = "hdmi_force_hotplug=1\r\n";
            break;
        case 1: /* HDMI VGA */
            dispOptions = "hdmi_ignore_edid=0xa5000080\r\nhdmi_force_hotplug=1\r\nhdmi_group=2\r\nhdmi_mode=4\r\n";
            break;
        case 2: /* PAL */
            dispOptions = "hdmi_ignore_hotplug=1\r\nsdtv_mode=2\r\n";
            break;
        case 3: /* NTSC */
            dispOptions = "hdmi_ignore_hotplug=1\r\nsdtv_mode=0\r\n";
            break;
        }

        QFile f("/mnt2/config.txt");
        f.open(f.Append);
        f.write("\r\n# NOOBS Auto-generated Settings:\r\n"+dispOptions);
        f.close();
}

QByteArray MultiImageWrite::getLabel(const QString part) {
    QByteArray result;
    QProcess p;
    p.start("/sbin/blkid -s LABEL -o value "+part);
    p.waitForFinished();

    if (p.exitCode() == 0) {
        result = p.readAll().trimmed();
    }

    return result;
}

QByteArray MultiImageWrite::getUUID(const QString part) {
    QByteArray result;
    QProcess p;
    p.start("/sbin/blkid -s UUID -o value "+part);
    p.waitForFinished();

    if (p.exitCode() == 0) {
        result = p.readAll().trimmed();
    }

    return result;
}

bool MultiImageWrite::isURL(const QString &s) {
    return s.startsWith("http:") || s.startsWith("https:");
}

//
// Created by Frank Steiler on 9/14/16.
// Copyright (c) 2016 Hewlett-Packard. All rights reserved.
//
// InstallManager.cpp: [...]
//

#include <QFile>
#include <QProcess>
#include <QSettings>
#include <QDir>
#include "InstallManager.h"
#include "Utility.h"
#include "BootManager.h"
#include <QDebug>
#include <QTime>

InstallManager::InstallManager() {
    _extraSpacePerPartition = 0;
    _part = 5,
    _totalnominalsize = 0;
    _totaluncompressedsize = 0;
    _numparts = 0;
    _numexpandparts = 0;
    _startSector = Utility::Sys::getFileContents("/sys/class/block/mmcblk0p5/start").trimmed().toULongLong()
                      + Utility::Sys::getFileContents("/sys/class/block/mmcblk0p5/size").trimmed().toULongLong();
    _totalSectors = Utility::Sys::getFileContents("/sys/class/block/mmcblk0/size").trimmed().toULongLong();
    _availableMB = (_totalSectors-_startSector)/2048;

    LDEBUG << "Mounting systems partition";
    Utility::Sys::mountSystemsPartition();


    QDir dir;
    if (!dir.exists("/mnt2")) {
        dir.mkdir("/mnt2");
    }

    _partitionMap = QMap<int, PartitionInfo*>();
    _osList = new QList<OSInfo*>();
    _installed_os = QVariantList();
}

InstallManager::~InstallManager() {
    Utility::Sys::unmountSystemsPartition();
    qDeleteAll(*_osList);
    delete(_osList);
}


bool InstallManager::installOS(QList<OSInfo> &os) {
    if(!prepareImage(os)) {
        LFATAL << "Unable to prepare images";
        return false;
    } else {
        LDEBUG << "Successfully prepared images";
    }

    if(!partitionSDCard()) {
        LFATAL << "Unable to partition & prepare SD Card";
        return false;
    } else {
        LDEBUG << "Successfully partitioned & prepared SD Card";
    }

    if(!writeImage(os)) {
        LFATAL << "Unable to write images";
    } else {
        LDEBUG << "Successfully written images";
    }
    BootManager::setDefaultBootPartition(os.first());

    LINFO << "Finish writing (sync)";
    sync();
    return true;
}


bool InstallManager::installOS(OSInfo &os) {
    if(!prepareImage(os)) {
        LFATAL << "Unable to prepare image for " << os.name().toUtf8().constData();
        return false;
    } else {
        LDEBUG << "Successfully prepared image for " << os.name().toUtf8().constData();
    }

    if(!partitionSDCard()) {
        LFATAL << "Unable to partition & prepare SD Card";
        return false;
    } else {
        LDEBUG << "Successfully partitioned & prepared SD Card";
    }

    if(!writeImage(os)) {
        LFATAL << "Unable to write image " << os.name().toUtf8().constData();
    } else {
        LDEBUG << "Successfully written image " << os.name().toUtf8().constData();
    }
    BootManager::setDefaultBootPartition(os);

    LINFO << "Finish writing (sync)";
    sync();
    return true;
}

bool InstallManager::prepareImage(QList<OSInfo> &osList) {
    foreach(OSInfo os, osList) {
        if(!checkImage(os)) {
            LFATAL << "Unable to prepare image " << os.name().toUtf8().constData();
            return false;
        }
    }
    if(!calculateSpaceRequirements()) {
        LFATAL << "Unable to calculate space requirements";
        return false;
    }
    return true;
}

bool InstallManager::prepareImage(OSInfo &os) {
    if(!checkImage(os)) {
        LFATAL << "Unable to prepare image " << os.name().toUtf8().constData();
        return false;
    }
    if(!calculateSpaceRequirements()) {
        LFATAL << "Unable to calculate space requirements";
        return false;
    }
    return true;
}


bool InstallManager::checkImage(OSInfo &os) {
    LDEBUG << "Checking image " << os.name().toUtf8().constData();
    QList<PartitionInfo *> *partitions = os.partitions();
    if (partitions->isEmpty()) {
        LFATAL << "No partitions specified!";
        return false;
    }

    if (os.name().contains("risc", Qt::CaseInsensitive)) {
        LDEBUG << "Detected a Risc OS Image, checking requirements";
        if (_startSector > RISCOS_SECTOR_OFFSET-2048) {
            LFATAL << "RISCOS cannot be installed. Size of recovery partition too large.";
            return false;
        }

        _totalnominalsize += (RISCOS_SECTOR_OFFSET - _startSector)/2048;

        partitions->first()->setRequiresPartitionNumber(6);
        partitions->first()->setOffset(RISCOS_SECTOR_OFFSET);
        partitions->last()->setRequiresPartitionNumber(7);
    }

    foreach (PartitionInfo *partition, *partitions) {
        if(!checkPartition(partition)) {
            LFATAL << "Partition check for partition " << partition->label().constData() << " failed!";
            return false;
        }
    }
    LDEBUG << "Finished partition checks for " << os.name().toUtf8().constData();
    return true;
}

bool InstallManager::checkPartition(PartitionInfo *partitionInfo) {
    LDEBUG << "Checking partition "  << partitionInfo->label().constData();
    _numparts++;
    if (partitionInfo->wantMaximised()) {
        _numexpandparts++;
    }

    _totalnominalsize += partitionInfo->partitionSizeNominal();
    _totaluncompressedsize += partitionInfo->uncompressedTarballSize();

    if (partitionInfo->fsType() == "ext4") {
        _totaluncompressedsize += /*0.035*/ 0.01 * _totalnominalsize; /* overhead for file system meta data */
    }
    LDEBUG << "Checking required partition number";
    int reqPart = partitionInfo->requiresPartitionNumber();
    if (!reqPart) {
        if(_partitionMap.isEmpty()) {
            LDEBUG << "First partition in partition map, using 6";
            reqPart = 6;
        } else {
            reqPart = qMax(_partitionMap.keys().last(), 5)+1;
        }
        LDEBUG << "Not requiring partition number, calculated: " << reqPart;
    }

    if (_partitionMap.contains(reqPart)) {
        LFATAL << "More than one operating system requires partition number " << reqPart;
        return false;
    }
    if (reqPart == 1 || reqPart == 5) {
        LFATAL << "Operating system cannot require a system partition (1,5)";
        return false;
    }
    if ((reqPart == 2 && _partitionMap.contains(4)) || (reqPart == 4 && _partitionMap.contains(2))) {
        LFATAL << "Operating system cannot claim both primary partitions 2 and 4";
        return false;
    }

    partitionInfo->setPartitionDevice("/dev/mmcblk0p"+QByteArray::number(reqPart));
    _partitionMap.insert(reqPart, partitionInfo);

    /* Maximum overhead per partition for alignment */
#ifdef SHRINK_PARTITIONS_TO_MINIMIZE_GAPS
    if (partitionInfo->wantMaximised() || (partitionInfo->partitionSizeNominal()*2048) % PARTITION_ALIGNMENT != 0) {
        _totalnominalsize += PARTITION_ALIGNMENT / 2048;
    }
#else
    totalnominalsize += PARTITION_ALIGNMENT/2048;
#endif
    return true;
}

bool InstallManager::calculateSpaceRequirements() {
    if (_numexpandparts) {
        /* Extra spare space available for partitions that want to be expanded */
        _extraSpacePerPartition = (_availableMB-_totalnominalsize)/_numexpandparts;
        LDEBUG << "Extra space of " << _extraSpacePerPartition << " MB per partition";
    }

    LINFO << "Parsed Image Size: " << qint64(_totaluncompressedsize)*1024*1024;

    if (_totalnominalsize > _availableMB) {
        LFATAL << "Not enough disk space. Need " << _totalnominalsize << " MB, got " << _availableMB << " MB left";
        return false;
    } else {
        LDEBUG << "Enough space available!";
    }


    /* Set partition starting sectors and sizes.
     * First allocate space to all logical partitions, then to primary partitions */
    QList<PartitionInfo *> log_before_prim = _partitionMap.values();
    if (!log_before_prim.isEmpty() && log_before_prim.first()->requiresPartitionNumber() == 2) {
        log_before_prim.push_back(log_before_prim.takeFirst());
    }
    if (!log_before_prim.isEmpty() && log_before_prim.first()->requiresPartitionNumber() == 3) {
        log_before_prim.push_back(log_before_prim.takeFirst());
    }
    if (!log_before_prim.isEmpty() && log_before_prim.first()->requiresPartitionNumber() == 4) {
        log_before_prim.push_back(log_before_prim.takeFirst());
    }

    int offset = _startSector;

    foreach (PartitionInfo *p, log_before_prim) {
        LDEBUG << "Getting start sector for partition";
        if (p->offset()) { /* OS wants its partition at a fixed offset */
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
            int spaceleft = _totalSectors - offset - partsizeSectors;

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
    return true;
}

bool InstallManager::partitionSDCard() {
    /* Delete information about previously installed operating systems */
    QFile f(SETTINGS_DIR "/installed_os.json");
    if (f.exists()) {
        f.remove();
    }

    LINFO << "Writing partition table";
    if (!writePartitionTable()) {
        LFATAL << "Unable to write partition table";
        return false;
    } else {
        LINFO << "Partition table successfully written";
    }

    /* Zero out first sector of partitions, to make sure to get rid of previous file system (label) */
    LINFO << "Zero'ing start of each partition";
    foreach (PartitionInfo *p, _partitionMap.values()) {
        if (p->partitionSizeSectors()) {
            if(QProcess::execute("/bin/dd count=1 bs=512 if=/dev/zero of="+p->partitionDevice()) != 0) {
                LINFO << "Zero'ing start of partition " << p->label().constData() << " failed!";
                return false;
            }
        }
    }
    return true;
}

bool InstallManager::writeImage(QList<OSInfo> &osList) {
    foreach(OSInfo os, osList) {
        if(!writeImage(os)) {
            LFATAL << "Unable to write OS " << os.name().toUtf8().constData();
            return false;
        }
    }
    return true;
}

bool InstallManager::writeImage(OSInfo &image) {
    QString os_name = image.name();
    LDEBUG << "Processing OS:" << os_name.toUtf8().constData();

    foreach (PartitionInfo *curPartition, *image.partitions()) {
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
    if(!image.partitions()->first()->mountPartition("/mnt2")) {
        LFATAL << os_name.toUtf8().constData() << ": Error mounting file system on partition " << image.partitions()->first()->partitionDevice().constData();
        return false;
    }

    LINFO << os_name.toUtf8().constData() << ": Creating os_config.json";


    QSettings settings("/settings/noobs.conf", QSettings::IniFormat);

    QVariantMap qm;
    qm.insert("flavour", image.flavour());
    qm.insert("release_date", image.releaseDate());
    qm.insert("imagefolder", image.folder());
    qm.insert("description", image.description());
    qm.insert("videomode", settings.value("display_mode", 0).toInt());
    qm.insert("partitions", image.vPartitionList());
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

    if(!image.partitionSetupScript()->isEmpty()) {
        LDEBUG << "Writing partition setup script to disc, in order to execute it then";
        //Todo: Implement overwrite function
        if (!Utility::Sys::putFileContents(OS_PARTITION_SETUP_PATH, *image.partitionSetupScript())) {
            LFATAL << "Unable to write partition setup script to disc!";
            return false;
        } else {
            LINFO << os_name.toUtf8().constData() << ": Running partition setup script from "
                  << OS_PARTITION_SETUP_PATH;
            QProcess proc;
            QProcessEnvironment env;
            QStringList args(OS_PARTITION_SETUP_PATH);
            env.insert("PATH", "/bin:/usr/bin:/sbin:/usr/sbin");

            /* - Parameters to the partition-setup script are supplied both as
             *   command line parameters and set as environement variables
             * - Boot partition is mounted, working directory is set to its mnt folder
             *
             *  partition_setup.sh part1=/dev/mmcblk0p3 id1=LABEL=BOOT part2=/dev/mmcblk0p4
             *  id2=UUID=550e8400-e29b-41d4-a716-446655440000
             */
            int pnr = 1;
            foreach (PartitionInfo *p, *image.partitions()) {
                QString part = p->partitionDevice();
                QString nr = QString::number(pnr);
                QString uuid = getUUID(part);
                QString label = getLabel(part);
                QString id;
                if (!label.isEmpty()) {
                    id = "LABEL=" + label;
                } else {
                    id = "UUID=" + uuid;
                }

                LDEBUG << "part" << part.toUtf8().constData() << uuid.toUtf8().constData()
                       << label.toUtf8().constData();

                args << "part" + nr + "=" + part << "id" + nr + "=" + id;
                env.insert("part" + nr, part);
                env.insert("id" + nr, id);
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
                LFATAL << os_name.toUtf8().constData() << ": Error executing partition setup script: "
                       << proc.readAll().constData();
                return false;
            } else {
                LINFO << "Successfully ran post-install scripts";
            }
            LDEBUG << "Removing post-install script";
            QFile script(OS_PARTITION_SETUP_PATH);
            if(!script.remove()) {
                LFATAL << "Unable to delete post-install script";
                return false;
            }
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
    ventry["name"]        = image.flavour();
    ventry["description"] = image.description();
    ventry["folder"]      = image.folder();
    ventry["release_date"]= image.releaseDate();
    ventry["partitions"]  = image.vPartitionList();
    ventry["bootable"]    = image.bootable();
    _installed_os.append(ventry);

    if(!Utility::Json::saveToFile("/settings/installed_os.json", _installed_os)) {
        LFATAL << "Unable to save installed_os.json to /settings/installed_os.json";
        return false;
    } else {
        LDEBUG << "Successfully saved installed_os.json";
        return true;
    }
}

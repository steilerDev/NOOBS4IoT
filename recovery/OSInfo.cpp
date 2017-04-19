//
// Created by Frank Steiler on 10/24/16 as part of NOOBS4IoT (https://github.com/steilerDev/NOOBS4IoT)
//
// OSInfo.cpp:
//      This class holds all necessary information about the installation of an Operating System. It can be created
//      through the QMap representation of a Json file based on the default NOOBS layout: http://downloads.raspberrypi.org/os_list_v3.json
//      For more information see https://github.com/steilerDev/NOOBS4IoT/wiki.
//
// This file is based on several files from the NOOBS project (c) 2013, Raspberry Pi All rights reserved.
// See https://github.com/raspberrypi/noobs for more information.
//
// This file is licensed under a GNU General Public License v3.0 (c) Frank Steiler.
// See https://raw.githubusercontent.com/steilerDev/NOOBS4IoT/master/LICENSE for more information.
//

#include <QStringList>
#include <QDir>
#include "OSInfo.h"
#include "libs/easylogging++.h"
#include "Utility.h"
#include "libs/Web/WebClient.h"

OSInfo::OSInfo(const QMap<QString, QVariant> &os) {
    LDEBUG << "Creating OSInfo object from JSON";

    _valid = parseOS(os);
}

OSInfo::~OSInfo() {
    qDeleteAll(_partitions);
    _partitions.clear();
}

bool OSInfo::parseOS(const QMap<QString, QVariant> &os) {
    /*
     * OS Name
     */
    if(!(Utility::Json::parseEntry<QString>(os, "name", &_name, false, "os name") ||
         Utility::Json::parseEntry<QString>(os, "os_name", &_name, false, "os name"))) {
        return false;
    } else {
        LDEBUG << "Found " << _name.toUtf8().constData();
    }

    /*
     * Bootable
     */
    if(!Utility::Json::parseEntry<bool>(os, OS_BOOTABLE, &_bootable, false, "bootability")) {
        _bootable = !_name.contains("Data Partition", Qt::CaseInsensitive);
    }
    if(!_bootable) {
        LFATAL << "OS does not seem to be bootable!";
        return false;
    } else {
        LDEBUG << "OS seems to be bootable";
    }

    /*
     * RISC OS
     */
    if (_name.contains("risc", Qt::CaseInsensitive)) {
        if(!Utility::Json::parseEntry<int>(os, OS_RISCOS_OFFSET, &_riscosOffset, false, "RiscOS offset")) {
            LWARNING << "Using default RiscOS offset";
            _riscosOffset = RISCOS_OFFSET;
        }

        if(_riscosOffset != RISCOS_OFFSET) {
            LWARNING << "RiscOS offset does not match expected value";
        }
        LDEBUG << "Found RiscOS offset " << _riscosOffset;
    }

    /*
     * Model support
     */
    QStringList supportedModels;
    QString _model = Utility::Sys::getFileContents("/proc/device-tree/model");
    _supported = false;
    if(Utility::Json::parseEntry<QStringList>(os, OS_SUP_MODELS, &supportedModels, true, "OS Support")) {
        _supported = true; //If no supported model that's ok
        foreach (QString m, supportedModels) {
            /*
             * Check if the full formal model name (e.g. "Raspberry Pi 2 Model B Rev 1.1")
             * contains the string we are told to look for (e.g. "Pi 2")
             */
            _supported = _supported || _model.contains(m, Qt::CaseInsensitive);
        }
    }
    if(!_supported) {
        LFATAL << "Model " << _model.toUtf8().constData()
               << " is not listed as a supported device: "
               << os.value("supported_models").toStringList().join(", ").toUtf8().constData();
        return false;
    } else {
        LDEBUG << "Model seems to be supported";
    }

    /*
     * Tarball information
     */
    if(!Utility::Json::parseEntry<QStringList>(os, OS_TARBALLS, &_tarballs, false, "tarball information")) {
        return false;
    }

    /*
     * Parsing nested OS info
     */
    QString url;
    if(!Utility::Json::parseEntry<QString>(os, OS_INFO, &url, false, "OS info URL")) {
        return false;
    } else {
        LDEBUG << "Found OS info URL " << url.toUtf8().constData();
        if(!parseOSInfo(url)) {
            LFATAL << "Error while parsing os info";
            return false;
        }
    }

    /*
     * Parsing nested OS partition info
     */
    if(!Utility::Json::parseEntry<QString>(os, OS_PARTITION_INFO, &url, false, "OS partition info URL")) {
        return false;
    } else {
        LDEBUG << "Found OS partition info URL " << url.toUtf8().constData();
        if(!parsePartitionInfo(url)) {
            LFATAL << "Error while parsing OS partition info";
            return false;
        }
    }

    url.clear();
    if(!Utility::Json::parseEntry<QString>(os, OS_PARTITION_SETUP, &url, true, "OS partition setup URL")) {
        return false;
    } else if (url.isEmpty()) {
        LWARNING << "Unable to find OS partition setup URL";
    } else {
        LDEBUG << "Found OS partition setup URL " << url.toUtf8().constData();
        _partitionSetupScript = downloadRessource(url);
        if(_partitionSetupScript.isEmpty()) {
            LFATAL << "Unable to download OS partition setup script";
            return false;
        }
    }

    return true;
}

bool OSInfo::parseOSInfo(QString &url) {
    LDEBUG << "Processing OS info";
    QMap<QString, QVariant> osInfo = Utility::Json::parseJson(downloadRessource(url));

    /*
     * Values that might already have been set previously
     */

    if(Utility::Json::parseEntry<QString>(osInfo, "name", &_name, true, "OS name")) {
        LDEBUG << "Eventually found new OS name " << _name.toUtf8().constData();
    }

    if(Utility::Json::parseEntry<bool>(osInfo, OS_BOOTABLE, &_bootable, true, "bootability")) {
        if(!_bootable) {
            LFATAL << "Detected non-bootability";
            return false;
        } else {
            LDEBUG << "Found that OS is bootable";
        }
    }

    /*
     * New information
     */

    if(Utility::Json::parseEntry<QString>(osInfo, OS_VERSION, &_version, true, "OS version")) {
        LDEBUG << "Found OS version " << _version.toUtf8().constData();
    }

    if(Utility::Json::parseEntry<QString>(osInfo, OS_DESC, &_description, true, "OS description")) {
        LDEBUG << "Found OS description " << _description.toUtf8().constData();
    }

    if(Utility::Json::parseEntry<QString>(osInfo, OS_RELEASE_DATE, &_releaseDate, true, "OS release date")) {
        LDEBUG << "Found OS release date " << _releaseDate.toUtf8().constData();
    }

    return true;
}

bool OSInfo::parsePartitionInfo(QString &url) {
    LINFO << "Processing partition info";
    QMap<QString, QVariant> partitionInfo = Utility::Json::parseJson(downloadRessource(url));
    QVariantList partitionInfoList;

    if(!Utility::Json::parseEntry<QVariantList>(partitionInfo, OS_PARTITIONS, &partitionInfoList, false, "OS partitions")) {
        return false;
    } else {
        if(partitionInfoList.size() < _tarballs.size()) {
            LFATAL << "More tarballs specified than partitions available";
            return false;
        } else if(partitionInfoList.size() > _tarballs.size()) {
            LERROR << "More partitions specified than tarballs, this might be intentional, filling gap with empty partitions";
            for (int i = partitionInfoList.size() - _tarballs.size(); i > 0; i--) {
                _tarballs.append("");
            }
        }
        PartitionInfo *partInfo;
        int i = 0;
        foreach(QVariant partition, partitionInfoList) {
            if(partition.canConvert<QVariantMap>()) {
                partInfo = new PartitionInfo(partition.toMap(), _tarballs[i++]);
                if(partInfo->isValid()) {
                    _partitions.append(partInfo);
                } else {
                    LFATAL << "Invalid partition info found";
                    return false;
                }
            } else {
                LFATAL << "Unable to parse partition due to type error";
                return false;
            }
        }
    }
    return true;
}

QByteArray OSInfo::downloadRessource(QString &url) {
    LDEBUG << "Downloading " << url.toUtf8().constData();

    Web::WebClient webClient;
    std::string urlString = std::string(url.toUtf8().constData());
    std::string response = webClient.get(urlString);


    if(response.empty()) {
        LFATAL << "Unable to download " << url.toUtf8().constData();
        return QByteArray();
    } else {
        LDEBUG << "Successfully downloaded " << url.toUtf8().constData();
        return QByteArray(response.c_str());
    }
}

QVariantList OSInfo::vPartitionList() const {
    QVariantList vPartitions;
    foreach (PartitionInfo *p, _partitions) {
        vPartitions.append(p->partitionDevice());
    }
    return vPartitions;
}

void OSInfo::printOSInfo() {

    LDEBUG << "OSInfo: " << _name.toUtf8().constData();
    LDEBUG << "    Description: " << _description.toUtf8().constData();
    LDEBUG << "    Version: " << _version.toUtf8().constData();
    LDEBUG << "    Release Date: " << _releaseDate.toUtf8().constData();
    LDEBUG << "    Folder: " << _folder.toUtf8().constData();
    LDEBUG << "    Flavour: " << _flavour.toUtf8().constData();
    LDEBUG << "";
    LDEBUG << "    Tarballs: " << _tarballs.join(" ").toUtf8().constData();
    if(bootable()) {
        LDEBUG << "    OS is bootable";
    } else {
        LDEBUG << "    OS is not bootable";
    }
    if(_valid) {
        LDEBUG << "    OS is valid";
    } else {
        LDEBUG << "    OS is not valid";
    }
    if(_supported) {
        LDEBUG << "    OS is supported";
    } else {
        LDEBUG << "    OS is not supported";
    }
    if(_riscosOffset) {
        LDEBUG << "    RiscOS offset: " << _riscosOffset;
    } else {
        LDEBUG << "    No RiscOS offset defined";
    }

    foreach(PartitionInfo *part, _partitions) {
        part->printPartitionInfo();
    }
}

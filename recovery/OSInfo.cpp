#include <QStringList>
#include <QDir>
#include <QUrl>
#include <QtNetwork/QNetworkReply>
#include <QEventLoop>
#include <QtNetwork/QNetworkConfigurationManager>
#include <QtNetwork/QNetworkDiskCache>
#include "OSInfo.h"
#include "libs/easylogging++.h"
#include "Utility.h"

OSInfo::OSInfo(const QMap<QString, QVariant> &os) {
    LDEBUG << "Creating OSInfo object from JSON";

    initNetwork();
    _valid = parseOS(os);
}

OSInfo::~OSInfo() {
    if(_netaccess) {
        delete(_netaccess);
    }
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
        LDEBUG << "Found OS partition setup URL" << url.toUtf8().constData();
        _partitionSetupScript = downloadRessource(url);
        if(_partitionSetupScript.isEmpty()) {
            LFATAL << "Unable to download OS partition setup script";
            return false;
        }
    }

    return true;
}

bool OSInfo::parseOSInfo(const QString &url) {
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

bool OSInfo::parsePartitionInfo(const QString &url) {
    LINFO << "Processing partition info";
    QMap<QString, QVariant> partitionInfo = Utility::Json::parseJson(downloadRessource(url));
    QVariantList partitionInfoList;

    if(!Utility::Json::parseEntry<QVariantList>(partitionInfo, OS_PARTITIONS, &partitionInfoList, false, "OS partitions")) {
        return false;
    } else {
        if(partitionInfoList.size() != _tarballs.size()) {
            LFATAL << "Mismatch between number of available tarballs and OS partitions!";
            return false;
        } else {
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
    }
    return true;
}

QByteArray OSInfo::downloadRessource(const QString &url) {
    LDEBUG << "Downloading " << url.toUtf8().constData();

    QEventLoop eventLoop;

    QNetworkReply *reply = _netaccess->get(QNetworkRequest(url));

    QObject::connect(reply, SIGNAL(finished()), &eventLoop, SLOT(quit()));
    //QObject::connect(reply, SIGNAL(error(NetworkError)), &eventLoop, SLOT(quit()));
    // Waiting for request to finish
    LDEBUG << "Waiting for download to finish";
    eventLoop.exec();

    int httpStatusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QString redirectionUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toString();

    if (httpStatusCode > 300 && httpStatusCode < 400) {                                             // Redirect
        LDEBUG << "Redirection - Re-trying download from" << redirectionUrl.toUtf8().constData();
        delete (reply);
        return downloadRessource(redirectionUrl);
    } else if (reply->error() != reply->NoError || httpStatusCode < 200 || httpStatusCode > 399) {  // Not found or unavailable
        LFATAL << "Unable to download " << url.toUtf8().constData();
        LDEBUG << "Error Code " << reply->error() << ", status code " << httpStatusCode << ", content: " << reply->readAll().constData();
        delete (reply);
        return QByteArray();
    } else {
        LDEBUG << "Successfully downloaded " << url.toUtf8().constData();
        QByteArray response(reply->readAll());
        delete (reply);
        return response;
    }
}

void OSInfo::initNetwork() {
    LINFO << " Configuring network access...";
    //QDir dir;
    //dir.mkdir(CACHE_DIR);
    _netaccess = new QNetworkAccessManager();
    //QNetworkDiskCache *_cache = new QNetworkDiskCache();
    //_cache->setCacheDirectory(CACHE_DIR);
    //_cache->setMaximumCacheSize(CACHE_SIZE);
    //_netaccess->setCache(_cache);
    QNetworkConfigurationManager manager;
    _netaccess->setConfiguration(manager.defaultConfiguration());
}

QVariantList OSInfo::vPartitionList() const {
    QVariantList vPartitions;
    foreach (PartitionInfo *p, _partitions) {
        vPartitions.append(p->partitionDevice());
    }
    return vPartitions;
}

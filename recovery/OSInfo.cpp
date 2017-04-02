#include <QStringList>
#include <QDir>
#include <QUrl>
#include <QtNetwork/QNetworkReply>
#include <QEventLoop>
#include <QtNetwork/QNetworkConfigurationManager>
#include <QtNetwork/QNetworkDiskCache>
#include <QProcess>
#include "OSInfo.h"
#include "easylogging++.h"
#include "Utility.h"

OSInfo::OSInfo(const QVariantMap &os) : _valid(true), _supported(false)
{
    LDEBUG << "Creating OS object from JSON";

    /*
     * Netaccess
     */

    LINFO << " Configuring network access...";
    QDir dir;
    dir.mkdir(CACHE_DIR);
    _netaccess = new QNetworkAccessManager();
    QNetworkDiskCache *_cache = new QNetworkDiskCache();
    _cache->setCacheDirectory(CACHE_DIR);
    _cache->setMaximumCacheSize(CACHE_SIZE);
    _netaccess->setCache(_cache);
    QNetworkConfigurationManager manager;
    _netaccess->setConfiguration(manager.defaultConfiguration());

    /*
     * Parsing JSON
     */

    LDEBUG << "Checking for OS name";
    if(os.contains("name")) {
        _name = os.value("name").toString();
        LDEBUG << "Found " << _name.toUtf8().constData();
    } else if (os.contains("os_name")) {
        _name = os.value("os_name").toString();
        LDEBUG << "Found " << _name.toUtf8().constData();
    } else {
        LFATAL << "OS Name not specified!";
        _valid = false;
    }

    LDEBUG << "Checking if config indicates non-bootability";
    _bootable = os.value(OS_BOOTABLE, true).toBool() && !os.value(OS_NAME).toString().contains("Data Partition", Qt::CaseInsensitive);
    if(!_bootable) {
        LFATAL << "OS does not seem to be bootable!";
        _valid = false;
    } else {
        LDEBUG << "OS seems to be bootable";
    }

    /* RISC_OS needs a matching riscos_offset */
    LDEBUG << "Checking for a RISC OS configuration";
    if (_name.contains("risc", Qt::CaseInsensitive)) {
        if(os.contains(OS_RISCOS_OFFSET)) {
            _riscosOffset = os.value(OS_RISCOS_OFFSET).toInt();
            if (_riscosOffset != RISCOS_OFFSET) {
                LWARNING << "RiscOS Offset does not match expected value";
            }
        } else {
            _riscosOffset = RISCOS_OFFSET;
            LWARNING << "No RiscOS Offset available, using default value";
        }
    }

    LDEBUG << "Checking if current model is supported by OS";
    QString _model = Utility::Sys::getFileContents("/proc/device-tree/model");
    if (os.contains(OS_SUP_MODELS)) {
        QStringList supportedModels = os.value(OS_SUP_MODELS).toStringList();
        foreach (QString m, supportedModels) {
            /*
             * Check if the full formal model name (e.g. "Raspberry Pi 2 Model B Rev 1.1")
             * contains the string we are told to look for (e.g. "Pi 2")
             */
            _supported = _supported || _model.contains(m, Qt::CaseInsensitive);
        }
        if(!_supported) {
            LFATAL << "Model " << _model.toUtf8().constData()
                   << " is not listed as a supported device: "
                   << os.value("supported_models").toStringList().join(", ").toUtf8().constData();
            _valid = false;
        } else {
            LDEBUG << "Model seems to be supported";
        }
    } else {
        LWARNING << "No information about supported devices available";
    }

    LDEBUG << "Parsing tarball information";
    if(os.contains(OS_TARBALLS)) {
        _tarballs = os.value(OS_TARBALLS).toStringList();
    } else {
        LFATAL << "No tarball information available!";
        _valid = false;
    }

    /*
     * Parsing nested JSON files, that need to be downloaded
     */

    LDEBUG << "Checking for os info";
    if(os.contains(OS_INFO)) {
        parseOSInfo(os.value(OS_INFO).toString());
    } else {
        LFATAL << "No os info available!";
        _valid = false;
    }

    LDEBUG << "Checking for partition info";
    if(os.contains(OS_PARTITION_INFO)) {
        parsePartitionInfo(os.value(OS_PARTITION_INFO).toString());
    } else {
        LFATAL << "No partition info available!";
        _valid = false;
    }

    /*
     * Optional metadata
     */

    LINFO << "Downloading optional meta-files...";

    // Existence is optional, but if it should be there it's bad
    if (os.contains(OS_PARTITION_SETUP)) {
        if(!Utility::Sys::putFileContents(_folder + OS_PARTITION_SETUP_MF, downloadRessource(os.value(OS_PARTITION_SETUP).toString()))) {
            LFATAL << "Unable to save partition setup script to " << _folder.toUtf8().constData() << OS_PARTITION_SETUP_MF;
            _valid = false;
        }
    }

    // Rest is nice to have (actually we need either for this use case, keeping it though due to legacy reasons)
    if (os.contains(OS_MARKETING_INFO)) {
        if(!Utility::Sys::putFileContents(_folder + OS_MARKETING_INFO_MF, downloadRessource(os.value(OS_MARKETING_INFO).toString()))) {
            LERROR << "Unable to save marketing info to " << _folder.toUtf8().constData() << OS_MARKETING_INFO_MF;
        } else {
            LDEBUG << "Extracting marketing archive";
            QProcess::execute("tar xf " + _folder + OS_MARKETING_INFO_MF + " -C " + _folder);
            QFile::remove(_folder + OS_MARKETING_INFO_MF);
        }
    }

    if (os.contains(OS_ICON)) {
        if(!Utility::Sys::putFileContents(_folder + OS_ICON_MF, downloadRessource(os.value(OS_ICON).toString()))) {
            LERROR << "Unable to save os icon to " << _folder.toUtf8().constData() << OS_ICON_MF;
        }
    }
}

OSInfo::~OSInfo() {
    if(_netaccess) {
        delete(_netaccess);
    }
    qDeleteAll(_partitions);
    _partitions.clear();
}


void OSInfo::parseOSInfo(const QString &url) {
    LDEBUG << "Processing OS info";
    QVariantMap osInfo = Utility::Json::parse(downloadRessource(url)).toMap();

    /*
     * Values that might already have been set previously
     */

    if(osInfo.contains("name")) {
        if (_name != osInfo.value("name").toString()) {
            LWARNING << "Provided name does not match name from OS info, using OS info's value";
            _name = osInfo.value("name").toString();
        }
    } else {
        LWARNING << "OS name is not available in OS info";
    }

    if(osInfo.contains(OS_BOOTABLE)) {
        if (_bootable != osInfo.value(OS_BOOTABLE).toBool()) {
            LWARNING << "Bootable property does not match property from OS info, using OS info's value";
            _bootable = osInfo.value(OS_BOOTABLE).toBool();
        }
    } else {
        LWARNING << "Bootable property is not available in OS info";
    }

    if(osInfo.contains(OS_RISCOS_OFFSET)) {
        if(_riscosOffset != osInfo.value(OS_RISCOS_OFFSET).toInt()) {
            LWARNING << "RiscOS property does not match property from OS info, using OS info's value";
            _riscosOffset = osInfo.value(OS_RISCOS_OFFSET).toInt();
        }
    } else {
        LWARNING << "RiscOS offset is not available in OS info";
    }

    /*
     * New information
     */

    if(osInfo.contains(OS_VERSION)) {
        _version = osInfo.value(OS_VERSION).toString();
    } else {
        LWARNING << "OS version is not available in OS info";
    }

    if(osInfo.contains(OS_DESC)) {
        _description = osInfo.value(OS_DESC).toString();
    } else {
        LWARNING << "Description is not available in OS info";
    }

    if(osInfo.contains(OS_RELEASE_DATE)) {
        _releaseDate = osInfo.value(OS_RELEASE_DATE).toString();
    } else {
        LWARNING << "Release date is not available in OS info";
    }

    LDEBUG << "Writing OS info to disk";
    createFolderStructure(); //Creating folder structure here, because os name could have changed

    if(!Utility::Json::saveToFile(_folder + OS_INFO_MF, osInfo)) {
        LERROR << "Unable to save os info to " << _folder.toUtf8().constData() << OS_INFO_MF;
    } else {
        LINFO << "Successfully processed OS info";
    }
}


void OSInfo::parsePartitionInfo(const QString &url) {
    LINFO << "Processing partition info";
    QVariantMap partitionInfo = Utility::Json::parse(downloadRessource(url)).toMap();
    if(partitionInfo.contains(OS_PARTITIONS)) {
        int i = 0;
        if(partitionInfo.value(OS_PARTITIONS).toList().size() != _tarballs.size()) {
            LFATAL << "Mismatch between number of available tarballs and OS partitions!";
            _valid = false;
        } else {
            PartitionInfo *partInfo;
            foreach(QVariant partition, partitionInfo.value(OS_PARTITIONS).toList()) {
                partInfo = new PartitionInfo(partition.toMap(), _tarballs[i++]);
                if(partInfo->isValid()) {
                    _partitions.append(partInfo);
                } else {
                    LFATAL << "Invalid partition info found";
                    _valid = false;
                }
            }
        }
    }

    LDEBUG << "Writing partition info to disk";
    if(!Utility::Json::saveToFile(_folder + OS_PARTITION_INFO_MF, partitionInfo)) {
        LERROR << "Unable to save partition information to " << _folder.toUtf8().constData() << OS_PARTITION_INFO_MF;
    } else {
        LINFO << "Successfully processed partition info";
    }
}

QByteArray OSInfo::downloadRessource(const QString &url) {
    LDEBUG << "Downloading " << url.toUtf8().constData();

    QEventLoop eventLoop;

    QNetworkReply *reply = _netaccess->get(QNetworkRequest(url));

    QObject::connect(reply, SIGNAL(finished()), &eventLoop, SLOT(quit()));
    QObject::connect(reply, SIGNAL(error(NetworkError)), &eventLoop, SLOT(quit()));
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
        delete (reply);
        return QByteArray();
    } else {
        LDEBUG << "Successfully downloaded " << url.toUtf8().constData();
        QByteArray response(reply->readAll());
        delete (reply);
        return response;
    }
}

// This function checks if the folder is available, creates it and stores the path
void OSInfo::createFolderStructure() {
    QDir d;
    _folder = METADATA_DIR + _name;
    _folder.replace(' ', '_');
    LDEBUG << "Using " << _folder.toUtf8().constData() << "/ as target";
    if (!d.exists(_folder)){
        d.mkpath(_folder);
    }
    _folder.append("/");
}

QVariantList OSInfo::vPartitionList() {
    QVariantList vPartitions;
    foreach (PartitionInfo *p, _partitions) {
        vPartitions.append(p->partitionDevice());
    }
    return vPartitions;
}

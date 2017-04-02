#ifndef OSINFO_H
#define OSINFO_H

/**
 * OS info model
 * Contains the information from os.json, and has a pointer to the partitions
 */

#include <QObject>
#include <QList>
#include <QtNetwork/QNetworkAccessManager>
#include <QStringList>
#include "PartitionInfo.h"

// Keys of the OS map
#define OS_DESC "description"
#define OS_MARKETING_INFO "marketing_info"
#define OS_INFO "os_info"
#define OS_NAME "os_name"
#define OS_PARTITION_INFO "partition_info"
#define OS_PARTITIONS "partitions"
#define OS_PARTITION_SETUP "partition_setup"
#define OS_RELEASE_DATE "release_date"
#define OS_SUP_MODELS "supported_models"
#define OS_TARBALLS "tarballs"
#define OS_BOOTABLE "bootable"
#define OS_ICON "icon"
#define OS_VERSION "version"
// Formerly known as RISCOS_OFFSET_KEY:
#define OS_RISCOS_OFFSET "riscos_offset"

// Directory for all metadata
#define METADATA_DIR "/settings/os/"
#define CACHE_DIR "/settings/cache"
#define CACHE_SIZE 8 * 1024 * 1024

// Meta-Data filenames, stored in METADATA_DIR/<os_name>/
#define OS_INFO_MF "os.json"
#define OS_PARTITION_INFO_MF "partitions.json"
#define OS_MARKETING_INFO_MF "marketing.tar"
#define OS_PARTITION_SETUP_MF "partition_setup.sh"
#define OS_ICON_MF "icon.png"

class OSInfo {

public:
    // The constructor takes the JSON, formatted as Map, downloads all necessary metadata and stores it in this variable
    explicit OSInfo(const QVariantMap &os);
    ~OSInfo();


    /*
     * Virtual getter
     */
    QVariantList vPartitionList();

    /*
     * Getter methods
     */
    inline QString folder() { return _folder; }
    inline QString flavour() { return _flavour; }
    inline QString name() { return _name; }
    inline QString description() { return _description; }
    inline QString releaseDate() { return _releaseDate; }
    inline QList<PartitionInfo *> *partitions() { return &_partitions; }
    inline bool bootable() { return _bootable; }
    inline bool isValid() { return _valid; } // Shows if config looks valid

protected:
    QString _folder,
            _flavour,
            _name,
            _description,
            _version,
            _releaseDate;

    QStringList _tarballs;

    bool _bootable,
         _valid, // Check for this variable to see if the object is usable
         _supported;

    int _riscosOffset;

    QList<PartitionInfo *> _partitions;
    QNetworkAccessManager *_netaccess;

private:
    /*
     * This function retrieves the os_info.json file, parses its information into the object and stores the file persistently
     */
    void parseOSInfo(const QString &url);

    /*
     * This function retrieves the partitions.json file, parses its information into the object and stores the file persistently
     */
    void parsePartitionInfo(const QString &url);

    QByteArray downloadRessource(const QString &url);
    void createFolderStructure();
};

#endif // OSINFO_H

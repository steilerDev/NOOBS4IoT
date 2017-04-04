#ifndef PARTITIONINFO_H
#define PARTITIONINFO_H

/*
 * Partition information model
 * Contains information about a single partition from partitions.json
 * and runtime information like the partition device (/dev/mmcblk0pX) it was assigned
 */

#include <QObject>
#include <QVariantMap>

class PartitionInfo
{
public:
    /* Constructor. Gets called from OsInfo with info from json file */
    explicit PartitionInfo(const QMap<QString, QVariant> &m, const QString &tarball);
    explicit PartitionInfo(int partitionNr, int offset, int sectors, const QByteArray &partType);

    bool mountPartition(const QString &dir, const char* args = "");
    bool unmountPartition();

    /*
     * Setter
     */
    inline void setPartitionDevice(const QByteArray &partdevice) { _partitionDevice = partdevice; }
    inline void setRequiresPartitionNumber(int nr) { _requiresPartitionNumber = nr; }
    inline void setPartitionSizeSectors(int size) { _partitionSizeSectors = size; }
    inline void setOffset(int offset) { _offset = offset; }

    /*
     * Getter
     */
    inline QByteArray partitionDevice() { return _partitionDevice; }
    inline QByteArray fsType() { return _fstype; }
    inline QByteArray mkfsOptions() { return _mkfsOptions; }
    inline QByteArray label() { return _label; }
    inline QByteArray partitionType() { return _partitionType; }
    inline QString tarball() { return _tarball; }
    inline int partitionSizeNominal() { return _partitionSizeNominal; }
    inline int requiresPartitionNumber() { return _requiresPartitionNumber; }
    inline int uncompressedTarballSize() { return _uncompressedTarballSize; }
    inline int offset() { return _offset; }
    inline int partitionSizeSectors() { return _partitionSizeSectors; }
    inline int endSector() { return _offset + _partitionSizeSectors; }
    inline bool emptyFS() { return _emptyFS; }
    inline bool wantMaximised() { return _wantMaximised; }
    inline bool active() { return _active; }
    inline bool isValid() { return _valid; } // Indicates if config is valid

protected:
    QByteArray _fstype,
               _mkfsOptions,
               _label,
               _partitionDevice,
               _partitionType;
    QString _tarball,
            _mountedDir;
    int _partitionSizeNominal,
        _requiresPartitionNumber,
        _offset,
        _uncompressedTarballSize,
        _partitionSizeSectors;
    bool _emptyFS,
         _wantMaximised,
         _active,
         _valid;

private:
    bool parsePartitionInfo(QMap<QString, QVariant> partitionInfo);
};

#endif // PARTITIONINFO_H

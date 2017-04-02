#ifndef MULTIIMAGEWRITETHREAD_H
#define MULTIIMAGEWRITETHREAD_H

#include <QThread>
#include <QStringList>
#include <QMultiMap>
#include <QVariantList>
#include <QList>
#include "OSInfo.h"


class MultiImageWrite {
public:
    explicit MultiImageWrite(const QList<OSInfo*> &oses);
    bool writeImages();

protected:
    bool processImage(OSInfo *image);
    bool mkfs(const QByteArray &device, const QByteArray &fstype = "ext4", const QByteArray &label = "", const QByteArray &mkfsopt = "");
    bool dd(const QString &imagePath, const QString &device);
    bool partclone_restore(const QString &imagePath, const QString &device);
    bool untar(const QString &tarball);
    bool isLabelAvailable(const QByteArray &label);
    QByteArray getLabel(const QString part);
    QByteArray getUUID(const QString part);
    void patchConfigTxt();
    bool writePartitionTable(const QMap<int, PartitionInfo *> &partitionMap);
    bool isURL(const QString &s);

    int _extraSpacePerPartition, _part;
    QList<OSInfo *> _images;

    QVariantList installed_os;
};

#endif // MULTIIMAGEWRITETHREAD_H

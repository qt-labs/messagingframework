/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#include <QCoreApplication>
#include <QPair>
#include <QString>

#ifdef QMAIL_QTOPIA

#include <QStorageMetaInfo>
#include <QFileSystem>


QList<QPair<QString, QString> > storageLocations()
{
    QList<QPair<QString, QString> > locations;

    locations.append(qMakePair(qApp->translate("Qtopiamailfile", "Default"), QString("")));

    // If there are any removable partitions we could use, add them to the list
    QFileSystemFilter filter;
    QStorageMetaInfo storage;

    foreach (QFileSystem *fs, storage.fileSystems(&filter, true)) {
        if (fs->isRemovable() && fs->isWritable() && fs->isConnected())
            locations.append(qMakePair(fs->name(), fs->path()));
    }

    return locations;
}

#else

QList<QPair<QString, QString> > storageLocations()
{
    QList<QPair<QString, QString> > locations;
    locations.append(qMakePair(QString("qtopiamailfile"),QString("")));
    return locations;
}

#endif

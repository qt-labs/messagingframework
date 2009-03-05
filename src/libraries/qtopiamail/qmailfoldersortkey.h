/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef QMAILFOLDERSORTKEY_H
#define QMAILFOLDERSORTKEY_H

#include "qmailglobal.h"
#include <QSharedData>
#include <QtGlobal>
#include <QPair>

class QMailFolderSortKeyPrivate;

class QTOPIAMAIL_EXPORT QMailFolderSortKey
{
public:
    enum Property
    {
        Id,
        Path,
        ParentFolderId,
        ParentAccountId,
        DisplayName,
        Status,
        ServerCount,
        ServerUnreadCount,
    };

    typedef QPair<Property, Qt::SortOrder> ArgumentType;

public:
    QMailFolderSortKey();
    QMailFolderSortKey(const QMailFolderSortKey& other);
    virtual ~QMailFolderSortKey();

    QMailFolderSortKey operator&(const QMailFolderSortKey& other) const;
    QMailFolderSortKey& operator&=(const QMailFolderSortKey& other);

    bool operator==(const QMailFolderSortKey& other) const;
    bool operator !=(const QMailFolderSortKey& other) const;

    QMailFolderSortKey& operator=(const QMailFolderSortKey& other);

    bool isEmpty() const;

    const QList<ArgumentType> &arguments() const;

    static QMailFolderSortKey id(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailFolderSortKey path(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailFolderSortKey parentFolderId(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailFolderSortKey parentAccountId(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailFolderSortKey displayName(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailFolderSortKey status(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailFolderSortKey serverCount(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailFolderSortKey serverUnreadCount(Qt::SortOrder order = Qt::AscendingOrder);

private:
    QMailFolderSortKey(Property p, Qt::SortOrder order);

    friend class QMailStore;
    friend class QMailStorePrivate;

    QSharedDataPointer<QMailFolderSortKeyPrivate> d;
};

#endif

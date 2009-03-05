/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef QMAILMESSAGESORTKEY_H
#define QMAILMESSAGESORTKEY_H

#include "qmailglobal.h"
#include "qmailipc.h"
#include <QSharedData>
#include <QtGlobal>
#include <QPair>

class QMailMessageSortKeyPrivate;

class QTOPIAMAIL_EXPORT QMailMessageSortKey
{
public:
    enum Property
    {
        Id,
        Type,
        ParentFolderId,
        Sender,
        Recipients,
        Subject,
        TimeStamp,
        ReceptionTimeStamp,
        Status,
        ServerUid,
        Size,
        ParentAccountId,
        ContentType,
        PreviousParentFolderId
    };

    typedef QPair<Property, Qt::SortOrder> ArgumentType;

public:
    QMailMessageSortKey();
    QMailMessageSortKey(const QMailMessageSortKey& other);
    virtual ~QMailMessageSortKey();

    QMailMessageSortKey operator&(const QMailMessageSortKey& other) const;
    QMailMessageSortKey& operator&=(const QMailMessageSortKey& other);

    bool operator==(const QMailMessageSortKey& other) const;
    bool operator !=(const QMailMessageSortKey& other) const;

    QMailMessageSortKey& operator=(const QMailMessageSortKey& other);

    bool isEmpty() const;

    const QList<ArgumentType> &arguments() const;

    template <typename Stream> void serialize(Stream &stream) const;
    template <typename Stream> void deserialize(Stream &stream);

    static QMailMessageSortKey id(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailMessageSortKey messageType(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailMessageSortKey parentFolderId(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailMessageSortKey sender(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailMessageSortKey recipients(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailMessageSortKey subject(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailMessageSortKey timeStamp(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailMessageSortKey receptionTimeStamp(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailMessageSortKey status(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailMessageSortKey serverUid(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailMessageSortKey size(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailMessageSortKey parentAccountId(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailMessageSortKey contentType(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailMessageSortKey previousParentFolderId(Qt::SortOrder order = Qt::AscendingOrder);
        
private:
    QMailMessageSortKey(Property p, Qt::SortOrder order);

    friend class QMailStore;
    friend class QMailStorePrivate;

    QSharedDataPointer<QMailMessageSortKeyPrivate> d;
};

Q_DECLARE_USER_METATYPE(QMailMessageSortKey);

#endif

/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef QMAILACCOUNTSORTKEY_H
#define QMAILACCOUNTSORTKEY_H

#include "qmailglobal.h"
#include <QSharedData>
#include <QtGlobal>
#include <QPair>

class QMailAccountSortKeyPrivate;

class QTOPIAMAIL_EXPORT QMailAccountSortKey
{
public:
    enum Property
    {
        Id,
        Name,
        MessageType,
        Status
    };

    typedef QPair<Property, Qt::SortOrder> ArgumentType;

public:
    QMailAccountSortKey();
    QMailAccountSortKey(const QMailAccountSortKey& other);
    virtual ~QMailAccountSortKey();

    QMailAccountSortKey operator&(const QMailAccountSortKey& other) const;
    QMailAccountSortKey& operator&=(const QMailAccountSortKey& other);

    bool operator==(const QMailAccountSortKey& other) const;
    bool operator !=(const QMailAccountSortKey& other) const;

    QMailAccountSortKey& operator=(const QMailAccountSortKey& other);

    bool isEmpty() const;

    const QList<ArgumentType> &arguments() const;

    static QMailAccountSortKey id(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailAccountSortKey name(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailAccountSortKey messageType(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailAccountSortKey status(Qt::SortOrder order = Qt::AscendingOrder);

private:
    QMailAccountSortKey(Property p, Qt::SortOrder order);

    friend class QMailStore;
    friend class QMailStorePrivate;

    QSharedDataPointer<QMailAccountSortKeyPrivate> d;
};

#endif


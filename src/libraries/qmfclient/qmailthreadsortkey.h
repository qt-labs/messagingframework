/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QMAILTHREADSORTKEY_H
#define QMAILTHREADSORTKEY_H

#include "qmailglobal.h"
#include "qmailipc.h"
#include "qmailsortkeyargument.h"
#include <QSharedData>
#include <QtGlobal>

class QMailThreadSortKeyPrivate;

class QMF_EXPORT QMailThreadSortKey
{
public:
    enum Property
    {
        Id,
        ParentAccountId,
        ServerUid,
        UnreadCount,
        MessageCount,
        Subject,
        Senders,
        Preview,
        StartedDate,
        LastDate,
        Status
    };

    typedef QMailSortKeyArgument<Property> ArgumentType;

public:
    QMailThreadSortKey();
    QMailThreadSortKey(const QMailThreadSortKey& other);
    virtual ~QMailThreadSortKey();

    QMailThreadSortKey operator&(const QMailThreadSortKey& other) const;
    QMailThreadSortKey& operator&=(const QMailThreadSortKey& other);

    bool operator==(const QMailThreadSortKey& other) const;
    bool operator!=(const QMailThreadSortKey& other) const;

    QMailThreadSortKey& operator=(const QMailThreadSortKey& other);

    bool isEmpty() const;

    const QList<ArgumentType> &arguments() const;

    template <typename Stream> void serialize(Stream &stream) const;
    template <typename Stream> void deserialize(Stream &stream);

    static QMailThreadSortKey id(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailThreadSortKey parentAccountId(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailThreadSortKey serverUid(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailThreadSortKey unreadCount(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailThreadSortKey messageCount(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailThreadSortKey subject(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailThreadSortKey senders(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailThreadSortKey preview(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailThreadSortKey startedDate(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailThreadSortKey lastDate(Qt::SortOrder order = Qt::AscendingOrder);

    static QMailThreadSortKey status(quint64 mask, Qt::SortOrder order = Qt::DescendingOrder);
private:
    QMailThreadSortKey(Property p, Qt::SortOrder order, quint64 mask = 0);
    QMailThreadSortKey(const QList<ArgumentType> &args);

    friend class QMailStore;
    friend class QMailStorePrivate;

    QSharedDataPointer<QMailThreadSortKeyPrivate> d;
};

Q_DECLARE_USER_METATYPE(QMailThreadSortKey)

#endif

/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QMAILACCOUNTSORTKEY_H
#define QMAILACCOUNTSORTKEY_H

#include "qmailglobal.h"
#include "qmailipc.h"
#include "qmailsortkeyargument.h"
#include <QSharedData>
#include <QtGlobal>

class QMailAccountSortKeyPrivate;

class QMF_EXPORT QMailAccountSortKey
{
public:
    enum Property
    {
        Id,
        Name,
        MessageType,
        Status,
        LastSynchronized
    };

    typedef QMailSortKeyArgument<Property> ArgumentType;

public:
    QMailAccountSortKey();
    QMailAccountSortKey(const QMailAccountSortKey& other);
    virtual ~QMailAccountSortKey();

    QMailAccountSortKey operator&(const QMailAccountSortKey& other) const;
    QMailAccountSortKey& operator&=(const QMailAccountSortKey& other);

    bool operator==(const QMailAccountSortKey& other) const;
    bool operator!=(const QMailAccountSortKey& other) const;

    QMailAccountSortKey& operator=(const QMailAccountSortKey& other);

    bool isEmpty() const;

    const QList<ArgumentType> &arguments() const;

    template <typename Stream> void serialize(Stream &stream) const;
    template <typename Stream> void deserialize(Stream &stream);

    static QMailAccountSortKey id(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailAccountSortKey name(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailAccountSortKey messageType(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailAccountSortKey lastSynchronized(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailAccountSortKey status(quint64 mask, Qt::SortOrder order = Qt::DescendingOrder);

private:
    QMailAccountSortKey(Property p, Qt::SortOrder order, quint64 mask = 0);
    QMailAccountSortKey(const QList<ArgumentType> &args);

    friend class QMailStore;
    friend class QMailStorePrivate;

    QSharedDataPointer<QMailAccountSortKeyPrivate> d;
};

Q_DECLARE_USER_METATYPE(QMailAccountSortKey)

#endif


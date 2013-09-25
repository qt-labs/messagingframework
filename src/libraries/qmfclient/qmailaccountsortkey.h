/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
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
        LastSynchronized,
        IconPath
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
    static QMailAccountSortKey iconPath(Qt::SortOrder order = Qt::AscendingOrder);


private:
    QMailAccountSortKey(Property p, Qt::SortOrder order, quint64 mask = 0);
    QMailAccountSortKey(const QList<ArgumentType> &args);

    friend class QMailStore;
    friend class QMailStorePrivate;

    QSharedDataPointer<QMailAccountSortKeyPrivate> d;
};

Q_DECLARE_USER_METATYPE(QMailAccountSortKey)

#endif


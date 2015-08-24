/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QMAILFOLDERSORTKEY_H
#define QMAILFOLDERSORTKEY_H

#include "qmailglobal.h"
#include "qmailipc.h"
#include "qmailsortkeyargument.h"
#include <QSharedData>
#include <QtGlobal>

class QMailFolderSortKeyPrivate;

class QMF_EXPORT QMailFolderSortKey
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
        ServerUndiscoveredCount,
    };

    typedef QMailSortKeyArgument<Property> ArgumentType;

public:
    QMailFolderSortKey();
    QMailFolderSortKey(const QMailFolderSortKey& other);
    virtual ~QMailFolderSortKey();

    QMailFolderSortKey operator&(const QMailFolderSortKey& other) const;
    QMailFolderSortKey& operator&=(const QMailFolderSortKey& other);

    bool operator==(const QMailFolderSortKey& other) const;
    bool operator!=(const QMailFolderSortKey& other) const;

    QMailFolderSortKey& operator=(const QMailFolderSortKey& other);

    bool isEmpty() const;

    const QList<ArgumentType> &arguments() const;

    template <typename Stream> void serialize(Stream &stream) const;
    template <typename Stream> void deserialize(Stream &stream);

    static QMailFolderSortKey id(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailFolderSortKey path(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailFolderSortKey parentFolderId(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailFolderSortKey parentAccountId(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailFolderSortKey displayName(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailFolderSortKey serverCount(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailFolderSortKey serverUnreadCount(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailFolderSortKey serverUndiscoveredCount(Qt::SortOrder order = Qt::AscendingOrder);

    static QMailFolderSortKey status(quint64 mask, Qt::SortOrder order = Qt::DescendingOrder);

private:
    QMailFolderSortKey(Property p, Qt::SortOrder order, quint64 mask = 0);
    QMailFolderSortKey(const QList<ArgumentType> &args);

    friend class QMailStore;
    friend class QMailStorePrivate;

    QSharedDataPointer<QMailFolderSortKeyPrivate> d;
};

Q_DECLARE_USER_METATYPE(QMailFolderSortKey)

#endif

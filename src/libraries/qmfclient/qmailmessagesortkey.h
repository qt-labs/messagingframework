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

#ifndef QMAILMESSAGESORTKEY_H
#define QMAILMESSAGESORTKEY_H

#include "qmailglobal.h"
#include "qmailipc.h"
#include "qmailsortkeyargument.h"
#include <QSharedData>
#include <QtGlobal>

class QMailMessageSortKeyPrivate;

class QMF_EXPORT QMailMessageSortKey
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
        PreviousParentFolderId,
        CopyServerUid,
        ListId,
        RestoreFolderId,
        RfcId,
        ParentThreadId
    };

    typedef QMailSortKeyArgument<Property> ArgumentType;

public:
    QMailMessageSortKey();
    QMailMessageSortKey(const QMailMessageSortKey& other);
    virtual ~QMailMessageSortKey();

    QMailMessageSortKey operator&(const QMailMessageSortKey& other) const;
    QMailMessageSortKey& operator&=(const QMailMessageSortKey& other);

    bool operator==(const QMailMessageSortKey& other) const;
    bool operator!=(const QMailMessageSortKey& other) const;

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
    static QMailMessageSortKey serverUid(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailMessageSortKey size(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailMessageSortKey parentAccountId(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailMessageSortKey contentType(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailMessageSortKey previousParentFolderId(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailMessageSortKey copyServerUid(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailMessageSortKey restoreFolderId(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailMessageSortKey listId(Qt::SortOrder order = Qt::AscendingOrder);
    static QMailMessageSortKey rfcId(Qt::SortOrder order = Qt::AscendingOrder);
        
    static QMailMessageSortKey status(quint64 mask, Qt::SortOrder order = Qt::DescendingOrder);

private:
    QMailMessageSortKey(Property p, Qt::SortOrder order, quint64 mask = 0);
    QMailMessageSortKey(const QList<ArgumentType> &args);

    friend class QMailStore;
    friend class QMailStorePrivate;

    QSharedDataPointer<QMailMessageSortKeyPrivate> d;
};

Q_DECLARE_USER_METATYPE(QMailMessageSortKey)

#endif

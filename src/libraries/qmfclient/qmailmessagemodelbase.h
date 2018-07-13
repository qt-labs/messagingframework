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

#ifndef QMAILMESSAGEMODELBASE_H
#define QMAILMESSAGEMODELBASE_H

#include <QAbstractListModel>
#include "qmailglobal.h"
#include "qmailmessagekey.h"
#include "qmailmessagesortkey.h"


class QMailMessageModelImplementation
{
public:
    virtual ~QMailMessageModelImplementation();

    virtual QMailMessageKey key() const = 0;
    virtual void setKey(const QMailMessageKey& key) = 0;

    virtual QMailMessageSortKey sortKey() const = 0;
    virtual void setSortKey(const QMailMessageSortKey& sortKey) = 0;

    virtual uint limit() const = 0;
    virtual void setLimit(uint limit) = 0;
    virtual int totalCount() const = 0;

    virtual bool isEmpty() const = 0;

    virtual int rowCount(const QModelIndex& idx) const = 0;
    virtual int columnCount(const QModelIndex& idx) const = 0;

    virtual QMailMessageId idFromIndex(const QModelIndex& index) const = 0;
    virtual QModelIndex indexFromId(const QMailMessageId& id) const = 0;

    virtual Qt::CheckState checkState(const QModelIndex &idx) const = 0;
    virtual void setCheckState(const QModelIndex &idx, Qt::CheckState state) = 0;

    virtual void reset() = 0;

    virtual bool ignoreMailStoreUpdates() const = 0;
    virtual bool setIgnoreMailStoreUpdates(bool ignore) = 0;

    virtual bool processMessagesAdded(const QMailMessageIdList& ids) = 0;
    virtual bool processMessagesUpdated(const QMailMessageIdList& ids) = 0;
    virtual bool processMessagesRemoved(const QMailMessageIdList& ids) = 0;
};


class QMF_EXPORT QMailMessageModelBase : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum Roles 
    {
        MessageAddressTextRole = Qt::UserRole,
        MessageSubjectTextRole,
        MessageFilterTextRole,
        MessageTimeStampTextRole,
        MessageSizeTextRole,
        MessageTypeIconRole,
        MessageStatusIconRole,
        MessageDirectionIconRole,
        MessagePresenceIconRole,
        MessageBodyTextRole,
        MessageIdRole
    };

    QMailMessageModelBase(QObject* parent = Q_NULLPTR);
    virtual ~QMailMessageModelBase();

    int rowCount(const QModelIndex& index = QModelIndex()) const;
    int columnCount(const QModelIndex& index = QModelIndex()) const;

    bool isEmpty() const;

    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);

    QMailMessageKey key() const;
    void setKey(const QMailMessageKey& key);

    QMailMessageSortKey sortKey() const;
    void setSortKey(const QMailMessageSortKey& sortKey);

    uint limit() const;
    void setLimit(uint limit);
    int totalCount() const;

    QMailMessageId idFromIndex(const QModelIndex& index) const;
    QModelIndex indexFromId(const QMailMessageId& id) const;

    bool ignoreMailStoreUpdates() const;
    void setIgnoreMailStoreUpdates(bool ignore);

    void fullRefresh(bool modelChanged);

    virtual QModelIndex generateIndex(int row, int column, void *ptr) = 0;

    void emitDataChanged(const QModelIndex &idx, const QModelIndex &jdx);

    void emitBeginInsertRows(const QModelIndex &idx, int start, int end);
    void emitEndInsertRows();

    void emitBeginRemoveRows(const QModelIndex &idx, int start, int end);
    void emitEndRemoveRows();

Q_SIGNALS:
    void modelChanged();

protected Q_SLOTS:
    void messagesAdded(const QMailMessageIdList& ids); 
    void messagesUpdated(const QMailMessageIdList& ids);
    void messagesRemoved(const QMailMessageIdList& ids);

protected:
    virtual QVariant data(const QMailMessageMetaData &metaData, int role) const;

    virtual QMailMessageModelImplementation *impl() = 0;
    virtual const QMailMessageModelImplementation *impl() const = 0;
};

#endif

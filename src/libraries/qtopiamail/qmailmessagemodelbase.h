/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
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
    virtual QMailMessageKey key() const = 0;
    virtual void setKey(const QMailMessageKey& key) = 0;

    virtual QMailMessageSortKey sortKey() const = 0;
    virtual void setSortKey(const QMailMessageSortKey& sortKey) = 0;

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


class QTOPIAMAIL_EXPORT QMailMessageModelBase : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum Roles 
    {
        MessageAddressTextRole = Qt::UserRole,
        MessageSubjectTextRole,
        MessageFilterTextRole,
        MessageTimeStampTextRole,
        MessageTypeIconRole,
        MessageStatusIconRole,
        MessageDirectionIconRole,
        MessagePresenceIconRole,
        MessageBodyTextRole,
        MessageIdRole
    };

    QMailMessageModelBase(QObject* parent = 0);
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

    QMailMessageId idFromIndex(const QModelIndex& index) const;
    QModelIndex indexFromId(const QMailMessageId& id) const;

    bool ignoreMailStoreUpdates() const;
    void setIgnoreMailStoreUpdates(bool ignore);

    virtual QModelIndex generateIndex(int row, int column, void *ptr) = 0;

    void emitDataChanged(const QModelIndex &idx);

    void emitBeginInsertRows(const QModelIndex &idx, int start, int end);
    void emitEndInsertRows();

    void emitBeginRemoveRows(const QModelIndex &idx, int start, int end);
    void emitEndRemoveRows();

signals:
    void modelChanged();

protected slots:
    void messagesAdded(const QMailMessageIdList& ids); 
    void messagesUpdated(const QMailMessageIdList& ids);
    void messagesRemoved(const QMailMessageIdList& ids);

protected:
    virtual QVariant data(const QMailMessageMetaData &metaData, int role) const;

    void fullRefresh(bool modelChanged);

    virtual QMailMessageModelImplementation *impl() = 0;
    virtual const QMailMessageModelImplementation *impl() const = 0;
};

#endif
/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef QMAILMESSAGELISTMODEL_H
#define QMAILMESSAGELISTMODEL_H

#include <QAbstractListModel>
#include "qmailmessagemodelbase.h"
#include "qmailmessagekey.h"
#include "qmailmessagesortkey.h"

class QMailMessageListModelPrivate;

class QTOPIAMAIL_EXPORT QMailMessageListModel : public QAbstractListModel, public QMailMessageModelBase
{
    Q_OBJECT 

public:
    QMailMessageListModel(QObject* parent = 0);
    virtual ~QMailMessageListModel();

    int rowCount(const QModelIndex& index = QModelIndex()) const;

    bool isEmpty() const;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);

    QMailMessageKey key() const;
    void setKey(const QMailMessageKey& key);

    QMailMessageSortKey sortKey() const;
    void setSortKey(const QMailMessageSortKey& sortKey);

    QMailMessageId idFromIndex(const QModelIndex& index) const;
    QModelIndex indexFromId(const QMailMessageId& id) const;

    bool ignoreMailStoreUpdates() const;
    void setIgnoreMailStoreUpdates(bool ignore);

signals:
    void modelChanged();

private slots:
    void messagesAdded(const QMailMessageIdList& ids); 
    void messagesUpdated(const QMailMessageIdList& ids);
    void messagesRemoved(const QMailMessageIdList& ids);

private:
    void fullRefresh(bool modelChanged);

private:
    friend class QMailMessageListModelPrivate;

    QModelIndex generateIndex(int row, const QModelIndex &idx);

    QMailMessageListModelPrivate* d;
};

#endif

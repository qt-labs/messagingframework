/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef QMAILMESSAGETHREADEDMODEL_H
#define QMAILMESSAGETHREADEDMODEL_H

#include <QAbstractItemModel>
#include "qmailmessagekey.h"
#include "qmailmessagemodelbase.h"
#include "qmailmessagesortkey.h"

class QMailMessageThreadedModelPrivate;
class QMailMessageThreadedModelItem;

class QTOPIAMAIL_EXPORT QMailMessageThreadedModel : public QAbstractItemModel, public QMailMessageModelBase
{
    Q_OBJECT 

public:
    QMailMessageThreadedModel(QObject* parent = 0);
    virtual ~QMailMessageThreadedModel();

    int rowCount(const QModelIndex& index = QModelIndex()) const;
    int columnCount(const QModelIndex& index = QModelIndex()) const;

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

    int rootRow(const QModelIndex& index) const;

    virtual QModelIndex index(int row, int column, const QModelIndex &parentIndex = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex &index) const;

signals:
    void modelChanged();

# if 0
private slots:
    void messagesAdded(const QMailMessageIdList& ids); 
    void messagesUpdated(const QMailMessageIdList& ids);
    void messagesRemoved(const QMailMessageIdList& ids);
#endif

private:
    void fullRefresh(bool modelChanged);

    QModelIndex index(QMailMessageThreadedModelItem *item, int column) const;
    QModelIndex parentIndex(QMailMessageThreadedModelItem *item, int column) const;

    QMailMessageThreadedModelItem *itemFromIndex(const QModelIndex &index) const;
    QModelIndex indexFromItem(QMailMessageThreadedModelItem *item) const;

private:
    QMailMessageThreadedModelPrivate* d;
};

#endif

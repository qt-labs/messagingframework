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
#include "qmailmessagekey.h"
#include "qmailmessagesortkey.h"

class QMailMessageListModelPrivate;

class QTOPIAMAIL_EXPORT QMailMessageListModel : public QAbstractListModel
{
    Q_OBJECT 

public:
    enum Roles 
    {
        MessageAddressTextRole = Qt::UserRole, //sender or recipient address
        MessageSubjectTextRole, //subject
        MessageFilterTextRole, //the address text role concatenated with subject text role
        MessageTimeStampTextRole, //timestamp text
        MessageTypeIconRole, //mms,email,sms icon
        MessageStatusIconRole, //read,unread,downloaded icon 
        MessageDirectionIconRole, //incoming or outgoing message icon
        MessagePresenceIconRole, 
        MessageBodyTextRole, //body text, only if textual data
        MessageIdRole
    };

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
    QMailMessageListModelPrivate* d;
};

#endif

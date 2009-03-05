/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef EMAILFOLDERMODEL_H
#define EMAILFOLDERMODEL_H

#include "foldermodel.h"
#include <qmailfolder.h>

// A message set that returns only the email messages within a folder:

class EmailFolderMessageSet : public QMailFolderMessageSet
{
    Q_OBJECT

public:
    EmailFolderMessageSet(QMailMessageSetContainer *container, const QMailFolderId &folderId, bool hierarchical);

    virtual QMailMessageKey messageKey() const;
    virtual QMailMessageKey descendantsMessageKey() const;

    static QMailMessageKey contentKey(const QMailFolderId &id, bool descendants);

protected:
    virtual void createChild(const QMailFolderId &childId);
};


// An account folder element which returns only email messages

class EmailAccountMessageSet : public QMailAccountMessageSet
{
    Q_OBJECT

public:
    EmailAccountMessageSet(QMailMessageSetContainer *container, const QMailAccountId &accountId);

    virtual QMailMessageKey messageKey() const;
    virtual QMailMessageKey descendantsMessageKey() const;

    static QMailMessageKey contentKey(const QMailAccountId &id);

protected:
    virtual void createChild(const QMailFolderId &childId);
};


// A folder element which includes all email accounts as sub-folders:

class InboxMessageSet : public EmailFolderMessageSet
{
    Q_OBJECT

public:
    InboxMessageSet(QMailMessageSetContainer *container);

    virtual QMailMessageKey messageKey() const;
    virtual QMailMessageKey descendantsMessageKey() const;

    static QMailMessageKey contentKey();

protected slots:
    virtual void accountsAdded(const QMailAccountIdList &ids);
    virtual void accountsUpdated(const QMailAccountIdList &ids);
    virtual void accountsRemoved(const QMailAccountIdList &ids);
    virtual void accountContentsModified(const QMailAccountIdList &ids);

protected:
    virtual void init();
    virtual void synchronizeAccountChildren();
    virtual void resyncState();

    static QMailAccountKey emailAccountKey();

protected:
    QMailAccountIdList _accountIds;
};


class EmailFolderModel : public FolderModel
{
    Q_OBJECT

public:
    using FolderModel::data;

    enum Roles 
    {
        FolderIconRole = FolderModel::FolderIconRole,
        FolderStatusRole = FolderModel::FolderStatusRole,
        FolderStatusDetailRole = FolderModel::FolderStatusDetailRole,
        FolderIdRole = FolderModel::FolderIdRole,
        FolderSynchronizationEnabledRole,
        ContextualAccountIdRole
    };

    EmailFolderModel(QObject *parent = 0);
    ~EmailFolderModel();

    virtual QVariant data(QMailMessageSet *item, int role, int column) const;
    virtual QVariant headerData(int section, Qt::Orientation, int role) const;

protected:
    virtual void init();

    virtual QString itemStatusDetail(QMailMessageSet *item) const;

    virtual bool itemSynchronizationEnabled(QMailMessageSet *item) const;
    virtual QMailAccountId itemContextualAccountId(QMailMessageSet *item) const;
};


#endif


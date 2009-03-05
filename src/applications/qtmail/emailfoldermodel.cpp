/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#include "emailfoldermodel.h"
#include <qmailmessageset.h>
#include <qmailaccount.h>
#include <qmailstore.h>

/* EmailFolderMessageSet */

EmailFolderMessageSet::EmailFolderMessageSet(QMailMessageSetContainer *container, const QMailFolderId &folderId, bool hierarchical)
    : QMailFolderMessageSet(container, folderId, hierarchical)
{
}

QMailMessageKey EmailFolderMessageSet::messageKey() const
{
    return contentKey(folderId(), false);
}

QMailMessageKey EmailFolderMessageSet::descendantsMessageKey() const
{
    return contentKey(folderId(), true);
}

QMailMessageKey EmailFolderMessageSet::contentKey(const QMailFolderId &id, bool descendants)
{
    // Only return email messages from this folder
    return (QMailFolderMessageSet::contentKey(id, descendants) &
            QMailMessageKey::messageType(QMailMessage::Email));
}

void EmailFolderMessageSet::createChild(const QMailFolderId &childId)
{
    // Our child folders should also be email-only
    EmailFolderMessageSet *child = new EmailFolderMessageSet(this, childId, hierarchical());
    append(child);
}


/* EmailAccountMessageSet */

EmailAccountMessageSet::EmailAccountMessageSet(QMailMessageSetContainer *container, const QMailAccountId &accountId)
    : QMailAccountMessageSet(container, accountId)
{
}

QMailMessageKey EmailAccountMessageSet::messageKey() const
{
    return contentKey(accountId());
}

QMailMessageKey EmailAccountMessageSet::descendantsMessageKey() const
{
    // No such concept for accounts
    return QMailMessageKey::nonMatchingKey();
}

QMailMessageKey EmailAccountMessageSet::contentKey(const QMailAccountId &id)
{
    // Only return incoming messages from this account, and not those in the Trash folder
    return (QMailAccountMessageSet::contentKey(id, false) &
            QMailMessageKey::parentFolderId(QMailFolderId(QMailFolder::TrashFolder), QMailDataComparator::NotEqual) &
            QMailMessageKey::status(QMailMessage::Incoming, QMailDataComparator::Includes));
}

void EmailAccountMessageSet::createChild(const QMailFolderId &childId)
{
    // Our child folders should also be email-only
    EmailFolderMessageSet *child = new EmailFolderMessageSet(this, childId, hierarchical());
    append(child);
}


/* InboxMessageSet */

InboxMessageSet::InboxMessageSet(QMailMessageSetContainer *container)
    : EmailFolderMessageSet(container, QMailFolderId(QMailFolder::InboxFolder), false)
{
}

QMailMessageKey InboxMessageSet::messageKey() const
{
    return contentKey();
}

QMailMessageKey InboxMessageSet::descendantsMessageKey() const
{
    // No such concept for the Inbox
    return QMailMessageKey::nonMatchingKey();
}

QMailMessageKey InboxMessageSet::contentKey()
{
    // Return all incoming messages for any email acount, unless in the Trash folder
    return (QMailMessageKey::parentAccountId(emailAccountKey()) &
            QMailMessageKey::parentFolderId(QMailFolderId(QMailFolder::TrashFolder), QMailDataComparator::NotEqual) &
            QMailMessageKey::status(QMailMessage::Incoming, QMailDataComparator::Includes));
}

void InboxMessageSet::accountsAdded(const QMailAccountIdList &)
{
    synchronizeAccountChildren();
}

void InboxMessageSet::accountsUpdated(const QMailAccountIdList &)
{
    synchronizeAccountChildren();
}

void InboxMessageSet::accountsRemoved(const QMailAccountIdList &)
{
    synchronizeAccountChildren();
}

void InboxMessageSet::accountContentsModified(const QMailAccountIdList &ids)
{
    foreach (const QMailAccountId &id, ids) {
        if (_accountIds.contains(id)) {
            update(this);
            return;
        }
    }
}

void InboxMessageSet::init()
{
    // Add every email account as a folder within the inbox
    synchronizeAccountChildren();

    connect(model(), SIGNAL(accountsAdded(QMailAccountIdList)), this, SLOT(accountsAdded(QMailAccountIdList)));
    connect(model(), SIGNAL(accountsUpdated(QMailAccountIdList)), this, SLOT(accountsUpdated(QMailAccountIdList)));
    connect(model(), SIGNAL(accountsRemoved(QMailAccountIdList)), this, SLOT(accountsRemoved(QMailAccountIdList)));
    connect(model(), SIGNAL(accountContentsModified(QMailAccountIdList)), this, SLOT(accountContentsModified(QMailAccountIdList)));

    EmailFolderMessageSet::init();
}

void InboxMessageSet::resyncState()
{
    synchronizeAccountChildren();

    EmailFolderMessageSet::resyncState();
}

void InboxMessageSet::synchronizeAccountChildren()
{
    QMailAccountIdList newAccountIds(QMailStore::instance()->queryAccounts(emailAccountKey()));
    if (newAccountIds != _accountIds) {
        // Our subfolder set has changed
        _accountIds = newAccountIds;

        // Delete any accounts that are no longer present
        QList<QMailMessageSet*> obsoleteChildren;
        for (int i = 0; i < count(); ++i) {
            QMailAccountId childId = static_cast<QMailAccountMessageSet*>(at(i))->accountId();
            if (newAccountIds.contains(childId)) {
                newAccountIds.removeAll(childId);
            } else {
                obsoleteChildren.append(at(i));
            }
        }
        remove(obsoleteChildren);

        // Add any child folders we don't already contain
        foreach (const QMailAccountId &accountId, newAccountIds) {
            append(new EmailAccountMessageSet(this, accountId));
        }

        update(this);
    }
}

QMailAccountKey InboxMessageSet::emailAccountKey()
{
  return (QMailAccountKey::messageType(QMailMessage::Email) &
          QMailAccountKey::status(QMailAccount::Enabled, QMailDataComparator::Includes));
}


/* EmailFolderModel */

EmailFolderModel::EmailFolderModel(QObject *parent)
    : FolderModel(parent)
{
    init();
}

EmailFolderModel::~EmailFolderModel()
{
}

QVariant EmailFolderModel::data(QMailMessageSet *item, int role, int column) const
{
    if (item) {
        if (role == FolderSynchronizationEnabledRole) {
            return itemSynchronizationEnabled(item);
        } else if (role == ContextualAccountIdRole) {
            return itemContextualAccountId(item);
        }

        return FolderModel::data(item, role, column);
    }

    return QVariant();
}

QVariant EmailFolderModel::headerData(int section, Qt::Orientation, int role) const
{
    if (role == Qt::DisplayRole && section == 0)
        return tr("Folder");

    return QVariant();
}

void EmailFolderModel::init()
{
    // Add the special Inbox folder
    append(new InboxMessageSet(this));

    // Add the remainder of the standard folders
    foreach (QMailFolder::StandardFolder identifier, 
             QList<QMailFolder::StandardFolder>() << QMailFolder::OutboxFolder
                                                  << QMailFolder::DraftsFolder
                                                  << QMailFolder::SentFolder
                                                  << QMailFolder::TrashFolder) {
        append(new EmailFolderMessageSet(this, QMailFolderId(identifier), false));
    }
}

QString EmailFolderModel::itemStatusDetail(QMailMessageSet *item) const
{
    // Don't report any state for excluded folders
    if (!itemSynchronizationEnabled(item))
        return QString();

    return FolderModel::itemStatusDetail(item);
}

bool EmailFolderModel::itemSynchronizationEnabled(QMailMessageSet *item) const
{
    if (QMailFolderMessageSet *folderItem = qobject_cast<QMailFolderMessageSet*>(item)) {
        // Only relevant for account folders
        QMailFolder folder(folderItem->folderId());
        if (folder.parentAccountId().isValid())
            return (folder.status() & QMailFolder::SynchronizationEnabled);
    }

    return true;
}

QMailAccountId EmailFolderModel::itemContextualAccountId(QMailMessageSet *item) const
{
    if (QMailAccountMessageSet *accountItem = qobject_cast<QMailAccountMessageSet*>(item)) {
        return accountItem->accountId();
    } else if (QMailFolderMessageSet *folderItem = qobject_cast<QMailFolderMessageSet*>(item)) {
        QMailFolder folder(folderItem->folderId());
        if (folder.id().isValid())
            return folder.parentAccountId();
    }

    return QMailAccountId();
}


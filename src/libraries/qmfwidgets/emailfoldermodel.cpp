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

#include "emailfoldermodel.h"
#include <qmailmessageset.h>
#include <qmailaccount.h>
#include <qmailstore.h>
#include "qtmailnamespace.h"

/* EmailStandardFolderMessageSet */

EmailStandardFolderMessageSet::EmailStandardFolderMessageSet(QMailMessageSetContainer *container, QMailFolder::StandardFolder folderType, const QString &name)
    : QMailFilterMessageSet(container, contentKey(folderType), name),
      _type(folderType)
{
}

QMailFolder::StandardFolder EmailStandardFolderMessageSet::standardFolderType() const
{
    return _type;
}

QMailMessageKey EmailStandardFolderMessageSet::contentKey(QMailFolder::StandardFolder type)
{
    QMailMessageKey key;

    quint64 setMask = 0;
    quint64 unsetMask = 0;

    switch (type) {
    case QMailFolder::OutboxFolder:
        setMask = QMailMessage::Outbox;
        unsetMask = QMailMessage::Trash;
        break;

    case QMailFolder::DraftsFolder:
        setMask = QMailMessage::Draft;
        unsetMask = QMailMessage::Trash | QMailMessage::Outbox;
        break;

    case QMailFolder::TrashFolder:
        setMask = QMailMessage::Trash;
        break;

    case QMailFolder::SentFolder:
        setMask = QMailMessage::Sent;
        unsetMask = QMailMessage::Trash;
        break;

    case QMailFolder::JunkFolder:
        setMask = QMailMessage::Junk;
        unsetMask = QMailMessage::Trash;
        break;

    default:
        break;
    }

    if (setMask) {
        key &= QMailMessageKey(QMailMessageKey::status(setMask, QMailDataComparator::Includes));
    }
    if (unsetMask) {
        key &= QMailMessageKey(QMailMessageKey::status(unsetMask, QMailDataComparator::Excludes));
    }

    if (key.isEmpty()) {
        return QMailMessageKey::nonMatchingKey();
    }

    return key;
}


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
    QMailMessageKey key(QMailFolderMessageSet::contentKey(id, descendants) & QMailMessageKey::messageType(QMailMessage::Email));

    quint64 exclusions = 0;

    QMailFolder folder(id);
    if ((folder.status() & QMailFolder::Trash) == 0) {
        exclusions |= QMailMessage::Trash;
    }
    if ((folder.status() & QMailFolder::Junk) == 0) {
        exclusions |= QMailMessage::Junk;
    }

    if (exclusions) {
        key &= QMailMessageKey::status(exclusions, QMailDataComparator::Excludes);
    }

    return key;
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
    // Only return incoming messages from this account, and not Trash messages
    return (QMailAccountMessageSet::contentKey(id, false) &
            QMailMessageKey::status(QMailMessage::Trash, QMailDataComparator::Excludes) &
            QMailMessageKey::status(QMailMessage::Junk, QMailDataComparator::Excludes) &
            QMailMessageKey::status(QMailMessage::Outgoing, QMailDataComparator::Excludes));
}

void EmailAccountMessageSet::createChild(const QMailFolderId &childId)
{
    // Our child folders should also be email-only
    EmailFolderMessageSet *child = new EmailFolderMessageSet(this, childId, hierarchical());
    append(child);
}


/* InboxMessageSet */

InboxMessageSet::InboxMessageSet(QMailMessageSetContainer *container)
    : EmailStandardFolderMessageSet(container, QMailFolder::InboxFolder, tr("Inbox"))
{
}

QMailMessageKey InboxMessageSet::messageKey() const
{
    return contentKey();
}

QMailMessageKey InboxMessageSet::contentKey()
{
    // Return all incoming messages for any email account, unless in the Trash/Junk folder
    return (QMailMessageKey::parentAccountId(emailAccountKey()) &
            QMailMessageKey::status(QMailMessage::Trash | QMailMessage::Junk | QMailMessage::Outgoing, QMailDataComparator::Excludes));
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

    EmailStandardFolderMessageSet::init();
}

void InboxMessageSet::resyncState()
{
    synchronizeAccountChildren();

    EmailStandardFolderMessageSet::resyncState();
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
}

EmailFolderModel::~EmailFolderModel()
{
}

QVariant EmailFolderModel::data(QMailMessageSet *item, int role, int column) const
{
    if (item) {
        if (role == FolderSynchronizationEnabledRole) {
            return itemSynchronizationEnabled(item);
        } else if(role == FolderChildCreationPermittedRole || role == FolderDeletionPermittedRole
                  || role == FolderRenamePermittedRole) {
            return itemPermitted(item, static_cast<Roles>(role));
        }
        else if (role == ContextualAccountIdRole) {
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

    // Add the remainder of the standard folders as status filters
    append(new EmailStandardFolderMessageSet(this, QMailFolder::OutboxFolder, tr("Outbox")));
    append(new EmailStandardFolderMessageSet(this, QMailFolder::DraftsFolder, tr("Drafts")));
    append(new EmailStandardFolderMessageSet(this, QMailFolder::SentFolder, tr("Sent")));
    append(new EmailStandardFolderMessageSet(this, QMailFolder::JunkFolder, tr("Junk")));
    append(new EmailStandardFolderMessageSet(this, QMailFolder::TrashFolder, tr("Trash")));
}

QIcon EmailFolderModel::itemIcon(QMailMessageSet *item) const
{
    if (EmailStandardFolderMessageSet *standardItem = qobject_cast<EmailStandardFolderMessageSet*>(item)) {
        return standardFolderIcon(standardItem);
    } else if (EmailFolderMessageSet *emailItem = qobject_cast<EmailFolderMessageSet*>(item)) {
        return emailFolderIcon(emailItem);
    }

    return FolderModel::itemIcon(item);
}

QString EmailFolderModel::itemStatusDetail(QMailMessageSet *item) const
{
    // Don't report any state for excluded folders
    if (!itemSynchronizationEnabled(item))
        return QString();

    return FolderModel::itemStatusDetail(item);
}

FolderModel::StatusText EmailFolderModel::itemStatusText(QMailMessageSet *item) const
{
    if (EmailStandardFolderMessageSet *standardItem = qobject_cast<EmailStandardFolderMessageSet*>(item)) {
        return standardFolderStatusText(standardItem);
    }

    return FolderModel::itemStatusText(item);
}

static QMap<QMailFolder::StandardFolder, QIcon> iconMapInit()
{
    QMap<QMailFolder::StandardFolder, QIcon> map;

    map[QMailFolder::InboxFolder] = Qtmail::icon(QLatin1String("inboxfolder"));
    map[QMailFolder::OutboxFolder] = Qtmail::icon(QLatin1String("outboxfolder"));
    map[QMailFolder::DraftsFolder] = Qtmail::icon(QLatin1String("draftfolder"));
    map[QMailFolder::SentFolder] = Qtmail::icon(QLatin1String("sentfolder"));
    map[QMailFolder::JunkFolder] = Qtmail::icon(QLatin1String("junkfolder"));
    map[QMailFolder::TrashFolder] = Qtmail::icon(QLatin1String("trashfolder"));

    return map;
}

static QIcon folderIcon(QMailFolder::StandardFolder type)
{
    const QMap<QMailFolder::StandardFolder, QIcon> iconMap(iconMapInit());

    QMap<QMailFolder::StandardFolder, QIcon>::const_iterator it = iconMap.find(type);
    if (it != iconMap.end())
        return it.value();

    return Qtmail::icon(QLatin1String("folder"));
}

QIcon EmailFolderModel::standardFolderIcon(EmailStandardFolderMessageSet *item) const
{
    return folderIcon(item->standardFolderType());
}

QIcon EmailFolderModel::emailFolderIcon(EmailFolderMessageSet *item) const
{
    QMailFolder folder(item->folderId());
    if (folder.status() & QMailFolder::Trash) {
        return folderIcon(QMailFolder::TrashFolder);
    } else if (folder.status() & QMailFolder::Sent) {
        return folderIcon(QMailFolder::SentFolder);
    } else if (folder.status() & QMailFolder::Drafts) {
        return folderIcon(QMailFolder::DraftsFolder);
    } else if (folder.status() & QMailFolder::Junk) {
        return folderIcon(QMailFolder::JunkFolder);
    }

    return Qtmail::icon(QLatin1String("folder"));
}

FolderModel::StatusText EmailFolderModel::standardFolderStatusText(EmailStandardFolderMessageSet *item) const
{
    QMailFolder::StandardFolder standardType(item->standardFolderType());
    if ((standardType != QMailFolder::TrashFolder) && 
        (standardType != QMailFolder::DraftsFolder) &&
        (standardType != QMailFolder::OutboxFolder)) {
        // No special handling
        return filterStatusText(static_cast<QMailFilterMessageSet*>(item));
    }

    QString status, detail;

    if (QMailStore* store = QMailStore::instance()) {
        // Find the total and unread total for this folder
        QMailMessageKey itemKey = item->messageKey();
        int total = store->countMessages(itemKey);

        // Find the subtotal for this folder
        int subTotal = 0;
        SubTotalType type = Unread;

        if (standardType == QMailFolder::TrashFolder) {
            // For Trash, report the 'new' count, or else the 'unread' count
            subTotal = store->countMessages(itemKey & QMailMessageKey::status(QMailMessage::New));
            if (subTotal) {
                type = New;
            } else {
                subTotal = store->countMessages(itemKey & unreadKey());
            }
        } else if ((standardType == QMailFolder::DraftsFolder) || (standardType == QMailFolder::OutboxFolder)) {
            // For Drafts and Outbox, suppress the 'unread' count
            subTotal = 0;
        }

        detail = describeFolderCount(total, subTotal, type);
        status = formatCounts(total, subTotal, false, false);
    }

    return qMakePair(status, detail);
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

bool EmailFolderModel::itemPermitted(QMailMessageSet *item, Roles role) const
{
    if (QMailFolderMessageSet *folderItem = qobject_cast<QMailFolderMessageSet*>(item)) {
        // Only relevant for account folders
        QMailFolder folder(folderItem->folderId());
        if (folder.parentAccountId().isValid()) {
            quint64 folderStatus = folder.status();
            switch(role) {
            case FolderChildCreationPermittedRole:
                return (folderStatus & QMailFolder::ChildCreationPermitted);
            case FolderDeletionPermittedRole:
                return (folderStatus & QMailFolder::DeletionPermitted);
            case FolderRenamePermittedRole:
                return (folderStatus & QMailFolder::RenamePermitted);
            default:
                qWarning() << "itemPermitted has been called on an unknown role: " << role;
            }
        }
    }

    return false;
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

/* AccountFolderModel */

AccountFolderModel::AccountFolderModel(const QMailAccountId &id, QObject *parent)
    : EmailFolderModel(parent),
      accountId(id)
{
}

void AccountFolderModel::init()
{
    // Show only the folders for our account
    append(new EmailAccountMessageSet(this, accountId));
}


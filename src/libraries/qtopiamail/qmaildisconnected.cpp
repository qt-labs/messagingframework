/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qmaildisconnected.h"
#include "qmailstore.h"
#include "qmaillog.h"

QMailMessageKey QMailDisconnected::sourceKey(const QMailFolderId &folderId)
{
    QMailMessageKey result(QMailMessageKey::parentFolderId(folderId));
    result &= QMailMessageKey::previousParentFolderId(QMailFolderId());
    result |= QMailMessageKey::previousParentFolderId(folderId);
    return result;
}

QMailFolderId QMailDisconnected::sourceFolderId(const QMailMessageMetaData &metaData)
{
    QMailFolderId previousParentFolderId(metaData.previousParentFolderId());
    if (previousParentFolderId.isValid())
        return previousParentFolderId;
    
    return metaData.parentFolderId();
}

QMailMessageKey::Properties QMailDisconnected::parentFolderProperties()
{
    return QMailMessageKey::ParentFolderId | QMailMessageKey::PreviousParentFolderId;
}

void QMailDisconnected::clearPreviousFolder(QMailMessageMetaData *message)
{
    message->setPreviousParentFolderId(QMailFolderId());
}

void QMailDisconnected::copyPreviousFolder(const QMailMessageMetaData &source, QMailMessageMetaData *dest)
{
    dest->setPreviousParentFolderId(source.previousParentFolderId());
}

QMap<QMailFolderId, QMailMessageIdList> QMailDisconnected::restoreMap(const QMailMessageIdList &messageIds)
{
    QMap<QMailFolderId, QMailMessageIdList> result;
    QMailMessageKey key(QMailMessageKey::id(messageIds));
    QMailMessageKey::Properties props(QMailMessageKey::Id | QMailMessageKey::PreviousParentFolderId);
    
    foreach (const QMailMessageMetaData &metaData, QMailStore::instance()->messagesMetaData(key, props)) {
        if (metaData.previousParentFolderId().isValid()) {
            result[metaData.previousParentFolderId()].append(metaData.id());
        }
    }
    return result;
}

static QMap<QMailFolder::StandardFolder,quint64> flagMap()
{
    static QMap<QMailFolder::StandardFolder,quint64> sFlagMap;
    if(sFlagMap.isEmpty())
    {
        sFlagMap.insert(QMailFolder::DraftsFolder,QMailMessage::Draft);
        sFlagMap.insert(QMailFolder::TrashFolder,QMailMessage::Trash);
        sFlagMap.insert(QMailFolder::SentFolder,QMailMessage::Sent);
        sFlagMap.insert(QMailFolder::JunkFolder,QMailMessage::Junk);
    }
    return sFlagMap;
}

static void syncStatusWithFolder(QMailMessageMetaData& message, QMailFolder::StandardFolder standardFolder)
{
    quint64 clearFolderStatus = message.status() &~ (QMailMessage::Draft | QMailMessage::Sent | QMailMessage::Trash | QMailMessage::Junk);
    message.setStatus(clearFolderStatus);
    message.setStatus(flagMap().value(standardFolder),true);
}

static void syncStatusWithFolder(QMailMessageMetaData& message)
{
    if (!message.parentAccountId().isValid())
        return;

    QMailAccount messageAccount(message.parentAccountId());

    foreach(QMailFolder::StandardFolder sf, flagMap().keys()) {
        if (message.parentFolderId().isValid() 
            && messageAccount.standardFolder(sf) == message.parentFolderId()) {
            syncStatusWithFolder(message, sf);
            return;
        }
    }
}

bool QMailDisconnected::updatesOutstanding(const QMailAccountId &mailAccountId)
{
    QMailFolderKey accountFoldersKey(QMailFolderKey::parentAccountId(mailAccountId));
    QMailMessageKey copiedKey(QMailMessageKey::parentAccountId(mailAccountId) 
                              & QMailMessageKey::status(QMailMessage::LocalOnly));
    QMailMessageKey movedKey(QMailMessageKey::previousParentFolderId(accountFoldersKey));
    QMailMessageIdList copiedIds = QMailStore::instance()->queryMessages(copiedKey);
    QMailMessageIdList movedIds = QMailStore::instance()->queryMessages(movedKey);

    if(copiedIds.isEmpty() && movedIds.isEmpty())
        return false;
    return true;
}

void QMailDisconnected::rollBackUpdates(const QMailAccountId &mailAccountId)
{
    QMailFolderKey accountFoldersKey(QMailFolderKey::parentAccountId(mailAccountId));
    QMailMessageKey copiedKey(QMailMessageKey::parentAccountId(mailAccountId) 
                              & QMailMessageKey::status(QMailMessage::LocalOnly));
    QMailMessageKey movedKey(QMailMessageKey::previousParentFolderId(accountFoldersKey));
    QMailMessageIdList copiedIds = QMailStore::instance()->queryMessages(copiedKey);
    QMailMessageIdList movedIds = QMailStore::instance()->queryMessages(movedKey);

    if(copiedIds.isEmpty() && movedIds.isEmpty())
        return;

    //remove copies
    if(!copiedIds.isEmpty())
        QMailStore::instance()->removeMessages(QMailMessageKey::id(copiedIds));

    //undo moves
    foreach(const QMailMessageId& id, movedIds) {
        QMailMessageMetaData mail(id);
        mail.setParentFolderId(mail.previousParentFolderId());
        mail.setPreviousParentFolderId(QMailFolderId());
        syncStatusWithFolder(mail);           
            QMailStore::instance()->updateMessage(&mail);
    }
}

// move messages to their standard account folders setting flags as necessary

void QMailDisconnected::moveToStandardFolder(const QMailMessageIdList& ids, QMailFolder::StandardFolder standardFolder)
{
    QMailAccountIdList allAccounts = QMailStore::instance()->queryAccounts();

    foreach(const QMailAccountId& id, allAccounts) {
        QMailAccount account(id);
        QMailFolderId standardFolderId = account.standardFolder(standardFolder);
        if (standardFolderId.isValid())
            moveToFolder(ids,standardFolderId);
    }
}

void QMailDisconnected::moveToFolder(const QMailMessageIdList& ids, const QMailFolderId& folderId)
{
    if (!folderId.isValid())
        return;

    QMailFolder folder(folderId);

    if(!folder.parentAccountId().isValid())
        return;

    QMailMessageKey key(QMailMessageKey::id(ids) & QMailMessageKey::parentAccountId(folder.parentAccountId()));
    QMailMessageIdList messageIds = QMailStore::instance()->queryMessages(key);
    foreach(const QMailMessageId& messageId, messageIds) {
        QMailMessageMetaData msg(messageId);
        if (msg.parentFolderId() == folderId)
            continue;
        if (!(msg.status() & QMailMessage::LocalOnly) && !msg.serverUid().isEmpty())
            msg.setPreviousParentFolderId(msg.parentFolderId());
        msg.setParentFolderId(folderId);
        syncStatusWithFolder(msg);
        QMailStore::instance()->updateMessage(&msg);
    }
}

void QMailDisconnected::copyToStandardFolder(const QMailMessageIdList& ids, QMailFolder::StandardFolder standardFolder)
{
    QMailAccountIdList allAccounts = QMailStore::instance()->queryAccounts();

    foreach(const QMailAccountId& id, allAccounts) {
        QMailAccount account(id);
        QMailFolderId standardFolderId = account.standardFolder(standardFolder);
        if (standardFolderId.isValid())
            copyToFolder(ids,standardFolderId);
   }
}

void QMailDisconnected::copyToFolder(const QMailMessageIdList& ids, const QMailFolderId& folderId)
{
    if (!folderId.isValid())
        return;

    QMailFolder folder(folderId);

    QMailMessageKey key(QMailMessageKey::id(ids) & QMailMessageKey::parentAccountId(folder.parentAccountId()));
    QMailMessageIdList messageIds = QMailStore::instance()->queryMessages(key);
    foreach (const QMailMessageId& messageId, messageIds) {
        QMailMessage mail(messageId);
        QMailMessage copy(QMailMessage::fromRfc2822(mail.toRfc2822()));
        copy.setMessageType(QMailMessage::Email);
        copy.setPreviousParentFolderId(QMailFolderId());
        copy.setParentFolderId(folderId);
        copy.setParentAccountId(mail.parentAccountId());
        copy.setSize(mail.size());
        copy.setStatus(mail.status());
        copy.setStatus(QMailMessage::LocalOnly,true);
        copy.setStatus(QMailMessage::Removed,false);
        syncStatusWithFolder(copy);
        QMailStore::instance()->addMessage(&copy);
    }
}

//flag messages functions are used to perform local operations. i.e marking messages and "move to trash"

void QMailDisconnected::flagMessages(const QMailMessageIdList &ids, quint64 setMask, quint64 unsetMask, const QString& description)
{
    Q_UNUSED(description)
    if (setMask && !QMailStore::instance()->updateMessagesMetaData(QMailMessageKey::id(ids), setMask, true)) {
        qMailLog(Messaging) << "Unable to flag messages:" << ids;
    }

    if (unsetMask && !QMailStore::instance()->updateMessagesMetaData(QMailMessageKey::id(ids), unsetMask, false)) {
        qMailLog(Messaging) << "Unable to flag messages:" << ids;
    }
}

void QMailDisconnected::flagMessage(const QMailMessageId &id, quint64 setMask, quint64 unsetMask, const QString& description)
{
    flagMessages(QMailMessageIdList() << id, setMask, unsetMask, description);
}


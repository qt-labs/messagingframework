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

#ifndef QMAILMESSAGESERVER_H
#define QMAILMESSAGESERVER_H

#include "qmailglobal.h"
#include <QList>
#include "qmailmessage.h"
#include "qmailmessagekey.h"
#include "qmailmessagesortkey.h"
#include "qmailserviceaction.h"
#include "qmailstore.h"
#include <QMap>
#include <QSharedDataPointer>
#include <QString>
#include <QStringList>
#include "qmailipc.h"

class QMailMessageServerPrivate;

typedef QMap<QMailMessage::MessageType, int> QMailMessageCountMap;

class QMF_EXPORT QMailMessageServer : public QObject
{
    Q_OBJECT

public:
    QMailMessageServer(QObject* parent = 0);
    ~QMailMessageServer();

private:
    // Disallow copying
    QMailMessageServer(const QMailMessageServer&);
    void operator=(const QMailMessageServer&);

signals:
    void newCountChanged(const QMailMessageCountMap&);

    void actionStarted(QMailActionData);
    void activityChanged(quint64, QMailServiceAction::Activity);
    void connectivityChanged(quint64, QMailServiceAction::Connectivity);
    void statusChanged(quint64, const QMailServiceAction::Status);
    void progressChanged(quint64, uint, uint);

    void retrievalCompleted(quint64);

    void messagesTransmitted(quint64, const QMailMessageIdList&);
    void messagesFailedTransmission(quint64, const QMailMessageIdList&, QMailServiceAction::Status::ErrorCode);
    void transmissionCompleted(quint64);

    void messagesDeleted(quint64, const QMailMessageIdList&);
    void messagesCopied(quint64, const QMailMessageIdList&);
    void messagesMoved(quint64, const QMailMessageIdList&);
    void messagesFlagged(quint64, const QMailMessageIdList&);
    void messagesAdded(quint64, const QMailMessageIdList&);
    void messagesUpdated(quint64, const QMailMessageIdList&);

    void folderCreated(quint64, const QMailFolderId&);
    void folderRenamed(quint64, const QMailFolderId&);
    void folderDeleted(quint64, const QMailFolderId&);

    void storageActionCompleted(quint64);

    void matchingMessageIds(quint64, const QMailMessageIdList&);
    void searchCompleted(quint64);

    void actionsListed(const QMailActionDataList &);

    void protocolResponse(quint64, const QString &response, const QVariant &data);
    void protocolRequestCompleted(quint64);

    void connectionDown();
    void reconnectionTimeout();

public slots:
    void acknowledgeNewMessages(const QMailMessageTypeList& types);

    void transmitMessages(quint64, const QMailAccountId &accountId);

    void retrieveFolderList(quint64, const QMailAccountId &accountId, const QMailFolderId &folderId, bool descending);
    void retrieveMessageList(quint64, const QMailAccountId &accountId, const QMailFolderId &folderId, uint minimum, const QMailMessageSortKey &sort);

    void retrieveMessages(quint64, const QMailMessageIdList &messageIds, QMailRetrievalAction::RetrievalSpecification spec);
    void retrieveMessagePart(quint64, const QMailMessagePart::Location &partLocation);

    void retrieveMessageRange(quint64, const QMailMessageId &messageId, uint minimum);
    void retrieveMessagePartRange(quint64, const QMailMessagePart::Location &partLocation, uint minimum);

    void retrieveAll(quint64, const QMailAccountId &accountId);
    void exportUpdates(quint64, const QMailAccountId &accountId);

    void synchronize(quint64, const QMailAccountId &accountId);

    void copyMessages(quint64, const QMailMessageIdList& mailList, const QMailFolderId &destinationId);
    void moveMessages(quint64, const QMailMessageIdList& mailList, const QMailFolderId &destinationId);
    void flagMessages(quint64, const QMailMessageIdList& mailList, quint64 setMask, quint64 unsetMask);
    void addMessages(quint64, const QString &filename);
    void addMessages(quint64, const QMailMessageMetaDataList &messages);
    void updateMessages(quint64, const QString &filename);
    void updateMessages(quint64, const QMailMessageMetaDataList &messages);

    void createFolder(quint64, const QString &name, const QMailAccountId &accountId, const QMailFolderId &parentId);
    void renameFolder(quint64, const QMailFolderId &folderId, const QString &name);
    void deleteFolder(quint64, const QMailFolderId &folderId);

    void cancelTransfer(quint64);

    void deleteMessages(quint64, const QMailMessageIdList& mailList, QMailStore::MessageRemovalOption);

    void searchMessages(quint64, const QMailMessageKey& filter, const QString& bodyText, QMailSearchAction::SearchSpecification spec, const QMailMessageSortKey &sort);
    void cancelSearch(quint64);

    void shutdown();

    void listActions();

    void protocolRequest(quint64, const QMailAccountId &accountId, const QString &request, const QVariant &data);

private:
    QMailMessageServerPrivate* d;
};

Q_DECLARE_METATYPE(QMailMessageCountMap)
Q_DECLARE_USER_METATYPE_TYPEDEF(QMailMessageCountMap, QMailMessageCountMap)

#endif

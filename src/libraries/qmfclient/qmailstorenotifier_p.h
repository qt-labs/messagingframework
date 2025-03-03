/****************************************************************************
**
** Copyright (C) 2025 Caliste Damien.
** Contact: Damien Caliste <dcaliste@free.fr>
**
** Copyright (C) 2025 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
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

#ifndef QMAILSTORENOTIFIER_P_H
#define QMAILSTORENOTIFIER_P_H

#include "qmailstore.h"

#include <QDBusContext>
#include <QObject>
#include <QTimer>

QT_BEGIN_NAMESPACE

class MailstoreAdaptor;

QT_END_NAMESPACE

class QMF_EXPORT QMailStoreNotifier : public QObject, protected QDBusContext
{
    Q_OBJECT

public:
    QMailStoreNotifier(QObject* parent = nullptr);
    ~QMailStoreNotifier();

    bool asynchronousEmission() const;

    void notifyAccountsChange(QMailStore::ChangeType changeType,
                              const QMailAccountIdList& ids);
    void notifyMessagesChange(QMailStore::ChangeType changeType,
                              const QMailMessageIdList& ids);
    void notifyMessagesDataChange(QMailStore::ChangeType changeType,
                                  const QMailMessageMetaDataList& data);
    void notifyMessagesDataChange(const QMailMessageIdList& ids,
                                  const QMailMessageKey::Properties& properties,
                                  const QMailMessageMetaData& data);
    void notifyMessagesDataChange(const QMailMessageIdList& ids,
                                  quint64 status, bool set);
    void notifyFoldersChange(QMailStore::ChangeType changeType,
                             const QMailFolderIdList& ids);
    void notifyThreadsChange(QMailStore::ChangeType changeType,
                             const QMailThreadIdList &ids);
    void notifyMessageRemovalRecordsChange(QMailStore::ChangeType changeType,
                                           const QMailAccountIdList& ids);
    void notifyRetrievalInProgress(const QMailAccountIdList& ids);
    void notifyTransmissionInProgress(const QMailAccountIdList& ids);

    bool setRetrievalInProgress(const QMailAccountIdList &ids);
    bool setTransmissionInProgress(const QMailAccountIdList &ids);

    bool isIpcConnectionEstablished() const;

    virtual void disconnectIpc();
    virtual void reconnectIpc();

public slots:
    void flushIpcNotifications();

protected slots:
    virtual void accountsRemotelyChanged(QMailStore::ChangeType changeType,
                                         const QMailAccountIdList& ids) = 0;
    virtual void messageRemovalRecordsRemotelyChanged(QMailStore::ChangeType changeType,
                                                      const QMailAccountIdList& ids) = 0;
    virtual void remoteTransmissionInProgress(const QMailAccountIdList& ids) = 0;
    virtual void remoteRetrievalInProgress(const QMailAccountIdList& ids) = 0;
    virtual void messagesRemotelyChanged(QMailStore::ChangeType changeType,
                                         const QMailMessageIdList& ids) = 0;
    virtual void messageMetaDataRemotelyChanged(QMailStore::ChangeType changeType,
                                                const QMailMessageMetaDataList& data) = 0;
    virtual void messagePropertiesRemotelyChanged(const QMailMessageIdList& ids,
                                                  QMailMessageKey::Properties properties,
                                                  const QMailMessageMetaData& data) = 0;
    virtual void messageStatusRemotelyChanged(const QMailMessageIdList& ids,
                                              quint64 status, bool set) = 0;
    virtual void threadsRemotelyChanged(QMailStore::ChangeType changeType,
                                        const QMailThreadIdList& ids) = 0;
    virtual void foldersRemotelyChanged(QMailStore::ChangeType changeType,
                                        const QMailFolderIdList& ids) = 0;

private:
    QTimer preFlushTimer;
    QTimer flushTimer;

    QSet<QMailAccountId> addAccountsBuffer;
    QSet<QMailFolderId> addFoldersBuffer;
    QSet<QMailThreadId> addThreadsBuffer;
    QSet<QMailMessageId> addMessagesBuffer;
    QSet<QMailAccountId> addMessageRemovalRecordsBuffer;

    QMailMessageMetaDataList addMessagesDataBuffer;
    QMailMessageMetaDataList updateMessagesDataBuffer;

    typedef QPair<QPair<QMailMessageKey::Properties, QMailMessageMetaData>, QSet<QMailMessageId> > MessagesProperties;
    typedef QList<MessagesProperties> MessagesPropertiesBuffer;
    MessagesPropertiesBuffer messagesPropertiesBuffer;

    typedef QPair<quint64, bool> MessagesStatus;
    typedef QMap<MessagesStatus, QSet<QMailMessageId> > MessagesStatusBuffer;
    MessagesStatusBuffer messagesStatusBuffer;

    QSet<QMailMessageId> updateMessagesBuffer;
    QSet<QMailFolderId> updateFoldersBuffer;
    QSet<QMailThreadId> updateThreadsBuffer;
    QSet<QMailAccountId> updateAccountsBuffer;

    QSet<QMailAccountId> removeMessageRemovalRecordsBuffer;
    QSet<QMailMessageId> removeMessagesBuffer;
    QSet<QMailFolderId> removeFoldersBuffer;
    QSet<QMailThreadId> removeThreadsBuffer;
    QSet<QMailAccountId> removeAccountsBuffer;

    QSet<QMailFolderId> folderContentsModifiedBuffer;
    QSet<QMailThreadId> threadContentsModifiedBuffer;
    QSet<QMailAccountId> accountContentsModifiedBuffer;
    QSet<QMailMessageId> messageContentsModifiedBuffer;

    bool retrievalSetInitialized;
    bool transmissionSetInitialized;

    QSet<QMailAccountId> retrievalInProgressIds;
    QSet<QMailAccountId> transmissionInProgressIds;

    MailstoreAdaptor *ipcAdaptor;
};

#endif

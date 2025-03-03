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

#include "qmailstorenotifier_p.h"
#include "qmailstore_adaptor.h"
#include "qmaillog.h"
#include <qmailipc.h>

#include <QCoreApplication>

namespace {

void dispatchAccountNotifications(MailstoreAdaptor &adaptor, QSet<QMailAccountId> &ids, QMailStore::ChangeType changeType)
{
    if (!ids.isEmpty()) {
        emit adaptor.accountsChanged(changeType, ids.values());
        ids.clear();
    }
}

void dispatchMessageRemovalRecordNotifications(MailstoreAdaptor &adaptor, QSet<QMailAccountId> &ids, QMailStore::ChangeType changeType)
{
    if (!ids.isEmpty()) {
        emit adaptor.messageRemovalRecordsChanged(changeType, ids.values());
        ids.clear();
    }
}

void dispatchMessageNotifications(MailstoreAdaptor &adaptor, QSet<QMailMessageId> &ids, QMailStore::ChangeType changeType)
{
    if (!ids.isEmpty()) {
        emit adaptor.messagesChanged(changeType, ids.values());
        ids.clear();
    }
}

void dispatchMessageMetaDataNotifications(MailstoreAdaptor &adaptor, QMailMessageMetaDataList& data, QMailStore::ChangeType changeType)
{
    if (!data.isEmpty()) {
        emit adaptor.messageMetaDataChanged(changeType, data);
        data.clear();
    }
}

typedef QPair<QPair<QMailMessageKey::Properties, QMailMessageMetaData>, QSet<QMailMessageId> > MessagesProperties;
typedef QList <MessagesProperties> MessagesPropertiesBuffer;

void dispatchMessagePropertyNotifications(MailstoreAdaptor &adaptor, MessagesPropertiesBuffer& data)
{
    if (!data.isEmpty()) {
        foreach (const MessagesProperties& props, data) {
            emit adaptor.messagePropertiesChanged(props.second.values(), props.first.first, props.first.second);
        }
        data.clear();
    }
}

typedef QPair<quint64, bool> MessagesStatus;
typedef QMap<MessagesStatus, QSet<QMailMessageId> > MessagesStatusBuffer;

void dispatchMessageStatusNotifications(MailstoreAdaptor &adaptor, MessagesStatusBuffer& data)
{
    if (!data.isEmpty()) {
        foreach (const MessagesStatus& status, data.keys()) {
            const QSet<QMailMessageId> ids = data[status];
            emit adaptor.messageStatusChanged(ids.values(), status.first, status.second);
        }
        data.clear();
    }
}

void dispatchThreadNotifications(MailstoreAdaptor &adaptor, QSet<QMailThreadId> &ids, QMailStore::ChangeType changeType)
{
    if (!ids.isEmpty()) {
        emit adaptor.threadsChanged(changeType, ids.values());
        ids.clear();
    }
}

void dispatchFolderNotifications(MailstoreAdaptor &adaptor, QSet<QMailFolderId> &ids, QMailStore::ChangeType changeType)
{
    if (!ids.isEmpty()) {
        emit adaptor.foldersChanged(changeType, ids.values());
        ids.clear();
    }
}

}

QMailStoreNotifier::QMailStoreNotifier(QObject* parent)
    : QObject(parent)
    , retrievalSetInitialized(false)
    , transmissionSetInitialized(false)
    , ipcAdaptor(new MailstoreAdaptor(this))
{
    if (!isIpcConnectionEstablished()) {
        qCritical() << "Failed to connect D-Bus, notifications to/from other clients will not work.";
    }

    if (!QDBusConnection::sessionBus().registerObject(QString::fromLatin1("/mailstore/client"), this)) {
        qCritical() << "Failed to register to D-Bus, notifications to other clients will not work.";
    }

    static bool registrationDone = false;
    if (!registrationDone) {
        qRegisterMetaType<QMailStore::ChangeType>("QMailStore::ChangeType");
        qDBusRegisterMetaType<QMailStore::ChangeType>();
        qRegisterMetaType<QMailAccountId>("QMailAccountId");
        qDBusRegisterMetaType<QMailAccountId>();
        qRegisterMetaType<QMailAccountIdList>("QMailAccountIdList");
        qDBusRegisterMetaType<QMailAccountIdList>();
        qRegisterMetaType<QMailFolderId>("QMailFolderId");
        qDBusRegisterMetaType<QMailFolderId>();
        qRegisterMetaType<QMailFolderIdList>("QMailFolderIdList");
        qDBusRegisterMetaType<QMailFolderIdList>();
        qRegisterMetaType<QMailThreadId>("QMailThreadId");
        qDBusRegisterMetaType<QMailThreadId>();
        qRegisterMetaType<QMailThreadIdList>("QMailThreadIdList");
        qDBusRegisterMetaType<QMailThreadIdList>();
        qRegisterMetaType<QMailMessageId>("QMailMessageId");
        qDBusRegisterMetaType<QMailMessageId>();
        qRegisterMetaType<QMailMessageIdList>("QMailMessageIdList");
        qDBusRegisterMetaType<QMailMessageIdList>();
        qRegisterMetaType<QMailMessageMetaDataList>("QMailMessageMetaDataList");
        qDBusRegisterMetaType<QMailMessageMetaDataList>();
        qRegisterMetaType<QMailMessageKey::Properties>("QMailMessageKey::Properties");
        qDBusRegisterMetaType<QMailMessageKey::Properties>();
        qRegisterMetaType<QMailMessageMetaData>("QMailMessageMetaData");
        qDBusRegisterMetaType<QMailMessageMetaData>();
        registrationDone = true;
    }

    reconnectIpc();

    // Events occurring within this period after a previous event begin batching
    const int preFlushTimeout = 250;
    preFlushTimer.setSingleShot(true);
    preFlushTimer.setInterval(preFlushTimeout);

    // Events occurring within this period are batched
    const int flushTimeout = 1000;
    flushTimer.setSingleShot(true);
    flushTimer.setInterval(flushTimeout);
    connect(&flushTimer, &QTimer::timeout,
            this, &QMailStoreNotifier::flushIpcNotifications);

    // Ensure that any pending updates are flushed
    connect(qApp, &QCoreApplication::aboutToQuit,
            this, &QMailStoreNotifier::flushIpcNotifications);
}

QMailStoreNotifier::~QMailStoreNotifier()
{
    QDBusConnection::sessionBus().unregisterObject(QString::fromLatin1("/mailstore/client"));
}

bool QMailStoreNotifier::asynchronousEmission() const
{
    return calledFromDBus()
        && message().service() == QDBusConnection::sessionBus().baseService();
}

void QMailStoreNotifier::notifyAccountsChange(QMailStore::ChangeType changeType, const QMailAccountIdList& ids)
{
    // Use the preFlushTimer to activate buffering when multiple changes occur proximately
    if (preFlushTimer.isActive() || flushTimer.isActive()) {
        if (!flushTimer.isActive()) {
            // Wait for a period to batch up incoming changes
            flushTimer.start();
        }

        QSet<QMailAccountId> idsSet = QSet<QMailAccountId>(ids.constBegin(), ids.constEnd());
        switch (changeType)
        {
        case QMailStore::Added:
            addAccountsBuffer += idsSet;
            break;
        case QMailStore::Removed:
            removeAccountsBuffer += idsSet;
            break;
        case QMailStore::Updated:
            updateAccountsBuffer += idsSet;
            break;
        case QMailStore::ContentsModified:
            accountContentsModifiedBuffer += idsSet;
            break;
        default:
            qMailLog(Messaging) << "Unhandled account notification received";
            break;
        }
    } else {
        emit ipcAdaptor->accountsChanged(changeType, ids);

        preFlushTimer.start();
    }
}

void QMailStoreNotifier::notifyMessagesChange(QMailStore::ChangeType changeType, const QMailMessageIdList& ids)
{
    // Use the preFlushTimer to activate buffering when multiple changes occur proximately
    if (preFlushTimer.isActive() || flushTimer.isActive()) {
        if (!flushTimer.isActive()) {
            // Wait for a period to batch up incoming changes
            flushTimer.start();
        }

        QSet<QMailMessageId> idsSet = QSet<QMailMessageId>(ids.constBegin(), ids.constEnd());
        switch (changeType)
        {
        case QMailStore::Added:
            addMessagesBuffer += idsSet;
            break;
        case QMailStore::Removed:
            removeMessagesBuffer += idsSet;
            break;
        case QMailStore::Updated:
            updateMessagesBuffer += idsSet;
            break;
        case QMailStore::ContentsModified:
            messageContentsModifiedBuffer += idsSet;
            break;
        default:
            qMailLog(Messaging) << "Unhandled message notification received";
            break;
        }
    } else {
        emit ipcAdaptor->messagesChanged(changeType, ids);

        preFlushTimer.start();
    }
}

void QMailStoreNotifier::notifyMessagesDataChange(QMailStore::ChangeType changeType, const QMailMessageMetaDataList& data)
{
    for (const QMailMessageMetaData &meta : data) {
        // Avoid loading custom fields from the D-Bus thread
        // when serialising the data.
        meta.customField(QString());
    }
    // Use the preFlushTimer to activate buffering when multiple changes occur proximately
    if (preFlushTimer.isActive() || flushTimer.isActive()) {
        if (!flushTimer.isActive()) {
            // Wait for a period to batch up incoming changes
            flushTimer.start();
        }

        switch (changeType)
        {
        case QMailStore::Added:
            addMessagesDataBuffer.append(data);
            break;
        case QMailStore::Updated:
            updateMessagesDataBuffer.append(data);
            break;
        default:
            qMailLog(Messaging) << "Unhandled folder notification received";
            break;
        }

    } else {
        emit ipcAdaptor->messageMetaDataChanged(changeType, data);

        preFlushTimer.start();
    }
}

void QMailStoreNotifier::notifyMessagesDataChange(const QMailMessageIdList& ids,
                                                  const QMailMessageKey::Properties& properties,
                                                  const QMailMessageMetaData& data)
{
    // Avoid loading custom fields from the D-Bus thread
    // when serialising the data.
    data.customField(QString());
    // Use the preFlushTimer to activate buffering when multiple changes occur proximately
    if (preFlushTimer.isActive() || flushTimer.isActive()) {
        if (!flushTimer.isActive()) {
            // Wait for a period to batch up incoming changes
            flushTimer.start();
        }

        MessagesProperties props(QPair<QMailMessageKey::Properties, QMailMessageMetaData>(properties, data), QSet<QMailMessageId>(ids.constBegin(), ids.constEnd()));
        messagesPropertiesBuffer.append(props);

    } else {
        emit ipcAdaptor->messagePropertiesChanged(ids, properties, data);

        preFlushTimer.start();
    }
}

void QMailStoreNotifier::notifyMessagesDataChange(const QMailMessageIdList& ids, quint64 status, bool set)
{
    // Use the preFlushTimer to activate buffering when multiple changes occur proximately
    if (preFlushTimer.isActive() || flushTimer.isActive()) {
        if (!flushTimer.isActive()) {
            // Wait for a period to batch up incoming changes
            flushTimer.start();
        }

        MessagesStatus messageStatus(status, set);
        messagesStatusBuffer[messageStatus] += QSet<QMailMessageId>(ids.constBegin(), ids.constEnd());

    } else {
        emit ipcAdaptor->messageStatusChanged(ids, status, set);

        preFlushTimer.start();
    }
}

void QMailStoreNotifier::notifyThreadsChange(QMailStore::ChangeType changeType, const QMailThreadIdList& ids)
{
    // Use the preFlushTimer to activate buffering when multiple changes occur proximately
    if (preFlushTimer.isActive() || flushTimer.isActive()) {
        if (!flushTimer.isActive()) {
            // Wait for a period to batch up incoming changes
            flushTimer.start();
        }

        QSet<QMailThreadId> idsSet = QSet<QMailThreadId>(ids.constBegin(), ids.constEnd());
        switch (changeType)
        {
        case QMailStore::Added:
            addThreadsBuffer += idsSet;
            break;
        case QMailStore::Removed:
            removeThreadsBuffer += idsSet;
            break;
        case QMailStore::Updated:
            updateThreadsBuffer += idsSet;
            break;
        case QMailStore::ContentsModified:
            threadContentsModifiedBuffer += idsSet;
            break;
        default:
            qMailLog(Messaging) << "Unhandled folder notification received";
            break;
        }
    } else {
        emit ipcAdaptor->threadsChanged(changeType, ids);

        preFlushTimer.start();
    }
}


void QMailStoreNotifier::notifyFoldersChange(QMailStore::ChangeType changeType, const QMailFolderIdList& ids)
{
    // Use the preFlushTimer to activate buffering when multiple changes occur proximately
    if (preFlushTimer.isActive() || flushTimer.isActive()) {
        if (!flushTimer.isActive()) {
            // Wait for a period to batch up incoming changes
            flushTimer.start();
        }

        QSet<QMailFolderId> idsSet = QSet<QMailFolderId>(ids.constBegin(), ids.constEnd());
        switch (changeType)
        {
        case QMailStore::Added:
            addFoldersBuffer += idsSet;
            break;
        case QMailStore::Removed:
            removeFoldersBuffer += idsSet;
            break;
        case QMailStore::Updated:
            updateFoldersBuffer += idsSet;
            break;
        case QMailStore::ContentsModified:
            folderContentsModifiedBuffer += idsSet;
            break;
        default:
            qMailLog(Messaging) << "Unhandled folder notification received";
            break;
        }
    } else {
        emit ipcAdaptor->foldersChanged(changeType, ids);

        preFlushTimer.start();
    }
}

void QMailStoreNotifier::notifyMessageRemovalRecordsChange(QMailStore::ChangeType changeType, const QMailAccountIdList& ids)
{
    // Use the preFlushTimer to activate buffering when multiple changes occur proximately
    if (preFlushTimer.isActive() || flushTimer.isActive()) {
        if (!flushTimer.isActive()) {
            // Wait for a period to batch up incoming changes
            flushTimer.start();
        }

        QSet<QMailAccountId> idsSet = QSet<QMailAccountId>(ids.constBegin(), ids.constEnd());
        switch (changeType)
        {
        case QMailStore::Added:
            addMessageRemovalRecordsBuffer += idsSet;
            break;
        case QMailStore::Removed:
            removeMessageRemovalRecordsBuffer += idsSet;
            break;
        default:
            qMailLog(Messaging) << "Unhandled message removal record notification received";
            break;
        }
    } else {
        emit ipcAdaptor->messageRemovalRecordsChanged(changeType, ids);

        preFlushTimer.start();
    }
}

void QMailStoreNotifier::notifyRetrievalInProgress(const QMailAccountIdList& ids)
{
    // Clients may want to enable or disable event handling based on this event, therefore
    // we must ensure that all previous events are actually delivered before this one is.
    flushIpcNotifications();

    emit ipcAdaptor->retrievalInProgress(ids);
}

void QMailStoreNotifier::notifyTransmissionInProgress(const QMailAccountIdList& ids)
{
    flushIpcNotifications();

    emit ipcAdaptor->transmissionInProgress(ids);
}

bool QMailStoreNotifier::setRetrievalInProgress(const QMailAccountIdList& ids)
{
    QSet<QMailAccountId> idSet(ids.constBegin(), ids.constEnd());
    if ((idSet != retrievalInProgressIds) || !retrievalSetInitialized) {
        retrievalInProgressIds = idSet;
        retrievalSetInitialized = true;
        return true;
    }

    return false;
}

bool QMailStoreNotifier::setTransmissionInProgress(const QMailAccountIdList& ids)
{
    QSet<QMailAccountId> idSet(ids.constBegin(), ids.constEnd());
    if ((idSet != transmissionInProgressIds) || !transmissionSetInitialized) {
        transmissionInProgressIds = idSet;
        transmissionSetInitialized = true;
        return true;
    }

    return false;
}

bool QMailStoreNotifier::isIpcConnectionEstablished() const
{
    return QDBusConnection::sessionBus().isConnected();
}

static void dBusDisconnect(const char *method, QMailStoreNotifier *obj, const char *slot)
{
    QDBusConnection::sessionBus().disconnect(QString(), QString(), QString::fromLatin1("org.qt.mailstore"), QString::fromLatin1(method), obj, slot);
}

void QMailStoreNotifier::disconnectIpc()
{
    dBusDisconnect("accountsChanged", this,
                   SLOT(accountsRemotelyChanged(QMailStore::ChangeType, const QMailAccountIdList&)));
    dBusDisconnect("messageRemovalRecordsChanged", this,
                   SLOT(messageRemovalRecordsRemotelyChanged(QMailStore::ChangeType, const QMailAccountIdList&)));
    dBusDisconnect("retrievalInProgress", this,
                   SLOT(remoteRetrievalInProgress(const QMailAccountIdList&)));
    dBusDisconnect("transmissionInProgress", this,
                   SLOT(remoteTransmissionInProgress(const QMailAccountIdList&)));
    dBusDisconnect("messagesChanged", this,
                   SLOT(messagesRemotelyChanged(QMailStore::ChangeType, const QMailMessageIdList&)));
    dBusDisconnect("messageMetaDataChanged", this,
                   SLOT(messageMetaDataRemotelyChanged(QMailStore::ChangeType, const QMailMessageMetaDataList&)));
    dBusDisconnect("messagePropertiesChanged", this,
                   SLOT(messagePropertiesRemotelyChanged(const QMailMessageIdList&, QMailMessageKey::Properties, const QMailMessageMetaData&)));
    dBusDisconnect("messageStatusChanged", this,
                   SLOT(messageStatusRemotelyChanged(const QMailMessageIdList&, quint64, bool)));
    dBusDisconnect("threadsChanged", this,
                   SLOT(threadsRemotelyChanged(QMailStore::ChangeType, const QMailThreadIdList&)));
    dBusDisconnect("foldersChanged", this,
                   SLOT(foldersRemotelyChanged(QMailStore::ChangeType, const QMailFolderIdList&)));
}

static void dBusConnect(const char *method, QMailStoreNotifier *obj, const char *slot)
{
    QDBusConnection::sessionBus().connect(QString(), QString(), QString::fromLatin1("org.qt.mailstore"), QString::fromLatin1(method), obj, slot);
}

void QMailStoreNotifier::reconnectIpc()
{
    dBusConnect("accountsChanged", this,
                SLOT(accountsRemotelyChanged(QMailStore::ChangeType, const QMailAccountIdList&)));
    dBusConnect("messageRemovalRecordsChanged", this,
                SLOT(messageRemovalRecordsRemotelyChanged(QMailStore::ChangeType, const QMailAccountIdList&)));
    dBusConnect("retrievalInProgress", this,
                SLOT(remoteRetrievalInProgress(const QMailAccountIdList&)));
    dBusConnect("transmissionInProgress", this,
                SLOT(remoteTransmissionInProgress(const QMailAccountIdList&)));
    dBusConnect("messagesChanged", this,
                SLOT(messagesRemotelyChanged(QMailStore::ChangeType, const QMailMessageIdList&)));
    dBusConnect("messageMetaDataChanged", this,
                SLOT(messageMetaDataRemotelyChanged(QMailStore::ChangeType, const QMailMessageMetaDataList&)));
    dBusConnect("messagePropertiesChanged", this,
                SLOT(messagePropertiesRemotelyChanged(const QMailMessageIdList&, QMailMessageKey::Properties, const QMailMessageMetaData&)));
    dBusConnect("messageStatusChanged", this,
                SLOT(messageStatusRemotelyChanged(const QMailMessageIdList&, quint64, bool)));
    dBusConnect("threadsChanged", this,
                SLOT(threadsRemotelyChanged(QMailStore::ChangeType, const QMailThreadIdList&)));
    dBusConnect("foldersChanged", this,
                SLOT(foldersRemotelyChanged(QMailStore::ChangeType, const QMailFolderIdList&)));
}

void QMailStoreNotifier::flushIpcNotifications()
{
    // There is no need to emit content modification notifications for items subsequently deleted
    folderContentsModifiedBuffer -= removeFoldersBuffer;
    accountContentsModifiedBuffer -= removeAccountsBuffer;

    // The order of emission is significant:
    dispatchAccountNotifications(*ipcAdaptor, addAccountsBuffer, QMailStore::Added);
    dispatchFolderNotifications(*ipcAdaptor, addFoldersBuffer, QMailStore::Added);
    dispatchMessageNotifications(*ipcAdaptor, addMessagesBuffer, QMailStore::Added);
    dispatchThreadNotifications(*ipcAdaptor, addThreadsBuffer, QMailStore::Added);
    dispatchMessageRemovalRecordNotifications(*ipcAdaptor, addMessageRemovalRecordsBuffer, QMailStore::Added);

    dispatchMessageNotifications(*ipcAdaptor, messageContentsModifiedBuffer, QMailStore::ContentsModified);
    dispatchMessageNotifications(*ipcAdaptor, updateMessagesBuffer, QMailStore::Updated);
    dispatchThreadNotifications(*ipcAdaptor, updateThreadsBuffer, QMailStore::Updated);
    dispatchFolderNotifications(*ipcAdaptor, updateFoldersBuffer, QMailStore::Updated);
    dispatchAccountNotifications(*ipcAdaptor, updateAccountsBuffer, QMailStore::Updated);

    dispatchMessageRemovalRecordNotifications(*ipcAdaptor, removeMessageRemovalRecordsBuffer, QMailStore::Removed);
    dispatchMessageNotifications(*ipcAdaptor, removeMessagesBuffer, QMailStore::Removed);
    dispatchThreadNotifications(*ipcAdaptor, removeThreadsBuffer, QMailStore::Removed);
    dispatchFolderNotifications(*ipcAdaptor, removeFoldersBuffer, QMailStore::Removed);
    dispatchAccountNotifications(*ipcAdaptor, removeAccountsBuffer, QMailStore::Removed);

    dispatchFolderNotifications(*ipcAdaptor, folderContentsModifiedBuffer, QMailStore::ContentsModified);
    dispatchAccountNotifications(*ipcAdaptor, accountContentsModifiedBuffer, QMailStore::ContentsModified);

    dispatchMessageMetaDataNotifications(*ipcAdaptor, addMessagesDataBuffer, QMailStore::Added);
    dispatchMessageMetaDataNotifications(*ipcAdaptor, updateMessagesDataBuffer, QMailStore::Updated);

    dispatchMessagePropertyNotifications(*ipcAdaptor, messagesPropertiesBuffer);
    dispatchMessageStatusNotifications(*ipcAdaptor, messagesStatusBuffer);
}

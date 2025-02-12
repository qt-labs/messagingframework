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

// Events occurring within this period after a previous event begin batching
const int preFlushTimeout = 250;

// Events occurring within this period are batched
const int flushTimeout = 1000;

typedef QPair<int,int> Segment; //start,end - end non inclusive
typedef QList<Segment> SegmentList;

// Process lists in size-limited batches
SegmentList createSegments(int numItems, int segmentSize)
{
    Q_ASSERT(segmentSize > 0);

    if(numItems <= 0)
        return SegmentList();

    int segmentCount = numItems % segmentSize ? 1 : 0;
    segmentCount += numItems / segmentSize;

    SegmentList segments;
    for(int i = 0; i < segmentCount ; ++i) {
        int start = segmentSize * i;
        int end = (i+1) == segmentCount ? numItems : (i+1) * segmentSize;
        segments.append(Segment(start,end)); 
    }

    return segments;
}

template<typename IDListType>
void emitIpcUpdates(MailstoreAdaptor &adaptor, const IDListType& ids, const QString& sig, int max = QMailStoreNotifier::maxNotifySegmentSize)
{
    if (max > 0) {
        SegmentList segmentList = createSegments(ids.count(), max);
        foreach (const Segment& segment, segmentList) {
            IDListType idSegment = ids.mid(segment.first, (segment.second - segment.first));

            QByteArray payload;
            QDataStream stream(&payload, QIODevice::WriteOnly);
            stream << idSegment;
            emit adaptor.updated(sig, payload);
        }
    } else {
        QByteArray payload;
        QDataStream stream(&payload, QIODevice::WriteOnly);
        stream << ids;
        emit adaptor.updated(sig, payload);
    }
}

void emitIpcUpdates(MailstoreAdaptor &adaptor, const QMailMessageMetaDataList& data, const QString& sig)
{
    QByteArray payload;
    QDataStream stream(&payload, QIODevice::WriteOnly);
    stream << data;
    emit adaptor.updated(sig, payload);
}

void emitIpcUpdates(MailstoreAdaptor &adaptor, const QMailMessageIdList& ids,  const QMailMessageKey::Properties& properties,
                    const QMailMessageMetaData& data, const QString& sig)
{
    QByteArray payload;
    QDataStream stream(&payload, QIODevice::WriteOnly);
    stream << ids << static_cast<qint32>(properties) << data;
    emit adaptor.updated(sig, payload);
}

void emitIpcUpdates(MailstoreAdaptor &adaptor, const QMailMessageIdList& ids,  quint64 status, bool set, const QString& sig)
{
    QByteArray payload;
    QDataStream stream(&payload, QIODevice::WriteOnly);
    stream << ids << status << set;
    emit adaptor.updated(sig, payload);
}

template<typename IDType>
void dispatchNotifications(MailstoreAdaptor &adaptor, QSet<IDType> &ids, const QString &sig)
{
    if (!ids.isEmpty()) {
        emitIpcUpdates(adaptor, ids.values(), sig);
        ids.clear();
    }
} 

void dispatchNotifications(MailstoreAdaptor &adaptor, QMailMessageMetaDataList& data, const QString &sig)
{
    if (!data.isEmpty()) {
        emitIpcUpdates(adaptor, data, sig);
        data.clear();
    }
}

typedef QPair<QPair<QMailMessageKey::Properties, QMailMessageMetaData>, QSet<QMailMessageId> > MessagesProperties;
typedef QList <MessagesProperties> MessagesPropertiesBuffer;

void dispatchNotifications(MailstoreAdaptor &adaptor, MessagesPropertiesBuffer& data, const QString &sig)
{
    if (!data.isEmpty()) {
        foreach (const MessagesProperties& props, data) {
            emitIpcUpdates(adaptor, props.second.values(), props.first.first, props.first.second, sig);
        }
        data.clear();
    }
}

typedef QPair<quint64, bool> MessagesStatus;
typedef QMap<MessagesStatus, QSet<QMailMessageId> > MessagesStatusBuffer;

void dispatchNotifications(MailstoreAdaptor &adaptor, MessagesStatusBuffer& data, const QString &sig)
{
    if (!data.isEmpty()) {
        foreach (const MessagesStatus& status, data.keys()) {
            const QSet<QMailMessageId> ids = data[status];
            emitIpcUpdates(adaptor,ids.values(), status.first, status.second, sig);
        }
        data.clear();
    }
}

} 

QMailStoreNotifier::QMailStoreNotifier(QMailStore* parent)
    : QObject(parent)
    , asyncEmission(false)
    , retrievalSetInitialized(false)
    , transmissionSetInitialized(false)
    , q(parent)
    , ipcAdaptor(new MailstoreAdaptor(this))
{
    Q_ASSERT(q);

    if (!isIpcConnectionEstablished()) {
        qCritical() << "Failed to connect D-Bus, notifications to/from other clients will not work.";
    }

    if (!QDBusConnection::sessionBus().registerObject(QString::fromLatin1("/mailstore/client"), this)) {
        qCritical() << "Failed to register to D-Bus, notifications to other clients will not work.";
    }

    reconnectIpc();

    preFlushTimer.setSingleShot(true);

    flushTimer.setSingleShot(true);
    connect(&flushTimer,
            SIGNAL(timeout()),
            this,
            SLOT(flushIpcNotifications()));

    connect(qApp, &QCoreApplication::aboutToQuit, this, &QMailStoreNotifier::aboutToQuit);
}

QMailStoreNotifier::~QMailStoreNotifier()
{
    QDBusConnection::sessionBus().unregisterObject(QString::fromLatin1("/mailstore/client"));
}

bool QMailStoreNotifier::asynchronousEmission() const
{
    return asyncEmission;
}

void QMailStoreNotifier::aboutToQuit()
{
    // Ensure that any pending updates are flushed
    flushIpcNotifications();
}

typedef QMap<QMailStore::ChangeType, QString> NotifyFunctionMap;

static QString accountAddedSig = QStringLiteral("accountAdded(uint,QList<quint64>)");
static QString accountRemovedSig = QStringLiteral("accountRemoved(uint,QList<quint64>)");
static QString accountUpdatedSig = QStringLiteral("accountUpdated(uint,QList<quint64>)");
static QString accountContentsModifiedSig = QStringLiteral("accountContentsModified(uint,QList<quint64>)");

static QString messageAddedSig = QStringLiteral("messageAdded(uint,QList<quint64>)");
static QString messageRemovedSig = QStringLiteral("messageRemoved(uint,QList<quint64>)");
static QString messageUpdatedSig = QStringLiteral("messageUpdated(uint,QList<quint64>)");
static QString messageContentsModifiedSig = QStringLiteral("messageContentsModified(uint,QList<quint64>)");

static QString messageMetaDataAddedSig = QStringLiteral("messageDataAdded(QMailMessageMetaDataList)");
static QString messageMetaDataUpdatedSig = QStringLiteral("messageDataUpdated(QMailMessageMetaDataList)");
static QString messagePropertyUpdatedSig = QStringLiteral("messagePropertyUpdated(QList<quint64>,QFlags,QMailMessageMetaData)");
static QString messageStatusUpdatedSig = QStringLiteral("messageStatusUpdated(QList<quint64>,quint64,bool)");

static QString folderAddedSig = QStringLiteral("folderAdded(uint,QList<quint64>)");
static QString folderUpdatedSig = QStringLiteral("folderUpdated(uint,QList<quint64>)");
static QString folderRemovedSig = QStringLiteral("folderRemoved(uint,QList<quint64>)");
static QString folderContentsModifiedSig = QStringLiteral("folderContentsModified(uint,QList<quint64>)");

static QString threadAddedSig = QStringLiteral("threadAdded(uint,QList<quint64>)");
static QString threadUpdatedSig = QStringLiteral("threadUpdated(uint,QList<quint64>)");
static QString threadRemovedSig = QStringLiteral("threadRemoved(uint,QList<quint64>)");
static QString threadContentsModifiedSig = QStringLiteral("threadContentsModified(uint,QList<quint64>)");

static QString messageRemovalRecordsAddedSig = QStringLiteral("messageRemovalRecordsAdded(uint,QList<quint64>)");
static QString messageRemovalRecordsRemovedSig = QStringLiteral("messageRemovalRecordsRemoved(uint,QList<quint64>)");

static QString retrievalInProgressSig = QStringLiteral("retrievalInProgress(QList<quint64>)");
static QString transmissionInProgressSig = QStringLiteral("transmissionInProgress(QList<quint64>)");

static NotifyFunctionMap initAccountFunctions()
{
    NotifyFunctionMap sig;
    sig[QMailStore::Added] = accountAddedSig;
    sig[QMailStore::Updated] = accountUpdatedSig;
    sig[QMailStore::Removed] = accountRemovedSig;
    sig[QMailStore::ContentsModified] = accountContentsModifiedSig;
    return sig;
}

static NotifyFunctionMap initMessageFunctions()
{
    NotifyFunctionMap sig;
    sig[QMailStore::Added] = messageAddedSig;
    sig[QMailStore::Updated] = messageUpdatedSig;
    sig[QMailStore::Removed] = messageRemovedSig;
    sig[QMailStore::ContentsModified] = messageContentsModifiedSig;
    return sig;
}

static NotifyFunctionMap initFolderFunctions()
{
    NotifyFunctionMap sig;
    sig[QMailStore::Added] = folderAddedSig;
    sig[QMailStore::Updated] = folderUpdatedSig;
    sig[QMailStore::Removed] = folderRemovedSig;
    sig[QMailStore::ContentsModified] = folderContentsModifiedSig;
    return sig;
}

static NotifyFunctionMap initThreadFunctions()
{
    NotifyFunctionMap sig;
    sig[QMailStore::Added] = threadAddedSig;
    sig[QMailStore::Updated] = threadUpdatedSig;
    sig[QMailStore::Removed] = threadRemovedSig;
    sig[QMailStore::ContentsModified] = threadContentsModifiedSig;
    return sig;
}

static NotifyFunctionMap initMessageRemovalRecordFunctions()
{
    NotifyFunctionMap sig;
    sig[QMailStore::Added] = messageRemovalRecordsAddedSig;
    sig[QMailStore::Removed] = messageRemovalRecordsRemovedSig;
    return sig;
}

static NotifyFunctionMap initMessageDataFunctions()
{
    NotifyFunctionMap sig;
    sig[QMailStore::Added] = messageMetaDataAddedSig;
    sig[QMailStore::Updated] = messageMetaDataUpdatedSig;
    return sig;
}

void QMailStoreNotifier::notifyAccountsChange(QMailStore::ChangeType changeType, const QMailAccountIdList& ids)
{
    static NotifyFunctionMap sig(initAccountFunctions());

    // Use the preFlushTimer to activate buffering when multiple changes occur proximately
    if (preFlushTimer.isActive() || flushTimer.isActive()) {
        if (!flushTimer.isActive()) {
            // Wait for a period to batch up incoming changes
            flushTimer.start(flushTimeout);
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
        emitIpcUpdates(*ipcAdaptor, ids, sig[changeType]);
        
        preFlushTimer.start(preFlushTimeout);
    }
}

void QMailStoreNotifier::notifyMessagesChange(QMailStore::ChangeType changeType, const QMailMessageIdList& ids)
{
    static NotifyFunctionMap sig(initMessageFunctions());

    // Use the preFlushTimer to activate buffering when multiple changes occur proximately
    if (preFlushTimer.isActive() || flushTimer.isActive()) {
        if (!flushTimer.isActive()) {
            // Wait for a period to batch up incoming changes
            flushTimer.start(flushTimeout);
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
        emitIpcUpdates(*ipcAdaptor, ids, sig[changeType]);

        preFlushTimer.start(preFlushTimeout);
    }
}

void QMailStoreNotifier::notifyMessagesDataChange(QMailStore::ChangeType changeType, const QMailMessageMetaDataList& data)
{
    static NotifyFunctionMap sig(initMessageDataFunctions());
    // Use the preFlushTimer to activate buffering when multiple changes occur proximately
    if (preFlushTimer.isActive() || flushTimer.isActive()) {
        if (!flushTimer.isActive()) {
            // Wait for a period to batch up incoming changes
            flushTimer.start(flushTimeout);
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
        emitIpcUpdates(*ipcAdaptor, data, sig[changeType]);

        preFlushTimer.start(preFlushTimeout);
    }
}

void QMailStoreNotifier::notifyMessagesDataChange(const QMailMessageIdList& ids,
                                                  const QMailMessageKey::Properties& properties,
                                                  const QMailMessageMetaData& data)
{
    // Use the preFlushTimer to activate buffering when multiple changes occur proximately
    if (preFlushTimer.isActive() || flushTimer.isActive()) {
        if (!flushTimer.isActive()) {
            // Wait for a period to batch up incoming changes
            flushTimer.start(flushTimeout);
        }

        MessagesProperties props(QPair<QMailMessageKey::Properties, QMailMessageMetaData>(properties, data), QSet<QMailMessageId>(ids.constBegin(), ids.constEnd()));
        messagesPropertiesBuffer.append(props);

    } else {
        emitIpcUpdates(*ipcAdaptor, ids, properties, data, messagePropertyUpdatedSig);

        preFlushTimer.start(preFlushTimeout);
    }
}

void QMailStoreNotifier::notifyMessagesDataChange(const QMailMessageIdList& ids, quint64 status, bool set)
{
    // Use the preFlushTimer to activate buffering when multiple changes occur proximately
    if (preFlushTimer.isActive() || flushTimer.isActive()) {
        if (!flushTimer.isActive()) {
            // Wait for a period to batch up incoming changes
            flushTimer.start(flushTimeout);
        }

        MessagesStatus messageStatus(status, set);
        messagesStatusBuffer[messageStatus] += QSet<QMailMessageId>(ids.constBegin(), ids.constEnd());

    } else {
        emitIpcUpdates(*ipcAdaptor, ids, status, set, messageStatusUpdatedSig);

        preFlushTimer.start(preFlushTimeout);
    }
}

void QMailStoreNotifier::notifyThreadsChange(QMailStore::ChangeType changeType, const QMailThreadIdList& ids)
{
    static NotifyFunctionMap sig(initThreadFunctions());

    // Use the preFlushTimer to activate buffering when multiple changes occur proximately
    if (preFlushTimer.isActive() || flushTimer.isActive()) {
        if (!flushTimer.isActive()) {
            // Wait for a period to batch up incoming changes
            flushTimer.start(flushTimeout);
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
        emitIpcUpdates(*ipcAdaptor, ids, sig[changeType]);

        preFlushTimer.start(preFlushTimeout);
    }
}


void QMailStoreNotifier::notifyFoldersChange(QMailStore::ChangeType changeType, const QMailFolderIdList& ids)
{
    static NotifyFunctionMap sig(initFolderFunctions());

    // Use the preFlushTimer to activate buffering when multiple changes occur proximately
    if (preFlushTimer.isActive() || flushTimer.isActive()) {
        if (!flushTimer.isActive()) {
            // Wait for a period to batch up incoming changes
            flushTimer.start(flushTimeout);
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
        emitIpcUpdates(*ipcAdaptor, ids, sig[changeType]);

        preFlushTimer.start(preFlushTimeout);
    }
}

void QMailStoreNotifier::notifyMessageRemovalRecordsChange(QMailStore::ChangeType changeType, const QMailAccountIdList& ids)
{
    static NotifyFunctionMap sig(initMessageRemovalRecordFunctions());

    // Use the preFlushTimer to activate buffering when multiple changes occur proximately
    if (preFlushTimer.isActive() || flushTimer.isActive()) {
        if (!flushTimer.isActive()) {
            // Wait for a period to batch up incoming changes
            flushTimer.start(flushTimeout);
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
        emitIpcUpdates(*ipcAdaptor, ids, sig[changeType]);

        preFlushTimer.start(preFlushTimeout);
    }
}

void QMailStoreNotifier::notifyRetrievalInProgress(const QMailAccountIdList& ids)
{
    // Clients may want to enable or disable event handling based on this event, therefore
    // we must ensure that all previous events are actually delivered before this one is.
    flushIpcNotifications();

    emitIpcUpdates(*ipcAdaptor, ids, retrievalInProgressSig);
}

void QMailStoreNotifier::notifyTransmissionInProgress(const QMailAccountIdList& ids)
{
    flushIpcNotifications();

    emitIpcUpdates(*ipcAdaptor, ids, transmissionInProgressSig);
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

void QMailStoreNotifier::disconnectIpc()
{
    QDBusConnection::sessionBus().disconnect(QString(), QString(), QString::fromLatin1("org.qt.mailstore"),
                                             QString::fromLatin1("updated"), this,
                                             SLOT(ipcMessage(const QString&, const QByteArray&)));
}

void QMailStoreNotifier::reconnectIpc()
{
    QDBusConnection::sessionBus().connect(QString(), QString(), QString::fromLatin1("org.qt.mailstore"),
                                          QString::fromLatin1("updated"), this,
                                          SLOT(ipcMessage(const QString&, const QByteArray&)));
}

QMailStoreNotifier::AccountUpdateSignalMap QMailStoreNotifier::initAccountUpdateSignals()
{
    AccountUpdateSignalMap sig;
    sig[accountAddedSig] = &QMailStore::accountsAdded;
    sig[accountUpdatedSig] = &QMailStore::accountsUpdated;
    sig[accountRemovedSig] = &QMailStore::accountsRemoved;
    sig[accountContentsModifiedSig] = &QMailStore::accountContentsModified;
    sig[messageRemovalRecordsAddedSig] = &QMailStore::messageRemovalRecordsAdded;
    sig[messageRemovalRecordsRemovedSig] = &QMailStore::messageRemovalRecordsRemoved;
    sig[retrievalInProgressSig] = &QMailStore::retrievalInProgress;
    sig[transmissionInProgressSig] = &QMailStore::transmissionInProgress;
    return sig;
}

QMailStoreNotifier::FolderUpdateSignalMap QMailStoreNotifier::initFolderUpdateSignals()
{
    FolderUpdateSignalMap sig;
    sig[folderAddedSig] = &QMailStore::foldersAdded;
    sig[folderUpdatedSig] = &QMailStore::foldersUpdated;
    sig[folderRemovedSig] = &QMailStore::foldersRemoved;
    sig[folderContentsModifiedSig] = &QMailStore::folderContentsModified;
    return sig;
}

QMailStoreNotifier::ThreadUpdateSignalMap QMailStoreNotifier::initThreadUpdateSignals()
{
    ThreadUpdateSignalMap sig;
    sig[threadAddedSig] = &QMailStore::threadsAdded;
    sig[threadUpdatedSig] = &QMailStore::threadsUpdated;
    sig[threadRemovedSig] = &QMailStore::threadsRemoved;
    sig[threadContentsModifiedSig] = &QMailStore::threadContentsModified;
    return sig;
}

QMailStoreNotifier::MessageUpdateSignalMap QMailStoreNotifier::initMessageUpdateSignals()
{
    MessageUpdateSignalMap sig;
    sig[messageAddedSig] = &QMailStore::messagesAdded;
    sig[messageUpdatedSig] = &QMailStore::messagesUpdated;
    sig[messageRemovedSig] = &QMailStore::messagesRemoved;
    sig[messageContentsModifiedSig] = &QMailStore::messageContentsModified;
    return sig;
}

QMailStoreNotifier::MessageDataPreCacheSignalMap QMailStoreNotifier::initMessageDataPreCacheSignals()
{
    MessageDataPreCacheSignalMap sig;
    sig[messageMetaDataAddedSig] = &QMailStore::messageDataAdded;
    sig[messageMetaDataUpdatedSig] = &QMailStore::messageDataUpdated;
    return sig;
}

void QMailStoreNotifier::flushIpcNotifications()
{
    static NotifyFunctionMap sigAccount(initAccountFunctions());
    static NotifyFunctionMap sigFolder(initFolderFunctions());
    static NotifyFunctionMap sigMessage(initMessageFunctions());
    static NotifyFunctionMap sigthread(initThreadFunctions());
    static NotifyFunctionMap sigRemoval(initMessageRemovalRecordFunctions());
    static NotifyFunctionMap sigMessageData(initMessageDataFunctions());

    // There is no need to emit content modification notifications for items subsequently deleted
    folderContentsModifiedBuffer -= removeFoldersBuffer;
    accountContentsModifiedBuffer -= removeAccountsBuffer;

    // The order of emission is significant:
    dispatchNotifications(*ipcAdaptor, addAccountsBuffer, sigAccount[QMailStore::Added]);
    dispatchNotifications(*ipcAdaptor, addFoldersBuffer, sigFolder[QMailStore::Added]);
    dispatchNotifications(*ipcAdaptor, addMessagesBuffer, sigMessage[QMailStore::Added]);
    dispatchNotifications(*ipcAdaptor, addThreadsBuffer, sigthread[QMailStore::Added]);
    dispatchNotifications(*ipcAdaptor, addMessageRemovalRecordsBuffer, sigRemoval[QMailStore::Added]);

    dispatchNotifications(*ipcAdaptor, messageContentsModifiedBuffer, sigMessage[QMailStore::ContentsModified]);
    dispatchNotifications(*ipcAdaptor, updateMessagesBuffer, sigMessage[QMailStore::Updated]);
    dispatchNotifications(*ipcAdaptor, updateThreadsBuffer, sigthread[QMailStore::Updated]);
    dispatchNotifications(*ipcAdaptor, updateFoldersBuffer, sigFolder[QMailStore::Updated]);
    dispatchNotifications(*ipcAdaptor, updateAccountsBuffer, sigAccount[QMailStore::Updated]);

    dispatchNotifications(*ipcAdaptor, removeMessageRemovalRecordsBuffer, sigRemoval[QMailStore::Removed]);
    dispatchNotifications(*ipcAdaptor, removeMessagesBuffer, sigMessage[QMailStore::Removed]);
    dispatchNotifications(*ipcAdaptor, removeThreadsBuffer, sigthread[QMailStore::Removed]);
    dispatchNotifications(*ipcAdaptor, removeFoldersBuffer, sigFolder[QMailStore::Removed]);
    dispatchNotifications(*ipcAdaptor, removeAccountsBuffer, sigAccount[QMailStore::Removed]);

    dispatchNotifications(*ipcAdaptor, folderContentsModifiedBuffer, sigFolder[QMailStore::ContentsModified]);
    dispatchNotifications(*ipcAdaptor, accountContentsModifiedBuffer, sigAccount[QMailStore::ContentsModified]);

    dispatchNotifications(*ipcAdaptor, addMessagesDataBuffer, sigMessageData[QMailStore::Added]);
    dispatchNotifications(*ipcAdaptor, updateMessagesDataBuffer, sigMessageData[QMailStore::Updated]);

    dispatchNotifications(*ipcAdaptor, messagesPropertiesBuffer, messagePropertyUpdatedSig);
    dispatchNotifications(*ipcAdaptor, messagesStatusBuffer, messageStatusUpdatedSig);
}

void QMailStoreNotifier::ipcMessage(const QString& signal, const QByteArray& data)
{
    if (!calledFromDBus()
        || message().service() == QDBusConnection::sessionBus().baseService()) {
        // don't notify ourselves
        return;
    }

    QDataStream ds(data);

    static AccountUpdateSignalMap accountUpdateSignals(initAccountUpdateSignals());
    static FolderUpdateSignalMap folderUpdateSignals(initFolderUpdateSignals());
    static ThreadUpdateSignalMap threadUpdateSignals(initThreadUpdateSignals());
    static MessageUpdateSignalMap messageUpdateSignals(initMessageUpdateSignals());
    static MessageDataPreCacheSignalMap messageDataPreCacheSignals(initMessageDataPreCacheSignals());

    AccountUpdateSignalMap::const_iterator ait;
    FolderUpdateSignalMap::const_iterator fit;
    ThreadUpdateSignalMap::const_iterator tit;
    MessageUpdateSignalMap::const_iterator mit;
    MessageDataPreCacheSignalMap::const_iterator mdit;

    if ((ait = accountUpdateSignals.find(signal)) != accountUpdateSignals.end()) {
        QMailAccountIdList ids;
        ds >> ids;

        emitIpcNotification(ait.value(), ids);
    } else if ((fit = folderUpdateSignals.find(signal)) != folderUpdateSignals.end()) {
        QMailFolderIdList ids;
        ds >> ids;

        emitIpcNotification(fit.value(), ids);
    } else if ((mit = messageUpdateSignals.find(signal)) != messageUpdateSignals.end()) {
        QMailMessageIdList ids;
        ds >> ids;

        emitIpcNotification(mit.value(), ids);
    } else if ((mdit = messageDataPreCacheSignals.find(signal)) != messageDataPreCacheSignals.end()) {
        QMailMessageMetaDataList data;
        ds >> data;

        emitIpcNotification(mdit.value(), data);
    } else if (signal == messagePropertyUpdatedSig) {
        QMailMessageIdList ids;
        ds >> ids;
        int props = 0;
        ds >> props;
        QMailMessageMetaData data;
        ds >> data;

        emitIpcNotification(ids, static_cast<QMailMessageKey::Property>(props), data);
    } else if (signal == messageStatusUpdatedSig) {
        QMailMessageIdList ids;
        ds >> ids;
        quint64 status = 0;
        ds >> status;
        bool set = false;
        ds >> set;

        emitIpcNotification(ids, status, set);
    } else if ((tit = threadUpdateSignals.find(signal)) != threadUpdateSignals.end()) {
        QMailThreadIdList ids;
        ds >> ids;

        emitIpcNotification(tit.value(), ids);
    } else {
        qWarning() << "No update signal for message:" << signal;
    }
}

void QMailStoreNotifier::emitIpcNotification(AccountUpdateSignal signal, const QMailAccountIdList &ids)
{
    asyncEmission = true;
    emit (q->*signal)(ids);
    asyncEmission = false;
}

void QMailStoreNotifier::emitIpcNotification(FolderUpdateSignal signal, const QMailFolderIdList &ids)
{
    asyncEmission = true;
    emit (q->*signal)(ids);
    asyncEmission = false;
}

void QMailStoreNotifier::emitIpcNotification(ThreadUpdateSignal signal, const QMailThreadIdList &ids)
{
    asyncEmission = true;
    emit (q->*signal)(ids);
    asyncEmission = false;
}

void QMailStoreNotifier::emitIpcNotification(MessageUpdateSignal signal, const QMailMessageIdList &ids)
{
    asyncEmission = true;
    emit (q->*signal)(ids);
    asyncEmission = false;
}

void QMailStoreNotifier::emitIpcNotification(MessageDataPreCacheSignal signal, const QMailMessageMetaDataList &data)
{
    asyncEmission = true;
    emit (q->*signal)(data);
    asyncEmission = false;
}

void QMailStoreNotifier::emitIpcNotification(const QMailMessageIdList& ids,  const QMailMessageKey::Properties& properties,
                                 const QMailMessageMetaData& data)
{
    asyncEmission = true;
    emit q->messagePropertyUpdated(ids, properties, data);
    asyncEmission = false;
}

void QMailStoreNotifier::emitIpcNotification(const QMailMessageIdList& ids, quint64 status, bool set)
{
    asyncEmission = true;
    emit q->messageStatusUpdated(ids, status, set);
    asyncEmission = false;
}

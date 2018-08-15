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

#include "qmailstoreimplementation_p.h"
#include <qmailipc.h>
#include "qmaillog.h"
#include <qcopadaptor_p.h>
#include <qcopchannel_p.h>
#include <qcopserver_p.h>
#include <qmailnamespace.h>
#include <QCoreApplication>
#include <QFileSystemWatcher>
#include <QFile>

namespace {

// Events occurring within this period after a previous event begin batching
const int preFlushTimeout = 250;

// Events occurring within this period are batched
const int flushTimeout = 1000;

const uint pid = static_cast<uint>(QCoreApplication::applicationPid() & 0xffffffff);

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
void emitIpcUpdates(const IDListType& ids, const QString& sig, int max = QMailStoreImplementationBase::maxNotifySegmentSize)
{
    if (!sig.isEmpty()) {
        if (max > 0) {
            SegmentList segmentList = createSegments(ids.count(), max);
            foreach (const Segment& segment, segmentList) {
                IDListType idSegment = ids.mid(segment.first, (segment.second - segment.first));

                QCopAdaptor a(QLatin1String("QPE/qmf"));
                QCopAdaptorEnvelope e = a.send(sig.toLatin1());
				e << pid;
                e << idSegment; 
            }
        } else {

            QCopAdaptor a(QLatin1String("QPE/qmf"));
            QCopAdaptorEnvelope e = a.send(sig.toLatin1());
            e << pid;
            e << ids;
        }
    } else {
        qWarning() << "No signature for IPC updates!";
    }
}

void emitIpcUpdates(const QMailMessageMetaDataList& data, const QString& sig)
{
    if (!sig.isEmpty()) {
            static QCopAdaptor a(QLatin1String("QPE/qmf")); // do not want to recreate heavy objects
            QCopAdaptorEnvelope e = a.send(sig.toLatin1());
            e << pid;
            e << data;
    } else {
        qWarning() << "No signature for IPC updates!";
    }
}

void emitIpcUpdates(const QMailMessageIdList& ids,  const QMailMessageKey::Properties& properties,
                    const QMailMessageMetaData& data, const QString& sig)
{
    if (!sig.isEmpty()) {
            QCopAdaptor a(QLatin1String("QPE/qmf")); // to do: not want to recreate heavy objects
            QCopAdaptorEnvelope e = a.send(sig.toLatin1());
            e << pid;
            e << ids;
            e << int(properties);
            e << data;
    } else {
        qWarning() << "No signature for IPC updates!";
    }
}

void emitIpcUpdates(const QMailMessageIdList& ids,  quint64 status, bool set, const QString& sig)
{
    if (!sig.isEmpty()) {
            QCopAdaptor a(QLatin1String("QPE/qmf")); // to do: not want to recreate heavy objects
            QCopAdaptorEnvelope e = a.send(sig.toLatin1());
            e << pid;
            e << ids;
            e << status;
            e << set;
    } else {
        qWarning() << "No signature for IPC updates!";
    }
}

template<typename IDSetType>
void dispatchNotifications(IDSetType &ids, const QString &sig)
{
    if (!ids.isEmpty()) {
        emitIpcUpdates(ids.toList(), sig);
        ids.clear();
    }
} 

void dispatchNotifications(QMailMessageMetaDataList& data, const QString &sig)
{
    if (!data.isEmpty()) {
        emitIpcUpdates(data, sig);
        data.clear();
    }
}

typedef QPair<QPair<QMailMessageKey::Properties, QMailMessageMetaData>, QSet<QMailMessageId> > MessagesProperties;
typedef QList <MessagesProperties> MessagesPropertiesBuffer;

void dispatchNotifications(MessagesPropertiesBuffer& data, const QString &sig)
{
    if (!data.isEmpty()) {
        foreach (const MessagesProperties& props, data) {
            emitIpcUpdates(props.second.toList(), props.first.first, props.first.second, sig);
        }
        data.clear();
    }
}

typedef QPair<quint64, bool> MessagesStatus;
typedef QMap<MessagesStatus, QSet<QMailMessageId> > MessagesStatusBuffer;

void dispatchNotifications(MessagesStatusBuffer& data, const QString &sig)
{
    if (!data.isEmpty()) {
        foreach (const MessagesStatus& status, data.keys()) {
            emitIpcUpdates(data[status].toList(), status.first, status.second, sig);
        }
        data.clear();
    }
}

} 


QMailStore::InitializationState QMailStoreImplementationBase::initState = QMailStore::Uninitialized;

QMailStoreImplementationBase::QMailStoreImplementationBase(QMailStore* parent)
    : QObject(parent),
      q(parent),
      errorCode(QMailStore::NoError),
      asyncEmission(false),
      retrievalSetInitialized(false),
      transmissionSetInitialized(false),
      watcher(0)
{
    Q_ASSERT(q);

    ipcChannel = new QCopChannel(QLatin1String("QPE/qmf"), this);
    ENFORCE (connect (ipcChannel, SIGNAL(connected()), q, SIGNAL(ipcConnectionEstablished())));
    ENFORCE (connect (ipcChannel, SIGNAL(connectionFailed()), this, SLOT(ipcConnectionFailed())));

    connect(ipcChannel,
            SIGNAL(received(QString,QByteArray)),
            this,
            SLOT(ipcMessage(QString,QByteArray)));

    preFlushTimer.setSingleShot(true);

    flushTimer.setSingleShot(true);
    connect(&flushTimer,
            SIGNAL(timeout()),
            this,
            SLOT(flushNotifications()));
    connect(&queueTimer,
            SIGNAL(timeout()),
            this,
            SLOT(processIpcMessageQueue()));

    connect(qApp,
            SIGNAL(aboutToQuit()),
            this,
            SLOT(aboutToQuit()));
}

QMailStoreImplementationBase::~QMailStoreImplementationBase()
{
}

void QMailStoreImplementationBase::initialize()
{
    initState = (initStore() ? QMailStore::Initialized : QMailStore::InitializationFailed);
}

QMailStore::InitializationState QMailStoreImplementationBase::initializationState()
{
    return initState;
}

QMailStore::ErrorCode QMailStoreImplementationBase::lastError() const
{
    return errorCode;
}

void QMailStoreImplementationBase::setLastError(QMailStore::ErrorCode code) const
{
    if (initState == QMailStore::InitializationFailed) {
        // Enforce the error code to be this if we can't init:
        code = QMailStore::StorageInaccessible;
    }

    if (errorCode != code) {
        errorCode = code;

        if (errorCode != QMailStore::NoError) {
            q->emitErrorNotification(errorCode);
        }
    }
}

bool QMailStoreImplementationBase::asynchronousEmission() const
{
    return asyncEmission;
}

void QMailStoreImplementationBase::flushIpcNotifications()
{
    // We need to emit all pending IPC notifications
    flushNotifications();

    // Tell the recipients to process the notifications synchronously
    QCopAdaptor a(QLatin1String("QPE/qmf"));
    QCopAdaptorEnvelope e = a.send("forceIpcFlush");
    e << pid;

    if (flushTimer.isActive()) {
        // We interrupted a batching period - reset the flush timer to its full period
        flushTimer.start(flushTimeout);
    }
}

void QMailStoreImplementationBase::processIpcMessageQueue()
{
    if (messageQueue.isEmpty()) {
        queueTimer.stop();
        return;
    }

   if (emitIpcNotification())
        queueTimer.start(0);
}

void QMailStoreImplementationBase::aboutToQuit()
{
    // Ensure that any pending updates are flushed
    flushNotifications();
}

void QMailStoreImplementationBase::ipcConnectionFailed()
{    
    if (!watcher) {
        watcher = new QFileSystemWatcher(this);
        ENFORCE (connect(watcher, SIGNAL(fileChanged(QString)), this, SLOT(lockFileUpdated())));
        const QString& path = QMail::messageServerLockFilePath();
        if (!QFile::exists(path)) {
            QFile file(path);
            file.open(QIODevice::WriteOnly);
            file.close();
        }
        watcher->addPath(path);       
    }
}

void QMailStoreImplementationBase::lockFileUpdated()
{
    if (!ipcChannel->isConnected()) {
        ipcChannel->connectRepeatedly();
        // should be invoked only once, then it will reconnect automatically
        Q_ASSERT (watcher);
        ENFORCE (watcher->disconnect(this, SLOT(lockFileUpdated())));
    }
}

typedef QMap<QMailStore::ChangeType, QString> NotifyFunctionMap;

static NotifyFunctionMap initAccountFunctions()
{
    NotifyFunctionMap sig;
    sig[QMailStore::Added] = QMailStoreImplementationBase::accountAddedSig();
    sig[QMailStore::Updated] = QMailStoreImplementationBase::accountUpdatedSig();
    sig[QMailStore::Removed] = QMailStoreImplementationBase::accountRemovedSig();
    sig[QMailStore::ContentsModified] = QMailStoreImplementationBase::accountContentsModifiedSig();
    return sig;
}

static NotifyFunctionMap initMessageFunctions()
{
    NotifyFunctionMap sig;
    sig[QMailStore::Added] = QMailStoreImplementationBase::messageAddedSig();
    sig[QMailStore::Updated] = QMailStoreImplementationBase::messageUpdatedSig();
    sig[QMailStore::Removed] = QMailStoreImplementationBase::messageRemovedSig();
    sig[QMailStore::ContentsModified] = QMailStoreImplementationBase::messageContentsModifiedSig();
    return sig;
}

static NotifyFunctionMap initFolderFunctions()
{
    NotifyFunctionMap sig;
    sig[QMailStore::Added] = QMailStoreImplementationBase::folderAddedSig();
    sig[QMailStore::Updated] = QMailStoreImplementationBase::folderUpdatedSig();
    sig[QMailStore::Removed] = QMailStoreImplementationBase::folderRemovedSig();
    sig[QMailStore::ContentsModified] = QMailStoreImplementationBase::folderContentsModifiedSig();
    return sig;
}

static NotifyFunctionMap initThreadFunctions()
{
    NotifyFunctionMap sig;
    sig[QMailStore::Added] = QMailStoreImplementationBase::threadAddedSig();
    sig[QMailStore::Updated] = QMailStoreImplementationBase::threadUpdatedSig();
    sig[QMailStore::Removed] = QMailStoreImplementationBase::threadRemovedSig();
    sig[QMailStore::ContentsModified] = QMailStoreImplementationBase::threadContentsModifiedSig();
    return sig;
}

static NotifyFunctionMap initMessageRemovalRecordFunctions()
{
    NotifyFunctionMap sig;
    sig[QMailStore::Added] = QMailStoreImplementationBase::messageRemovalRecordsAddedSig();
    sig[QMailStore::Removed] = QMailStoreImplementationBase::messageRemovalRecordsRemovedSig();
    return sig;
}

static NotifyFunctionMap initMessageDataFunctions()
{
    NotifyFunctionMap sig;
    sig[QMailStore::Added] = QMailStoreImplementationBase::messageMetaDataAddedSig();
    sig[QMailStore::Updated] = QMailStoreImplementationBase::messageMetaDataUpdatedSig();
    return sig;
}

void QMailStoreImplementationBase::notifyAccountsChange(QMailStore::ChangeType changeType, const QMailAccountIdList& ids)
{
    static NotifyFunctionMap sig(initAccountFunctions());

    // Use the preFlushTimer to activate buffering when multiple changes occur proximately
    if (preFlushTimer.isActive() || flushTimer.isActive()) {
        if (!flushTimer.isActive()) {
            // Wait for a period to batch up incoming changes
            flushTimer.start(flushTimeout);
        }

        QSet<QMailAccountId> idsSet = QSet<QMailAccountId>::fromList(ids);
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
        emitIpcUpdates(ids, sig[changeType]);
        
        preFlushTimer.start(preFlushTimeout);
    }
}

void QMailStoreImplementationBase::notifyMessagesChange(QMailStore::ChangeType changeType, const QMailMessageIdList& ids)
{
    static NotifyFunctionMap sig(initMessageFunctions());

    // Use the preFlushTimer to activate buffering when multiple changes occur proximately
    if (preFlushTimer.isActive() || flushTimer.isActive()) {
        if (!flushTimer.isActive()) {
            // Wait for a period to batch up incoming changes
            flushTimer.start(flushTimeout);
        }

        QSet<QMailMessageId> idsSet = QSet<QMailMessageId>::fromList(ids);
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
        emitIpcUpdates(ids, sig[changeType]);

        preFlushTimer.start(preFlushTimeout);
    }
}

void QMailStoreImplementationBase::notifyMessagesDataChange(QMailStore::ChangeType changeType, const QMailMessageMetaDataList& data)
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
        emitIpcUpdates(data, sig[changeType]);

        preFlushTimer.start(preFlushTimeout);
    }
}

void QMailStoreImplementationBase::notifyMessagesDataChange(const QMailMessageIdList& ids,  const QMailMessageKey::Properties& properties,
                                                            const QMailMessageMetaData& data)
{
    // Use the preFlushTimer to activate buffering when multiple changes occur proximately
    if (preFlushTimer.isActive() || flushTimer.isActive()) {
        if (!flushTimer.isActive()) {
            // Wait for a period to batch up incoming changes
            flushTimer.start(flushTimeout);
        }

        MessagesProperties props(QPair<QMailMessageKey::Properties, QMailMessageMetaData>(properties, data), ids.toSet());
        messagesPropertiesBuffer.append(props);;

    } else {
        emitIpcUpdates(ids, properties, data, messagePropertyUpdatedSig());

        preFlushTimer.start(preFlushTimeout);
    }
}

void QMailStoreImplementationBase::notifyMessagesDataChange(const QMailMessageIdList& ids,  quint64 status, bool set)
{
    // Use the preFlushTimer to activate buffering when multiple changes occur proximately
    if (preFlushTimer.isActive() || flushTimer.isActive()) {
        if (!flushTimer.isActive()) {
            // Wait for a period to batch up incoming changes
            flushTimer.start(flushTimeout);
        }

        MessagesStatus messageStatus(status, set);
        messagesStatusBuffer[messageStatus] += ids.toSet();

    } else {
        emitIpcUpdates(ids, status, set, messageStatusUpdatedSig());

        preFlushTimer.start(preFlushTimeout);
    }
}

void QMailStoreImplementationBase::notifyThreadsChange(QMailStore::ChangeType changeType, const QMailThreadIdList& ids)
{
    static NotifyFunctionMap sig(initThreadFunctions());

    // Use the preFlushTimer to activate buffering when multiple changes occur proximately
    if (preFlushTimer.isActive() || flushTimer.isActive()) {
        if (!flushTimer.isActive()) {
            // Wait for a period to batch up incoming changes
            flushTimer.start(flushTimeout);
        }

        QSet<QMailThreadId> idsSet = QSet<QMailThreadId>::fromList(ids);
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
        emitIpcUpdates(ids, sig[changeType]);

        preFlushTimer.start(preFlushTimeout);
    }
}


void QMailStoreImplementationBase::notifyFoldersChange(QMailStore::ChangeType changeType, const QMailFolderIdList& ids)
{
    static NotifyFunctionMap sig(initFolderFunctions());

    // Use the preFlushTimer to activate buffering when multiple changes occur proximately
    if (preFlushTimer.isActive() || flushTimer.isActive()) {
        if (!flushTimer.isActive()) {
            // Wait for a period to batch up incoming changes
            flushTimer.start(flushTimeout);
        }

        QSet<QMailFolderId> idsSet = QSet<QMailFolderId>::fromList(ids);
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
        emitIpcUpdates(ids, sig[changeType]);

        preFlushTimer.start(preFlushTimeout);
    }
}

void QMailStoreImplementationBase::notifyMessageRemovalRecordsChange(QMailStore::ChangeType changeType, const QMailAccountIdList& ids)
{
    static NotifyFunctionMap sig(initMessageRemovalRecordFunctions());

    // Use the preFlushTimer to activate buffering when multiple changes occur proximately
    if (preFlushTimer.isActive() || flushTimer.isActive()) {
        if (!flushTimer.isActive()) {
            // Wait for a period to batch up incoming changes
            flushTimer.start(flushTimeout);
        }

        QSet<QMailAccountId> idsSet = QSet<QMailAccountId>::fromList(ids);
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
        emitIpcUpdates(ids, sig[changeType]);

        preFlushTimer.start(preFlushTimeout);
    }
}

void QMailStoreImplementationBase::notifyRetrievalInProgress(const QMailAccountIdList& ids)
{
    // Clients may want to enable or disable event handling based on this event, therefore
    // we must ensure that all previous events are actually delivered before this one is.
    flushIpcNotifications();

    emitIpcUpdates(ids, retrievalInProgressSig());
}

void QMailStoreImplementationBase::notifyTransmissionInProgress(const QMailAccountIdList& ids)
{
    flushIpcNotifications();

    emitIpcUpdates(ids, transmissionInProgressSig());
}

bool QMailStoreImplementationBase::setRetrievalInProgress(const QMailAccountIdList& ids)
{
    QSet<QMailAccountId> idSet(ids.toSet());
    if ((idSet != retrievalInProgressIds) || !retrievalSetInitialized) {
        retrievalInProgressIds = idSet;
        retrievalSetInitialized = true;
        return true;
    }

    return false;
}

bool QMailStoreImplementationBase::setTransmissionInProgress(const QMailAccountIdList& ids)
{
    QSet<QMailAccountId> idSet(ids.toSet());
    if ((idSet != transmissionInProgressIds) || !transmissionSetInitialized) {
        transmissionInProgressIds = idSet;
        transmissionSetInitialized = true;
        return true;
    }

    return false;
}

bool QMailStoreImplementationBase::isIpcConnectionEstablished() const
{
    return ipcChannel->isConnected();
}

void QMailStoreImplementationBase::disconnectIpc()
{
    ipcChannel->disconnectFromServer();
}

void QMailStoreImplementationBase::reconnectIpc()
{
    ipcChannel->connectRepeatedly();
}

QString QMailStoreImplementationBase::accountAddedSig()
{
    return QStringLiteral("accountAdded(uint,QList<quint64>)");
}

QString QMailStoreImplementationBase::accountRemovedSig()
{
    return QStringLiteral("accountRemoved(uint,QList<quint64>)");
}

QString QMailStoreImplementationBase::accountUpdatedSig()
{
    return QStringLiteral("accountUpdated(uint,QList<quint64>)");
}

QString QMailStoreImplementationBase::accountContentsModifiedSig()
{
    return QStringLiteral("accountContentsModified(uint,QList<quint64>)");
}

QString QMailStoreImplementationBase::messageAddedSig()
{
    return QStringLiteral("messageAdded(uint,QList<quint64>)");
}

QString QMailStoreImplementationBase::messageRemovedSig()
{
    return QStringLiteral("messageRemoved(uint,QList<quint64>)");
}

QString QMailStoreImplementationBase::messageUpdatedSig()
{
    return QStringLiteral("messageUpdated(uint,QList<quint64>)");
}

QString QMailStoreImplementationBase::messageContentsModifiedSig()
{
    return QStringLiteral("messageContentsModified(uint,QList<quint64>)");
}

QString QMailStoreImplementationBase::messageMetaDataAddedSig()
{
    return QStringLiteral("messageDataAdded(QMailMessageMetaDataList)");
}

QString QMailStoreImplementationBase::messageMetaDataUpdatedSig()
{
    return QStringLiteral("messageDataUpdated(QMailMessageMetaDataList)");
}

QString QMailStoreImplementationBase::messagePropertyUpdatedSig()
{
    return QStringLiteral("messagePropertyUpdated(QList<quint64>,QFlags,QMailMessageMetaData)");
}

QString QMailStoreImplementationBase::messageStatusUpdatedSig()
{
    return QStringLiteral("messageStatusUpdated(QList<quint64>,quint64,bool)");
}

QString QMailStoreImplementationBase::folderAddedSig()
{
    return QStringLiteral("folderAdded(uint,QList<quint64>)");
}

QString QMailStoreImplementationBase::folderRemovedSig()
{
    return QStringLiteral("folderRemoved(uint,QList<quint64>)");
}

QString QMailStoreImplementationBase::folderUpdatedSig()
{
    return QStringLiteral("folderUpdated(uint,QList<quint64>)");
}

QString QMailStoreImplementationBase::folderContentsModifiedSig()
{
    return QStringLiteral("folderContentsModified(uint,QList<quint64>)");
}

QString QMailStoreImplementationBase::threadAddedSig()
{
    return QStringLiteral("threadAdded(uint,QList<quint64>)");
}

QString QMailStoreImplementationBase::threadRemovedSig()
{
    return QStringLiteral("threadRemoved(uint,QList<quint64>)");
}

QString QMailStoreImplementationBase::threadUpdatedSig()
{
    return QStringLiteral("threadUpdated(uint,QList<quint64>)");
}

QString QMailStoreImplementationBase::threadContentsModifiedSig()
{
    return QStringLiteral("threadContentsModified(uint,QList<quint64>)");
}

QString QMailStoreImplementationBase::messageRemovalRecordsAddedSig()
{
    return QStringLiteral("messageRemovalRecordsAdded(uint,QList<quint64>)");
}

QString QMailStoreImplementationBase::messageRemovalRecordsRemovedSig()
{
    return QStringLiteral("messageRemovalRecordsRemoved(uint,QList<quint64>)");
}

QString QMailStoreImplementationBase::retrievalInProgressSig()
{
	// TODO: why no PID in theses messages?
    return QStringLiteral("retrievalInProgress(QList<quint64>)");
}

QString QMailStoreImplementationBase::transmissionInProgressSig()
{
    return QStringLiteral("transmissionInProgress(QList<quint64>)");
}

QMailStoreImplementationBase::AccountUpdateSignalMap QMailStoreImplementationBase::initAccountUpdateSignals()
{
    AccountUpdateSignalMap sig;
    sig[QMailStoreImplementationBase::accountAddedSig()] = &QMailStore::accountsAdded;
    sig[QMailStoreImplementationBase::accountUpdatedSig()] = &QMailStore::accountsUpdated;
    sig[QMailStoreImplementationBase::accountRemovedSig()] = &QMailStore::accountsRemoved;
    sig[QMailStoreImplementationBase::accountContentsModifiedSig()] = &QMailStore::accountContentsModified;
    sig[QMailStoreImplementationBase::messageRemovalRecordsAddedSig()] = &QMailStore::messageRemovalRecordsAdded;
    sig[QMailStoreImplementationBase::messageRemovalRecordsRemovedSig()] = &QMailStore::messageRemovalRecordsRemoved;
    return sig;
}

QMailStoreImplementationBase::FolderUpdateSignalMap QMailStoreImplementationBase::initFolderUpdateSignals()
{
    FolderUpdateSignalMap sig;
    sig[QMailStoreImplementationBase::folderAddedSig()] = &QMailStore::foldersAdded;
    sig[QMailStoreImplementationBase::folderUpdatedSig()] = &QMailStore::foldersUpdated;
    sig[QMailStoreImplementationBase::folderRemovedSig()] = &QMailStore::foldersRemoved;
    sig[QMailStoreImplementationBase::folderContentsModifiedSig()] = &QMailStore::folderContentsModified;
    return sig;
}

QMailStoreImplementationBase::ThreadUpdateSignalMap QMailStoreImplementationBase::initThreadUpdateSignals()
{
    ThreadUpdateSignalMap sig;
    sig[QMailStoreImplementationBase::threadAddedSig()] = &QMailStore::threadsAdded;
    sig[QMailStoreImplementationBase::threadUpdatedSig()] = &QMailStore::threadsUpdated;
    sig[QMailStoreImplementationBase::threadRemovedSig()] = &QMailStore::threadsRemoved;
    sig[QMailStoreImplementationBase::threadContentsModifiedSig()] = &QMailStore::threadContentsModified;
    return sig;
}

QMailStoreImplementationBase::MessageUpdateSignalMap QMailStoreImplementationBase::initMessageUpdateSignals()
{
    MessageUpdateSignalMap sig;
    sig[QMailStoreImplementationBase::messageAddedSig()] = &QMailStore::messagesAdded;
    sig[QMailStoreImplementationBase::messageUpdatedSig()] = &QMailStore::messagesUpdated;
    sig[QMailStoreImplementationBase::messageRemovedSig()] = &QMailStore::messagesRemoved;
    sig[QMailStoreImplementationBase::messageContentsModifiedSig()] = &QMailStore::messageContentsModified;
    return sig;
}

QMailStoreImplementationBase::MessageDataPreCacheSignalMap QMailStoreImplementationBase::initMessageDataPreCacheSignals()
{
    MessageDataPreCacheSignalMap sig;
    sig[QMailStoreImplementationBase::messageMetaDataAddedSig()] = &QMailStore::messageDataAdded;
    sig[QMailStoreImplementationBase::messageMetaDataUpdatedSig()] = &QMailStore::messageDataUpdated;
    return sig;
}

void QMailStoreImplementationBase::flushNotifications()
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
    dispatchNotifications(addAccountsBuffer, sigAccount[QMailStore::Added]);
    dispatchNotifications(addFoldersBuffer, sigFolder[QMailStore::Added]);
    dispatchNotifications(addMessagesBuffer, sigMessage[QMailStore::Added]);
    dispatchNotifications(addThreadsBuffer, sigthread[QMailStore::Added]);
    dispatchNotifications(addMessageRemovalRecordsBuffer, sigRemoval[QMailStore::Added]);

    dispatchNotifications(messageContentsModifiedBuffer, sigMessage[QMailStore::ContentsModified]);
    dispatchNotifications(updateMessagesBuffer, sigMessage[QMailStore::Updated]);
    dispatchNotifications(updateThreadsBuffer, sigthread[QMailStore::Updated]);
    dispatchNotifications(updateFoldersBuffer, sigFolder[QMailStore::Updated]);
    dispatchNotifications(updateAccountsBuffer, sigAccount[QMailStore::Updated]);

    dispatchNotifications(removeMessageRemovalRecordsBuffer, sigRemoval[QMailStore::Removed]);
    dispatchNotifications(removeMessagesBuffer, sigMessage[QMailStore::Removed]);
    dispatchNotifications(removeThreadsBuffer, sigthread[QMailStore::Removed]);
    dispatchNotifications(removeFoldersBuffer, sigFolder[QMailStore::Removed]);
    dispatchNotifications(removeAccountsBuffer, sigAccount[QMailStore::Removed]);

    dispatchNotifications(folderContentsModifiedBuffer, sigFolder[QMailStore::ContentsModified]);
    dispatchNotifications(accountContentsModifiedBuffer, sigAccount[QMailStore::ContentsModified]);

    dispatchNotifications(addMessagesDataBuffer, sigMessageData[QMailStore::Added]);
    dispatchNotifications(updateMessagesDataBuffer, sigMessageData[QMailStore::Updated]);

    dispatchNotifications(messagesPropertiesBuffer, messagePropertyUpdatedSig());
    dispatchNotifications(messagesStatusBuffer, messageStatusUpdatedSig());
}

void QMailStoreImplementationBase::ipcMessage(const QString& message, const QByteArray& data) 
{
    QDataStream ds(data);

    uint origin;
    ds >> origin;

    if (pid == origin) // don't notify ourselves
        return;

    if (message == QLatin1String("forceIpcFlush")) {
        // We have been told to flush any pending ipc notifications
        queueTimer.stop();
        while (emitIpcNotification()) {}
    } else if ((message == retrievalInProgressSig()) || (message == transmissionInProgressSig())) {
        // Emit this message immediately
        QMailAccountIdList ids;
        ds >> ids;

        if (message == retrievalInProgressSig()) {
            emitIpcNotification(&QMailStore::retrievalInProgress, ids);
        } else {
            emitIpcNotification(&QMailStore::transmissionInProgress, ids);
        }
    } else {
        // Queue this message for batched delivery
        messageQueue.append(qMakePair(message, data));
        queueTimer.start(0);
    }
}

bool QMailStoreImplementationBase::emitIpcNotification()
{
    if (messageQueue.isEmpty())
        return false;
    
    const QPair<QString, QByteArray> &notification = messageQueue.first();
    const QString &message = notification.first;
    const QByteArray &data = notification.second;

    QDataStream ds(data);

    uint origin;
    ds >> origin;

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

    if ((ait = accountUpdateSignals.find(message)) != accountUpdateSignals.end()) {
        QMailAccountIdList ids;
        ds >> ids;
        messageQueue.removeFirst(); // messageQueue may be modified by the emitIpcNotification below

        emitIpcNotification(ait.value(), ids);
    } else if ((fit = folderUpdateSignals.find(message)) != folderUpdateSignals.end()) {
        QMailFolderIdList ids;
        ds >> ids;
        messageQueue.removeFirst();

        emitIpcNotification(fit.value(), ids);
    } else if ((mit = messageUpdateSignals.find(message)) != messageUpdateSignals.end()) {
        QMailMessageIdList ids;
        ds >> ids;
        messageQueue.removeFirst();

        emitIpcNotification(mit.value(), ids);
    } else if ((mdit = messageDataPreCacheSignals.find(message)) != messageDataPreCacheSignals.end()) {
        QMailMessageMetaDataList data;
        ds >> data;
        messageQueue.removeFirst();

        emitIpcNotification(mdit.value(), data);
    } else if (message == messagePropertyUpdatedSig()) {
        QMailMessageIdList ids;
        ds >> ids;
        int props = 0;
        ds >> props;
        QMailMessageMetaData data;
        ds >> data;
        messageQueue.removeFirst();

        emitIpcNotification(ids, static_cast<QMailMessageKey::Property>(props), data);
    } else if (message == messageStatusUpdatedSig()) {
        QMailMessageIdList ids;
        ds >> ids;
        quint64 status = 0;
        ds >> status;
        bool set = false;
        ds >> set;
        messageQueue.removeFirst();

        emitIpcNotification(ids, status, set);
    } else if ((tit = threadUpdateSignals.find(message)) != threadUpdateSignals.end()) {
        QMailThreadIdList ids;
        ds >> ids;
        messageQueue.removeFirst();

        emitIpcNotification(tit.value(), ids);
    } else {
        qWarning() << "No update signal for message:" << message;
        messageQueue.removeFirst();
    }
    
    return !messageQueue.isEmpty();
}

void QMailStoreImplementationBase::emitIpcNotification(AccountUpdateSignal signal, const QMailAccountIdList &ids)
{
    asyncEmission = true;
    emit (q->*signal)(ids);
    asyncEmission = false;
}

void QMailStoreImplementationBase::emitIpcNotification(FolderUpdateSignal signal, const QMailFolderIdList &ids)
{
    asyncEmission = true;
    emit (q->*signal)(ids);
    asyncEmission = false;
}

void QMailStoreImplementationBase::emitIpcNotification(ThreadUpdateSignal signal, const QMailThreadIdList &ids)
{
    asyncEmission = true;
    emit (q->*signal)(ids);
    asyncEmission = false;
}

void QMailStoreImplementationBase::emitIpcNotification(MessageUpdateSignal signal, const QMailMessageIdList &ids)
{
    asyncEmission = true;
    emit (q->*signal)(ids);
    asyncEmission = false;
}

void QMailStoreImplementationBase::emitIpcNotification(MessageDataPreCacheSignal signal, const QMailMessageMetaDataList &data)
{
    asyncEmission = true;
    emit (q->*signal)(data);
    asyncEmission = false;
}

void QMailStoreImplementationBase::emitIpcNotification(const QMailMessageIdList& ids,  const QMailMessageKey::Properties& properties,
                                 const QMailMessageMetaData& data)
{
    asyncEmission = true;
    emit q->messagePropertyUpdated(ids, properties, data);
    asyncEmission = false;
}

void QMailStoreImplementationBase::emitIpcNotification(const QMailMessageIdList& ids, quint64 status, bool set)
{
    asyncEmission = true;
    emit q->messageStatusUpdated(ids, status, set);
    asyncEmission = false;
}

QMailStoreImplementation::QMailStoreImplementation(QMailStore* parent)
    : QMailStoreImplementationBase(parent)
{
}


QMailStoreNullImplementation::QMailStoreNullImplementation(QMailStore* parent)
    : QMailStoreImplementation(parent)
{
    setLastError(QMailStore::StorageInaccessible);
}

void QMailStoreNullImplementation::clearContent()
{
}

bool QMailStoreNullImplementation::addAccount(QMailAccount *, QMailAccountConfiguration *, QMailAccountIdList *)
{
    return false;
}

bool QMailStoreNullImplementation::addFolder(QMailFolder *, QMailFolderIdList *, QMailAccountIdList *)
{
    return false;
}

bool QMailStoreNullImplementation::addMessages(const QList<QMailMessage *> &, QMailMessageIdList *, QMailThreadIdList *, QMailMessageIdList *, QMailThreadIdList *, QMailFolderIdList *, QMailThreadIdList *, QMailAccountIdList *)
{
    return false;
}

bool QMailStoreNullImplementation::addMessages(const QList<QMailMessageMetaData *> &, QMailMessageIdList *, QMailThreadIdList *, QMailMessageIdList *, QMailThreadIdList *, QMailFolderIdList *, QMailThreadIdList *, QMailAccountIdList *)
{
    return false;
}

bool QMailStoreNullImplementation::addThread(QMailThread *, QMailThreadIdList *)
{
    return false;
}

bool QMailStoreNullImplementation::removeAccounts(const QMailAccountKey &, QMailAccountIdList *, QMailFolderIdList *, QMailThreadIdList *, QMailMessageIdList *, QMailMessageIdList *, QMailFolderIdList *, QMailThreadIdList *, QMailAccountIdList *)
{
    return false;
}

bool QMailStoreNullImplementation::removeFolders(const QMailFolderKey &, QMailStore::MessageRemovalOption, QMailFolderIdList *, QMailMessageIdList *, QMailThreadIdList *, QMailMessageIdList *, QMailFolderIdList *, QMailThreadIdList *, QMailAccountIdList *)
{
    return false;
}

bool QMailStoreNullImplementation::removeMessages(const QMailMessageKey &, QMailStore::MessageRemovalOption, QMailMessageIdList *, QMailThreadIdList*, QMailMessageIdList *, QMailFolderIdList *, QMailThreadIdList *, QMailAccountIdList *)
{
    return false;
}

bool QMailStoreNullImplementation::removeThreads(const QMailThreadKey &, QMailStore::MessageRemovalOption ,
                                                              QMailThreadIdList *, QMailMessageIdList *, QMailMessageIdList *, QMailFolderIdList *, QMailThreadIdList *, QMailAccountIdList *)
{
    return false;
}

bool QMailStoreNullImplementation::updateAccount(QMailAccount *, QMailAccountConfiguration *, QMailAccountIdList *)
{
    return false;
}

bool QMailStoreNullImplementation::updateAccountConfiguration(QMailAccountConfiguration *, QMailAccountIdList *)
{
    return false;
}

bool QMailStoreNullImplementation::updateFolder(QMailFolder *, QMailFolderIdList *, QMailAccountIdList *)
{
    return false;
}

bool QMailStoreNullImplementation::updateMessages(const QList<QPair<QMailMessageMetaData *, QMailMessage *> > &, QMailMessageIdList *, QMailThreadIdList *, QMailMessageIdList *, QMailFolderIdList *, QMailAccountIdList *)
{
    return false;
}

bool QMailStoreNullImplementation::updateMessagesMetaData(const QMailMessageKey &, const QMailMessageKey::Properties &, const QMailMessageMetaData &, QMailMessageIdList *, QMailThreadIdList *, QMailThreadIdList *, QMailFolderIdList *, QMailAccountIdList *)
{
    return false;
}

bool QMailStoreNullImplementation::updateMessagesMetaData(const QMailMessageKey &, quint64, bool, QMailMessageIdList *, QMailThreadIdList *, QMailFolderIdList *, QMailAccountIdList *)
{
    return false;
}

bool QMailStoreNullImplementation::updateThread(QMailThread *, QMailThreadIdList *)
{
    return false;
}

bool QMailStoreNullImplementation::ensureDurability()
{
    return false;
}

void QMailStoreNullImplementation::lock()
{
}

void QMailStoreNullImplementation::unlock()
{
}

bool QMailStoreNullImplementation::purgeMessageRemovalRecords(const QMailAccountId &, const QStringList &)
{
    return false;
}

int QMailStoreNullImplementation::countAccounts(const QMailAccountKey &) const
{
    return 0;
}

int QMailStoreNullImplementation::countFolders(const QMailFolderKey &) const
{
    return 0;
}

int QMailStoreNullImplementation::countMessages(const QMailMessageKey &) const
{
    return 0;
}

int QMailStoreNullImplementation::countThreads(const QMailThreadKey &) const
{
    return 0;
}

int QMailStoreNullImplementation::sizeOfMessages(const QMailMessageKey &) const
{
    return 0;
}

QMailAccountIdList QMailStoreNullImplementation::queryAccounts(const QMailAccountKey &, const QMailAccountSortKey &, uint, uint) const
{
    return QMailAccountIdList();
}

QMailFolderIdList QMailStoreNullImplementation::queryFolders(const QMailFolderKey &, const QMailFolderSortKey &, uint, uint) const
{
    return QMailFolderIdList();
}

QMailMessageIdList QMailStoreNullImplementation::queryMessages(const QMailMessageKey &, const QMailMessageSortKey &, uint, uint) const
{
    return QMailMessageIdList();
}

QMailThreadIdList QMailStoreNullImplementation::queryThreads(const QMailThreadKey &, const QMailThreadSortKey &, uint, uint) const
{
    return QMailThreadIdList();
}

QMailAccount QMailStoreNullImplementation::account(const QMailAccountId &) const
{
    return QMailAccount();
}

QMailAccountConfiguration QMailStoreNullImplementation::accountConfiguration(const QMailAccountId &) const
{
    return QMailAccountConfiguration();
}

QMailFolder QMailStoreNullImplementation::folder(const QMailFolderId &) const
{
    return QMailFolder();
}

QMailMessage QMailStoreNullImplementation::message(const QMailMessageId &) const
{
    return QMailMessage();
}

QMailMessage QMailStoreNullImplementation::message(const QString &, const QMailAccountId &) const
{
    return QMailMessage();
}

QMailThread QMailStoreNullImplementation::thread(const QMailThreadId &) const
{
    return QMailThread();
}

QMailMessageMetaData QMailStoreNullImplementation::messageMetaData(const QMailMessageId &) const
{
    return QMailMessageMetaData();
}

QMailMessageMetaData QMailStoreNullImplementation::messageMetaData(const QString &, const QMailAccountId &) const
{
    return QMailMessageMetaData();
}

QMailMessageMetaDataList QMailStoreNullImplementation::messagesMetaData(const QMailMessageKey &, const QMailMessageKey::Properties &, QMailStore::ReturnOption) const
{
    return QMailMessageMetaDataList();
}

QMailThreadList QMailStoreNullImplementation::threads(const QMailThreadKey &, QMailStore::ReturnOption) const
{
    return QMailThreadList();
}

QMailMessageRemovalRecordList QMailStoreNullImplementation::messageRemovalRecords(const QMailAccountId &, const QMailFolderId &) const
{
    return QMailMessageRemovalRecordList();
}

bool QMailStoreNullImplementation::registerAccountStatusFlag(const QString &)
{
    return false;
}

quint64 QMailStoreNullImplementation::accountStatusMask(const QString &) const
{
    return 0;
}

bool QMailStoreNullImplementation::registerFolderStatusFlag(const QString &)
{
    return false;
}

quint64 QMailStoreNullImplementation::folderStatusMask(const QString &) const
{
    return 0;
}

bool QMailStoreNullImplementation::registerMessageStatusFlag(const QString &)
{
    return false;
}

quint64 QMailStoreNullImplementation::messageStatusMask(const QString &) const
{
    return 0;
}

QMap<QString, QString> QMailStoreNullImplementation::messageCustomFields(const QMailMessageId &)
{
    return QMap<QString, QString>();
}

bool QMailStoreNullImplementation::initStore()
{
    return false;
}


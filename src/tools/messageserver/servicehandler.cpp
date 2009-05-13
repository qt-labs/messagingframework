/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/


#include "servicehandler.h"
#include <longstream_p.h>
#include <QDataStream>
#include <QIODevice>
#include <qmailmessageserver.h>
#include <qmailserviceconfiguration.h>
#include <qmailstore.h>
#include <qmaillog.h>
#include <QApplication>
#include <QDir>
#include <QTimer>

// Account preparation is handled by an external function
extern void prepareAccounts();

namespace {

template <typename T1>
QByteArray serialize(const T1& v1)
{
    QByteArray data;
    QDataStream os(&data, QIODevice::WriteOnly);
    os << v1;
    return data;
}

template <typename T1, typename T2>
QByteArray serialize(const T1& v1, const T2& v2)
{
    QByteArray data;
    QDataStream os(&data, QIODevice::WriteOnly);
    os << v1 << v2;
    return data;
}

template <typename T1, typename T2, typename T3>
QByteArray serialize(const T1& v1, const T2& v2, const T3& v3)
{
    QByteArray data;
    QDataStream os(&data, QIODevice::WriteOnly);
    os << v1 << v2 << v3;
    return data;
}

template <typename T1, typename T2, typename T3, typename T4>
QByteArray serialize(const T1& v1, const T2& v2, const T3& v3, const T4& v4)
{
    QByteArray data;
    QDataStream os(&data, QIODevice::WriteOnly);
    os << v1 << v2 << v3 << v4;
    return data;
}

template <typename T1>
void deserialize(const QByteArray &data, T1& v1)
{
    QDataStream is(data);
    is >> v1;
}

template <typename T1, typename T2>
void deserialize(const QByteArray &data, T1& v1, T2& v2)
{
    QDataStream is(data);
    is >> v1 >> v2;
}

template <typename T1, typename T2, typename T3>
void deserialize(const QByteArray &data, T1& v1, T2& v2, T3& v3)
{
    QDataStream is(data);
    is >> v1 >> v2 >> v3;
}

template <typename T1, typename T2, typename T3, typename T4>
void deserialize(const QByteArray &data, T1& v1, T2& v2, T3& v3, T4& v4)
{
    QDataStream is(data);
    is >> v1 >> v2 >> v3 >> v4;
}

QSet<QMailAccountId> messageAccounts(const QMailMessageIdList &ids)
{
    // Find the accounts that own these messages
    QSet<QMailAccountId> accountIds;
    if (!ids.isEmpty()) {
        foreach (const QMailMessageMetaData &metaData, QMailStore::instance()->messagesMetaData(QMailMessageKey::id(ids), 
                                                                                                QMailMessageKey::ParentAccountId, 
                                                                                                QMailStore::ReturnDistinct)) {
            if (metaData.parentAccountId().isValid())
                accountIds.insert(metaData.parentAccountId());
        }
    }

    return accountIds;
}

QSet<QMailAccountId> messageAccount(const QMailMessageId &id)
{
    return messageAccounts(QMailMessageIdList() << id);
}

QSet<QMailAccountId> folderAccounts(const QMailFolderIdList &ids)
{
    // Find the accounts that own these folders
    QSet<QMailAccountId> accountIds;
    if (!ids.isEmpty()) {
        foreach (const QMailFolderId &folderId, ids) {
            QMailFolder folder(folderId);
            if (folder.id().isValid() && folder.parentAccountId().isValid())
                accountIds.insert(folder.parentAccountId());
        }
    }

    return accountIds;
}

QSet<QMailAccountId> folderAccount(const QMailFolderId &id)
{
    return folderAccounts(QMailFolderIdList() << id);
}

QMap<QMailAccountId, QMailMessageIdList> accountMessages(const QMailMessageIdList &ids)
{
    // Allocate each message to the relevant account
    QMap<QMailAccountId, QMailMessageIdList> map;
    if (!ids.isEmpty()) {
        foreach (const QMailMessageMetaData &metaData, QMailStore::instance()->messagesMetaData(QMailMessageKey::id(ids), 
                                                                                                QMailMessageKey::Id | QMailMessageKey::ParentAccountId, 
                                                                                                QMailStore::ReturnAll)) {
            if (metaData.id().isValid() && metaData.parentAccountId().isValid())
                map[metaData.parentAccountId()].append(metaData.id());
        }
    }

    return map;
}

void extractAccounts(const QMailMessageKey &key, bool parentNegated, QSet<QMailAccountId> &include, QSet<QMailAccountId> &exclude)
{
    bool isNegated(parentNegated);
    if (key.isNegated())
        isNegated = !isNegated;

    // Process the child keys
    const QList<QMailMessageKey> &subKeys(key.subKeys());
    foreach (const QMailMessageKey &subKey, subKeys)
        extractAccounts(subKey, key.isNegated(), include, exclude);

    // Process any arguments
    const QList<QMailMessageKey::ArgumentType> &args(key.arguments());
    foreach (const QMailMessageKey::ArgumentType &arg, args) {
        if (arg.property == QMailMessageKey::ParentAccountId) {
            QSet<QMailAccountId> *set = 0;

            if ((arg.op == QMailKey::Equal) || (arg.op == QMailKey::Includes)) {
                set = (isNegated ? &exclude : &include);
            } else if ((arg.op == QMailKey::NotEqual) || (arg.op == QMailKey::Excludes)) {
                set = (isNegated ? &include : &exclude);
            }

            if (set)
                foreach (const QVariant &v, arg.valueList)
                    set->insert(qVariantValue<QMailAccountId>(v));
        }
    }
}

// TODO: make sure this works correctly:
QSet<QMailAccountId> keyAccounts(const QMailMessageKey &key, const QSet<QMailAccountId> &complete)
{
    QSet<QMailAccountId> include;
    QSet<QMailAccountId> exclude;

    // Find all accounts that are relevant to this key
    extractAccounts(key, key.isNegated(), include, exclude);

    if (!exclude.isEmpty()) {
        // We have to consider all accounts apart from the exclusions, and 
        // any specific inclusions should be re-added
        return ((complete - exclude) + include);
    }

    if (include.isEmpty()) {
        // No accounts qualifications were specified at all
        return complete;
    }

    return include;
}

bool messageBodyContainsText(const QMailMessage &message, const QString& text)
{
    // Search only messages or message parts that are of type 'text/*'
    if (message.hasBody()) {
        if (message.contentType().type().toLower() == "text") {
            if (message.body().data().contains(text, Qt::CaseInsensitive))
                return true;
        }
    } else if (message.multipartType() != QMailMessage::MultipartNone) {
        // We could do a recursive search for text elements, but since we can't currently
        // display generic multipart messages anyway, let's just search the top level
        for (uint i = 0; i < message.partCount(); ++i) {
            const QMailMessagePart &part = message.partAt(i);

            if (part.contentType().type().toLower() == "text") {
                if (part.body().data().contains(text, Qt::CaseInsensitive))
                    return true;
            }
        }
    }

    return false;
}

QString requestsFileName()
{
    return QDir::tempPath() + "/qmf-messageserver-requests";
}

}


ServiceHandler::MessageSearch::MessageSearch(quint64 action, const QMailMessageIdList &ids, const QString &text)
    : _action(action),
      _ids(ids),
      _text(text),
      _active(false),
      _total(ids.count()),
      _progress(0)
{
}

quint64 ServiceHandler::MessageSearch::action() const
{
    return _action;
}

const QString &ServiceHandler::MessageSearch::bodyText() const
{
    return _text;
}

bool ServiceHandler::MessageSearch::pending() const
{
    return !_active;
}

void ServiceHandler::MessageSearch::inProgress()
{
    _active = true;
}

bool ServiceHandler::MessageSearch::isEmpty() const
{
    return _ids.isEmpty();
}

uint ServiceHandler::MessageSearch::total() const
{
    return _total;
}

uint ServiceHandler::MessageSearch::progress() const
{
    return _progress;
}

QMailMessageIdList ServiceHandler::MessageSearch::takeBatch()
{
    // TODO: this should be smarter
    static const int BatchSize = 10;

    QMailMessageIdList result;
    if (_ids.count() <= BatchSize) {
        result = _ids;
        _ids.clear();
    } else {
        result = _ids.mid(0, BatchSize);
        _ids = _ids.mid(BatchSize, -1);
    }

    _progress += result.count();
    return result;
}


ServiceHandler::ServiceHandler(QObject* parent)
    : QObject(parent),
      _requestsFile(requestsFileName())
{
    LongStream::cleanupTempFiles();

    ::prepareAccounts();

    if (QMailStore *store = QMailStore::instance()) {
        connect(store, SIGNAL(accountsAdded(QMailAccountIdList)), this, SLOT(accountsAdded(QMailAccountIdList)));
        connect(store, SIGNAL(accountsUpdated(QMailAccountIdList)), this, SLOT(accountsUpdated(QMailAccountIdList)));
        connect(store, SIGNAL(accountsRemoved(QMailAccountIdList)), this, SLOT(accountsRemoved(QMailAccountIdList)));

        // All necessary accounts now exist - create the service instances
        registerAccountServices(store->queryAccounts());
    }

    connect(this, SIGNAL(remoteSearchCompleted(quint64)), this, SLOT(finaliseSearch(quint64)));

    // See if there are any requests remaining from our previous run
    if (_requestsFile.exists()) {
        if (!_requestsFile.open(QIODevice::ReadOnly)) {
            qWarning() << "Unable to open requests file for read!";
        } else {
            QString line;

            // Every request still in the file failed to complete
            for (QByteArray line = _requestsFile.readLine(); !line.isEmpty(); line = _requestsFile.readLine()) {
                if (quint64 action = line.trimmed().toULongLong()) {
                    _failedRequests.append(action);
                }
            }

            _requestsFile.close();
        }
    }

    if (!_requestsFile.open(QIODevice::WriteOnly)) {
        qWarning() << "Unable to open requests file for write!" << _requestsFile.fileName();
    } else {
        if (!_requestsFile.resize(0)) {
            qWarning() << "Unable to truncate requests file!";
        } else {
            _requestsFile.flush();
        }
    }

    if (!_failedRequests.isEmpty()) {
        // Allow the clients some time to reconnect, then report our failures
        QTimer::singleShot(2000, this, SLOT(reportFailures()));
    }
}

ServiceHandler::~ServiceHandler()
{
    foreach (QMailMessageService *service, serviceMap.values()) {
        service->cancelOperation();
        delete service;
    }

    serviceMap.clear();
}

void ServiceHandler::registerAccountServices(const QMailAccountIdList &ids)
{
    foreach (const QMailAccountId &id, ids) {
        QMailAccount account(id);
        if (account.status() & QMailAccount::Enabled) {
            QMailAccountConfiguration config(id);
            
            // See if this account is configured to use a master account
            QMailServiceConfiguration internalCfg(&config, "qtopiamail");
            QString masterId(internalCfg.value("masterAccountId", ""));
            if (!masterId.isEmpty()) {
                QMailAccount master(QMailAccountId(masterId.toInt()));
                if (master.id().isValid()) {
                    masterAccount.insert(id, master.id());
                } else {
                    qMailLog(Messaging) << "Unable to locate master account:" << masterId << "for account:" << id;
                }
            } else {
                foreach (const QString& service, config.services()) {
                    QMailServiceConfiguration svcCfg(&config, service);
                    if ((svcCfg.type() != QMailServiceConfiguration::Storage) &&
                        (svcCfg.type() != QMailServiceConfiguration::Unknown)) {
                        // We need to instantiate this service for this account
                        registerAccountService(id, svcCfg);
                    }
                }
            }
        }
    }
}

void ServiceHandler::deregisterAccountServices(const QMailAccountIdList &ids)
{
    QMap<QPair<QMailAccountId, QString>, QMailMessageService*>::iterator it = serviceMap.begin(), end = serviceMap.end();
    while (it != end) {
        if (ids.contains(it.key().first)) {
            // Remove any services associated with this account
            if (QMailMessageService *service = it.value()) {
                qMailLog(Messaging) << "Deregistering service:" << service->service() << "for account:" << it.key().first;
                service->cancelOperation();
                delete service;
            }

            it = serviceMap.erase(it);
        } else {
            ++it;
        }
    }

    foreach (const QMailAccountId &id, ids) {
        // Remove trailing references to this account's serviecs
        QMap<QMailAccountId, QMailAccountId>::iterator it = masterAccount.find(id);
        if (it != masterAccount.end()) {
            masterAccount.erase(it);
        } else {
            if (QMailMessageSource *source = accountSource(id))
                sourceService.remove(source);

            if (QMailMessageSink *sink = accountSink(id))
                sinkService.remove(sink);
        }
    }
}

void ServiceHandler::reregisterAccountServices(const QMailAccountIdList &ids)
{
    // Remove and re-create these accounts' services
    deregisterAccountServices(ids);
    registerAccountServices(ids);
}

void ServiceHandler::accountsAdded(const QMailAccountIdList &ids)
{
    registerAccountServices(ids);
}

void ServiceHandler::accountsUpdated(const QMailAccountIdList &ids)
{
    // Only respond to updates that were generated by other processes
    if (QMailStore::instance()->asynchronousEmission()) {
        reregisterAccountServices(ids);
    }
}

void ServiceHandler::accountsRemoved(const QMailAccountIdList &ids)
{
    deregisterAccountServices(ids);
}

QMailAccountId ServiceHandler::transmissionAccountId(const QMailAccountId &accountId) const
{
    QMap<QMailAccountId, QMailAccountId>::const_iterator it = masterAccount.find(accountId);
    if (it != masterAccount.end())
        return it.value();

    return accountId;
}

void ServiceHandler::registerAccountSource(const QMailAccountId &accountId, QMailMessageSource *source, QMailMessageService *service)
{
    sourceMap.insert(accountId, source);
    sourceService.insert(source, service);

    connect(source, SIGNAL(newMessagesAvailable()), this, SIGNAL(newMessagesAvailable()));
    connect(source, SIGNAL(messagesDeleted(QMailMessageIdList)), this, SLOT(messagesDeleted(QMailMessageIdList)));
    connect(source, SIGNAL(messagesCopied(QMailMessageIdList)), this, SLOT(messagesCopied(QMailMessageIdList)));
    connect(source, SIGNAL(messagesMoved(QMailMessageIdList)), this, SLOT(messagesMoved(QMailMessageIdList)));
    connect(source, SIGNAL(messagesPrepared(QMailMessageIdList)), this, SLOT(messagesPrepared(QMailMessageIdList)));
    connect(source, SIGNAL(matchingMessageIds(QMailMessageIdList)), this, SLOT(matchingMessageIds(QMailMessageIdList)));
    connect(source, SIGNAL(protocolResponse(QString, QVariant)), this, SLOT(protocolResponse(QString, QVariant)));
}

QMailMessageSource *ServiceHandler::accountSource(const QMailAccountId &accountId) const
{
    QMap<QMailAccountId, QMailMessageSource*>::const_iterator it = sourceMap.find(transmissionAccountId(accountId));
    if (it != sourceMap.end())
        return *it;

    return 0;
}

void ServiceHandler::registerAccountSink(const QMailAccountId &accountId, QMailMessageSink *sink, QMailMessageService *service)
{
    sinkMap.insert(accountId, sink);
    sinkService.insert(sink, service);

    connect(sink, SIGNAL(messagesTransmitted(QMailMessageIdList)), this, SLOT(messagesTransmitted(QMailMessageIdList)));
}

QMailMessageSink *ServiceHandler::accountSink(const QMailAccountId &accountId) const
{
    QMap<QMailAccountId, QMailMessageSink*>::const_iterator it = sinkMap.find(transmissionAccountId(accountId));
    if (it != sinkMap.end())
        return *it;

    return 0;
}

QMailMessageService *ServiceHandler::createService(const QString &name, const QMailAccountId &accountId)
{
    QMailMessageService *service = QMailMessageServiceFactory::createService(name, accountId);

    if (service) {
        connect(service, SIGNAL(availabilityChanged(bool)), this, SLOT(availabilityChanged(bool)));
        connect(service, SIGNAL(connectivityChanged(QMailServiceAction::Connectivity)), this, SLOT(connectivityChanged(QMailServiceAction::Connectivity)));
        connect(service, SIGNAL(activityChanged(QMailServiceAction::Activity)), this, SLOT(activityChanged(QMailServiceAction::Activity)));
        connect(service, SIGNAL(statusChanged(const QMailServiceAction::Status)), this, SLOT(statusChanged(const QMailServiceAction::Status)));
        connect(service, SIGNAL(progressChanged(uint, uint)), this, SLOT(progressChanged(uint, uint)));
        connect(service, SIGNAL(actionCompleted(bool)), this, SLOT(actionCompleted(bool)));
    }

    return service;
}

void ServiceHandler::registerAccountService(const QMailAccountId &accountId, const QMailServiceConfiguration &svcCfg)
{
    QMap<QPair<QMailAccountId, QString>, QMailMessageService*>::const_iterator it = serviceMap.find(qMakePair(accountId, svcCfg.service()));
    if (it == serviceMap.end()) {
        // We need to create a new service for this account
        if (QMailMessageService *service = createService(svcCfg.service(), accountId)) {
            serviceMap.insert(qMakePair(accountId, svcCfg.service()), service);
            qMailLog(Messaging) << "Registering service:" << svcCfg.service() << "for account:" << accountId;

            if (service->hasSource())
                registerAccountSource(accountId, &service->source(), service);

            if (service->hasSink())
                registerAccountSink(accountId, &service->sink(), service);

            if (!service->available())
                mUnavailableServices.insert(service);
        } else {
            qMailLog(Messaging) << "Unable to instantiate service:" << svcCfg.service();
        }
    }
}

bool ServiceHandler::serviceAvailable(QMailMessageService *service) const
{
    if (mServiceAction.contains(service) || mUnavailableServices.contains(service))
        return false;

    return true;
}

QSet<QMailMessageService*> ServiceHandler::sourceServiceSet(const QMailAccountId &id) const
{
    QSet<QMailMessageService*> services;

    // See if this account's service has a source
    if (QMailMessageSource *source = accountSource(id)) {
        services.insert(sourceService[source]);
    } else {
        qMailLog(Messaging) << "Unable to find message source for account:" << id;
    }

    return services;
}

QSet<QMailMessageService*> ServiceHandler::sourceServiceSet(const QSet<QMailAccountId> &ids) const
{
    QSet<QMailMessageService*> services;

    foreach (const QMailAccountId &id, ids) {
        // See if this account's service has a source
        if (QMailMessageSource *source = accountSource(id)) {
            services.insert(sourceService[source]);
        } else {
            qMailLog(Messaging) << "Unable to find message source for account:" << id;
            return QSet<QMailMessageService*>();
        }
    }

    return services;
}

QSet<QMailMessageService*> ServiceHandler::sinkServiceSet(const QMailAccountId &id) const
{
    QSet<QMailMessageService*> services;

    if (QMailMessageSink *sink = accountSink(id)) {
        services.insert(sinkService[sink]);
    } else {
        qMailLog(Messaging) << "Unable to find message sink for account:" << id;
    }

    return services;
}

QSet<QMailMessageService*> ServiceHandler::sinkServiceSet(const QSet<QMailAccountId> &ids) const
{
    QSet<QMailMessageService*> services;

    foreach (const QMailAccountId &id, ids) {
        if (QMailMessageSink *sink = accountSink(id)) {
            services.insert(sinkService[sink]);
        } else {
            qMailLog(Messaging) << "Unable to find message sink for account:" << id;
            return QSet<QMailMessageService*>();
        }
    }

    return services;
}

quint64 ServiceHandler::sourceAction(QMailMessageSource *source) const
{
    if (source)
        if (QMailMessageService *service = sourceService[source])
            return serviceAction(service);

    return 0;
}

quint64 ServiceHandler::sinkAction(QMailMessageSink *sink) const
{
    if (sink)
        if (QMailMessageService *service = sinkService[sink])
            return serviceAction(service);

    return 0;
}

quint64 ServiceHandler::serviceAction(QMailMessageService *service) const
{
    QMap<QMailMessageService*, quint64>::const_iterator it = mServiceAction.find(service);
    if (it != mServiceAction.end())
        return it.value();

    return 0;
}

void ServiceHandler::enqueueRequest(quint64 action, const QByteArray &data, const QSet<QMailMessageService*> &services, RequestServicer servicer, CompletionSignal completion)
{
    Request req;
    req.action = action;
    req.data = data;
    req.services = services;
    req.servicer = servicer;
    req.completion = completion;

    mRequests.append(req);

    // Add this request to the outstanding list
    if (!_outstandingRequests.contains(action)) {
        _outstandingRequests.insert(action);

        QByteArray requestNumber(QByteArray::number(action));
        _requestsFile.write(requestNumber.append("\n"));
        _requestsFile.flush();
    }

    QTimer::singleShot(0, this, SLOT(dispatchRequest()));
}

void ServiceHandler::dispatchRequest()
{
    if (!mRequests.isEmpty()) {
        const Request &request(mRequests.first());

        foreach (QMailMessageService *service, request.services) {
            if (!serviceAvailable(service)) {
                // We can't dispatch this request yet...
                return;
            }
        }

        // Associate the services with the action, so that signals are reported correctly
        foreach (QMailMessageService *service, request.services)
            mServiceAction.insert(service, request.action);

        // The services required for this request are available
        if ((this->*request.servicer)(request.action, request.data)) {
            activateAction(request.action, request.services, request.completion);
        } else {
            qMailLog(Messaging) << "Unable to dispatch request:" << request.action << "to services:" << request.services;
            emit activityChanged(request.action, QMailServiceAction::Failed);

            foreach (QMailMessageService *service, request.services)
                mServiceAction.remove(service);
        }

        mRequests.takeFirst();
    }
}

void ServiceHandler::activateAction(quint64 action, const QSet<QMailMessageService*> &services, CompletionSignal completion)
{
    // The specified services are now busy
    ActionData data;
    data.services = services;
    data.completion = completion;
    data.expiry = QTime::currentTime().addMSecs(ExpiryPeriod);
    data.reported = false;

    mActiveActions.insert(action, data);

    if (mActionExpiry.isEmpty()) {
        // Start the expiry timer
        QTimer::singleShot(ExpiryPeriod, this, SLOT(expireAction()));
    }

    mActionExpiry.append(action);

    // This action is now underway
    emit activityChanged(action, QMailServiceAction::InProgress);
}

void ServiceHandler::updateAction(quint64 action)
{
    QLinkedList<quint64>::iterator it = qFind(mActionExpiry.begin(), mActionExpiry.end(), action);
    if (it != mActionExpiry.end()) {
        // Move this action to the end of the list
        mActionExpiry.erase(it);
        mActionExpiry.append(action);

        // Update the expiry time for this action
        mActiveActions[action].expiry = QTime::currentTime().addMSecs(ExpiryPeriod);
    }
}

void ServiceHandler::expireAction()
{
    if (!mActionExpiry.isEmpty()) {
        quint64 action = *mActionExpiry.begin();

        QMap<quint64, ActionData>::iterator it = mActiveActions.find(action);
        if (it != mActiveActions.end()) {
            ActionData &data(it.value());

            // Is the oldest action expired?
            QTime now = QTime::currentTime();
            if (data.expiry <= now) {
                qMailLog(Messaging) << "Expired request:" << action;
                reportFailure(action, QMailServiceAction::Status::ErrTimeout, tr("Request is not progressing"));
                emit activityChanged(action, QMailServiceAction::Failed);

                QSet<QMailAccountId> serviceAccounts;

                bool retrievalSetModified(false);
                bool transmissionSetModified(false);

                // Remove this action
                foreach (QMailMessageService *service, data.services) {
                    serviceAccounts.insert(service->accountId());
                    mServiceAction.remove(service);

                    QMailAccountId accountId(service->accountId());
                    if (accountId.isValid()) {
                        if (data.completion == &ServiceHandler::retrievalCompleted) {
                            if (_retrievalAccountIds.contains(accountId)) {
                                _retrievalAccountIds.remove(accountId);
                                retrievalSetModified = true;
                            }
                        } else if (data.completion == &ServiceHandler::transmissionCompleted) {
                            if (_transmissionAccountIds.contains(accountId)) {
                                _transmissionAccountIds.remove(accountId);
                                transmissionSetModified = true;
                            }
                        }
                    }
                }

                if (retrievalSetModified) {
                    QMailStore::instance()->setRetrievalInProgress(_retrievalAccountIds.toList());
                }
                if (transmissionSetModified) {
                    QMailStore::instance()->setTransmissionInProgress(_transmissionAccountIds.toList());
                }

                mActiveActions.erase(it);
                mActionExpiry.removeFirst();

                // Restart the service(s) for each of these accounts
                reregisterAccountServices(serviceAccounts.toList());

                // See if there are more actions to dispatch
                dispatchRequest();

                if (!mActionExpiry.isEmpty()) {
                    // Return here to test the new oldest action
                    QTimer::singleShot(0, this, SLOT(expireAction()));
                }
            } else {
                // Test again when it is due to expire
                QTimer::singleShot(now.msecsTo(data.expiry), this, SLOT(expireAction()));
            }
        } else {
            // Just remove this non-existent action
            mActionExpiry.removeFirst();
        }
    }
}

void ServiceHandler::cancelTransfer(quint64 action)
{
    QMap<quint64, ActionData>::iterator it = mActiveActions.find(action);
    if (it != mActiveActions.end()) {
        bool retrievalSetModified(false);
        bool transmissionSetModified(false);

        const ActionData &data(it.value());
        foreach (QMailMessageService *service, data.services) {
            service->cancelOperation();
            mServiceAction.remove(service);

            QMailAccountId accountId(service->accountId());
            if (accountId.isValid()) {
                if (data.completion == &ServiceHandler::retrievalCompleted) {
                    if (_retrievalAccountIds.contains(accountId)) {
                        _retrievalAccountIds.remove(accountId);
                        retrievalSetModified = true;
                    }
                } else if (data.completion == &ServiceHandler::transmissionCompleted) {
                    if (_transmissionAccountIds.contains(accountId)) {
                        _transmissionAccountIds.remove(accountId);
                        transmissionSetModified = true;
                    }
                }
            }
        }

        if (retrievalSetModified) {
            QMailStore::instance()->setRetrievalInProgress(_retrievalAccountIds.toList());
        }
        if (transmissionSetModified) {
            QMailStore::instance()->setTransmissionInProgress(_transmissionAccountIds.toList());
        }

        mActiveActions.erase(it);

        // See if there are more actions 
        QTimer::singleShot(0, this, SLOT(dispatchRequest()));
    } else {
        // See if this is a pending request that we can abort
        QList<Request>::iterator it = mRequests.begin(), end = mRequests.end();
        for ( ; it != end; ++it) {
            if ((*it).action == action) {
                mRequests.erase(it);
                break;
            }
        }

        // Report this action as failed
        reportFailure(action, QMailServiceAction::Status::ErrCancel, tr("Cancelled by user"));
    }
}

void ServiceHandler::transmitMessages(quint64 action, const QMailAccountId &accountId)
{
    // Ensure that this account has a sink configured
    QSet<QMailMessageService*> sinks(sinkServiceSet(accountId));
    if (sinks.isEmpty()) {
        reportFailure(action, QMailServiceAction::Status::ErrNoConnection, tr("Unable to enqueue messages for transmission"));
    } else {
        enqueueRequest(action, serialize(accountId), sinks, &ServiceHandler::dispatchTransmitMessages, &ServiceHandler::transmissionCompleted);
    }
}

bool ServiceHandler::dispatchTransmitMessages(quint64 action, const QByteArray &data)
{
    QMailAccountId accountId;

    deserialize(data, accountId);

    if (QMailMessageSink *sink = accountSink(accountId)) {
        // Transmit any messages in the Outbox for this account
        QMailMessageKey accountKey(QMailMessageKey::parentAccountId(accountId));
        QMailMessageKey folderKey(QMailMessageKey::parentFolderId(QMailFolderId(QMailFolder::OutboxFolder)));

        // TODO: Prepare any unresolved messages for transmission

        if (!sink->transmitMessages(QMailStore::instance()->queryMessages(accountKey & folderKey))) {
            qMailLog(Messaging) << "Unable to service request to add messages to sink for account:" << accountId;
            return false;
        } else {
            // This account is now transmitting
            setTransmissionInProgress(accountId, true);
        }
    } else {
        reportFailure(action, QMailServiceAction::Status::ErrFrameworkFault, tr("Unable to locate sink for account"), accountId);
        return false;
    }

    return true;
}

void ServiceHandler::retrieveFolderList(quint64 action, const QMailAccountId &accountId, const QMailFolderId &folderId, bool descending)
{
    QSet<QMailMessageService*> sources(sourceServiceSet(accountId));
    if (sources.isEmpty()) {
        reportFailure(action, QMailServiceAction::Status::ErrNoConnection, tr("Unable to retrieve folder list for unconfigured account"));
    } else {
        enqueueRequest(action, serialize(accountId, folderId, descending), sources, &ServiceHandler::dispatchRetrieveFolderListAccount, &ServiceHandler::retrievalCompleted);
    }
}

bool ServiceHandler::dispatchRetrieveFolderListAccount(quint64 action, const QByteArray &data)
{
    QMailAccountId accountId;
    QMailFolderId folderId;
    bool descending;

    deserialize(data, accountId, folderId, descending);

    if (QMailMessageSource *source = accountSource(accountId)) {
        if (!source->retrieveFolderList(accountId, folderId, descending)) {
            qMailLog(Messaging) << "Unable to service request to retrieve folder list for account:" << accountId;
            return false;
        } else {
            // This account is now retrieving (arguably...)
            setRetrievalInProgress(accountId, true);
        }
    } else {
        reportFailure(action, QMailServiceAction::Status::ErrFrameworkFault, tr("Unable to locate source for account"), accountId);
        return false;
    }

    return true;
}

void ServiceHandler::retrieveMessageList(quint64 action, const QMailAccountId &accountId, const QMailFolderId &folderId, uint minimum, const QMailMessageSortKey &sort)
{
    QSet<QMailMessageService*> sources(sourceServiceSet(accountId));
    if (sources.isEmpty()) {
        reportFailure(action, QMailServiceAction::Status::ErrNoConnection, tr("Unable to retrieve message list for unconfigured account"));
    } else {
        enqueueRequest(action, serialize(accountId, folderId, minimum, sort), sources, &ServiceHandler::dispatchRetrieveMessageList, &ServiceHandler::retrievalCompleted);
    }
}

bool ServiceHandler::dispatchRetrieveMessageList(quint64 action, const QByteArray &data)
{
    QMailAccountId accountId;
    QMailFolderId folderId;
    uint minimum;
    QMailMessageSortKey sort;

    deserialize(data, accountId, folderId, minimum, sort);

    if (QMailMessageSource *source = accountSource(accountId)) {
        if (!source->retrieveMessageList(accountId, folderId, minimum, sort)) {
            qMailLog(Messaging) << "Unable to service request to retrieve message list for folder:" << folderId;
            return false;
        } else {
            // This account is now retrieving
            setRetrievalInProgress(accountId, true);
        }
    } else {
        reportFailure(action, QMailServiceAction::Status::ErrFrameworkFault, tr("Unable to locate source for account"), accountId);
        return false;
    }

    return true;
}

void ServiceHandler::retrieveMessages(quint64 action, const QMailMessageIdList &messageIds, QMailRetrievalAction::RetrievalSpecification spec)
{
    QMap<QMailAccountId, QMailMessageIdList> messageLists(accountMessages(messageIds));

    QSet<QMailMessageService*> sources(sourceServiceSet(messageLists.keys().toSet()));
    if (sources.isEmpty()) {
        reportFailure(action, QMailServiceAction::Status::ErrNoConnection, tr("Unable to retrieve messages for unconfigured account"));
    } else {
        enqueueRequest(action, serialize(messageLists, spec), sources, &ServiceHandler::dispatchRetrieveMessages, &ServiceHandler::retrievalCompleted);
    }
}

bool ServiceHandler::dispatchRetrieveMessages(quint64 action, const QByteArray &data)
{
    QMap<QMailAccountId, QMailMessageIdList> messageLists;
    QMailRetrievalAction::RetrievalSpecification spec;

    deserialize(data, messageLists, spec);

    QMap<QMailAccountId, QMailMessageIdList>::const_iterator it = messageLists.begin(), end = messageLists.end();
    for ( ; it != end; ++it) {
        if (QMailMessageSource *source = accountSource(it.key())) {
            if (!source->retrieveMessages(it.value(), spec)) {
                qMailLog(Messaging) << "Unable to service request to retrieve messages for account:" << it.key();
                return false;
            } else if (spec != QMailRetrievalAction::Flags) {
                // This account is now retrieving
                if (!_retrievalAccountIds.contains(it.key())) {
                    _retrievalAccountIds.insert(it.key());
                }
            }
        } else {
            reportFailure(action, QMailServiceAction::Status::ErrFrameworkFault, tr("Unable to locate source for account"), it.key());
            return false;
        }
    }

    QMailStore::instance()->setRetrievalInProgress(_retrievalAccountIds.toList());
    return true;
}

void ServiceHandler::retrieveMessagePart(quint64 action, const QMailMessagePart::Location &partLocation)
{
    QSet<QMailAccountId> accountIds(messageAccount(partLocation.containingMessageId()));
    QSet<QMailMessageService*> sources(sourceServiceSet(accountIds));
    if (sources.isEmpty()) {
        reportFailure(action, QMailServiceAction::Status::ErrNoConnection, tr("Unable to retrieve message part for unconfigured account"));
    } else {
        enqueueRequest(action, serialize(*accountIds.begin(), partLocation), sources, &ServiceHandler::dispatchRetrieveMessagePart, &ServiceHandler::retrievalCompleted);
    }
}

bool ServiceHandler::dispatchRetrieveMessagePart(quint64 action, const QByteArray &data)
{
    QMailAccountId accountId;
    QMailMessagePart::Location partLocation;

    deserialize(data, accountId, partLocation);

    if (QMailMessageSource *source = accountSource(accountId)) {
        if (!source->retrieveMessagePart(partLocation)) {
            qMailLog(Messaging) << "Unable to service request to retrieve part for message:" << partLocation.containingMessageId();
            return false;
        } else {
            // This account is now retrieving
            setRetrievalInProgress(accountId, true);
        }
    } else {
        reportFailure(action, QMailServiceAction::Status::ErrFrameworkFault, tr("Unable to locate source for account"), accountId);
        return false;
    }

    return true;
}

void ServiceHandler::retrieveMessageRange(quint64 action, const QMailMessageId &messageId, uint minimum)
{
    QSet<QMailAccountId> accountIds(messageAccount(messageId));
    QSet<QMailMessageService*> sources(sourceServiceSet(accountIds));
    if (sources.isEmpty()) {
        reportFailure(action, QMailServiceAction::Status::ErrNoConnection, tr("Unable to retrieve message range for unconfigured account"));
    } else {
        enqueueRequest(action, serialize(*accountIds.begin(), messageId, minimum), sources, &ServiceHandler::dispatchRetrieveMessageRange, &ServiceHandler::retrievalCompleted);
    }
}

bool ServiceHandler::dispatchRetrieveMessageRange(quint64 action, const QByteArray &data)
{
    QMailAccountId accountId;
    QMailMessageId messageId;
    uint minimum;

    deserialize(data, accountId, messageId, minimum);

    if (QMailMessageSource *source = accountSource(accountId)) {
        if (!source->retrieveMessageRange(messageId, minimum)) {
            qMailLog(Messaging) << "Unable to service request to retrieve range:" << minimum << "for message:" << messageId;
            return false;
        } else {
            // This account is now retrieving
            setRetrievalInProgress(accountId, true);
        }
    } else {
        reportFailure(action, QMailServiceAction::Status::ErrFrameworkFault, tr("Unable to locate source for account"), accountId);
        return false;
    }

    return true;
}

void ServiceHandler::retrieveMessagePartRange(quint64 action, const QMailMessagePart::Location &partLocation, uint minimum)
{
    QSet<QMailAccountId> accountIds(messageAccount(partLocation.containingMessageId()));
    QSet<QMailMessageService*> sources(sourceServiceSet(accountIds));
    if (sources.isEmpty()) {
        reportFailure(action, QMailServiceAction::Status::ErrNoConnection, tr("Unable to retrieve message part range for unconfigured account"));
    } else {
        enqueueRequest(action, serialize(*accountIds.begin(), partLocation, minimum), sources, &ServiceHandler::dispatchRetrieveMessagePartRange, &ServiceHandler::retrievalCompleted);
    }
}

bool ServiceHandler::dispatchRetrieveMessagePartRange(quint64 action, const QByteArray &data)
{
    QMailAccountId accountId;
    QMailMessagePart::Location partLocation;
    uint minimum;

    deserialize(data, accountId, partLocation, minimum);

    if (QMailMessageSource *source = accountSource(accountId)) {
        if (!source->retrieveMessagePartRange(partLocation, minimum)) {
            qMailLog(Messaging) << "Unable to service request to retrieve range:" << minimum << "for part in message:" << partLocation.containingMessageId();
            return false;
        } else {
            // This account is now retrieving
            setRetrievalInProgress(accountId, true);
        }
    } else {
        reportFailure(action, QMailServiceAction::Status::ErrFrameworkFault, tr("Unable to locate source for account"), accountId);
        return false;
    }

    return true;
}

void ServiceHandler::retrieveAll(quint64 action, const QMailAccountId &accountId)
{
    QSet<QMailMessageService*> sources(sourceServiceSet(accountId));
    if (sources.isEmpty()) {
        reportFailure(action, QMailServiceAction::Status::ErrNoConnection, tr("Unable to retrieve all messages for unconfigured account"));
    } else {
        enqueueRequest(action, serialize(accountId), sources, &ServiceHandler::dispatchRetrieveAll, &ServiceHandler::retrievalCompleted);
    }
}

bool ServiceHandler::dispatchRetrieveAll(quint64 action, const QByteArray &data)
{
    QMailAccountId accountId;

    deserialize(data, accountId);

    if (QMailMessageSource *source = accountSource(accountId)){
        if (!source->retrieveAll(accountId)) {
            qMailLog(Messaging) << "Unable to service request to retrieve all messages for account:" << accountId;
            return false;
        } else {
            // This account is now retrieving
            setRetrievalInProgress(accountId, true);
        }
    } else {
        reportFailure(action, QMailServiceAction::Status::ErrFrameworkFault, tr("Unable to locate source for account"), accountId);
        return false;
    }

    return true;
}

void ServiceHandler::exportUpdates(quint64 action, const QMailAccountId &accountId)
{
    QSet<QMailMessageService*> sources(sourceServiceSet(accountId));
    if (sources.isEmpty()) {
        reportFailure(action, QMailServiceAction::Status::ErrNoConnection, tr("Unable to export updates for unconfigured account"));
    } else {
        enqueueRequest(action, serialize(accountId), sources, &ServiceHandler::dispatchExportUpdates, &ServiceHandler::retrievalCompleted);
    }
}

bool ServiceHandler::dispatchExportUpdates(quint64 action, const QByteArray &data)
{
    QMailAccountId accountId;

    deserialize(data, accountId);

    if (QMailMessageSource *source = accountSource(accountId)) {
        if (!source->exportUpdates(accountId)) {
            qMailLog(Messaging) << "Unable to service request to export updates for account:" << accountId;
            return false;
        }
    } else {
        reportFailure(action, QMailServiceAction::Status::ErrFrameworkFault, tr("Unable to locate source for account"), accountId);
        return false;
    }

    return true;
}

void ServiceHandler::synchronize(quint64 action, const QMailAccountId &accountId)
{
    QSet<QMailMessageService*> sources(sourceServiceSet(accountId));
    if (sources.isEmpty()) {
        reportFailure(action, QMailServiceAction::Status::ErrNoConnection, tr("Unable to synchronize unconfigured account"));
    } else {
        enqueueRequest(action, serialize(accountId), sources, &ServiceHandler::dispatchSynchronize, &ServiceHandler::retrievalCompleted);
    }
}

bool ServiceHandler::dispatchSynchronize(quint64 action, const QByteArray &data)
{
    QMailAccountId accountId;

    deserialize(data, accountId);

    if (QMailMessageSource *source = accountSource(accountId)) {
        if (!source->synchronize(accountId)) {
            qMailLog(Messaging) << "Unable to service request to synchronize account:" << accountId;
            return false;
        } else {
            // This account is now retrieving
            setRetrievalInProgress(accountId, true);
        }
    } else {
        reportFailure(action, QMailServiceAction::Status::ErrFrameworkFault, tr("Unable to locate source for account"), accountId);
        return false;
    }

    return true;
}

void ServiceHandler::deleteMessages(quint64 action, const QMailMessageIdList& messageIds, QMailStore::MessageRemovalOption option)
{
    QSet<QMailMessageService*> sources;

    if (option == QMailStore::NoRemovalRecord) {
        // Delete these records locally without involving the source
        enqueueRequest(action, serialize(messageIds), sources, &ServiceHandler::dispatchDiscardMessages, &ServiceHandler::storageActionCompleted);
    } else {
        QMap<QMailAccountId, QMailMessageIdList> messageLists(accountMessages(messageIds));
        sources = sourceServiceSet(messageLists.keys().toSet());
        if (sources.isEmpty()) {
            reportFailure(action, QMailServiceAction::Status::ErrNoConnection, tr("Unable to delete messages for unconfigured account"));
        } else {
            enqueueRequest(action, serialize(messageLists), sources, &ServiceHandler::dispatchDeleteMessages, &ServiceHandler::storageActionCompleted);
        }
    }
}

bool ServiceHandler::dispatchDiscardMessages(quint64 action, const QByteArray &data)
{
    QMailMessageIdList messageIds;

    deserialize(data, messageIds);
    if (messageIds.isEmpty())
        return false;

    uint progress = 0;
    uint total = messageIds.count();

    emit progressChanged(action, progress, total);

    // Just delete all these messages
    if (!QMailStore::instance()->removeMessages(QMailMessageKey::id(messageIds), QMailStore::NoRemovalRecord)) {
        qMailLog(Messaging) << "Unable to service request to discard messages";

        reportFailure(action, QMailServiceAction::Status::ErrEnqueueFailed, tr("Unable to discard messages"));
        return false;
    }

    emit progressChanged(action, total, total);
    emit activityChanged(action, QMailServiceAction::Successful);
    return true;
}

bool ServiceHandler::dispatchDeleteMessages(quint64 action, const QByteArray &data)
{
    QMap<QMailAccountId, QMailMessageIdList> messageLists;

    deserialize(data, messageLists);

    QMap<QMailAccountId, QMailMessageIdList>::const_iterator it = messageLists.begin(), end = messageLists.end();
    for ( ; it != end; ++it) {
        if (QMailMessageSource *source = accountSource(it.key())) {
            if (!source->deleteMessages(it.value())) {
                qMailLog(Messaging) << "Unable to service request to delete messages for account:" << it.key();
                return false;
            }
        } else {
            reportFailure(action, QMailServiceAction::Status::ErrFrameworkFault, tr("Unable to locate source for account"), it.key());
            return false;
        }
    }

    return true;
}

void ServiceHandler::copyMessages(quint64 action, const QMailMessageIdList& messageIds, const QMailFolderId &destination)
{
    QSet<QMailAccountId> accountIds(folderAccount(destination));

    if (accountIds.isEmpty()) {
        // Copy to a non-account folder
        QSet<QMailMessageService*> sources;
        enqueueRequest(action, serialize(messageIds, destination), sources, &ServiceHandler::dispatchCopyToLocal, &ServiceHandler::storageActionCompleted);
    } else {
        QSet<QMailMessageService*> sources(sourceServiceSet(accountIds));
        if (sources.isEmpty()) {
            reportFailure(action, QMailServiceAction::Status::ErrNoConnection, tr("Unable to copy messages to unconfigured account"));
        } else if (sources.count() > 1) {
            reportFailure(action, QMailServiceAction::Status::ErrFrameworkFault, tr("Unable to copy messages to multiple destination accounts!"));
        } else {
            enqueueRequest(action, serialize(messageIds, destination), sources, &ServiceHandler::dispatchCopyMessages, &ServiceHandler::storageActionCompleted);
        }
    }
}

bool ServiceHandler::dispatchCopyMessages(quint64 action, const QByteArray &data)
{
    QMailMessageIdList messageIds;
    QMailFolderId destination;

    deserialize(data, messageIds, destination);
    if (messageIds.isEmpty())
        return false;

    QSet<QMailAccountId> accountIds(folderAccount(destination));

    if (accountIds.count() != 1) {
        reportFailure(action, QMailServiceAction::Status::ErrNoConnection, tr("Unable to copy messages to unconfigured account"));
        return false;
    } else {
        QMailAccountId accountId(*accountIds.begin());
        if (QMailMessageSource *source = accountSource(accountId)) {
            if (!source->copyMessages(messageIds, destination)) {
                qMailLog(Messaging) << "Unable to service request to copy messages to folder:" << destination << "for account:" << accountId;
                return false;
            }
        } else {
            reportFailure(action, QMailServiceAction::Status::ErrFrameworkFault, tr("Unable to locate source for account"), accountId);
            return false;
        }
    }

    return true;
}

bool ServiceHandler::dispatchCopyToLocal(quint64 action, const QByteArray &data)
{
    QMailMessageIdList messageIds;
    QMailFolderId destination;

    deserialize(data, messageIds, destination);
    if (messageIds.isEmpty())
        return false;

    uint progress = 0;
    uint total = messageIds.count();

    emit progressChanged(action, progress, total);

    // Create a copy of each message
    foreach (const QMailMessageId id, messageIds) {
        QMailMessage message(id);

        message.setId(QMailMessageId());
        message.setContentIdentifier(QString());
        message.setParentFolderId(destination);

        if (!QMailStore::instance()->addMessage(&message)) {
            qMailLog(Messaging) << "Unable to service request to copy messages to folder:" << destination << "for account:" << message.parentAccountId();

            reportFailure(action, QMailServiceAction::Status::ErrFrameworkFault, tr("Unable to copy messages for account"), message.parentAccountId());
            return false;
        } else {
            emit progressChanged(action, ++progress, total);
        }
    }

    emit activityChanged(action, QMailServiceAction::Successful);
    return true;
}

void ServiceHandler::moveMessages(quint64 action, const QMailMessageIdList& messageIds, const QMailFolderId &destination)
{
    QSet<QMailMessageService*> sources;

    if (destination == QMailFolderId(QMailFolder::TrashFolder)) {
        // Special case - move to local Trash folder.  
        // Note - if the account has configured an external trash folder, the above test is not true
        enqueueRequest(action, serialize(messageIds, destination), sources, &ServiceHandler::dispatchMoveToTrash, &ServiceHandler::storageActionCompleted);
    } else {
        QMap<QMailAccountId, QMailMessageIdList> messageLists(accountMessages(messageIds));
        sources = sourceServiceSet(messageLists.keys().toSet());
        if (sources.isEmpty()) {
            reportFailure(action, QMailServiceAction::Status::ErrNoConnection, tr("Unable to move messages for unconfigured account"));
        } else {
            enqueueRequest(action, serialize(messageLists, destination), sources, &ServiceHandler::dispatchMoveMessages, &ServiceHandler::storageActionCompleted);
        }
    }
}

bool ServiceHandler::dispatchMoveMessages(quint64 action, const QByteArray &data)
{
    QMap<QMailAccountId, QMailMessageIdList> messageLists;
    QMailFolderId destination;

    deserialize(data, messageLists, destination);

    QMap<QMailAccountId, QMailMessageIdList>::const_iterator it = messageLists.begin(), end = messageLists.end();
    for ( ; it != end; ++it) {
        if (QMailMessageSource *source = accountSource(it.key())) {
            if (!source->moveMessages(it.value(), destination)) {
                qMailLog(Messaging) << "Unable to service request to move messages to folder:" << destination << "for account:" << it.key();
                return false;
            }
        } else {
            reportFailure(action, QMailServiceAction::Status::ErrFrameworkFault, tr("Unable to locate source for account"), it.key());
            return false;
        }
    }

    return true;
}

bool ServiceHandler::dispatchMoveToTrash(quint64 action, const QByteArray &data)
{
    QMailMessageIdList messageIds;
    QMailFolderId destination;

    deserialize(data, messageIds, destination);
    if (messageIds.isEmpty())
        return false;

    emit progressChanged(action, 0, messageIds.count());

    // Move these messages logically, but don't tell the source to physically move them
    QMailMessageMetaData metaData;
    metaData.setParentFolderId(destination);

    QMailMessageKey idsKey(QMailMessageKey::id(messageIds));
    if (QMailStore::instance()->updateMessagesMetaData(idsKey, QMailMessageKey::ParentFolderId, metaData)) {
        // Mark these messages as Trash messages
        if (QMailStore::instance()->updateMessagesMetaData(idsKey, QMailMessage::Trash, true)) {
            emit progressChanged(action, messageIds.count(), messageIds.count());
            emit activityChanged(action, QMailServiceAction::Successful);
            return true;
        }
    }

    reportFailure(action, QMailServiceAction::Status::ErrFrameworkFault, tr("Unable to move messages to Trash folder"));
    return false;
}

void ServiceHandler::searchMessages(quint64 action, const QMailMessageKey& filter, const QString& bodyText, QMailSearchAction::SearchSpecification spec, const QMailMessageSortKey &sort)
{
    if (spec == QMailSearchAction::Remote) {
        // Find the accounts that we need to search within from the criteria
        QSet<QMailAccountId> searchAccountIds(keyAccounts(filter, sourceMap.keys().toSet()));

        QSet<QMailMessageService*> sources(sourceServiceSet(searchAccountIds));
        if (sources.isEmpty()) {
            reportFailure(action, QMailServiceAction::Status::ErrNoConnection, tr("Unable to search messages for unconfigured account"));
        } else {
            enqueueRequest(action, serialize(searchAccountIds, filter, bodyText, sort), sources, &ServiceHandler::dispatchSearchMessages, &ServiceHandler::remoteSearchCompleted);
        }
    } else {
        // Find the messages that match the filter criteria
        QMailMessageIdList searchIds = QMailStore::instance()->queryMessages(filter, sort);

        // Schedule this search
        mSearches.append(MessageSearch(action, searchIds, bodyText));
        QTimer::singleShot(0, this, SLOT(continueSearch()));
    }
}

bool ServiceHandler::dispatchSearchMessages(quint64 action, const QByteArray &data)
{
    QMailAccountIdList accountIds;
    QMailMessageKey filter;
    QString bodyText;
    QMailMessageSortKey sort;

    deserialize(data, filter, bodyText, sort);

    foreach (const QMailAccountId &accountId, accountIds) {
        if (QMailMessageSource *source = accountSource(accountId)) {
            if (!source->searchMessages(filter, bodyText, sort)) {
                qMailLog(Messaging) << "Unable to service request to search messages for account:" << accountId;
                return false;
            }
        } else {
            reportFailure(action, QMailServiceAction::Status::ErrFrameworkFault, tr("Unable to locate source for account"), accountId);
            return false;
        }
    }

    // Find the messages that match the filter criteria
    QMailMessageIdList searchIds = QMailStore::instance()->queryMessages(filter, sort);

    // Schedule this search
    mSearches.append(MessageSearch(action, searchIds, bodyText));
    QTimer::singleShot(0, this, SLOT(continueSearch()));

    return true;
}

void ServiceHandler::cancelSearch(quint64 action)
{
    QList<MessageSearch>::iterator it = mSearches.begin(), end = mSearches.end();
    for ( ; it != end; ++it) {
        if ((*it).action() == action) {
            bool currentSearch(it == mSearches.begin());

            if (currentSearch && !mMatchingIds.isEmpty())
                emit matchingMessageIds(action, mMatchingIds);

            emit searchCompleted(action);

            mSearches.erase(it);

            if (currentSearch) {
                // Tell the sources to stop searching also
                cancelTransfer(action);
            }
            return;
        }
    }
}

void ServiceHandler::shutdown()
{
    QTimer::singleShot(0,qApp,SLOT(quit()));
}

void ServiceHandler::protocolRequest(quint64 action, const QMailAccountId &accountId, const QString &request, const QVariant &data)
{
    QSet<QMailMessageService*> sources(sourceServiceSet(accountId));
    if (sources.isEmpty()) {
        reportFailure(action, QMailServiceAction::Status::ErrNoConnection, tr("Unable to forward protocol-specific request for unconfigured account"));
    } else {
        enqueueRequest(action, serialize(accountId, request, data), sources, &ServiceHandler::dispatchProtocolRequest, &ServiceHandler::protocolRequestCompleted);
    }
}

bool ServiceHandler::dispatchProtocolRequest(quint64 action, const QByteArray &data)
{
    QMailAccountId accountId;
    QString request;
    QVariant requestData;

    deserialize(data, accountId, request, requestData);

    if (QMailMessageSource *source = accountSource(accountId)) {
        if (!source->protocolRequest(accountId, request, requestData)) {
            qMailLog(Messaging) << "Unable to service request to forward protocol-specific request to account:" << accountId;
            return false;
        }
    } else {
        reportFailure(action, QMailServiceAction::Status::ErrFrameworkFault, tr("Unable to locate source for account"), accountId);
        return false;
    }

    return true;
}

void ServiceHandler::messagesDeleted(const QMailMessageIdList &messageIds)
{
    if (quint64 action = sourceAction(qobject_cast<QMailMessageSource*>(sender())))
        emit messagesDeleted(action, messageIds);
}

void ServiceHandler::messagesCopied(const QMailMessageIdList &messageIds)
{
    if (quint64 action = sourceAction(qobject_cast<QMailMessageSource*>(sender())))
        emit messagesCopied(action, messageIds);
}

void ServiceHandler::messagesMoved(const QMailMessageIdList &messageIds)
{
    if (quint64 action = sourceAction(qobject_cast<QMailMessageSource*>(sender())))
        emit messagesMoved(action, messageIds);
}

void ServiceHandler::messagesPrepared(const QMailMessageIdList &messageIds)
{
    // TODO: transmit messages after preparation
    Q_UNUSED(messageIds)
}

void ServiceHandler::matchingMessageIds(const QMailMessageIdList &messageIds)
{
    if (quint64 action = sourceAction(qobject_cast<QMailMessageSource*>(sender())))
        emit matchingMessageIds(action, messageIds);
}

void ServiceHandler::protocolResponse(const QString &response, const QVariant &data)
{
    if (quint64 action = sourceAction(qobject_cast<QMailMessageSource*>(sender())))
        emit protocolResponse(action, response, data);
}

void ServiceHandler::messagesTransmitted(const QMailMessageIdList &messageIds)
{
    if (quint64 action = sinkAction(qobject_cast<QMailMessageSink*>(sender())))
        emit messagesTransmitted(action, messageIds);
}

void ServiceHandler::availabilityChanged(bool available)
{
    if (QMailMessageService *service = qobject_cast<QMailMessageService*>(sender())) {
        if (available) {
            mUnavailableServices.remove(service);

            // See if there are pending requests that can now be dispatched
            QTimer::singleShot(0, this, SLOT(dispatchRequest()));
        } else {
            mUnavailableServices.insert(service);
        }
    }
}

void ServiceHandler::connectivityChanged(QMailServiceAction::Connectivity connectivity)
{
    if (QMailMessageService *service = qobject_cast<QMailMessageService*>(sender()))
        if (quint64 action = serviceAction(service)) {
            updateAction(action);
            emit connectivityChanged(action, connectivity);
        }
}

void ServiceHandler::activityChanged(QMailServiceAction::Activity activity)
{
    if (QMailMessageService *service = qobject_cast<QMailMessageService*>(sender()))
        if (quint64 action = serviceAction(service)) {
            updateAction(action);
            emit activityChanged(action, activity);
        }
}

void ServiceHandler::statusChanged(const QMailServiceAction::Status status)
{
    if (QMailMessageService *service = qobject_cast<QMailMessageService*>(sender()))
        if (quint64 action = serviceAction(service)) {
            updateAction(action);
            reportFailure(action, status);
        }
}

void ServiceHandler::progressChanged(uint progress, uint total)
{
    if (QMailMessageService *service = qobject_cast<QMailMessageService*>(sender()))
        if (quint64 action = serviceAction(service)) {
            updateAction(action);
            emit progressChanged(action, progress, total);
        }
}

void ServiceHandler::actionCompleted(bool success)
{
    if (QMailMessageService *service = qobject_cast<QMailMessageService*>(sender())) {
        if (quint64 action = serviceAction(service)) {
            QMap<quint64, ActionData>::iterator it = mActiveActions.find(action);
            if (it != mActiveActions.end()) {
                ActionData &data(it.value());

                QSet<QMailMessageService*>::iterator sit = data.services.find(service);
                if (sit != data.services.end()) {
                    // Remove this service from the set for the action
                    data.services.erase(sit);

                    // This account is no longer retrieving/transmitting
                    QMailAccountId accountId(service->accountId());
                    if (accountId.isValid()) {
                        if (data.completion == &ServiceHandler::retrievalCompleted) {
                            setRetrievalInProgress(accountId, false);
                        } else if (data.completion == &ServiceHandler::transmissionCompleted) {
                            setTransmissionInProgress(accountId, false);
                        } 
                    }

                    if (success) {
                        if (data.services.isEmpty() && (data.reported == false)) {
                            // Report success
                            emit (this->*data.completion)(action);
                            data.reported = true;
                        }
                    } else {
                        // This action has failed - mark it so that it can't be reported as successful
                        data.reported = true;
                    }
                }

                if (data.services.isEmpty()) {
                    // This action is finished
                    mActiveActions.erase(it);
                }
            }

            if (_outstandingRequests.contains(action)) {
                _outstandingRequests.remove(action);

                // Ensure this request is no longer in the file
                _requestsFile.resize(0);
                foreach (quint64 req, _outstandingRequests) {
                    QByteArray requestNumber(QByteArray::number(req));
                    _requestsFile.write(requestNumber.append("\n"));
                }
                _requestsFile.flush();
            }

            mServiceAction.remove(service);
        }
    }

    // See if there are pending requests
    QTimer::singleShot(0, this, SLOT(dispatchRequest()));
}

void ServiceHandler::reportFailure(quint64 action, QMailServiceAction::Status::ErrorCode code, const QString &text, const QMailAccountId &accountId, const QMailFolderId &folderId, const QMailMessageId &messageId)
{
    reportFailure(action, QMailServiceAction::Status(code, text, accountId, folderId, messageId));
}

void ServiceHandler::reportFailure(quint64 action, const QMailServiceAction::Status status)
{
    emit statusChanged(action, status);

    // Treat all errors as failures
    if (status.errorCode != QMailServiceAction::Status::ErrNoError)
        emit activityChanged(action, QMailServiceAction::Failed);
}

void ServiceHandler::continueSearch()
{
    if (!mSearches.isEmpty()) {
        MessageSearch &currentSearch(mSearches.first());

        if (currentSearch.pending()) {
            mMatchingIds.clear();
            currentSearch.inProgress();

            if (!currentSearch.isEmpty()) {
                emit progressChanged(currentSearch.action(), 0, currentSearch.total());
            }
        }

        if (!currentSearch.isEmpty()) {
            foreach (const QMailMessageId &id, currentSearch.takeBatch()) {
                QMailMessage message(id);
                QString text(currentSearch.bodyText());
                if (text.isEmpty() || messageBodyContainsText(message, text))
                    mMatchingIds.append(id);
            }

            emit progressChanged(currentSearch.action(), currentSearch.progress(), currentSearch.total());
        }

        if (currentSearch.isEmpty()) {
            // Nothing more to search - we're finished
            emit matchingMessageIds(currentSearch.action(), mMatchingIds);
            emit searchCompleted(currentSearch.action());

            if (mActiveActions.contains(currentSearch.action())) {
                // There is remote searching in progress - wait for completion
            } else {
                // We're finished with this search
                mSearches.takeFirst();

                if (!mSearches.isEmpty())
                    QTimer::singleShot(0, this, SLOT(continueSearch()));
            }
        } else {
            QTimer::singleShot(0, this, SLOT(continueSearch()));
        }
    }
}

void ServiceHandler::finaliseSearch(quint64 action)
{
    if (!mSearches.isEmpty()) {
        qWarning() << "Remote search complete but none pending!" << action;
    } else {
        MessageSearch &currentSearch(mSearches.first());

        if (currentSearch.action() != action) {
            qWarning() << "Remote search complete but not current!" << action;
        } else {
            if (currentSearch.isEmpty()) {
                // This search is now finished
                emit searchCompleted(currentSearch.action());

                mSearches.takeFirst();

                if (!mSearches.isEmpty())
                    QTimer::singleShot(0, this, SLOT(continueSearch()));
            }
        }
    }
}

void ServiceHandler::reportFailures()
{
    // We have no active accounts at this point
    QMailStore::instance()->setRetrievalInProgress(QMailAccountIdList());
    QMailStore::instance()->setTransmissionInProgress(QMailAccountIdList());

    while (!_failedRequests.isEmpty()) {
        quint64 action(_failedRequests.takeFirst());
        reportFailure(action, QMailServiceAction::Status::ErrFrameworkFault, tr("Failed to perform requested action!"));
    }
}

void ServiceHandler::setRetrievalInProgress(const QMailAccountId &accountId, bool inProgress)
{
    bool modified(false);

    if (inProgress && !_retrievalAccountIds.contains(accountId)) {
        _retrievalAccountIds.insert(accountId);
        modified = true;
    } else if (!inProgress && _retrievalAccountIds.contains(accountId)) {
        _retrievalAccountIds.remove(accountId);
        modified = true;
    }

    if (modified) {
        QMailStore::instance()->setRetrievalInProgress(_retrievalAccountIds.toList());
    }
}

void ServiceHandler::setTransmissionInProgress(const QMailAccountId &accountId, bool inProgress)
{
    bool modified(false);

    if (inProgress && !_transmissionAccountIds.contains(accountId)) {
        _transmissionAccountIds.insert(accountId);
        modified = true;
    } else if (!inProgress && _transmissionAccountIds.contains(accountId)) {
        _transmissionAccountIds.remove(accountId);
        modified = true;
    }

    if (modified) {
        QMailStore::instance()->setTransmissionInProgress(_transmissionAccountIds.toList());
    }
}


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


#include "servicehandler.h"
#include <longstream_p.h>
#include <QDataStream>
#include <QIODevice>
#include <qmailmessageserver.h>
#include <qmailserviceconfiguration.h>
#include <qmailstore.h>
#include <qmaillog.h>
#include <QCoreApplication>
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
    QSet<QMailAccountId> accountIds; // accounts that own these messages

    foreach (const QMailMessageMetaData &metaData, QMailStore::instance()->messagesMetaData(
                                                       QMailMessageKey::id(ids),
                                                       QMailMessageKey::ParentAccountId,
                                                       QMailStore::ReturnDistinct))
    {
        if (metaData.parentAccountId().isValid())
            accountIds.insert(metaData.parentAccountId());
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

        foreach (const QMailMessageMetaData &metaData, QMailStore::instance()->messagesMetaData(QMailMessageKey::id(ids),
                                                                                                QMailMessageKey::Id | QMailMessageKey::ParentAccountId, 
                                                                                                QMailStore::ReturnAll)) {
            if (metaData.id().isValid() && metaData.parentAccountId().isValid())
                map[metaData.parentAccountId()].append(metaData.id());
        }


    return map;
}

namespace {
    quint64 newLocalActionId() {
        static quint64 id(QCoreApplication::applicationPid() << 32);
        return ++id;
    }

struct ResolverSet
{
    QMap<QMailAccountId, QList<QPair<QMailMessagePart::Location, QMailMessagePart::Location> > > map;

    void append(const QMailMessageMetaData &metaData)
    {
        QMailMessagePart::Location location;
        location.setContainingMessageId(metaData.id());

        map[metaData.parentAccountId()].append(qMakePair(location, QMailMessagePart::Location()));
    }

    bool operator()(const QMailMessagePart &part)
    {
        if ((part.referenceType() != QMailMessagePart::None) && part.referenceResolution().isEmpty()) {
            // We need to resolve this part's reference
            if (part.referenceType() == QMailMessagePart::MessageReference) {
                // Link this message to the referenced message
                QMailMessageMetaData referencedMessage(part.messageReference());

                QMailMessagePart::Location location;
                location.setContainingMessageId(referencedMessage.id());
                map[referencedMessage.parentAccountId()].append(qMakePair(location, part.location()));
            } else if (part.referenceType() == QMailMessagePart::PartReference) {
                // Link this message to the referenced part's location
                QMailMessageMetaData referencedMessage(part.partReference().containingMessageId());

                QMailMessagePart::Location location(part.partReference());
                map[referencedMessage.parentAccountId()].append(qMakePair(location, part.location()));
            }
        }

        return true;
    }
};

}

QMap<QMailAccountId, QList<QPair<QMailMessagePart::Location, QMailMessagePart::Location> > > messageResolvers(const QMailMessageIdList &ids)
{
    // Find all the unresolved references in these messages
    ResolverSet set;

    foreach (const QMailMessageId id, ids) {
        QMailMessage outgoing(id);

        if (outgoing.status() & QMailMessage::HasUnresolvedReferences) {
            // Record the locations of parts needed for this message
            outgoing.foreachPart<ResolverSet&>(set);
        }
        if (outgoing.status() & QMailMessage::TransmitFromExternal) {
            // Just record this message's location
            set.append(outgoing);
        }
    }

    return set.map;
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

            //TODO: handle ranges
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

namespace {

struct TextPartSearcher
{
    QString text;

    TextPartSearcher(const QString &text) : text(text) {}

    bool operator()(const QMailMessagePart &part)
    {
        if (part.contentType().type().toLower() == "text") {
            if (part.body().data().contains(text, Qt::CaseInsensitive)) {
                return false;
            }
        }

        // Keep searching
        return true;
    }
};

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
        TextPartSearcher searcher(text);
        if (message.foreachPart<TextPartSearcher&>(searcher) == false) {
            return true;
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

    // See if there are any requests remaining from our previous run
    if (_requestsFile.exists()) {
        if (!_requestsFile.open(QIODevice::ReadOnly)) {
            qWarning() << "Unable to open requests file for read!";
        } else {
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
        service->cancelOperation(QMailServiceAction::Status::ErrInternalStateReset, tr("Destroying Service handler"));
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

void ServiceHandler::removeServiceFromActions(QMailMessageService *removeService)
{
    if (removeService == NULL)
        return;

    for (QMap<quint64, ActionData>::iterator it(mActiveActions.begin()) ; it != mActiveActions.end(); ++it) {
       if (it->services.remove(removeService)) {
           qMailLog(Messaging) << "Removed service from action";
       }
    }

    for (QList<Request>::iterator it(mRequests.begin()); it != mRequests.end();) {
        if (it->services.contains(removeService) || it->preconditions.contains(removeService))
        {
            reportFailure(it->action, QMailServiceAction::Status::ErrFrameworkFault, tr("Service became unavailable, couldn't dispatch"));
            it = mRequests.erase(it);
            continue;
        }
        ++it;
    }
}

void ServiceHandler::deregisterAccountServices(const QMailAccountIdList &ids, QMailServiceAction::Status::ErrorCode code, const QString &text)
{
    QMap<QPair<QMailAccountId, QString>, QPointer<QMailMessageService> >::iterator it = serviceMap.begin();
    while (it != serviceMap.end()) {
        if (ids.contains(it.key().first)) {
            // Remove any services associated with this account
            if (QMailMessageService *service = it.value()) {
                qMailLog(Messaging) << "Deregistering service:" << service->service() << "for account:" << it.key().first;
                service->cancelOperation(code, text);
                removeServiceFromActions(service);
                delete service;
            }

            it = serviceMap.erase(it);
        } else {
            ++it;
        }
    }

    foreach (const QMailAccountId &id, ids) {
        // Remove trailing references to this account's services
        QMap<QMailAccountId, QMailAccountId>::iterator it = masterAccount.find(id);
        if (it != masterAccount.end()) {
            masterAccount.erase(it);
        } else {
            if (QMailMessageSource *source = accountSource(id))
                sourceService.remove(source);

            if (QMailMessageSink *sink = accountSink(id))
                sinkService.remove(sink);

            sourceMap.remove(id);
            sinkMap.remove(id);
        }
    }
}

void ServiceHandler::reregisterAccountServices(const QMailAccountIdList &ids, QMailServiceAction::Status::ErrorCode code, const QString &text)
{
    // Remove and re-create these accounts' services
    QMailAccountIdList reregisterIds;
    QMap<QPair<QMailAccountId, QString>,  QPointer<QMailMessageService> >::iterator it = serviceMap.begin();
    while (it != serviceMap.end()) {
        if (ids.contains(it.key().first)) {
           QMailMessageService *service = it.value();
           if (service && service->requiresReregistration()) {
               reregisterIds.append(it.key().first);
           }
        }
        ++it;
    }
    deregisterAccountServices(reregisterIds, code, text);
    registerAccountServices(reregisterIds);
}

void ServiceHandler::accountsAdded(const QMailAccountIdList &ids)
{
    registerAccountServices(ids);
}

void ServiceHandler::accountsUpdated(const QMailAccountIdList &ids)
{
    // Only respond to updates that were generated by other processes
    if (QMailStore::instance()->asynchronousEmission()) {
        reregisterAccountServices(ids, QMailServiceAction::Status::ErrInternalStateReset, tr("Account updated by other process"));
    }
}

void ServiceHandler::accountsRemoved(const QMailAccountIdList &ids)
{
    deregisterAccountServices(ids, QMailServiceAction::Status::ErrInternalStateReset, tr("Account removed"));
    foreach (const QMailAccountId &id, ids) {
        // remove messages from this account
        QMailMessageKey messageKey(QMailMessageKey::parentAccountId(id));
        QMailStore::instance()->removeMessages(messageKey);
        // remove folders from this account
        QMailFolderKey accountKey(QMailFolderKey::parentAccountId(id));
        QMailStore::instance()->removeFolders(accountKey);
    }
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
    connect(source, SIGNAL(messagesFlagged(QMailMessageIdList)), this, SLOT(messagesFlagged(QMailMessageIdList)));
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
    connect(sink, SIGNAL(messagesFailedTransmission(QMailMessageIdList, QMailServiceAction::Status::ErrorCode)), 
            this, SLOT(messagesFailedTransmission(QMailMessageIdList, QMailServiceAction::Status::ErrorCode)));
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
    QMap<QPair<QMailAccountId, QString>, QPointer<QMailMessageService> >::const_iterator it = serviceMap.find(qMakePair(accountId, svcCfg.service()));
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

bool ServiceHandler::servicesAvailable(const Request &req) const
{
    foreach(QPointer<QMailMessageService> service, (req.services + req.preconditions)) {
        if (!serviceAvailable(service))
            return false;
    }
    return true;
}

bool ServiceHandler::serviceAvailable(QPointer<QMailMessageService> service) const
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
        services.insert(sourceService.value(source));
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
            services.insert(sourceService.value(source));
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
    QMap<QPointer<QMailMessageService>, quint64>::const_iterator it = mServiceAction.find(service);
    if (it != mServiceAction.end())
        return it.value();

    return 0;
}

void ServiceHandler::enqueueRequest(quint64 action, const QByteArray &data, const QSet<QMailMessageService*> &services, RequestServicer servicer, CompletionSignal completion, QMailServerRequestType description, const QSet<QMailMessageService*> &preconditions)
{
    QSet<QPointer<QMailMessageService> > safeServices;
    QSet<QPointer<QMailMessageService> > safePreconditions;
    Request req;
    foreach(QMailMessageService *service, services)
        safeServices.insert(service);
    foreach(QMailMessageService *precondition, preconditions)
        safePreconditions.insert(precondition);
    req.action = action;
    req.data = data;
    req.services = safeServices;
    req.preconditions = safePreconditions;
    req.servicer = servicer;
    req.completion = completion;
    req.description = description;

    mRequests.append(req);

    // Add this request to the outstanding list
    if (!_outstandingRequests.contains(action)) {
        _outstandingRequests.insert(action);

        QByteArray requestNumber(QByteArray::number(action));
        _requestsFile.write(requestNumber.append('\n'));
        _requestsFile.flush();
    }

    QTimer::singleShot(0, this, SLOT(dispatchRequest()));
}

void ServiceHandler::dispatchRequest()
{
    QList<Request>::iterator request(mRequests.begin());
    while(request != mRequests.end())
    {
        if (!servicesAvailable(*request)) {
            ++request;
            continue;
        }

        // Associate the services with the action, so that signals are reported correctly
        foreach (QMailMessageService *service, request->services)
            mServiceAction.insert(service, request->action);

        // The services required for this request are available
        ActionData data;
        data.services = request->services;
        data.completion = request->completion;
        data.expiry = QTime::currentTime().addMSecs(ExpiryPeriod);
        data.reported = false;
        data.description = request->description;


        mActiveActions.insert(request->action, data);
        emit actionStarted(qMakePair(request->action, request->description));
        emit activityChanged(request->action, QMailServiceAction::InProgress);

        if ((this->*request->servicer)(request->action, request->data)) {
            // This action is now underway

            if (mActionExpiry.isEmpty()) {
                // Start the expiry timer
                QTimer::singleShot(ExpiryPeriod + 50, this, SLOT(expireAction()));
            }
            mActionExpiry.append(request->action);
        } else {
            mActiveActions.remove(request->action);

            qMailLog(Messaging) << "Unable to dispatch request:" << request->action << "to services:" << request->services;
            emit activityChanged(request->action, QMailServiceAction::Failed);

            foreach (QMailMessageService *service, request->services)
                mServiceAction.remove(service);
        }

        request = mRequests.erase(request);
    }
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

                // Defend against action being removed from mActiveActions when activityChanged signal is emitted
                it = mActiveActions.find(action);
                if (it != mActiveActions.end()) {
                    ActionData &data(it.value());
                    // Remove this action
                    foreach (QMailMessageService *service, data.services) {
                        if (!service)
                            continue;
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
                }

                mActionExpiry.removeFirst();

                // Restart the service(s) for each of these accounts
                reregisterAccountServices(serviceAccounts.toList(), QMailServiceAction::Status::ErrTimeout, tr("Request is not progressing"));

                // See if there are more actions to dispatch
                dispatchRequest();
            }
        }
    }

    QLinkedList<quint64>::iterator expiryIt(mActionExpiry.begin());
    while (expiryIt != mActionExpiry.end()) {
        if (mActiveActions.contains(*expiryIt)) {
            int nextExpiry(QTime::currentTime().msecsTo(mActiveActions.value(*expiryIt).expiry));
            QTimer::singleShot(qMax(nextExpiry+50, 0), this, SLOT(expireAction()));
            return;
        } else {
            expiryIt = mActionExpiry.erase(expiryIt); // Just remove this non-existent action
        }
    }
}

// Cancelled by user
void ServiceHandler::cancelTransfer(quint64 action)
{
    QMap<quint64, ActionData>::iterator it = mActiveActions.find(action);
    if (it != mActiveActions.end()) {
        bool retrievalSetModified(false);
        bool transmissionSetModified(false);

        const ActionData &data(it.value());
        foreach (QMailMessageService *service, data.services) {
            if (service)
                service->cancelOperation(QMailServiceAction::Status::ErrCancel, tr("Cancelled by user"));
            mServiceAction.remove(service);
            if (!service) {
                qMailLog(Messaging) << "Unable to cancel null service for action:" << action;
                continue;
            }

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
        // We need to see if any sources are required to prepare these messages
        QMailMessageKey accountKey(QMailMessageKey::parentAccountId(accountId));
        QMailMessageKey outboxKey(QMailMessageKey::status(QMailMessage::Outbox) & ~QMailMessageKey::status(QMailMessage::Trash));

        // We need to prepare messages to:
        // - resolve references
        // - move to the Sent folder prior to transmission
        quint64 preparationStatus(QMailMessage::HasUnresolvedReferences | QMailMessage::TransmitFromExternal);
        QMailMessageKey unresolvedKey(QMailMessageKey::status(preparationStatus, QMailDataComparator::Includes));

        QSet<QMailMessageService*> sources;

        QMailMessageIdList unresolvedMessages(QMailStore::instance()->queryMessages(accountKey & outboxKey & unresolvedKey));
        if (!unresolvedMessages.isEmpty()) {
            // Find the accounts that own these messages
            QMap<QMailAccountId, QList<QPair<QMailMessagePart::Location, QMailMessagePart::Location> > > unresolvedLists(messageResolvers(unresolvedMessages));

            sources = sourceServiceSet(unresolvedLists.keys().toSet());

            // Emit no signal after completing preparation
            enqueueRequest(action, serialize(unresolvedLists), sources, &ServiceHandler::dispatchPrepareMessages, 0, TransmitMessagesRequestType);
        }

        // The transmit action is dependent on the availability of the sources, since they must complete their preparation step first
        enqueueRequest(action, serialize(accountId), sinks, &ServiceHandler::dispatchTransmitMessages, &ServiceHandler::transmissionCompleted, TransmitMessagesRequestType, sources);
    }
}

bool ServiceHandler::dispatchPrepareMessages(quint64 action, const QByteArray &data)
{
    QMap<QMailAccountId, QList<QPair<QMailMessagePart::Location, QMailMessagePart::Location> > > unresolvedLists;

    deserialize(data, unresolvedLists);

    // Prepare any unresolved messages for transmission
    QMap<QMailAccountId, QList<QPair<QMailMessagePart::Location, QMailMessagePart::Location> > >::const_iterator it = unresolvedLists.begin(), end = unresolvedLists.end();
    for ( ; it != end; ++it) {
        if (QMailMessageSource *source = accountSource(it.key())) {
            if (!source->prepareMessages(it.value())) {
                qMailLog(Messaging) << "Unable to service request to prepare messages for account:" << it.key();
                return false;
            } else {
                // This account is now transmitting
                setTransmissionInProgress(it.key(), true);
            }
        } else {
            reportFailure(action, QMailServiceAction::Status::ErrFrameworkFault, tr("Unable to locate source for account"), it.key());
            return false;
        }
    }

    return true;
}

bool ServiceHandler::dispatchTransmitMessages(quint64 action, const QByteArray &data)
{
    QMailAccountId accountId;

    deserialize(data, accountId);

    if (QMailMessageSink *sink = accountSink(accountId)) {
        // Transmit any messages in the Outbox for this account
        QMailMessageKey accountKey(QMailMessageKey::parentAccountId(accountId));
        QMailMessageKey outboxKey(QMailMessageKey::status(QMailMessage::Outbox) & ~QMailMessageKey::status(QMailMessage::Trash));

        if (!sink->transmitMessages(QMailStore::instance()->queryMessages(accountKey & outboxKey))) {
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
        enqueueRequest(action, serialize(accountId, folderId, descending), sources, &ServiceHandler::dispatchRetrieveFolderListAccount, &ServiceHandler::retrievalCompleted, RetrieveFolderListRequestType);
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
        enqueueRequest(action, serialize(accountId, folderId, minimum, sort), sources, &ServiceHandler::dispatchRetrieveMessageList, &ServiceHandler::retrievalCompleted, RetrieveMessageListRequestType);
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
        enqueueRequest(action, serialize(messageLists, spec), sources, &ServiceHandler::dispatchRetrieveMessages, &ServiceHandler::retrievalCompleted, RetrieveMessagesRequestType);
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
        enqueueRequest(action, serialize(*accountIds.begin(), partLocation), sources, &ServiceHandler::dispatchRetrieveMessagePart, &ServiceHandler::retrievalCompleted, RetrieveMessagePartRequestType);
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
       enqueueRequest(action, serialize(*accountIds.begin(), messageId, minimum), sources, &ServiceHandler::dispatchRetrieveMessageRange, &ServiceHandler::retrievalCompleted, RetrieveMessageRangeRequestType);
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
        enqueueRequest(action, serialize(*accountIds.begin(), partLocation, minimum), sources, &ServiceHandler::dispatchRetrieveMessagePartRange, &ServiceHandler::retrievalCompleted, RetrieveMessagePartRangeRequestType);
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
        enqueueRequest(action, serialize(accountId), sources, &ServiceHandler::dispatchRetrieveAll, &ServiceHandler::retrievalCompleted, RetrieveAllRequestType);
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
        enqueueRequest(action, serialize(accountId), sources, &ServiceHandler::dispatchExportUpdates, &ServiceHandler::retrievalCompleted, ExportUpdatesRequestType);
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
        enqueueRequest(action, serialize(accountId), sources, &ServiceHandler::dispatchSynchronize, &ServiceHandler::retrievalCompleted, SynchronizeRequestType);
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
        enqueueRequest(action, serialize(messageIds), sources, &ServiceHandler::dispatchDiscardMessages, &ServiceHandler::storageActionCompleted, DeleteMessagesRequestType);
    } else {
        QMap<QMailAccountId, QMailMessageIdList> messageLists(accountMessages(messageIds));
        sources = sourceServiceSet(messageLists.keys().toSet());
        if (sources.isEmpty()) {
            reportFailure(action, QMailServiceAction::Status::ErrNoConnection, tr("Unable to delete messages for unconfigured account"));
        } else {
            QString description(tr("Deleting messages"));
            enqueueRequest(action, serialize(messageLists), sources, &ServiceHandler::dispatchDeleteMessages, &ServiceHandler::storageActionCompleted, DeleteMessagesRequestType);
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
        enqueueRequest(action, serialize(messageIds, destination), sources, &ServiceHandler::dispatchCopyToLocal, &ServiceHandler::storageActionCompleted, CopyMessagesRequestType);
    } else {
        QSet<QMailMessageService*> sources(sourceServiceSet(accountIds));
        if (sources.isEmpty()) {
            reportFailure(action, QMailServiceAction::Status::ErrNoConnection, tr("Unable to copy messages to unconfigured account"));
        } else if (sources.count() > 1) {
            reportFailure(action, QMailServiceAction::Status::ErrFrameworkFault, tr("Unable to copy messages to multiple destination accounts!"));
        } else {
            enqueueRequest(action, serialize(messageIds, destination), sources, &ServiceHandler::dispatchCopyMessages, &ServiceHandler::storageActionCompleted, CopyMessagesRequestType);
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

    QMap<QMailAccountId, QMailMessageIdList> messageLists(accountMessages(messageIds));
    sources = sourceServiceSet(messageLists.keys().toSet());
    if (sources.isEmpty()) {
        reportFailure(action, QMailServiceAction::Status::ErrNoConnection, tr("Unable to move messages for unconfigured account"));
    } else {
        enqueueRequest(action, serialize(messageLists, destination), sources, &ServiceHandler::dispatchMoveMessages, &ServiceHandler::storageActionCompleted, MoveMessagesRequestType);
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

void ServiceHandler::flagMessages(quint64 action, const QMailMessageIdList& messageIds, quint64 setMask, quint64 unsetMask)
{
    QSet<QMailMessageService*> sources;

    QMap<QMailAccountId, QMailMessageIdList> messageLists(accountMessages(messageIds));
    sources = sourceServiceSet(messageLists.keys().toSet());
    if (sources.isEmpty()) {
        reportFailure(action, QMailServiceAction::Status::ErrNoConnection, tr("Unable to flag messages for unconfigured account"));
    } else {
        enqueueRequest(action, serialize(messageLists, setMask, unsetMask), sources, &ServiceHandler::dispatchFlagMessages, &ServiceHandler::storageActionCompleted, FlagMessagesRequestType);
    }
}

bool ServiceHandler::dispatchFlagMessages(quint64 action, const QByteArray &data)
{
    QMap<QMailAccountId, QMailMessageIdList> messageLists;
    quint64 setMask;
    quint64 unsetMask;

    deserialize(data, messageLists, setMask, unsetMask);

    QMap<QMailAccountId, QMailMessageIdList>::const_iterator it = messageLists.begin(), end = messageLists.end();
    for ( ; it != end; ++it) {
        if (QMailMessageSource *source = accountSource(it.key())) {
            if (!source->flagMessages(it.value(), setMask, unsetMask)) {
                qMailLog(Messaging) << "Unable to service request to flag messages for account:" << it.key();
                return false;
            }
        } else {
            reportFailure(action, QMailServiceAction::Status::ErrFrameworkFault, tr("Unable to locate source for account"), it.key());
            return false;
        }
    }

    return true;
}

void ServiceHandler::createFolder(quint64 action, const QString &name, const QMailAccountId &accountId, const QMailFolderId &parentId)
{
    if(accountId.isValid()) {
        QSet<QMailAccountId> accounts = folderAccount(parentId);
        QSet<QMailMessageService *> sources(sourceServiceSet(accounts));

        enqueueRequest(action, serialize(name, accountId, parentId), sources, &ServiceHandler::dispatchCreateFolder, &ServiceHandler::storageActionCompleted, CreateFolderRequestType);
    } else {
        reportFailure(action, QMailServiceAction::Status::ErrInvalidData, tr("Unable to create folder for invalid account"));
    }
}

bool ServiceHandler::dispatchCreateFolder(quint64 action, const QByteArray &data)
{
    QString name;
    QMailAccountId accountId;
    QMailFolderId parentId;

    deserialize(data, name, accountId, parentId);

    if(QMailMessageSource *source = accountSource(accountId)) {
        if(source->createFolder(name, accountId, parentId)) {
            return true;
        } else {
            qMailLog(Messaging) << "Unable to service request to create folder for account:" << accountId;
            return false;
        }

    } else {
        reportFailure(action, QMailServiceAction::Status::ErrFrameworkFault, tr("Unable to locate source for account"), accountId);
        return false;
    }
}

void ServiceHandler::renameFolder(quint64 action, const QMailFolderId &folderId, const QString &name)
{
    if(folderId.isValid()) {
        QSet<QMailAccountId> accounts = folderAccount(folderId);
        QSet<QMailMessageService *> sources(sourceServiceSet(accounts));

        enqueueRequest(action, serialize(folderId, name), sources, &ServiceHandler::dispatchRenameFolder, &ServiceHandler::storageActionCompleted, RenameFolderRequestType);
    } else {
        reportFailure(action, QMailServiceAction::Status::ErrInvalidData, tr("Unable to rename invalid folder"));
    }
}

bool ServiceHandler::dispatchRenameFolder(quint64 action, const QByteArray &data)
{
    QMailFolderId folderId;
    QString newFolderName;

    deserialize(data, folderId, newFolderName);

    if(QMailMessageSource *source = accountSource(QMailFolder(folderId).parentAccountId())) {
        if(source->renameFolder(folderId, newFolderName)) {
            return true;
        } else {
            qMailLog(Messaging) << "Unable to service request to rename folder id:" << folderId;
            return false;
        }

    } else {
        reportFailure(action, QMailServiceAction::Status::ErrFrameworkFault, tr("Unable to locate source for account"), QMailFolder(folderId).parentAccountId());
        return false;
    }
}

void ServiceHandler::deleteFolder(quint64 action, const QMailFolderId &folderId)
{
    if(folderId.isValid()) {
        QSet<QMailAccountId> accounts = folderAccount(folderId);
        QSet<QMailMessageService *> sources(sourceServiceSet(accounts));

        enqueueRequest(action, serialize(folderId), sources, &ServiceHandler::dispatchDeleteFolder, &ServiceHandler::storageActionCompleted, DeleteFolderRequestType);
    } else {
        reportFailure(action, QMailServiceAction::Status::ErrInvalidData, tr("Unable to delete invalid folder"));
    }
}

bool ServiceHandler::dispatchDeleteFolder(quint64 action, const QByteArray &data)
{
    QMailFolderId folderId;

    deserialize(data, folderId);

    if(QMailMessageSource *source = accountSource(QMailFolder(folderId).parentAccountId())) {
        if(source->deleteFolder(folderId)) {
            return true;
        } else {
            qMailLog(Messaging) << "Unable to service request to delete folder id:" << folderId;
            return false;
        }
    } else {
        reportFailure(action, QMailServiceAction::Status::ErrFrameworkFault, tr("Unable to locate source for account"), QMailFolder(folderId).parentAccountId());
        return false;
    }
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
            enqueueRequest(action, serialize(searchAccountIds, filter, bodyText, sort), sources, &ServiceHandler::dispatchSearchMessages, &ServiceHandler::searchCompleted, SearchMessagesRequestType);
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
    QSet<QMailAccountId> accountIds;
    QMailMessageKey filter;
    QString bodyText;
    QMailMessageSortKey sort;
    bool sentSearch = false;


    deserialize(data, accountIds, filter, bodyText, sort);

    foreach (const QMailAccountId &accountId, accountIds) {
        if (QMailMessageSource *source = accountSource(accountId)) {
            //only dispatch to appropriate account
            if (source->searchMessages(filter, bodyText, sort)) {
                sentSearch = true; //we've at least sent one
            } else {
                //do it locally instead
                qMailLog(Messaging) << "Unable to do remote search, doing it locally instead";
                mSearches.append(MessageSearch(action, QMailStore::instance()->queryMessages(filter, sort), bodyText));
                QTimer::singleShot(0, this, SLOT(continueSearch()));
            }
        } else {
            reportFailure(action, QMailServiceAction::Status::ErrFrameworkFault, tr("Unable to locate source for account"), accountId);
        }
    }

    return sentSearch;
}

void ServiceHandler::cancelSearch(quint64 action)
{
    QSet<QMailAccountId> accounts(sourceMap.keys().toSet());

    QSet<QMailMessageService*> sources(sourceServiceSet(accounts));
    if (!sources.isEmpty()) {
        enqueueRequest(action, serialize(accounts), sources, &ServiceHandler::dispatchCancelSearch, &ServiceHandler::searchCompleted, CancelSearchRequestType);
    }

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

bool ServiceHandler::dispatchCancelSearch(quint64 action, const QByteArray &data)
{
    QSet<QMailAccountId> accounts;

    deserialize(data, accounts);

    foreach (const QMailAccountId &accountId, accounts) {
        if (QMailMessageSource *source = accountSource(accountId)) {
            source->cancelSearch();
        } else {
            reportFailure(action, QMailServiceAction::Status::ErrFrameworkFault, tr("Unable to locate source for account"), accountId);
            return false;
        }
    }

    return true;
}

void ServiceHandler::shutdown()
{
    QTimer::singleShot(0,QCoreApplication::instance(),SLOT(quit()));
}

void ServiceHandler::listActions()
{
    QMailActionDataList list;

    for(QMap<quint64, ActionData>::iterator i(mActiveActions.begin()) ; i != mActiveActions.end(); ++i)
    {
        QMailActionData t = qMakePair(i.key(), i->description);
        list.append(t);
    }

    emit actionsListed(list);
}

void ServiceHandler::protocolRequest(quint64 action, const QMailAccountId &accountId, const QString &request, const QVariant &data)
{
    QSet<QMailMessageService*> sources(sourceServiceSet(accountId));
    if (sources.isEmpty()) {
        reportFailure(action, QMailServiceAction::Status::ErrNoConnection, tr("Unable to forward protocol-specific request for unconfigured account"));
    } else {
        enqueueRequest(action, serialize(accountId, request, data), sources, &ServiceHandler::dispatchProtocolRequest, &ServiceHandler::protocolRequestCompleted, ProtocolRequestRequestType);
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

void ServiceHandler::messagesFlagged(const QMailMessageIdList &messageIds)
{
    if (quint64 action = sourceAction(qobject_cast<QMailMessageSource*>(sender())))
        emit messagesFlagged(action, messageIds);
}

void ServiceHandler::messagesPrepared(const QMailMessageIdList &messageIds)
{
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
    if (QMailMessageSink *sink = qobject_cast<QMailMessageSink*>(sender())) {
        if (quint64 action = sinkAction(sink)) {
            mSentIds << messageIds;

            emit messagesTransmitted(action, messageIds);
        }
    }
}

void ServiceHandler::messagesFailedTransmission(const QMailMessageIdList &messageIds, QMailServiceAction::Status::ErrorCode error)
{
    if (QMailMessageSink *sink = qobject_cast<QMailMessageSink*>(sender())) {
        if (quint64 action = sinkAction(sink)) {
            emit messagesFailedTransmission(action, messageIds, error);
        }
    }
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

                if (!mSentIds.isEmpty() && (data.completion == &ServiceHandler::transmissionCompleted)) {
                    if (accountSource(service->accountId())) {
                        // Mark these message as Sent
                        quint64 setMask(QMailMessage::Sent);
                        quint64 unsetMask((QMailMessage::Outbox | QMailMessage::Draft | QMailMessage::LocalOnly));

                        QMailMessageKey idsKey(QMailMessageKey::id(mSentIds));

                        if (!QMailStore::instance()->updateMessagesMetaData(idsKey, setMask, true) ||
                            !QMailStore::instance()->updateMessagesMetaData(idsKey, unsetMask, false))
                        {
                            qMailLog(Messaging) << "Unable to flag messages:" << mSentIds;
                        }

                        enqueueRequest(newLocalActionId(),
                                       serialize(accountMessages(mSentIds), setMask, unsetMask),
                                        sourceServiceSet(service->accountId()),
                                        &ServiceHandler::dispatchFlagMessages,
                                        &ServiceHandler::storageActionCompleted,
                                        FlagMessagesRequestType);

                    }

                    mSentIds.clear();
                }

                QSet<QPointer<QMailMessageService> >::iterator sit = data.services.find(service);
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
                        if (data.services.isEmpty() && data.completion && (data.reported == false)) {
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
                    emit activityChanged(action, success ? QMailServiceAction::Successful : QMailServiceAction::Failed);
                }
            }

            if (_outstandingRequests.contains(action)) {
                _outstandingRequests.remove(action);

                // Ensure this request is no longer in the file
                _requestsFile.resize(0);
                foreach (quint64 req, _outstandingRequests) {
                    QByteArray requestNumber(QByteArray::number(req));
                    _requestsFile.write(requestNumber.append('\n'));
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
                mSearches.removeFirst();

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
    if (mSearches.isEmpty()) {
        qWarning() << "Remote search complete but none pending!" << action;
    } else {
        MessageSearch &currentSearch(mSearches.first());

        if (currentSearch.action() != action) {
            qWarning() << "Remote search complete but not current!" << action;
        } else {
            if (currentSearch.isEmpty()) {
                // This search is now finished
                emit searchCompleted(currentSearch.action());
                mSearches.removeFirst();
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

uint qHash(QPointer<QMailMessageService> service)
{
    return qHash(service.data());
}


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


#include "servicehandler.h"
#include <private/longstream_p.h>
#include <QDataStream>
#include <QIODevice>
#include <qmailmessageserver.h>
#include <qmailserviceconfiguration.h>
#include <qmailstore.h>
#include <qmaildisconnected.h>
#include <qmaillog.h>
#include <qmailmessage.h>
#include <qmailcontentmanager.h>
#include <qmailnamespace.h>
#include <QCoreApplication>
#include <QDir>
#include <QDateTime>
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

template <typename T1, typename T2, typename T3, typename T4, typename T5>
QByteArray serialize(const T1& v1, const T2& v2, const T3& v3, const T4& v4, const T5& v5)
{
    QByteArray data;
    QDataStream os(&data, QIODevice::WriteOnly);
    os << v1 << v2 << v3 << v4 << v5;
    return data;
}

template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
QByteArray serialize(const T1& v1, const T2& v2, const T3& v3, const T4& v4, const T5& v5, const T6& v6)
{
    QByteArray data;
    QDataStream os(&data, QIODevice::WriteOnly);
    os << v1 << v2 << v3 << v4 << v5 << v6;
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

template <typename T1, typename T2, typename T3, typename T4, typename T5>
void deserialize(const QByteArray &data, T1& v1, T2& v2, T3& v3, T4& v4, T5& v5)
{
    QDataStream is(data);
    is >> v1 >> v2 >> v3 >> v4 >> v5;
}

template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
void deserialize(const QByteArray &data, T1& v1, T2& v2, T3& v3, T4& v4, T5& v5, T6& v6)
{
    QDataStream is(data);
    is >> v1 >> v2 >> v3 >> v4 >> v5 >> v6;
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

QSet<QMailAccountId> accountsApplicableTo(QMailMessageKey messagekey, QSet<QMailAccountId> const& total)
{
    typedef QPair< QSet<QMailAccountId>, QSet<QMailAccountId> >  IncludedExcludedPair;

    struct L {
        static IncludedExcludedPair extractAccounts(QMailMessageKey const& key)
        {
            bool isNegated(key.isNegated());

            IncludedExcludedPair r;

            QSet<QMailAccountId> & included = isNegated ? r.second : r.first;
            QSet<QMailAccountId> & excluded = isNegated ? r.first : r.second;

            foreach(QMailMessageKey::ArgumentType const& arg, key.arguments())
            {
                switch (arg.property)
                {
                case QMailMessageKey::ParentFolderId:
                case QMailMessageKey::AncestorFolderIds:
                    if (arg.op == QMailKey::Equal || arg.op == QMailKey::Includes) {
                        Q_ASSERT(arg.valueList.count() == 1);
                        Q_ASSERT(arg.valueList[0].canConvert<QMailFolderId>());
                        included.insert(QMailFolder(arg.valueList[0].value<QMailFolderId>()).parentAccountId());
                    } else if (arg.op == QMailKey::NotEqual || arg.op == QMailKey::Excludes) {
                        Q_ASSERT(arg.valueList.count() == 1);
                        Q_ASSERT(arg.valueList[0].canConvert<QMailFolderId>());
                        excluded.insert(QMailFolder(arg.valueList[0].value<QMailFolderId>()).parentAccountId());
                    } else {
                        Q_ASSERT(false);
                    }
                    break;
                case QMailMessageKey::ParentAccountId:
                    if (arg.op == QMailKey::Equal || arg.op == QMailKey::Includes) {
                        Q_ASSERT(arg.valueList.count() == 1);
                        Q_ASSERT(arg.valueList[0].canConvert<QMailAccountId>());
                        included.insert(arg.valueList[0].value<QMailAccountId>());
                    } else if (arg.op == QMailKey::NotEqual || arg.op == QMailKey::Excludes) {
                        Q_ASSERT(arg.valueList.count() == 1);
                        Q_ASSERT(arg.valueList[0].canConvert<QMailAccountId>());
                        excluded.insert(arg.valueList[0].value<QMailAccountId>());
                    } else {
                        Q_ASSERT(false);
                    }
                default:
                    break;
                }
            }

            if (key.combiner() == QMailKey::None) {
                Q_ASSERT(key.subKeys().size() == 0);
                Q_ASSERT(key.arguments().size() <= 1);
            } else if (key.combiner() == QMailKey::Or) {
                foreach (QMailMessageKey const& k, key.subKeys()) {
                    IncludedExcludedPair v(extractAccounts(k));
                    included.unite(v.first);
                    excluded.unite(v.second);
                }
            } else if (key.combiner() == QMailKey::And) {
                bool filled(included.size() == 0 && excluded.size() == 0 ? false : true);

                for (QList<QMailMessageKey>::const_iterator it(key.subKeys().begin()) ; it != key.subKeys().end() ; ++it) {
                    IncludedExcludedPair next(extractAccounts(*it));
                    if (next.first.size() != 0 || next.second.size() != 0) {
                        if (filled) {
                            included.intersect(next.first);
                            excluded.intersect(next.second);
                        } else {
                            filled = true;
                            included.unite(next.first);
                            excluded.unite(next.second);
                        }
                    }
                }
            } else {
                Q_ASSERT(false);
            }

            return r;
        }
    };

    IncludedExcludedPair result(L::extractAccounts(messagekey));
    if (result.first.isEmpty()) {
        return total - result.second;
    } else {
        return (total & result.first) - result.second;
    }
}

struct TextPartSearcher
{
    QString text;

    TextPartSearcher(const QString &text) : text(text) {}

    bool operator()(const QMailMessagePart &part)
    {
        if (part.contentType().matches("text")) {
            if (part.body().data().contains(text, Qt::CaseInsensitive)) {
                return false;
            }
        }

        // Keep searching
        return true;
    }
};

bool messageBodyContainsText(const QMailMessage &message, const QString& text)
{
    // Search only messages or message parts that are of type 'text/*'
    if (message.hasBody()) {
        if (message.contentType().matches("text")) {
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

QList<QString> obsoleteContentIdentifiers(QList<QMailMessageMetaData*> list)
{
    QList<QString> result;
    foreach (QMailMessageMetaData *m, list) {
        QString identifier = m->customField("qmf-obsolete-contentid");
        if (!identifier.isEmpty()) {
            result.append(identifier);
        }
    }
    return result;
}

QList<QString> contentIdentifiers(QList<QMailMessageMetaData*> list)
{
    QList<QString> result;
    foreach (QMailMessageMetaData *m, list) {
        QString identifier = m->contentIdentifier();
        if (!identifier.isEmpty()) {
            result.append(identifier);
        }
    }
    return result;
}

const QString DontSend = "dontSend";
const QString SendFailedTime = "sendFailedTime";
const uint MaxSendFailedAttempts = 5;

void markFailedMessage(QMailServerRequestType requestType, const QMailServiceAction::Status & status)
{
    switch (requestType) {
    case TransmitMessagesRequestType:
        {
            if (status.messageId.isValid()) {
                QMailMessageMetaData metaData(status.messageId);

                if (metaData.customField(DontSend) != "true") {
                    uint sendFailedCount = metaData.customField(DontSend).toUInt();
                    ++sendFailedCount;
                    if (sendFailedCount >= MaxSendFailedAttempts) {
                        metaData.setCustomField(DontSend, "true");
                    }
                    else {
                        metaData.setCustomField(DontSend, QString::number(sendFailedCount));
                    }
                }
                metaData.setCustomField(SendFailedTime,
                                        QString::number(QDateTime::currentDateTimeUtc().toTime_t()));
                QMailStore::instance()->updateMessagesMetaData(
                            QMailMessageKey::id(status.messageId),
                            QMailMessageKey::Custom, metaData);
            }
        }
        break;
    default:
        break;
    }
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
            QMailServiceConfiguration internalCfg(&config, "qmf");
            QString masterId(internalCfg.value("masterAccountId", ""));
            if (!masterId.isEmpty()) {
                QMailAccount master(QMailAccountId(masterId.toInt()));
                if (master.id().isValid()) {
                    masterAccount.insert(id, master.id());
                } else {
                    qWarning() << "Unable to locate master account:" << masterId << "for account:" << id;
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

void ServiceHandler::reregisterAccountServices(QMailAccountIdList ids, QMailServiceAction::Status::ErrorCode code, const QString &text)
{
    QMailAccountIdList totalDeregisterAccountList; // Avoid deregistering each account individually (it's more expensive)

    // Remove and re-create these accounts' services (and only if we need to)
    foreach(QMailAccountId const& id, ids) {
        QMailAccount account(id);
        if (account.status() & QMailAccount::Enabled) {
            QMailAccountConfiguration config(id);

            // See if this account is configured to use a master account
            QMailServiceConfiguration internalCfg(&config, "qmf");
            QString masterId(internalCfg.value("masterAccountId", ""));
            if (!masterId.isEmpty()) {
                QMailAccount master(QMailAccountId(masterId.toInt()));
                if (master.id().isValid()) {
                    // It's possible that before it was using a normal account
                    deregisterAccountServices(QMailAccountIdList() << id, code, text); // this can't wait, as it clears the masterAccount list
                    Q_ASSERT(!masterAccount.contains(id)); // no duplicates, as deregisterAccountServies should remove from this list
                    masterAccount.insert(id, master.id());
                } else {
                    qWarning() << "Unable to locate master account:" << masterId << "for account:" << id;
                }
            } else { // not using a master account
                QSet<QString> newServices;
                QSet<QString> oldServices;

                foreach (const QString& service, config.services()) {
                    QMailServiceConfiguration svcCfg(&config, service);
                    if (svcCfg.type() == QMailServiceConfiguration::Source ||  svcCfg.type() == QMailServiceConfiguration::Sink || svcCfg.type() == QMailServiceConfiguration::SourceAndSink) {
                        newServices.insert(service);
                    }
                }

                // First we will go through all services in serviceMap that belong to this account
                // and check if they are still valid, if they are -- see if they need reloading
                // if they're not, remove them. If there's new ones, add them
                for (QMap< QPair<QMailAccountId, QString>, QPointer<QMailMessageService> >::iterator it(serviceMap.begin()); it != serviceMap.end(); ++it) {
                    if (it.key().first == id) { // we're in the right account
                        QString serviceName(it.key().second);
                        QSet<QString>::iterator newServiceIt(newServices.find(serviceName));
                        if (newServiceIt != newServices.end()) {
                            // only some inner details have changed, only re-register if need be
                            QPointer<QMailMessageService> service(it.value());
                            Q_ASSERT(!service.isNull());
                            if (service->requiresReregistration()) {
                                // Ok, we must remove it
                                qMailLog(Messaging) << "Deregistering service:" << service->service() << "for account:" << it.key().first;
                                service->cancelOperation(code, text);
                                removeServiceFromActions(service);
                                delete service;
                            } else {
                                // it could handle this change, we don't need to register a new one
                                newServices.erase(newServiceIt);
                            }
                        } else {
                            // It was using a service, that it is no longer using
                            oldServices.insert(serviceName);
                        }
                    }
                }

                foreach(QString const& oldService, oldServices) {
                    deregisterAccountService(id, oldService, code, text);
                }
                foreach(QString const& newService, newServices) {
                    registerAccountService(id, QMailServiceConfiguration(&config, newService));
                }
            }
        } else { // account is not enabled, disable all services
            totalDeregisterAccountList.push_back(id);
        }
    }

    deregisterAccountServices(totalDeregisterAccountList, code, text);
}

void ServiceHandler::deregisterAccountService(const QMailAccountId &id, const QString &serviceName, QMailServiceAction::Status::ErrorCode code, const QString &text)
{
    QMailMessageService *service = 0;

    QMap<QPair<QMailAccountId, QString>, QPointer<QMailMessageService> >::iterator it = serviceMap.begin();
    while (it != serviceMap.end()) {
        if (it.key().first == id && it.key().second == serviceName) {
            // Remove any services associated with this account
            service = it.value();
            qMailLog(Messaging) << "Deregistering service:" << service->service() << "for account:" << it.key().first;
            service->cancelOperation(code, text);
            removeServiceFromActions(service);

            it = serviceMap.erase(it);
            // Hm, probably should be breaking here ... but eh
        } else {
            ++it;
        }
    }

    if (service) {
        if (service->hasSource()) {
            if (QMailMessageSource *source = accountSource(id)) {
                sourceService.remove(source);
                sourceMap.remove(id);
            }
        }
        if (service->hasSink()) {
            if (QMailMessageSink *sink = accountSink(id)) {
                sinkService.remove(sink);
                sinkMap.remove(id);
            }
        }
    }

    delete service;
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

    connect(source, SIGNAL(newMessagesAvailable(quint64)), this, SIGNAL(newMessagesAvailable())); // TODO: <- I don't this this makes much sense..
    connect(source, SIGNAL(newMessagesAvailable()), this, SIGNAL(newMessagesAvailable()));

    // if (service->usesConcurrentActions()) {  // This can be uncommented to be stricter.
    connect(source, SIGNAL(messagesDeleted(QMailMessageIdList,quint64)), this, SLOT(messagesDeleted(QMailMessageIdList,quint64)));
    connect(source, SIGNAL(messagesCopied(QMailMessageIdList, quint64)), this, SLOT(messagesCopied(QMailMessageIdList, quint64)));
    connect(source, SIGNAL(messagesMoved(QMailMessageIdList, quint64)), this, SLOT(messagesMoved(QMailMessageIdList, quint64)));
    connect(source, SIGNAL(messagesFlagged(QMailMessageIdList, quint64)), this, SLOT(messagesFlagged(QMailMessageIdList, quint64)));
    connect(source, SIGNAL(messagesPrepared(QMailMessageIdList, quint64)), this, SLOT(messagesPrepared(QMailMessageIdList, quint64)));
    connect(source, SIGNAL(matchingMessageIds(QMailMessageIdList, quint64)), this, SLOT(matchingMessageIds(QMailMessageIdList, quint64)));
    connect(source, SIGNAL(remainingMessagesCount(uint, quint64)), this, SLOT(remainingMessagesCount(uint, quint64)));
    connect(source, SIGNAL(messagesCount(uint, quint64)), this, SLOT(messagesCount(uint, quint64)));
    connect(source, SIGNAL(protocolResponse(QString, QVariant, quint64)), this, SLOT(protocolResponse(QString, QVariant, quint64)));
    // } else {
    connect(source, SIGNAL(messagesDeleted(QMailMessageIdList)), this, SLOT(messagesDeleted(QMailMessageIdList)));
    connect(source, SIGNAL(messagesCopied(QMailMessageIdList)), this, SLOT(messagesCopied(QMailMessageIdList)));
    connect(source, SIGNAL(messagesMoved(QMailMessageIdList)), this, SLOT(messagesMoved(QMailMessageIdList)));
    connect(source, SIGNAL(messagesFlagged(QMailMessageIdList)), this, SLOT(messagesFlagged(QMailMessageIdList)));
    connect(source, SIGNAL(messagesPrepared(QMailMessageIdList)), this, SLOT(messagesPrepared(QMailMessageIdList)));
    connect(source, SIGNAL(matchingMessageIds(QMailMessageIdList)), this, SLOT(matchingMessageIds(QMailMessageIdList)));
    connect(source, SIGNAL(remainingMessagesCount(uint)), this, SLOT(remainingMessagesCount(uint)));
    connect(source, SIGNAL(messagesCount(uint)), this, SLOT(messagesCount(uint)));
    connect(source, SIGNAL(protocolResponse(QString, QVariant)), this, SLOT(protocolResponse(QString, QVariant)));
    // }
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

    // if (service->usesConcurrentActions()) { // this can be uncommented to be stricter
    connect(sink, SIGNAL(messagesTransmitted(QMailMessageIdList, quint64)), this, SLOT(messagesTransmitted(QMailMessageIdList, quint64)));
    connect(sink, SIGNAL(messagesFailedTransmission(QMailMessageIdList, QMailServiceAction::Status::ErrorCode, quint64)),
            this, SLOT(messagesFailedTransmission(QMailMessageIdList, QMailServiceAction::Status::ErrorCode, quint64)));

    // } else {
    connect(sink, SIGNAL(messagesTransmitted(QMailMessageIdList)), this, SLOT(messagesTransmitted(QMailMessageIdList)));
    connect(sink, SIGNAL(messagesFailedTransmission(QMailMessageIdList, QMailServiceAction::Status::ErrorCode)), 
            this, SLOT(messagesFailedTransmission(QMailMessageIdList, QMailServiceAction::Status::ErrorCode)));
    // }
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
    connect(service, SIGNAL(connectivityChanged(QMailServiceAction::Connectivity)), this, SLOT(connectivityChanged(QMailServiceAction::Connectivity)));
    connect(service, SIGNAL(availabilityChanged(bool)), this, SLOT(availabilityChanged(bool)));



    if (service) {
        // if (service->usesConcurrentActions()) { // this can be uncommented to be stricter
        connect(service, SIGNAL(activityChanged(QMailServiceAction::Activity, quint64)), this, SLOT(activityChanged(QMailServiceAction::Activity, quint64)));
        connect(service, SIGNAL(statusChanged(const QMailServiceAction::Status, quint64)), this, SLOT(statusChanged(const QMailServiceAction::Status, quint64)));
        connect(service, SIGNAL(progressChanged(uint, uint, quint64)), this, SLOT(progressChanged(uint, uint, quint64)));
        connect(service, SIGNAL(actionCompleted(bool, quint64)), this, SLOT(actionCompleted(bool, quint64)));
        // } else {
        connect(service, SIGNAL(activityChanged(QMailServiceAction::Activity)), this, SLOT(activityChanged(QMailServiceAction::Activity)));
        connect(service, SIGNAL(statusChanged(const QMailServiceAction::Status)), this, SLOT(statusChanged(const QMailServiceAction::Status)));
        connect(service, SIGNAL(progressChanged(uint, uint)), this, SLOT(progressChanged(uint, uint)));
        connect(service, SIGNAL(actionCompleted(bool)), this, SLOT(actionCompleted(bool)));
        // }
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
            qWarning() << "Unable to instantiate service:" << svcCfg.service();
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
    if (!service || (!service->usesConcurrentActions() && mServiceAction.contains(service)) || mUnavailableServices.contains(service))
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
        qWarning() << "Unable to find message source for account:" << id;
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
            qWarning() << "Unable to find message source for account:" << id;
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
        qWarning() << "Unable to find message sink for account:" << id;
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
            qWarning() << "Unable to find message sink for account:" << id;
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

namespace {
const char* requestTypeNames[] =
{
    "AcknowledgeNewMessagesRequest",
    "TransmitMessagesRequest",
    "RetrieveFolderListRequest",
    "RetrieveMessageListRequest",
    "RetrieveNewMessagesRequest",
    "RetrieveMessagesRequest",
    "RetrieveMessagePartRequest",
    "RetrieveMessageRangeRequest",
    "RetrieveMessagePartRangeRequest",
    "RetrieveAllRequest",
    "ExportUpdatesRequest",
    "SynchronizeRequest",
    "CopyMessagesRequest",
    "MoveMessagesRequest",
    "FlagMessagesRequest",
    "CreateFolderRequest",
    "RenameFolderRequest",
    "DeleteFolderRequest",
    "CancelTransferRequest",
    "DeleteMessagesRequest",
    "SearchMessagesRequest",
    "CancelSearchRequest",
    "ListActionsRequest",
    "ProtocolRequestRequestType"
};
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

        // Limit number of concurrent actions serviced on the device 
        if (mActiveActions.count() >= QMail::maximumConcurrentServiceActions())
            return;

        // Limit number of concurrent actions serviced per process
        const quint64 requestProcess = request->action >> 32;
        int requestProcessCount = 1; // including the request
        foreach(quint64 actionId, mActiveActions.keys()) {
            if ((actionId >> 32) == requestProcess) {
                if (++requestProcessCount > QMail::maximumConcurrentServiceActionsPerProcess()) {
                    break;
                }
            }
        }

        if (requestProcessCount > QMail::maximumConcurrentServiceActionsPerProcess()) {
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
        data.unixTimeExpiry = QDateTime::currentDateTime().toTime_t() + ExpirySeconds;
        data.reported = false;
        data.description = request->description;
        data.progressTotal = 0;
        data.progressCurrent = 0;
        data.status = QMailServiceAction::Status(QMailServiceAction::Status::ErrNoError, QString(), QMailAccountId(), QMailFolderId(), QMailMessageId());

        mActiveActions.insert(request->action, data);
        qMailLog(Messaging) << "Running action" << ::requestTypeNames[data.description] << request->action;
        emit actionStarted(QMailActionData(request->action, request->description, 0, 0, 
                                           data.status.errorCode, data.status.text, 
                                           data.status.accountId, data.status.folderId, data.status.messageId));
        emit activityChanged(request->action, QMailServiceAction::InProgress);

        if ((this->*request->servicer)(request->action, request->data)) {
            // This action is now underway

            if (mActionExpiry.isEmpty()) {
                // Start the expiry timer. Convert to miliseconds, and avoid shooting too early
                const int expiryMs = ExpirySeconds * 1000;
                QTimer::singleShot(expiryMs + 50, this, SLOT(expireAction()));
            }
            mActionExpiry.append(request->action);
        } else {
            mActiveActions.remove(request->action);

            qWarning() << "Unable to dispatch request:" << request->action << "to services:" << request->services;
            emit activityChanged(request->action, QMailServiceAction::Failed);

            foreach (QMailMessageService *service, request->services)
                mServiceAction.remove(service);
        }

        request = mRequests.erase(request);
    }
}

void ServiceHandler::updateAction(quint64 action)
{
    QLinkedList<quint64>::iterator it = std::find(mActionExpiry.begin(), mActionExpiry.end(), action);
    if (it != mActionExpiry.end()) {
        // Move this action to the end of the list
        mActionExpiry.erase(it);
        mActionExpiry.append(action);

        // Update the expiry time for this action
        mActiveActions[action].unixTimeExpiry = QDateTime::currentDateTime().toTime_t() + ExpirySeconds;
    }
}

void ServiceHandler::expireAction()
{
    uint now(QDateTime::currentDateTime().toTime_t());

    if (!mActionExpiry.isEmpty()) {
        quint64 action = *mActionExpiry.begin();

        QMap<quint64, ActionData>::iterator it = mActiveActions.find(action);
        if (it != mActiveActions.end()) {
            ActionData &data(it.value());

            // Is the oldest action expired?
            if (data.unixTimeExpiry <= now) {
                qWarning() << "Expired request:" << action;
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
                QMailAccountIdList ids(serviceAccounts.toList());
                deregisterAccountServices(ids, QMailServiceAction::Status::ErrTimeout, tr("Request is not progressing"));
                registerAccountServices(ids);

                // See if there are more actions to dispatch
                dispatchRequest();
            }
        }
    }

    QLinkedList<quint64>::iterator expiryIt(mActionExpiry.begin());
    while (expiryIt != mActionExpiry.end()) {
        if (mActiveActions.contains(*expiryIt)) {
            uint nextExpiry(mActiveActions.value(*expiryIt).unixTimeExpiry);

            // miliseconds until it expires..
            uint nextShot(nextExpiry <= now ? 0 : (nextExpiry - now) * 1000 + 50);
            QTimer::singleShot(nextShot, this, SLOT(expireAction()));
            return;
        } else {
            expiryIt = mActionExpiry.erase(expiryIt); // Just remove this non-existent action
        }
    }
}

// Cancelled by user
void ServiceHandler::cancelTransfer(quint64 action)
{
    cancelLocalSearch(action);
    QMap<quint64, ActionData>::iterator it = mActiveActions.find(action);
    if (it != mActiveActions.end()) {
        bool retrievalSetModified(false);
        bool transmissionSetModified(false);

        const ActionData &data(it.value());
        foreach (QMailMessageService *service, data.services) {
            if (service) {
                if (service->usesConcurrentActions())
                    service->cancelOperation(QMailServiceAction::Status::ErrCancel, tr("Cancelled by user"), it.key());
                else
                    service->cancelOperation(QMailServiceAction::Status::ErrCancel, tr("Cancelled by user"));
            }
            mServiceAction.remove(service);
            if (!service) {
                qWarning() << "Unable to cancel null service for action:" << action;
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

        //The ActionData might have already been deleted by actionCompleted, triggered by cancelOperation
        it = mActiveActions.find(action);
        if (it != mActiveActions.end()) mActiveActions.erase(it);

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

void ServiceHandler::transmitMessage(quint64 action, const QMailMessageId &messageId)
{
    QMailAccountId accountId(QMailMessageMetaData(messageId).parentAccountId());
    // Ensure that this account has a sink configured
    QSet<QMailMessageService*> sinks(sinkServiceSet(accountId));
    if (sinks.isEmpty()) {
        reportFailure(action, QMailServiceAction::Status::ErrNoConnection, tr("Unable to enqueue messages for transmission"));
    } else {
        // We need to see if any sources are required to prepare these messages
        // We need to prepare messages to:
        // - resolve references
        // - move to the Sent folder prior to transmission
        quint64 preparationStatus(QMailMessage::HasUnresolvedReferences | QMailMessage::TransmitFromExternal);

        QMailMessageIdList unresolvedMessages;
        QMailMessageMetaData metaData(messageId);
        if (metaData.status() & preparationStatus) {
            unresolvedMessages << messageId;
        }

        QSet<QMailMessageService*> sources;
        if (!unresolvedMessages.isEmpty()) {
            // Find the accounts that own these messages
            QMap<QMailAccountId, QList<QPair<QMailMessagePart::Location, QMailMessagePart::Location> > > unresolvedLists(messageResolvers(unresolvedMessages));

            sources = sourceServiceSet(unresolvedLists.keys().toSet());

            // Emit no signal after completing preparation
            enqueueRequest(action, serialize(unresolvedLists), sources, &ServiceHandler::dispatchPrepareMessages, 0, TransmitMessagesRequestType);
        }

        // The transmit action is dependent on the availability of the sources, since they must complete their preparation step first
        enqueueRequest(action, serialize(messageId), sinks, &ServiceHandler::dispatchTransmitMessage, &ServiceHandler::transmissionCompleted, TransmitMessagesRequestType, sources);
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
                qWarning() << "Unable to service request to prepare messages for account:" << it.key();
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
        QMailMessageKey sendKey(QMailMessageKey::customField("dontSend", "true", QMailDataComparator::NotEqual));
        QMailMessageKey noSendKey(QMailMessageKey::customField("dontSend", QMailDataComparator::Absent));

        QMailMessageIdList toTransmit(
                    QMailStore::instance()->queryMessages(
                        accountKey & outboxKey & (noSendKey | sendKey)));

        bool success(sinkService.value(sink)->usesConcurrentActions()
                     ? sink->transmitMessages(toTransmit, action)
                     : sink->transmitMessages(toTransmit));

        if (success) {
            // This account is now transmitting
            setTransmissionInProgress(accountId, true);
        } else {
            qWarning() << "Unable to service request to add messages to sink for account:" << accountId;
            return false;
        }
    } else {
        reportFailure(action, QMailServiceAction::Status::ErrFrameworkFault, tr("Unable to locate sink for account"), accountId);
        return false;
    }

    return true;
}

bool ServiceHandler::dispatchTransmitMessage(quint64 action, const QByteArray &data)
{
    QMailMessageId messageId;

    deserialize(data, messageId);

    QMailMessageMetaData metaData(messageId);
    QMailAccountId accountId(metaData.parentAccountId());
    if (QMailMessageSink *sink = accountSink(accountId)) {
        // Transmit the messages if it's in the Outbox.
        quint64 outboxStatus(QMailMessage::Outbox & ~QMailMessage::Trash);
        bool success = false;
        QMailMessageIdList toTransmit;
        if (metaData.status() & outboxStatus) {
            toTransmit << messageId;

            success = sinkService.value(sink)->usesConcurrentActions()
                ? sink->transmitMessages(toTransmit, action)
                : sink->transmitMessages(toTransmit);
        }

        if (success) {
            // This account is now transmitting
            setTransmissionInProgress(accountId, true);
        } else {
            qWarning() << "Unable to service request to add message " << messageId << " to sink for account:" << accountId;
            return false;
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
        bool success(sourceService.value(source)->usesConcurrentActions()
                            ? source->retrieveFolderList(accountId, folderId, descending, action)
                            : source->retrieveFolderList(accountId, folderId, descending));

        if (success) {
            // This account is now retrieving (arguably...)
            setRetrievalInProgress(accountId, true);
        } else {
            qWarning() << "Unable to service request to retrieve folder list for account:" << accountId;
            return false;
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
        bool success(sourceService.value(source)->usesConcurrentActions()
                            ? source->retrieveMessageList(accountId, folderId, minimum, sort, action)
                            : source->retrieveMessageList(accountId, folderId, minimum, sort));
        if (success) {
            // This account is now retrieving
            setRetrievalInProgress(accountId, true);
        } else {
            qWarning() << "Unable to service request to retrieve message list for folder:" << folderId;
            return false;
        }
    } else {
        reportFailure(action, QMailServiceAction::Status::ErrFrameworkFault, tr("Unable to locate source for account"), accountId);
        return false;
    }

    return true;
}

void ServiceHandler::retrieveMessageLists(quint64 action, const QMailAccountId &accountId, const QMailFolderIdList &folderIds, uint minimum, const QMailMessageSortKey &sort)
{
    QSet<QMailMessageService*> sources(sourceServiceSet(accountId));
    if (sources.isEmpty()) {
        reportFailure(action, QMailServiceAction::Status::ErrNoConnection, tr("Unable to retrieve message list for unconfigured account"));
    } else {
        enqueueRequest(action, serialize(accountId, folderIds, minimum, sort), sources, &ServiceHandler::dispatchRetrieveMessageLists, &ServiceHandler::retrievalCompleted, RetrieveMessageListRequestType);
    }
}

bool ServiceHandler::dispatchRetrieveMessageLists(quint64 action, const QByteArray &data)
{
    QMailAccountId accountId;
    QMailFolderIdList folderIds;
    uint minimum;
    QMailMessageSortKey sort;

    deserialize(data, accountId, folderIds, minimum, sort);

    if (QMailMessageSource *source = accountSource(accountId)) {
        bool success(sourceService.value(source)->usesConcurrentActions()
                            ? source->retrieveMessageLists(accountId, folderIds, minimum, sort, action)
                            : source->retrieveMessageLists(accountId, folderIds, minimum, sort));
        if (success) {
            // This account is now retrieving
            setRetrievalInProgress(accountId, true);
        } else {
            qWarning() << "Unable to service request to retrieve message list for folders:" << folderIds;
            return false;
        }
    } else {
        reportFailure(action, QMailServiceAction::Status::ErrFrameworkFault, tr("Unable to locate source for account"), accountId);
        return false;
    }

    return true;
}

void ServiceHandler::retrieveNewMessages(quint64 action, const QMailAccountId &accountId, const QMailFolderIdList &folderIds)
{
    QSet<QMailMessageService*> sources(sourceServiceSet(accountId));
    if (sources.isEmpty()) {
        reportFailure(action, QMailServiceAction::Status::ErrNoConnection, tr("Unable to retrieve new messages for unconfigured account"));
    } else {
        enqueueRequest(action, serialize(accountId, folderIds), sources, &ServiceHandler::dispatchRetrieveNewMessages, &ServiceHandler::retrievalCompleted, RetrieveNewMessagesRequestType);
    }
}

bool ServiceHandler::dispatchRetrieveNewMessages(quint64 action, const QByteArray &data)
{
    QMailAccountId accountId;
    QMailFolderIdList folderIds;

    deserialize(data, accountId, folderIds);

    if (QMailMessageSource *source = accountSource(accountId)) {
        bool success(sourceService.value(source)->usesConcurrentActions()
                            ? source->retrieveNewMessages(accountId, folderIds)
                            : source->retrieveNewMessages(accountId, folderIds));
        if (success) {
            // This account is now retrieving
            setRetrievalInProgress(accountId, true);
        } else {
            qWarning() << "Unable to service request to retrieve new messages for folders:" << folderIds;
            return false;
        }
    } else {
        reportFailure(action, QMailServiceAction::Status::ErrFrameworkFault, tr("Unable to locate source for account"), accountId);
        return false;
    }

    return true;
}

void ServiceHandler::createStandardFolders(quint64 action, const QMailAccountId &accountId)
{
    QSet<QMailMessageService*> sources(sourceServiceSet(accountId));
    if (sources.isEmpty()) {
        reportFailure(action, QMailServiceAction::Status::ErrNoConnection, tr("Unable to retrieve standard folders for unconfigured account"));
    }
    else {
        enqueueRequest(action, serialize(accountId), sources, &ServiceHandler::dispatchCreateStandardFolders, &ServiceHandler::retrievalCompleted, RetrieveFolderListRequestType);
    }
}

bool ServiceHandler::dispatchCreateStandardFolders(quint64 action, const QByteArray &data)
{
    QMailAccountId accountId;

    deserialize(data, accountId);

    if (QMailMessageSource *source = accountSource(accountId)) {
        bool success(sourceService.value(source)->usesConcurrentActions()
            ? source->createStandardFolders(accountId, action)
            : source->createStandardFolders(accountId));
        if (success) {
            return true;
        } else {
            qWarning() << "Unable to service request to create standard folder for account:" << accountId;
            return false;
        }

    } else {
        reportFailure(action, QMailServiceAction::Status::ErrFrameworkFault, tr("Unable to locate source for account"), accountId);
        return false;
    }
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
            bool success(sourceService.value(source)->usesConcurrentActions()
                                ? source->retrieveMessages(it.value(), spec, action)
                                : source->retrieveMessages(it.value(), spec));

            if (success) {
                // This account is now retrieving
                if (!_retrievalAccountIds.contains(it.key())) {
                    _retrievalAccountIds.insert(it.key());
                }
            } else {
                qWarning() << "Unable to service request to retrieve messages for account:" << it.key() << "with spec" << spec;
                return false;
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
        bool success(sourceService.value(source)->usesConcurrentActions()
            ? source->retrieveMessagePart(partLocation, action)
            : source->retrieveMessagePart(partLocation));

        if (success) {
           // This account is now retrieving
            setRetrievalInProgress(accountId, true);
        } else {
            qWarning() << "Unable to service request to retrieve part for message:" << partLocation.containingMessageId();
            return false;
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
        bool success(sourceService.value(source)->usesConcurrentActions()
            ? source->retrieveMessageRange(messageId, minimum, action)
            : source->retrieveMessageRange(messageId, minimum));

        if (success) {
            // This account is now retrieving
            setRetrievalInProgress(accountId, true);
        } else {
            qWarning() << "Unable to service request to retrieve range:" << minimum << "for message:" << messageId;
            return false;
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

        bool success(sourceService.value(source)->usesConcurrentActions()
            ? source->retrieveMessagePartRange(partLocation, minimum, action)
            : source->retrieveMessagePartRange(partLocation, minimum));

        if (success) {
            // This account is now retrieving
            setRetrievalInProgress(accountId, true);
        } else {
            qWarning() << "Unable to service request to retrieve range:" << minimum << "for part in message:" << partLocation.containingMessageId();
            return false;
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

    if (QMailMessageSource *source = accountSource(accountId)) {
        bool success(sourceService.value(source)->usesConcurrentActions()
            ? source->retrieveAll(accountId, action)
            : source->retrieveAll(accountId));

        if (success) {
            // This account is now retrieving
            setRetrievalInProgress(accountId, true);
        } else {
            qWarning() << "Unable to service request to retrieve all messages for account:" << accountId;
            return false;
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
        bool success(sourceService.value(source)->usesConcurrentActions()
            ? source->exportUpdates(accountId, action)
            : source->exportUpdates(accountId));

        if (!success) {
            qWarning() << "Unable to service request to export updates for account:" << accountId;
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

        bool success(sourceService.value(source)->usesConcurrentActions()
            ? source->exportUpdates(accountId, action)
            : source->exportUpdates(accountId));

        if (success) {
            // This account is now retrieving
            setRetrievalInProgress(accountId, true);
        } else {
            qWarning() << "Unable to service request to synchronize account:" << accountId;
            return false;
        }
    } else {
        reportFailure(action, QMailServiceAction::Status::ErrFrameworkFault, tr("Unable to locate source for account"), accountId);
        return false;
    }

    return true;
}

void ServiceHandler::onlineDeleteMessages(quint64 action, const QMailMessageIdList& messageIds, QMailStore::MessageRemovalOption option)
{
    QSet<QMailMessageService*> sources;

    if (option == QMailStore::NoRemovalRecord) {
        // Delete these records locally without involving the source
        discardMessages(action, messageIds);
    } else {
        QMap<QMailAccountId, QMailMessageIdList> messageLists(accountMessages(messageIds));
        sources = sourceServiceSet(messageLists.keys().toSet());
        if (sources.isEmpty()) {
            reportFailure(action, QMailServiceAction::Status::ErrNoConnection, tr("Unable to delete messages for unconfigured account"));
        } else {
            QString description(tr("Deleting messages"));
            enqueueRequest(action, serialize(messageLists), sources, &ServiceHandler::dispatchOnlineDeleteMessages, &ServiceHandler::storageActionCompleted, DeleteMessagesRequestType);
        }
    }
}

void ServiceHandler::discardMessages(quint64 action, QMailMessageIdList messageIds)
{
    uint progress = 0;
    uint total = messageIds.count();

    emit progressChanged(action, progress, total);

    // Just delete all these messages
    if (!messageIds.isEmpty() 
        && !QMailStore::instance()->removeMessages(QMailMessageKey::id(messageIds), QMailStore::NoRemovalRecord)) {
        qWarning() << "Unable to service request to discard messages";

        reportFailure(action, QMailServiceAction::Status::ErrEnqueueFailed, tr("Unable to discard messages"));
        return;
    }

    emit progressChanged(action, total, total);
    emit messagesDeleted(action, messageIds);
    emit storageActionCompleted(action);
    emit activityChanged(action, QMailServiceAction::Successful);
    QMailStore::instance()->flushIpcNotifications();
    return;
}

bool ServiceHandler::dispatchOnlineDeleteMessages(quint64 action, const QByteArray &data)
{
    QMap<QMailAccountId, QMailMessageIdList> messageLists;

    deserialize(data, messageLists);

    QMap<QMailAccountId, QMailMessageIdList>::const_iterator it = messageLists.begin(), end = messageLists.end();
    for ( ; it != end; ++it) {
        if (QMailMessageSource *source = accountSource(it.key())) {
            bool success(sourceService.value(source)->usesConcurrentActions()
                ? source->deleteMessages(it.value(), action)
                : source->deleteMessages(it.value()));

            if (!success) {
                qWarning() << "Unable to service request to delete messages for account:" << it.key();
                return false;
            }
        } else {
            reportFailure(action, QMailServiceAction::Status::ErrFrameworkFault, tr("Unable to locate source for account"), it.key());
            return false;
        }
    }

    return true;
}

void ServiceHandler::onlineCopyMessages(quint64 action, const QMailMessageIdList& messageIds, const QMailFolderId &destination)
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
            enqueueRequest(action, serialize(messageIds, destination), sources, &ServiceHandler::dispatchOnlineCopyMessages, &ServiceHandler::storageActionCompleted, CopyMessagesRequestType);
        }
    }
}

bool ServiceHandler::dispatchOnlineCopyMessages(quint64 action, const QByteArray &data)
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

            bool success(sourceService.value(source)->usesConcurrentActions()
                ? source->copyMessages(messageIds, destination, action)
                : source->copyMessages(messageIds, destination));

            if (!success) {
                qWarning() << "Unable to service request to copy messages to folder:" << destination << "for account:" << accountId;
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
            qWarning() << "Unable to service request to copy messages to folder:" << destination << "for account:" << message.parentAccountId();

            reportFailure(action, QMailServiceAction::Status::ErrFrameworkFault, tr("Unable to copy messages for account"), message.parentAccountId());
            return false;
        } else {
            emit progressChanged(action, ++progress, total);
        }
    }

    emit activityChanged(action, QMailServiceAction::Successful);
    return true;
}

void ServiceHandler::onlineMoveMessages(quint64 action, const QMailMessageIdList& messageIds, const QMailFolderId &destination)
{
    QSet<QMailMessageService*> sources;

    QMap<QMailAccountId, QMailMessageIdList> messageLists(accountMessages(messageIds));
    sources = sourceServiceSet(messageLists.keys().toSet());
    if (sources.isEmpty()) {
        reportFailure(action, QMailServiceAction::Status::ErrNoConnection, tr("Unable to move messages for unconfigured account"));
    } else {
        enqueueRequest(action, serialize(messageLists, destination), sources, &ServiceHandler::dispatchOnlineMoveMessages, &ServiceHandler::storageActionCompleted, MoveMessagesRequestType);
    }
}

bool ServiceHandler::dispatchOnlineMoveMessages(quint64 action, const QByteArray &data)
{
    QMap<QMailAccountId, QMailMessageIdList> messageLists;
    QMailFolderId destination;

    deserialize(data, messageLists, destination);

    QMap<QMailAccountId, QMailMessageIdList>::const_iterator it = messageLists.begin(), end = messageLists.end();
    for ( ; it != end; ++it) {       
        if (QMailMessageSource *source = accountSource(it.key())) {

            bool success(sourceService.value(source)->usesConcurrentActions()
                ? source->moveMessages(it.value(), destination, action)
                : source->moveMessages(it.value(), destination));

            if (!success) {
                qWarning() << "Unable to service request to move messages to folder:" << destination << "for account:" << it.key();
                return false;
            }
        } else {
            reportFailure(action, QMailServiceAction::Status::ErrFrameworkFault, tr("Unable to locate source for account"), it.key());
            return false;
        }
    }

    return true;
}

void ServiceHandler::onlineFlagMessagesAndMoveToStandardFolder(quint64 action, const QMailMessageIdList& messageIds, quint64 setMask, quint64 unsetMask)
{
    QSet<QMailMessageService*> sources;

    QMap<QMailAccountId, QMailMessageIdList> messageLists(accountMessages(messageIds));
    sources = sourceServiceSet(messageLists.keys().toSet());
    if (sources.isEmpty()) {
        reportFailure(action, QMailServiceAction::Status::ErrNoConnection, tr("Unable to flag messages for unconfigured account"));
    } else {
        enqueueRequest(action, serialize(messageLists, setMask, unsetMask), sources, &ServiceHandler::dispatchOnlineFlagMessagesAndMoveToStandardFolder, &ServiceHandler::storageActionCompleted, FlagMessagesRequestType);
    }
}

void ServiceHandler::addOrUpdateMessages(quint64 action, const QString &filename, bool add)
{
    QFile file(filename);
    QFileInfo fi(file);
    QMailMessageIdList ids;
    QString err;
    if (add) {
        err = tr("Unable to async add messages");
    } else {
        err = tr("Unable to async update messages");
    }
    if (fi.exists() && fi.isFile() && fi.isReadable()) {
        file.open(QIODevice::ReadOnly);
        QDataStream stream(&file);
        while (!stream.atEnd()) {
            QMailMessage message;
            QMailStore *store = QMailStore::instance();
            stream >> message;
            if (add) {
                store->addMessage(&message);
            } else {
                store->updateMessage(&message);
            }
            if (store->lastError() != QMailStore::NoError) {
               reportFailure(action, QMailServiceAction::Status::ErrFrameworkFault, err);
               return;
            }
            ids.append(message.id());
        }
        if (add) {
            emit messagesAdded(action, ids);
        } else {
            emit messagesUpdated(action, ids);
        }
        emit storageActionCompleted(action);
        emit activityChanged(action, QMailServiceAction::Successful);
        return;
    }
    file.remove();
    reportFailure(action, QMailServiceAction::Status::ErrFrameworkFault, err);
}

void ServiceHandler::addMessages(quint64 action, const QString &filename)
{
    addOrUpdateMessages(action, filename, true);
    QMailStore::instance()->flushIpcNotifications();
}

void ServiceHandler::updateMessages(quint64 action, const QString &filename)
{
    addOrUpdateMessages(action, filename, false);
    QMailStore::instance()->flushIpcNotifications();
}

void ServiceHandler::addMessages(quint64 action, const QMailMessageMetaDataList &messages)
{
    bool failure = false;
    QList<QMailMessageMetaData*> list;
    QString scheme;
    if (messages.count()) {
        scheme = messages.first().contentScheme();
    }
    foreach (QMailMessageMetaData m, messages) {
        if (m.contentScheme() != scheme) {
            reportFailure(action,
                          QMailServiceAction::Status::ErrFrameworkFault,
                          tr("Unable to async add messages, "
                             "inconsistent contentscheme"));
        }
    }
    foreach (QMailMessageMetaData m, messages) {
        list.append(new QMailMessageMetaData(m));
    }
    if (scheme.isEmpty()) {
        scheme = QMailContentManagerFactory::defaultScheme();
    }
    if (!list.isEmpty()) {
        QMailContentManager *content = QMailContentManagerFactory::create(scheme);
        content->ensureDurability(contentIdentifiers(list));
        QMailStore *store = QMailStore::instance();
        store->addMessages(list);
        failure |= (store->lastError() != QMailStore::NoError);
        store->ensureDurability();
        failure |= (store->lastError() != QMailStore::NoError);
    }

    QMailMessageIdList ids;
    while (!list.isEmpty()) {
        QMailMessageMetaData *data(list.takeFirst());
        ids.append(data->id());
        delete data;
    }

    if (failure) {
        reportFailure(action,
                      QMailServiceAction::Status::ErrFrameworkFault,
                      tr("Unable to async update messages"));
        return;
    }

    emit messagesAdded(action, ids);
    emit storageActionCompleted(action);
    emit activityChanged(action, QMailServiceAction::Successful);
    QMailStore::instance()->flushIpcNotifications();
}

void ServiceHandler::updateMessages(quint64 action, const QMailMessageMetaDataList &messages)
{
    bool failure = false;
    QList<QMailMessageMetaData*> list;
    QString scheme;
    if (messages.count()) {
        scheme = messages.first().contentScheme();
    }
    foreach (QMailMessageMetaData m, messages) {
        if (m.contentScheme() != scheme) {
            reportFailure(action,
                          QMailServiceAction::Status::ErrFrameworkFault,
                          tr("Unable to async update messages, "
                             "inconsistent contentscheme"));
        }
    }
    foreach (QMailMessageMetaData m, messages) {
        list.append(new QMailMessageMetaData(m));
    }
    if (scheme.isEmpty()) {
        scheme = QMailContentManagerFactory::defaultScheme();
    }
    if (!list.isEmpty()) {
        QMailContentManager *content = QMailContentManagerFactory::create(scheme);
        QList<QString> obsoleteIds(obsoleteContentIdentifiers(list));
        if (!obsoleteIds.isEmpty()) {
            content->ensureDurability(contentIdentifiers(list));
            foreach (QMailMessageMetaData *m, list) {
                m->removeCustomField("qmf-obsolete-contentid");
            }
        } // else only update metadata in mailstore
        QMailStore *store = QMailStore::instance();
        store->updateMessages(list);
        failure |= (store->lastError() != QMailStore::NoError);
        store->ensureDurability();
        failure |= (store->lastError() != QMailStore::NoError);

        content->remove(obsoleteIds);
    }

    QMailMessageIdList ids;
    while (!list.isEmpty()) {
        QMailMessageMetaData *data(list.takeFirst());
        ids.append(data->id());
        delete data;
    }

    if (failure) {
        reportFailure(action,
                      QMailServiceAction::Status::ErrFrameworkFault,
                      tr("Unable to async update messages"));
        return;
    }

    emit messagesUpdated(action, ids);
    emit storageActionCompleted(action);
    emit activityChanged(action, QMailServiceAction::Successful);
    QMailStore::instance()->flushIpcNotifications();
}

bool ServiceHandler::dispatchOnlineFlagMessagesAndMoveToStandardFolder(quint64 action, const QByteArray &data)
{
    QMap<QMailAccountId, QMailMessageIdList> messageLists;
    quint64 setMask;
    quint64 unsetMask;

    deserialize(data, messageLists, setMask, unsetMask);

    Q_ASSERT(!messageLists.empty());

    QMap<QMailAccountId, QMailMessageIdList>::const_iterator it = messageLists.begin(), end = messageLists.end();
    for ( ; it != end; ++it) {
        if (QMailMessageSource *source = accountSource(it.key())) {
            bool success(sourceService.value(source)->usesConcurrentActions()
                ? source->flagMessages(it.value(), setMask, unsetMask, action)
                : source->flagMessages(it.value(), setMask, unsetMask));

            if (!success) {
                qWarning() << "Unable to service request to flag messages for account:" << it.key();
                return false;
            }
        } else {
            reportFailure(action, QMailServiceAction::Status::ErrFrameworkFault, tr("Unable to locate source for account"), it.key());
            return false;
        }
    }

    return true;
}

void ServiceHandler::deleteMessages(quint64 action, const QMailMessageIdList& messageIds)
{
    uint total = messageIds.count();

    // Just delete all these messages from device and mark for deletion on server when exportUpdates is called
    if (!messageIds.isEmpty() && !QMailStore::instance()->removeMessages(QMailMessageKey::id(messageIds), QMailStore::CreateRemovalRecord)) {
        qWarning() << "Unable to service request to delete messages";
        reportFailure(action, QMailServiceAction::Status::ErrFrameworkFault, tr("Could not delete messages"));
        return;
    }

    emit progressChanged(action, total, total);
    emit messagesDeleted(action, messageIds);
    emit storageActionCompleted(action);
    emit activityChanged(action, QMailServiceAction::Successful);
    QMailStore::instance()->flushIpcNotifications();
    return;
}

void ServiceHandler::rollBackUpdates(quint64 action, const QMailAccountId &mailAccountId)
{
    QMailDisconnected::rollBackUpdates(mailAccountId);
    
    emit storageActionCompleted(action);
    emit activityChanged(action, QMailServiceAction::Successful);
    QMailStore::instance()->flushIpcNotifications();
    return;
}

void ServiceHandler::moveToStandardFolder(quint64 action, const QMailMessageIdList& ids, quint64 standardFolder)
{
    QMailDisconnected::moveToStandardFolder(ids, static_cast<QMailFolder::StandardFolder>(standardFolder));
    messagesMoved(ids, action);
    messagesFlagged(ids, action);

    emit storageActionCompleted(action);
    emit activityChanged(action, QMailServiceAction::Successful);
    QMailStore::instance()->flushIpcNotifications();
    return;    
}

void ServiceHandler::moveToFolder(quint64 action, const QMailMessageIdList& ids, const QMailFolderId& folderId)
{
    QMailDisconnected::moveToFolder(ids, folderId);
    messagesMoved(ids, action);
    
    emit storageActionCompleted(action);
    emit activityChanged(action, QMailServiceAction::Successful);
    QMailStore::instance()->flushIpcNotifications();
    return;
}

void ServiceHandler::flagMessages(quint64 action, const QMailMessageIdList& ids, quint64 setMask, quint64 unsetMask)
{
    QMailDisconnected::flagMessages(ids, setMask, unsetMask, "");
    messagesFlagged(ids, action);
    
    emit storageActionCompleted(action);
    emit activityChanged(action, QMailServiceAction::Successful);
    QMailStore::instance()->flushIpcNotifications();
    return;
}

void ServiceHandler::restoreToPreviousFolder(quint64 action, const QMailMessageKey& key)
{
    QMailDisconnected::restoreToPreviousFolder(key);
    
    emit storageActionCompleted(action);
    emit activityChanged(action, QMailServiceAction::Successful);
    QMailStore::instance()->flushIpcNotifications();
    return;
}

void ServiceHandler::onlineCreateFolder(quint64 action, const QString &name, const QMailAccountId &accountId, const QMailFolderId &parentId)
{
    if(accountId.isValid()) {

        QSet<QMailAccountId> accounts;
        if (parentId.isValid()) {
            accounts = folderAccount(parentId);
        } else {
            accounts.insert(accountId);
        }

        QSet<QMailMessageService *> sources(sourceServiceSet(accounts));

        enqueueRequest(action, serialize(name, accountId, parentId), sources, &ServiceHandler::dispatchOnlineCreateFolder, &ServiceHandler::storageActionCompleted, CreateFolderRequestType);
    } else {
        reportFailure(action, QMailServiceAction::Status::ErrInvalidData, tr("Unable to create folder for invalid account"));
    }
}

bool ServiceHandler::dispatchOnlineCreateFolder(quint64 action, const QByteArray &data)
{
    QString name;
    QMailAccountId accountId;
    QMailFolderId parentId;

    deserialize(data, name, accountId, parentId);

    if(QMailMessageSource *source = accountSource(accountId)) {
        bool success(sourceService.value(source)->usesConcurrentActions()
            ? source->createFolder(name, accountId, parentId, action)
            : source->createFolder(name, accountId, parentId));
        if (success) {
            return true;
        } else {
            qWarning() << "Unable to service request to create folder for account:" << accountId;
            return false;
        }

    } else {
        reportFailure(action, QMailServiceAction::Status::ErrFrameworkFault, tr("Unable to locate source for account"), accountId);
        return false;
    }
}

void ServiceHandler::onlineRenameFolder(quint64 action, const QMailFolderId &folderId, const QString &name)
{
    if(folderId.isValid()) {
        QSet<QMailAccountId> accounts = folderAccount(folderId);
        QSet<QMailMessageService *> sources(sourceServiceSet(accounts));

        enqueueRequest(action, serialize(folderId, name), sources, &ServiceHandler::dispatchOnlineRenameFolder, &ServiceHandler::storageActionCompleted, RenameFolderRequestType);
    } else {
        reportFailure(action, QMailServiceAction::Status::ErrInvalidData, tr("Unable to rename invalid folder"));
    }
}

bool ServiceHandler::dispatchOnlineRenameFolder(quint64 action, const QByteArray &data)
{
    QMailFolderId folderId;
    QString newFolderName;

    deserialize(data, folderId, newFolderName);

    if(QMailMessageSource *source = accountSource(QMailFolder(folderId).parentAccountId())) {
        bool success(sourceService.value(source)->usesConcurrentActions()
            ? source->renameFolder(folderId, newFolderName, action)
            : source->renameFolder(folderId, newFolderName));
        if (success) {
            return true;
        } else {
            qWarning() << "Unable to service request to rename folder id:" << folderId;
            return false;
        }

    } else {
        reportFailure(action, QMailServiceAction::Status::ErrFrameworkFault, tr("Unable to locate source for account"), QMailFolder(folderId).parentAccountId());
        return false;
    }
}

void ServiceHandler::onlineDeleteFolder(quint64 action, const QMailFolderId &folderId)
{
    if(folderId.isValid()) {
        QSet<QMailAccountId> accounts = folderAccount(folderId);
        QSet<QMailMessageService *> sources(sourceServiceSet(accounts));

        enqueueRequest(action, serialize(folderId), sources, &ServiceHandler::dispatchOnlineDeleteFolder, &ServiceHandler::storageActionCompleted, DeleteFolderRequestType);
    } else {
        reportFailure(action, QMailServiceAction::Status::ErrInvalidData, tr("Unable to delete invalid folder"));
    }
}

bool ServiceHandler::dispatchOnlineDeleteFolder(quint64 action, const QByteArray &data)
{
    QMailFolderId folderId;

    deserialize(data, folderId);

    if(QMailMessageSource *source = accountSource(QMailFolder(folderId).parentAccountId())) {
        if(source->deleteFolder(folderId)) {
            return true;
        } else {
            qWarning() << "Unable to service request to delete folder id:" << folderId;
            return false;
        }
    } else {
        reportFailure(action, QMailServiceAction::Status::ErrFrameworkFault, tr("Unable to locate source for account"), QMailFolder(folderId).parentAccountId());
        return false;
    }
}

void ServiceHandler::onlineMoveFolder(quint64 action, const QMailFolderId &folderId, const QMailFolderId &newParentId)
{
    if (folderId.isValid()) {
        QSet<QMailAccountId> accounts = folderAccount(folderId);
        QSet<QMailMessageService *> sources(sourceServiceSet(accounts));
        enqueueRequest(action, serialize(folderId, newParentId), sources, &ServiceHandler::dispatchOnlineMoveFolder, &ServiceHandler::storageActionCompleted, MoveFolderRequestType);
    } else {
        reportFailure(action, QMailServiceAction::Status::ErrInvalidData, tr("Unable to move invalid folder"));
    }
}

bool ServiceHandler::dispatchOnlineMoveFolder(quint64 action, const QByteArray &data)
{
    QMailFolderId folderId;
    QMailFolderId newParentId;

    deserialize(data, folderId, newParentId);

    if (QMailMessageSource *source = accountSource(QMailFolder(folderId).parentAccountId())) {
        bool success(sourceService.value(source)->usesConcurrentActions()
                     ? source->moveFolder(folderId, newParentId, action)
                     : source->moveFolder(folderId, newParentId));
        if (success) {
            return true;
        } else {
            qWarning() << "Unable to service request to move folder id:" << folderId;
            return false;
        }

    } else {
        reportFailure(action, QMailServiceAction::Status::ErrFrameworkFault, tr("Unable to locate source for account"), QMailFolder(folderId).parentAccountId());
        return false;
    }
}

void ServiceHandler::searchMessages(quint64 action, const QMailMessageKey& filter, const QString& bodyText, QMailSearchAction::SearchSpecification spec, const QMailMessageSortKey &sort)
{
    searchMessages(action, filter, bodyText, spec, 0, sort, NoLimit);
}

void ServiceHandler::searchMessages(quint64 action, const QMailMessageKey& filter, const QString& bodyText, QMailSearchAction::SearchSpecification spec, quint64 limit, const QMailMessageSortKey &sort)
{
    searchMessages(action, filter, bodyText, spec, limit, sort, Limit);
}

void ServiceHandler::countMessages(quint64 action, const QMailMessageKey& filter, const QString& bodyText)
{
    searchMessages(action, filter, bodyText, QMailSearchAction::Remote, 0, QMailMessageSortKey(), Count);
}

void ServiceHandler::searchMessages(quint64 action, const QMailMessageKey& filter, const QString& bodyText, QMailSearchAction::SearchSpecification spec, quint64 limit, const QMailMessageSortKey &sort, SearchType searchType)
{
    if (spec == QMailSearchAction::Remote) {
        // Find the accounts that we need to search within from the criteria
        QSet<QMailAccountId> searchAccountIds(accountsApplicableTo(filter, sourceMap.keys().toSet()));

        QSet<QMailMessageService*> sources(sourceServiceSet(searchAccountIds));
        if (sources.isEmpty()) {
            reportFailure(action, QMailServiceAction::Status::ErrNoConnection, tr("Unable to search messages for unconfigured account"));
        } else {
            enqueueRequest(action, serialize(searchAccountIds, filter, bodyText, limit, sort, static_cast<int>(searchType)), sources, &ServiceHandler::dispatchSearchMessages, &ServiceHandler::searchCompleted, SearchMessagesRequestType);
        }
    } else {
        // Find the messages that match the filter criteria
        QMailMessageIdList searchIds(QMailStore::instance()->queryMessages(filter, sort));

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
    qint64 limit;
    QMailMessageSortKey sort;
    int searchType;
    bool sentSearch = false;

    deserialize(data, accountIds, filter, bodyText, limit, sort, searchType);

    foreach (const QMailAccountId &accountId, accountIds) {
        if (QMailMessageSource *source = accountSource(accountId)) {
            bool success(false);
            bool concurrent(sourceService.value(source)->usesConcurrentActions());
            switch (searchType)
            {
            case NoLimit:
                if (concurrent) {
                    success = source->searchMessages(filter, bodyText, sort, action);
                } else {
                    success = source->searchMessages(filter, bodyText, sort);
                }
                break;
            case Limit:
                if (concurrent) {
                    success = source->searchMessages(filter, bodyText, limit, sort, action);
                } else {
                    success = source->searchMessages(filter, bodyText, limit, sort);
                }
                break;
            case Count:
                if (concurrent) {
                    success = source->countMessages(filter, bodyText, action);
                } else {
                    success = source->countMessages(filter, bodyText);
                }
                break;
            default:
                qWarning() << "Unknown SearchType";
                break;
            }

            //only dispatch to appropriate account
            if (success) {
                sentSearch = true; //we've at least sent one
            } else {
                //do it locally instead
                qWarning() << "Unable to do remote search, doing it locally instead";
                mSearches.append(MessageSearch(action, QMailStore::instance()->queryMessages(filter, sort), bodyText));
                QTimer::singleShot(0, this, SLOT(continueSearch()));
            }
        } else {
            reportFailure(action, QMailServiceAction::Status::ErrFrameworkFault, tr("Unable to locate source for account"), accountId);
        }
    }

    return sentSearch;
}


void ServiceHandler::cancelLocalSearch(quint64 action) // cancel local search
{
    QList<MessageSearch>::iterator it = mSearches.begin(), end = mSearches.end();
    for ( ; it != end; ++it) {
        if ((*it).action() == action) {
            if (!mMatchingIds.isEmpty())
                emit matchingMessageIds(action, mMatchingIds);

            emit searchCompleted(action);

            mSearches.erase(it);
            return;
        }
    }
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
        QMailActionData t(i.key(), i->description, i->progressTotal, i->progressCurrent,
                          i->status.errorCode, i->status.text,
                          i->status.accountId, i->status.folderId, i->status.messageId);
        list.append(t);
    }

    emit actionsListed(list);
}

// concurrent actions
void ServiceHandler::statusChanged(const QMailServiceAction::Status s, quint64 a)
{
    QMap<quint64, ActionData>::iterator it = mActiveActions.find(a);
    if (it != mActiveActions.end()) {
        it->status = s;
    }
    emit statusChanged(a, s);
}

void ServiceHandler::availabilityChanged(bool b, quint64 a)
{
    emit availabilityChanged(a, b);
}

void ServiceHandler::connectivityChanged(QMailServiceAction::Connectivity c, quint64 a)
{
    emit connectivityChanged(a, c);
}

void ServiceHandler::activityChanged(QMailServiceAction::Activity act, quint64 a)
{
    emit activityChanged(a, act);
}

void ServiceHandler::progressChanged(uint p, uint t, quint64 a)
{
    QMap<quint64, ActionData>::iterator it = mActiveActions.find(a);
    if (it != mActiveActions.end()) {
        it->progressCurrent = p;
        it->progressTotal = t;
    }
    emit progressChanged(a, p, t);
}

void ServiceHandler::actionCompleted(bool success, quint64 action)
{
   qMailLog(Messaging) << "Action completed" << action << "result" << (success ? "success" : "failure");
   QMailMessageService *service = qobject_cast<QMailMessageService*>(sender());
   Q_ASSERT(service);

   actionCompleted(success, service, action);
}

void ServiceHandler::actionCompleted(bool success, QMailMessageService *service, quint64 action)
{
    qMailLog(Messaging) << "Action completed" << action << "result" << (success ? "success" : "failure");
    QMap<quint64, ActionData>::iterator it = mActiveActions.find(action);
    if (it != mActiveActions.end()) {
        ActionData &data(it.value());

        if (!mSentIds.isEmpty() && (data.completion == &ServiceHandler::transmissionCompleted)) {
            if (accountSource(service->accountId())) {
                // Mark these message as Sent
                quint64 setMask(QMailMessage::Sent);
                quint64 unsetMask(QMailMessage::Outbox | QMailMessage::Draft | QMailMessage::LocalOnly);

                QMailMessageKey idsKey(QMailMessageKey::id(mSentIds));

                bool failed = !QMailStore::instance()->updateMessagesMetaData(idsKey, setMask, true);
                failed = !QMailStore::instance()->updateMessagesMetaData(idsKey, unsetMask, false) || failed;

                if (failed) {
                    qWarning() << "Unable to flag messages:" << mSentIds;
                }

                // FWOD messages have already been uploaded to the remote server, don't try to upload twice
                quint64 externalStatus(QMailMessage::TransmitFromExternal | QMailMessage::HasUnresolvedReferences);
                QMailMessageKey externalKey(QMailMessageKey::status(externalStatus, QMailDataComparator::Includes));
                QMailMessageKey sentIdsKey(QMailMessageKey::id(mSentIds));
                QMailMessageIdList sentNonFwodIds = QMailStore::instance()->queryMessages(sentIdsKey & ~externalKey);
                
                // Move sent messages to sent folder on remote server
                QMap<QMailAccountId, QMailMessageIdList> groupedMessages(accountMessages(sentNonFwodIds));
                if (!groupedMessages.empty()) { // messages are still around
                    enqueueRequest(newLocalActionId(),
                        serialize(groupedMessages, setMask, unsetMask),
                        sourceServiceSet(service->accountId()),
                        &ServiceHandler::dispatchOnlineFlagMessagesAndMoveToStandardFolder,
                        &ServiceHandler::storageActionCompleted,
                        FlagMessagesRequestType);
                }
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


    // See if there are pending requests
    QTimer::singleShot(0, this, SLOT(dispatchRequest()));
}

void ServiceHandler::messagesTransmitted(const QMailMessageIdList& ml, quint64 a)
{
    emit messagesTransmitted(a, ml);
}

void ServiceHandler::messagesFailedTransmission(const QMailMessageIdList& ml, QMailServiceAction::Status::ErrorCode err, quint64 a)
{
    emit messagesFailedTransmission(a, ml, err);
}

void ServiceHandler::messagesDeleted(const QMailMessageIdList& ml, quint64 a)
{
    emit messagesDeleted(a, ml);
}

void ServiceHandler::messagesCopied(const QMailMessageIdList& ml, quint64 a)
{
    emit messagesCopied(a, ml);
}

void ServiceHandler::messagesMoved(const QMailMessageIdList& ml, quint64 a)
{
    emit messagesMoved(a, ml);
}

void ServiceHandler::messagesFlagged(const QMailMessageIdList& ml, quint64 a)
{
    emit messagesFlagged(a, ml);
}

void ServiceHandler::messagesPrepared(const QMailMessageIdList& ml, quint64 a)
{
    Q_UNUSED(ml);
    Q_UNUSED(a);
}

void ServiceHandler::matchingMessageIds(const QMailMessageIdList& ml, quint64 a)
{
    emit matchingMessageIds(a, ml);
}

void ServiceHandler::remainingMessagesCount(uint count, quint64 a)
{
    emit remainingMessagesCount(a, count);
}

void ServiceHandler::messagesCount(uint count, quint64 a)
{
    emit messagesCount(a, count);
}

void ServiceHandler::protocolResponse(const QString &response, const QVariant &data, quint64 a)
{
    emit protocolResponse(a, response, data);
}

// end concurrent actions

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
        bool success(sourceService.value(source)->usesConcurrentActions()
                     ? source->protocolRequest(accountId, request, requestData, action)
                     : source->protocolRequest(accountId, request, requestData));
        if (!success) {
            qWarning() << "Unable to service request to forward protocol-specific request to account:" << accountId;
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

void ServiceHandler::remainingMessagesCount(uint count)
{
    if (quint64 action = sourceAction(qobject_cast<QMailMessageSource*>(sender())))
        emit remainingMessagesCount(action, count);
}

void ServiceHandler::messagesCount(uint count)
{
    if (quint64 action = sourceAction(qobject_cast<QMailMessageSource*>(sender())))
        emit messagesCount(action, count);
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
            QMap<quint64, ActionData>::iterator it = mActiveActions.find(action);
            if (it != mActiveActions.end()) {
                it->status = status;
            }
            reportFailure(action, status);
        }
}

void ServiceHandler::progressChanged(uint progress, uint total)
{
    if (QMailMessageService *service = qobject_cast<QMailMessageService*>(sender()))
        if (quint64 action = serviceAction(service)) {
            updateAction(action);
            QMap<quint64, ActionData>::iterator it = mActiveActions.find(action);
            if (it != mActiveActions.end()) {
                it->progressCurrent = progress;
                it->progressTotal = total;
            }
            emit progressChanged(action, progress, total);
        }
}

void ServiceHandler::actionCompleted(bool success)
{
    if (QMailMessageService *service = qobject_cast<QMailMessageService*>(sender())) {
        if (quint64 action = serviceAction(service)) {
            actionCompleted(success, service, action);
            return;
        }
    }

    qWarning() << "Would not determine server/action completing";
    // See if there are pending requests
    QTimer::singleShot(0, this, SLOT(dispatchRequest()));
}

void ServiceHandler::reportFailure(quint64 action, QMailServiceAction::Status::ErrorCode code, const QString &text, const QMailAccountId &accountId, const QMailFolderId &folderId, const QMailMessageId &messageId)
{
    reportFailure(action, QMailServiceAction::Status(code, text, accountId, folderId, messageId));
}

void ServiceHandler::reportFailure(quint64 action, const QMailServiceAction::Status status)
{
    if (status.errorCode != QMailServiceAction::Status::ErrNoError
        && mActiveActions.contains(action)) {
        markFailedMessage(mActiveActions[action].description, status);
    }

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


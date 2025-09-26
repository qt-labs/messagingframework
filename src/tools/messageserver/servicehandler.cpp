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
#include <longstream_p.h>
#include <qmflist.h>
#include <qmailmessageserver.h>
#include <qmailserviceconfiguration.h>
#include <qmailstore.h>
#include <qmaildisconnected.h>
#include <qmaillog.h>
#include <qmailmessage.h>
#include <qmailcontentmanager.h>
#include <qmailnamespace.h>

#include <QByteArray>
#include <QIODevice>
#include <QCoreApplication>
#include <QDir>
#include <QDateTime>
#include <QTimer>

namespace {
QSet<QMailAccountId> messageAccounts(const QMailMessageIdList &ids)
{
    QSet<QMailAccountId> accountIds; // accounts that own these messages

    for (const QMailMessageMetaData &metaData : QMailStore::instance()->messagesMetaData(
                                                       QMailMessageKey::id(ids),
                                                       QMailMessageKey::ParentAccountId,
                                                       QMailStore::ReturnDistinct)) {
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

        for (const QMailMessageMetaData &metaData : QMailStore::instance()->messagesMetaData(QMailMessageKey::id(ids),
                                                                                                QMailMessageKey::Id | QMailMessageKey::ParentAccountId,
                                                                                                QMailStore::ReturnAll)) {
            if (metaData.id().isValid() && metaData.parentAccountId().isValid())
                map[metaData.parentAccountId()].append(metaData.id());
        }

    return map;
}

quint64 newLocalActionId()
{
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

            for (QMailMessageKey::ArgumentType const& arg : key.arguments()) {
                switch (arg.property) {
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
                for (QMailMessageKey const& k : key.subKeys()) {
                    IncludedExcludedPair v(extractAccounts(k));
                    included.unite(v.first);
                    excluded.unite(v.second);
                }
            } else if (key.combiner() == QMailKey::And) {
                bool filled(included.size() != 0 || excluded.size() != 0);

                for (QmfList<QMailMessageKey>::const_iterator it(key.subKeys().begin()) ; it != key.subKeys().end() ; ++it) {
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
    return QMail::tempPath() + "/qmf-messageserver-requests";
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

QList<QString> contentIdentifiers(const QList<QMailMessageMetaData*> &list)
{
    QList<QString> result;
    for (QMailMessageMetaData *m : list) {
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

const int ExpirySeconds = 120;

void markFailedMessage(const QMailServiceAction::Status &status)
{
    if (status.messageId.isValid()) {
        QMailMessageMetaData metaData(status.messageId);

        if (metaData.customField(DontSend) != "true") {
            uint sendFailedCount = metaData.customField(DontSend).toUInt();
            ++sendFailedCount;
            if (sendFailedCount >= MaxSendFailedAttempts) {
                metaData.setCustomField(DontSend, "true");
            } else {
                metaData.setCustomField(DontSend, QString::number(sendFailedCount));
            }
        }
        metaData.setCustomField(SendFailedTime,
                                QString::number(QDateTime::currentDateTimeUtc().toMSecsSinceEpoch() / 1000));
        QMailStore::instance()->updateMessagesMetaData(QMailMessageKey::id(status.messageId),
                                                       QMailMessageKey::Custom, metaData);
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

    QMailMessageServer::registerTypes();

    if (QMailStore *store = QMailStore::instance()) {
        connect(store, &QMailStore::accountsAdded,
                this, &ServiceHandler::onAccountsAdded);
        connect(store, &QMailStore::accountsUpdated,
                this, &ServiceHandler::onAccountsUpdated);
        connect(store, &QMailStore::accountsRemoved,
                this, &ServiceHandler::onAccountsRemoved);

        // All necessary accounts now exist - create the service instances
        registerAccountServices(store->queryAccounts());
    }

    // See if there are any requests remaining from our previous run
    if (_requestsFile.exists()) {
        if (!_requestsFile.open(QIODevice::ReadOnly)) {
            qCWarning(lcMessaging) << "Unable to open requests file for read!";
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
        qCWarning(lcMessaging) << "Unable to open requests file for write!" << _requestsFile.fileName();
    } else {
        if (!_requestsFile.resize(0)) {
            qCWarning(lcMessaging) << "Unable to truncate requests file!";
        } else {
            _requestsFile.flush();
        }
    }

    if (!_failedRequests.isEmpty()) {
        // Allow the clients some time to reconnect, then report our failures
        QTimer::singleShot(2000, this, &ServiceHandler::reportPastFailures);
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
                    qCWarning(lcMessaging) << "Unable to locate master account:" << masterId << "for account:" << id;
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
    if (!removeService)
        return;

    for (QMap<quint64, ActionData>::iterator it(mActiveActions.begin()) ; it != mActiveActions.end(); ++it) {
       if (it->services.remove(removeService)) {
           qCDebug(lcMessaging) << "Removed service from action";
       }
    }

    for (auto it(mRequests.begin()); it != mRequests.end();) {
        if ((*it)->services.contains(removeService) || (*it)->preconditions.contains(removeService)) {
            reportFailure((*it)->action, QMailServiceAction::Status::ErrFrameworkFault,
                          tr("Service became unavailable, couldn't dispatch"));
            it = mRequests.erase(it);
            continue;
        }
        ++it;
    }
}

void ServiceHandler::deregisterAccountServices(const QMailAccountIdList &ids,
                                               QMailServiceAction::Status::ErrorCode code, const QString &text)
{
    QMap<QPair<QMailAccountId, QString>, QPointer<QMailMessageService> >::iterator it = serviceMap.begin();
    while (it != serviceMap.end()) {
        if (ids.contains(it.key().first)) {
            // Remove any services associated with this account
            if (QMailMessageService *service = it.value()) {
                qCDebug(lcMessaging) << "Deregistering service:" << service->service() << "for account:" << it.key().first;
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
    foreach (QMailAccountId const& id, ids) {
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
                    qCWarning(lcMessaging) << "Unable to locate master account:" << masterId << "for account:" << id;
                }
            } else { // not using a master account
                QSet<QString> newServices;
                QSet<QString> oldServices;

                foreach (const QString& service, config.services()) {
                    QMailServiceConfiguration svcCfg(&config, service);

                    if (svcCfg.type() == QMailServiceConfiguration::Source
                        || svcCfg.type() == QMailServiceConfiguration::Sink
                        || svcCfg.type() == QMailServiceConfiguration::SourceAndSink) {
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
                                qCDebug(lcMessaging) << "Deregistering service:" << service->service() << "for account:" << it.key().first;
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

                foreach (QString const& oldService, oldServices) {
                    deregisterAccountService(id, oldService, code, text);
                }
                foreach (QString const& newService, newServices) {
                    registerAccountService(id, QMailServiceConfiguration(&config, newService));
                }
            }
        } else { // account is not enabled, disable all services
            totalDeregisterAccountList.push_back(id);
        }
    }

    deregisterAccountServices(totalDeregisterAccountList, code, text);
}

void ServiceHandler::deregisterAccountService(const QMailAccountId &id, const QString &serviceName,
                                              QMailServiceAction::Status::ErrorCode code, const QString &text)
{
    QMailMessageService *service = nullptr;

    QMap<QPair<QMailAccountId, QString>, QPointer<QMailMessageService> >::iterator it = serviceMap.begin();
    while (it != serviceMap.end()) {
        if (it.key().first == id && it.key().second == serviceName) {
            // Remove any services associated with this account
            service = it.value();
            qCDebug(lcMessaging) << "Deregistering service:" << service->service() << "for account:" << it.key().first;
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

void ServiceHandler::onAccountsAdded(const QMailAccountIdList &ids)
{
    registerAccountServices(ids);
}

void ServiceHandler::onAccountsUpdated(const QMailAccountIdList &ids)
{
    // Only respond to updates that were generated by other processes
    if (QMailStore::instance()->asynchronousEmission()) {
        reregisterAccountServices(ids, QMailServiceAction::Status::ErrInternalStateReset, tr("Account updated by other process"));
    }
}

void ServiceHandler::onAccountsRemoved(const QMailAccountIdList &ids)
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

    connect(source, SIGNAL(newMessagesAvailable(quint64)), this, SIGNAL(newMessagesAvailable())); // TODO: <- I don't this this makes much sense...
    connect(source, SIGNAL(newMessagesAvailable()), this, SIGNAL(newMessagesAvailable()));

    // if (service->usesConcurrentActions()) {  // This can be uncommented to be stricter.
    connect(source, SIGNAL(messagesDeleted(QMailMessageIdList, quint64)), this, SLOT(emitMessagesDeleted(QMailMessageIdList, quint64)));
    connect(source, SIGNAL(messagesCopied(QMailMessageIdList, quint64)), this, SLOT(emitMessagesCopied(QMailMessageIdList, quint64)));
    connect(source, SIGNAL(messagesMoved(QMailMessageIdList, quint64)), this, SLOT(emitMessagesMoved(QMailMessageIdList, quint64)));
    connect(source, SIGNAL(messagesFlagged(QMailMessageIdList, quint64)), this, SLOT(emitMessagesFlagged(QMailMessageIdList, quint64)));
    connect(source, SIGNAL(messagesPrepared(QMailMessageIdList, quint64)), this, SLOT(emitMessagesPrepared(QMailMessageIdList, quint64)));
    connect(source, SIGNAL(matchingMessageIds(QMailMessageIdList, quint64)), this, SLOT(emitMatchingMessageIds(QMailMessageIdList, quint64)));
    connect(source, SIGNAL(remainingMessagesCount(uint, quint64)), this, SLOT(emitRemainingMessagesCount(uint, quint64)));
    connect(source, SIGNAL(messagesCount(uint, quint64)), this, SLOT(emitMessagesCount(uint, quint64)));
    connect(source, SIGNAL(protocolResponse(QString, QVariantMap, quint64)), this, SLOT(emitProtocolResponse(QString, QVariantMap, quint64)));
    // } else {
    connect(source, SIGNAL(messagesDeleted(QMailMessageIdList)), this, SLOT(onMessagesDeleted(QMailMessageIdList)));
    connect(source, SIGNAL(messagesCopied(QMailMessageIdList)), this, SLOT(onMessagesCopied(QMailMessageIdList)));
    connect(source, SIGNAL(messagesMoved(QMailMessageIdList)), this, SLOT(onMessagesMoved(QMailMessageIdList)));
    connect(source, SIGNAL(messagesFlagged(QMailMessageIdList)), this, SLOT(onMessagesFlagged(QMailMessageIdList)));
    connect(source, SIGNAL(messagesPrepared(QMailMessageIdList)), this, SLOT(onMessagesPrepared(QMailMessageIdList)));
    connect(source, SIGNAL(matchingMessageIds(QMailMessageIdList)), this, SLOT(onMatchingMessageIds(QMailMessageIdList)));
    connect(source, SIGNAL(remainingMessagesCount(uint)), this, SLOT(onRemainingMessagesCount(uint)));
    connect(source, SIGNAL(messagesCount(uint)), this, SLOT(onMessagesCount(uint)));
    connect(source, SIGNAL(protocolResponse(QString, QVariantMap)), this, SLOT(onProtocolResponse(QString, QVariantMap)));
    // }
}

QMailMessageSource *ServiceHandler::accountSource(const QMailAccountId &accountId) const
{
    QMap<QMailAccountId, QMailMessageSource*>::const_iterator it = sourceMap.find(transmissionAccountId(accountId));
    if (it != sourceMap.end())
        return *it;

    return nullptr;
}

void ServiceHandler::registerAccountSink(const QMailAccountId &accountId, QMailMessageSink *sink, QMailMessageService *service)
{
    sinkMap.insert(accountId, sink);
    sinkService.insert(sink, service);

    // if (service->usesConcurrentActions()) { // this can be uncommented to be stricter
    connect(sink, SIGNAL(messagesTransmitted(QMailMessageIdList, quint64)),
            this, SLOT(emitMessagesTransmitted(QMailMessageIdList, quint64)));
    connect(sink, SIGNAL(messagesFailedTransmission(QMailMessageIdList, QMailServiceAction::Status::ErrorCode, quint64)),
            this, SLOT(emitMessagesFailedTransmission(QMailMessageIdList, QMailServiceAction::Status::ErrorCode, quint64)));

    // } else {
    connect(sink, SIGNAL(messagesTransmitted(QMailMessageIdList)),
            this, SLOT(onMessagesTransmitted(QMailMessageIdList)));
    connect(sink, SIGNAL(messagesFailedTransmission(QMailMessageIdList, QMailServiceAction::Status::ErrorCode)),
            this, SLOT(onMessagesFailedTransmission(QMailMessageIdList, QMailServiceAction::Status::ErrorCode)));
    // }
}

QMailMessageSink *ServiceHandler::accountSink(const QMailAccountId &accountId) const
{
    QMap<QMailAccountId, QMailMessageSink*>::const_iterator it = sinkMap.find(transmissionAccountId(accountId));
    if (it != sinkMap.end())
        return *it;

    return nullptr;
}

QMailMessageService *ServiceHandler::createService(const QString &name, const QMailAccountId &accountId)
{
    QMailMessageService *service = QMailMessageServiceFactory::createService(name, accountId);
    connect(service, &QMailMessageService::connectivityChanged,
            this, &ServiceHandler::onConnectivityChanged);
    connect(service, &QMailMessageService::availabilityChanged,
            this, &ServiceHandler::onAvailabilityChanged);

    if (service) {
        // if (service->usesConcurrentActions()) { // this can be uncommented to be stricter
        connect(service, SIGNAL(activityChanged(QMailServiceAction::Activity, quint64)),
                this, SLOT(emitActivityChanged(QMailServiceAction::Activity, quint64)));
        connect(service, SIGNAL(statusChanged(const QMailServiceAction::Status, quint64)),
                this, SLOT(onStatusChanged(const QMailServiceAction::Status, quint64)));
        connect(service, SIGNAL(progressChanged(uint, uint, quint64)),
                this, SLOT(onProgressChanged(uint, uint, quint64)));
        connect(service, SIGNAL(actionCompleted(bool, quint64)),
                this, SLOT(onActionCompleted(bool, quint64)));
        // } else {
        connect(service, SIGNAL(activityChanged(QMailServiceAction::Activity)),
                this, SLOT(onActivityChanged(QMailServiceAction::Activity)));
        connect(service, SIGNAL(statusChanged(const QMailServiceAction::Status)),
                this, SLOT(onStatusChanged(const QMailServiceAction::Status)));
        connect(service, SIGNAL(progressChanged(uint, uint)),
                this, SLOT(onProgressChanged(uint, uint)));
        connect(service, SIGNAL(actionCompleted(bool)),
                this, SLOT(onActionCompleted(bool)));
        // }
    }

    return service;
}

void ServiceHandler::registerAccountService(const QMailAccountId &accountId, const QMailServiceConfiguration &svcCfg)
{
    QMap<QPair<QMailAccountId, QString>, QPointer<QMailMessageService> >::const_iterator it
        = serviceMap.find(qMakePair(accountId, svcCfg.service()));
    if (it == serviceMap.end()) {
        // We need to create a new service for this account
        if (QMailMessageService *service = createService(svcCfg.service(), accountId)) {
            serviceMap.insert(qMakePair(accountId, svcCfg.service()), service);
            qCDebug(lcMessaging) << "Registering service:" << svcCfg.service() << "for account:" << accountId;

            if (service->hasSource())
                registerAccountSource(accountId, &service->source(), service);

            if (service->hasSink())
                registerAccountSink(accountId, &service->sink(), service);

            if (!service->available())
                mUnavailableServices.insert(service);
        } else {
            qCWarning(lcMessaging) << "Unable to instantiate service:" << svcCfg.service();
        }
    }
}

bool ServiceHandler::servicesAvailable(const Request &req) const
{
    foreach (QPointer<QMailMessageService> service, (req.services + req.preconditions)) {
        if (!serviceAvailable(service))
            return false;
    }
    return true;
}

bool ServiceHandler::serviceAvailable(QPointer<QMailMessageService> service) const
{
    if (!service
        || (!service->usesConcurrentActions() && mServiceAction.contains(service))
        || mUnavailableServices.contains(service)) {
        return false;
    }

    return true;
}

QSet<QMailMessageService*> ServiceHandler::sourceServiceSet(const QMailAccountId &id) const
{
    QSet<QMailMessageService*> services;

    // See if this account's service has a source
    if (QMailMessageSource *source = accountSource(id)) {
        services.insert(sourceService.value(source));
    } else {
        qCWarning(lcMessaging) << "Unable to find message source for account:" << id;
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
            qCWarning(lcMessaging) << "Unable to find message source for account:" << id;
            return QSet<QMailMessageService*>();
        }
    }

    return services;
}

QSet<QMailMessageService*> ServiceHandler::sourceServiceSet(const QMailAccountIdList &ids) const
{
    return sourceServiceSet(QSet<QMailAccountId>(ids.constBegin(), ids.constEnd()));
}

QSet<QMailMessageService*> ServiceHandler::sinkServiceSet(const QMailAccountId &id) const
{
    QSet<QMailMessageService*> services;

    if (QMailMessageSink *sink = accountSink(id)) {
        services.insert(sinkService[sink]);
    } else {
        qCWarning(lcMessaging) << "Unable to find message sink for account:" << id;
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
            qCWarning(lcMessaging) << "Unable to find message sink for account:" << id;
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

void ServiceHandler::enqueueRequest(Request *req, quint64 action,
                                    const QSet<QMailMessageService*> &services,
                                    RequestServicer servicer, CompletionSignal completion,
                                    const QSet<QMailMessageService*> &preconditions)
{
    QSet<QPointer<QMailMessageService> > safeServices;
    QSet<QPointer<QMailMessageService> > safePreconditions;
    foreach (QMailMessageService *service, services)
        safeServices.insert(service);
    foreach (QMailMessageService *precondition, preconditions)
        safePreconditions.insert(precondition);

    req->action = action;
    req->services = safeServices;
    req->preconditions = safePreconditions;
    req->servicer = servicer;
    req->completion = completion;

    mRequests.append(QSharedPointer<Request>(req));

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
    auto it(mRequests.begin());

    while (it != mRequests.end()) {
        if (!servicesAvailable(*(*it))) {
            ++it;
            continue;
        }

        // Limit number of concurrent actions serviced on the device
        if (mActiveActions.count() >= QMail::maximumConcurrentServiceActions())
            return;

        Request *request = it->data();

        // Limit number of concurrent actions serviced per process
        const quint64 requestProcess = request->action >> 32;
        int requestProcessCount = 1; // including the request
        foreach (quint64 actionId, mActiveActions.keys()) {
            if ((actionId >> 32) == requestProcess) {
                if (++requestProcessCount > QMail::maximumConcurrentServiceActionsPerProcess()) {
                    break;
                }
            }
        }

        if (requestProcessCount > QMail::maximumConcurrentServiceActionsPerProcess()) {
            ++it;
            continue;
        }

        // Associate the services with the action, so that signals are reported correctly
        foreach (QMailMessageService *service, request->services)
            mServiceAction.insert(service, request->action);

        // The services required for this request are available
        ActionData data;
        data.services = request->services;
        data.completion = request->completion;
        // TODO: should use monotonic time, maybe QElapsedTimer
        data.unixTimeExpiry = QDateTime::currentDateTime().toMSecsSinceEpoch() / 1000 + ExpirySeconds;
        data.reported = false;
        data.requestType = request->requestType;
        data.progressTotal = 0;
        data.progressCurrent = 0;
        data.status = QMailServiceAction::Status(QMailServiceAction::Status::ErrNoError,
                                                 QString(), QMailAccountId(), QMailFolderId(), QMailMessageId());

        mActiveActions.insert(request->action, data);
        qCDebug(lcMessaging) << "Running action" << ::requestTypeNames[data.requestType] << request->action;

        emit actionStarted(QMailActionData(request->action, request->requestType, 0, 0,
                                           data.status.errorCode, data.status.text,
                                           data.status.accountId, data.status.folderId, data.status.messageId));
        emit activityChanged(request->action, QMailServiceAction::InProgress);

        if ((this->*request->servicer)(request)) {
            // This action is now underway

            if (mActionExpiry.isEmpty()) {
                // Start the expiry timer. Convert to milliseconds, and avoid shooting too early
                const int expiryMs = ExpirySeconds * 1000;
                QTimer::singleShot(expiryMs + 50, this, SLOT(expireAction()));
            }
            mActionExpiry.append(request->action);
        } else {
            mActiveActions.remove(request->action);

            qCWarning(lcMessaging) << "Unable to dispatch request:" << request->action << "to services:" << request->services;
            emit activityChanged(request->action, QMailServiceAction::Failed);

            foreach (QMailMessageService *service, request->services)
                mServiceAction.remove(service);
        }

        it = mRequests.erase(it);
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
        mActiveActions[action].unixTimeExpiry = QDateTime::currentDateTime().toMSecsSinceEpoch() / 1000 + ExpirySeconds;
    }
}

void ServiceHandler::expireAction()
{
    quint64 now(QDateTime::currentDateTime().toMSecsSinceEpoch() / 1000);

    if (!mActionExpiry.isEmpty()) {
        quint64 action = *mActionExpiry.begin();

        QMap<quint64, ActionData>::iterator it = mActiveActions.find(action);
        if (it != mActiveActions.end()) {
            ActionData &data(it.value());

            // Is the oldest action expired?
            if (data.unixTimeExpiry <= now) {
                qCWarning(lcMessaging) << "Expired request:" << action;
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
                            if (data.completion == &ServiceHandler::retrievalReady) {
                                if (_retrievalAccountIds.contains(accountId)) {
                                    _retrievalAccountIds.remove(accountId);
                                    retrievalSetModified = true;
                                }
                            } else if (data.completion == &ServiceHandler::transmissionReady) {
                                if (_transmissionAccountIds.contains(accountId)) {
                                    _transmissionAccountIds.remove(accountId);
                                    transmissionSetModified = true;
                                }
                            }
                        }
                    }

                    if (retrievalSetModified) {
                        QMailStore::instance()->setRetrievalInProgress(QMailAccountIdList(_retrievalAccountIds.constBegin(),
                                                                                          _retrievalAccountIds.constEnd()));
                    }
                    if (transmissionSetModified) {
                        QMailStore::instance()->setTransmissionInProgress(QMailAccountIdList(_transmissionAccountIds.constBegin(),
                                                                                             _transmissionAccountIds.constEnd()));
                    }

                    mActiveActions.erase(it);
                }

                mActionExpiry.removeFirst();

                // Restart the service(s) for each of these accounts
                QMailAccountIdList ids(serviceAccounts.constBegin(), serviceAccounts.constEnd());
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
            quint64 nextExpiry(mActiveActions.value(*expiryIt).unixTimeExpiry);

            // milliseconds until it expires
            quint64 nextShot(nextExpiry <= now ? 0 : (nextExpiry - now) * 1000 + 50);
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
                qCWarning(lcMessaging) << "Unable to cancel null service for action:" << action;
                continue;
            }

            QMailAccountId accountId(service->accountId());
            if (accountId.isValid()) {
                if (data.completion == &ServiceHandler::retrievalReady) {
                    if (_retrievalAccountIds.contains(accountId)) {
                        _retrievalAccountIds.remove(accountId);
                        retrievalSetModified = true;
                    }
                } else if (data.completion == &ServiceHandler::transmissionReady) {
                    if (_transmissionAccountIds.contains(accountId)) {
                        _transmissionAccountIds.remove(accountId);
                        transmissionSetModified = true;
                    }
                }
            }
        }

        if (retrievalSetModified) {
            QMailStore::instance()->setRetrievalInProgress(QMailAccountIdList(_retrievalAccountIds.constBegin(),
                                                                              _retrievalAccountIds.constEnd()));
        }
        if (transmissionSetModified) {
            QMailStore::instance()->setTransmissionInProgress(QMailAccountIdList(_transmissionAccountIds.constBegin(),
                                                                                 _transmissionAccountIds.constEnd()));
        }

        //The ActionData might have already been deleted by actionCompleted, triggered by cancelOperation
        it = mActiveActions.find(action);
        if (it != mActiveActions.end())
            mActiveActions.erase(it);

        // See if there are more actions
        QTimer::singleShot(0, this, SLOT(dispatchRequest()));
    } else {
        // See if this is a pending request that we can abort
        auto it = mRequests.begin(), end = mRequests.end();
        for ( ; it != end; ++it) {
            if ((*it)->action == action) {
                mRequests.erase(it);
                break;
            }
        }

        // Report this action as failed
        reportFailure(action, QMailServiceAction::Status::ErrCancel, tr("Cancelled by user"));
    }
}

// TODO: should preparation part be somehow combined with the transmit requests to avoid
// internal handling being visible outside
class PrepareMessagesRequest : public Request
{
public:
    PrepareMessagesRequest(const QMap<QMailAccountId,
                                      QList<QPair<QMailMessagePart::Location,
                                                  QMailMessagePart::Location> > > &unresolvedLists)
        : Request(TransmitMessagesRequestType)
        , unresolvedLists(unresolvedLists) {}

    QMap<QMailAccountId, QList<QPair<QMailMessagePart::Location, QMailMessagePart::Location> > > unresolvedLists;
};

class TransmitAccountMessagesRequest : public Request
{
public:
    TransmitAccountMessagesRequest(QMailAccountId accountId)
        : Request(TransmitMessagesRequestType)
        , accountId(accountId) {}

    QMailAccountId accountId;
};

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
            QMap<QMailAccountId,
                 QList<QPair<QMailMessagePart::Location,
                             QMailMessagePart::Location> > > unresolvedLists(messageResolvers(unresolvedMessages));

            sources = sourceServiceSet(unresolvedLists.keys());

            // Emit no signal after completing preparation
            enqueueRequest(new PrepareMessagesRequest(unresolvedLists), action, sources, &ServiceHandler::dispatchPrepareMessages, 0);
        }

        // The transmit action is dependent on the availability of the sources, since they must complete their preparation step first
        enqueueRequest(new TransmitAccountMessagesRequest(accountId), action, sinks,
                       &ServiceHandler::dispatchTransmitAccountMessages, &ServiceHandler::transmissionReady, sources);
    }
}

class TransmitMessageRequest : public Request
{
public:
    TransmitMessageRequest(QMailMessageId id)
        : Request(TransmitMessagesRequestType)
        , messageId(id) {}

    QMailMessageId messageId;
};

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
            QMap<QMailAccountId,
                 QList<QPair<QMailMessagePart::Location,
                             QMailMessagePart::Location> > > unresolvedLists(messageResolvers(unresolvedMessages));

            sources = sourceServiceSet(unresolvedLists.keys());

            // Emit no signal after completing preparation
            enqueueRequest(new PrepareMessagesRequest(unresolvedLists), action, sources, &ServiceHandler::dispatchPrepareMessages, 0);
        }

        // The transmit action is dependent on the availability of the sources, since they must complete their preparation step first
        enqueueRequest(new TransmitMessageRequest(messageId), action, sinks,
                       &ServiceHandler::dispatchTransmitMessage, &ServiceHandler::transmissionReady, sources);
    }
}

bool ServiceHandler::dispatchPrepareMessages(Request *req)
{
    PrepareMessagesRequest *request = static_cast<PrepareMessagesRequest*>(req);

    // Prepare any unresolved messages for transmission
    for (auto it = request->unresolvedLists.begin() ; it != request->unresolvedLists.end(); ++it) {
        if (QMailMessageSource *source = accountSource(it.key())) {
            if (!source->prepareMessages(it.value())) {
                qCWarning(lcMessaging) << "Unable to service request to prepare messages for account:" << it.key();
                return false;
            } else {
                // This account is now transmitting
                setTransmissionInProgress(it.key(), true);
            }
        } else {
            reportFailure(request->action, QMailServiceAction::Status::ErrFrameworkFault,
                          tr("Unable to locate source for account"), it.key());
            return false;
        }
    }

    return true;
}

bool ServiceHandler::dispatchTransmitAccountMessages(Request *req)
{
    TransmitAccountMessagesRequest *request = static_cast<TransmitAccountMessagesRequest*>(req);

    if (QMailMessageSink *sink = accountSink(request->accountId)) {
        // Transmit any messages in the Outbox for this account
        QMailMessageKey accountKey(QMailMessageKey::parentAccountId(request->accountId));
        QMailMessageKey outboxKey(QMailMessageKey::status(QMailMessage::Outbox) & ~QMailMessageKey::status(QMailMessage::Trash));
        QMailMessageKey sendKey(QMailMessageKey::customField("dontSend", "true", QMailDataComparator::NotEqual));
        QMailMessageKey noSendKey(QMailMessageKey::customField("dontSend", QMailDataComparator::Absent));

        QMailMessageIdList toTransmit(QMailStore::instance()->queryMessages(accountKey
                                                                            & outboxKey
                                                                            & (noSendKey | sendKey)));

        bool success(sinkService.value(sink)->usesConcurrentActions()
                     ? sink->transmitMessages(toTransmit, request->action)
                     : sink->transmitMessages(toTransmit));

        if (success) {
            // This account is now transmitting
            setTransmissionInProgress(request->accountId, true);
        } else {
            qCWarning(lcMessaging) << "Unable to service request to add messages to sink for account:" << request->accountId;
            return false;
        }
    } else {
        reportFailure(request->action, QMailServiceAction::Status::ErrFrameworkFault,
                      tr("Unable to locate sink for account"), request->accountId);
        return false;
    }

    return true;
}

bool ServiceHandler::dispatchTransmitMessage(Request *req)
{
    TransmitMessageRequest *request = static_cast<TransmitMessageRequest*>(req);

    QMailMessageMetaData metaData(request->messageId);
    QMailAccountId accountId(metaData.parentAccountId());
    if (QMailMessageSink *sink = accountSink(accountId)) {
        // Transmit the messages if it's in the Outbox.
        quint64 outboxStatus(QMailMessage::Outbox & ~QMailMessage::Trash);
        bool success = false;
        QMailMessageIdList toTransmit;
        if (metaData.status() & outboxStatus) {
            toTransmit << request->messageId;

            success = sinkService.value(sink)->usesConcurrentActions()
                ? sink->transmitMessages(toTransmit, request->action)
                : sink->transmitMessages(toTransmit);
        }

        if (success) {
            // This account is now transmitting
            setTransmissionInProgress(accountId, true);
        } else {
            qCWarning(lcMessaging) << "Unable to service request to add message " << request->messageId
                                   << " to sink for account:" << accountId;
            return false;
        }
    } else {
        reportFailure(request->action, QMailServiceAction::Status::ErrFrameworkFault,
                      tr("Unable to locate sink for account"), accountId);
        return false;
    }

    return true;
}

class RetrieveFolderListRequest : public Request
{
public:
    RetrieveFolderListRequest(QMailAccountId accountId, QMailFolderId folderId, bool descending)
        : Request(RetrieveFolderListRequestType)
        , accountId(accountId)
        , folderId(folderId)
        , descending(descending) {}

    QMailAccountId accountId;
    QMailFolderId folderId;
    bool descending;
};

void ServiceHandler::retrieveFolderList(quint64 action, const QMailAccountId &accountId, const QMailFolderId &folderId, bool descending)
{
    QSet<QMailMessageService*> sources(sourceServiceSet(accountId));
    if (sources.isEmpty()) {
        reportFailure(action, QMailServiceAction::Status::ErrNoConnection,
                      tr("Unable to retrieve folder list for unconfigured account"));
    } else {
        enqueueRequest(new RetrieveFolderListRequest(accountId, folderId, descending), action, sources,
                       &ServiceHandler::dispatchRetrieveFolderListAccount, &ServiceHandler::retrievalReady);
    }
}

bool ServiceHandler::dispatchRetrieveFolderListAccount(Request *req)
{
    RetrieveFolderListRequest *request = static_cast<RetrieveFolderListRequest*>(req);

    if (QMailMessageSource *source = accountSource(request->accountId)) {
        bool success(sourceService.value(source)->usesConcurrentActions()
                            ? source->retrieveFolderList(request->accountId, request->folderId, request->descending, request->action)
                            : source->retrieveFolderList(request->accountId, request->folderId, request->descending));

        if (success) {
            // This account is now retrieving (arguably...)
            setRetrievalInProgress(request->accountId, true);
        } else {
            qCWarning(lcMessaging) << "Unable to service request to retrieve folder list for account:" << request->accountId;
            return false;
        }
    } else {
        reportFailure(request->action, QMailServiceAction::Status::ErrFrameworkFault,
                      tr("Unable to locate source for account"), request->accountId);
        return false;
    }

    return true;
}

// singular list
class RetrieveMessageListRequest : public Request
{
public:
    RetrieveMessageListRequest(QMailAccountId accountId, QMailFolderId folderId, uint minimum, QMailMessageSortKey sort)
        : Request(RetrieveMessageListRequestType)
        , accountId(accountId)
        , folderId(folderId)
        , minimum(minimum)
        , sort(sort) {}

    QMailAccountId accountId;
    QMailFolderId folderId;
    uint minimum;
    QMailMessageSortKey sort;
};

void ServiceHandler::retrieveMessageList(quint64 action, const QMailAccountId &accountId,
                                         const QMailFolderId &folderId, uint minimum, const QMailMessageSortKey &sort)
{
    QSet<QMailMessageService*> sources(sourceServiceSet(accountId));
    if (sources.isEmpty()) {
        reportFailure(action, QMailServiceAction::Status::ErrNoConnection,
                      tr("Unable to retrieve message list for unconfigured account"));
    } else {
        enqueueRequest(new RetrieveMessageListRequest(accountId, folderId, minimum, sort), action, sources,
                       &ServiceHandler::dispatchRetrieveMessageList, &ServiceHandler::retrievalReady);
    }
}

bool ServiceHandler::dispatchRetrieveMessageList(Request *req)
{
    RetrieveMessageListRequest *request = static_cast<RetrieveMessageListRequest*>(req);

    if (QMailMessageSource *source = accountSource(request->accountId)) {
        bool success(sourceService.value(source)->usesConcurrentActions()
                            ? source->retrieveMessageList(request->accountId, request->folderId, request->minimum, request->sort, request->action)
                            : source->retrieveMessageList(request->accountId, request->folderId, request->minimum, request->sort));
        if (success) {
            // This account is now retrieving
            setRetrievalInProgress(request->accountId, true);
        } else {
            qCWarning(lcMessaging) << "Unable to service request to retrieve message list for folder:" << request->folderId;
            return false;
        }
    } else {
        reportFailure(request->action, QMailServiceAction::Status::ErrFrameworkFault,
                      tr("Unable to locate source for account"), request->accountId);
        return false;
    }

    return true;
}

// plural lists. TODO: should this be combined with the singular request?
class RetrieveMessageListsRequest : public Request
{
public:
    RetrieveMessageListsRequest(QMailAccountId accountId, const QMailFolderIdList &folderIds, uint minimum, QMailMessageSortKey sort)
        : Request(RetrieveMessageListRequestType)
        , accountId(accountId)
        , folderIds(folderIds)
        , minimum(minimum)
        , sort(sort) {}

    QMailAccountId accountId;
    QMailFolderIdList folderIds;
    uint minimum;
    QMailMessageSortKey sort;
};

void ServiceHandler::retrieveMessageLists(quint64 action, const QMailAccountId &accountId,
                                          const QMailFolderIdList &folderIds, uint minimum,
                                          const QMailMessageSortKey &sort)
{
    QSet<QMailMessageService*> sources(sourceServiceSet(accountId));
    if (sources.isEmpty()) {
        reportFailure(action, QMailServiceAction::Status::ErrNoConnection,
                      tr("Unable to retrieve message list for unconfigured account"));
    } else {
        enqueueRequest(new RetrieveMessageListsRequest(accountId, folderIds, minimum, sort), action, sources,
                       &ServiceHandler::dispatchRetrieveMessageLists, &ServiceHandler::retrievalReady);
    }
}

bool ServiceHandler::dispatchRetrieveMessageLists(Request *req)
{
    RetrieveMessageListsRequest *request = static_cast<RetrieveMessageListsRequest*>(req);

    if (QMailMessageSource *source = accountSource(request->accountId)) {
        bool success(sourceService.value(source)->usesConcurrentActions()
                            ? source->retrieveMessageLists(request->accountId, request->folderIds, request->minimum, request->sort, request->action)
                            : source->retrieveMessageLists(request->accountId, request->folderIds, request->minimum, request->sort));
        if (success) {
            // This account is now retrieving
            setRetrievalInProgress(request->accountId, true);
        } else {
            qCWarning(lcMessaging) << "Unable to service request to retrieve message list for folders:" << request->folderIds;
            return false;
        }
    } else {
        reportFailure(request->action, QMailServiceAction::Status::ErrFrameworkFault,
                      tr("Unable to locate source for account"), request->accountId);
        return false;
    }

    return true;
}

class RetrieveNewMessagesRequest : public Request
{
public:
    RetrieveNewMessagesRequest(QMailAccountId accountId, QMailFolderIdList folderIds)
        : Request(RetrieveNewMessagesRequestType)
        , accountId(accountId)
        , folderIds(folderIds) {}

    QMailAccountId accountId;
    QMailFolderIdList folderIds;
};

void ServiceHandler::retrieveNewMessages(quint64 action, const QMailAccountId &accountId, const QMailFolderIdList &folderIds)
{
    QSet<QMailMessageService*> sources(sourceServiceSet(accountId));
    if (sources.isEmpty()) {
        reportFailure(action, QMailServiceAction::Status::ErrNoConnection,
                      tr("Unable to retrieve new messages for unconfigured account"));
    } else {
        enqueueRequest(new RetrieveNewMessagesRequest(accountId, folderIds), action, sources,
                       &ServiceHandler::dispatchRetrieveNewMessages, &ServiceHandler::retrievalReady);
    }
}

bool ServiceHandler::dispatchRetrieveNewMessages(Request *req)
{
    RetrieveNewMessagesRequest *request = static_cast<RetrieveNewMessagesRequest*>(req);

    if (QMailMessageSource *source = accountSource(request->accountId)) {
        bool success(sourceService.value(source)->usesConcurrentActions()
                            ? source->retrieveNewMessages(request->accountId, request->folderIds, request->action)
                            : source->retrieveNewMessages(request->accountId, request->folderIds));
        if (success) {
            // This account is now retrieving
            setRetrievalInProgress(request->accountId, true);
        } else {
            qCWarning(lcMessaging) << "Unable to service request to retrieve new messages for folders:" << request->folderIds;
            return false;
        }
    } else {
        reportFailure(request->action, QMailServiceAction::Status::ErrFrameworkFault,
                      tr("Unable to locate source for account"), request->accountId);
        return false;
    }

    return true;
}

class CreateStandardFoldersRequest : public Request
{
public:
    CreateStandardFoldersRequest(QMailAccountId accountId)
        : Request(RetrieveFolderListRequestType)
        , accountId(accountId) {}

    QMailAccountId accountId;
};

void ServiceHandler::createStandardFolders(quint64 action, const QMailAccountId &accountId)
{
    QSet<QMailMessageService*> sources(sourceServiceSet(accountId));
    if (sources.isEmpty()) {
        reportFailure(action, QMailServiceAction::Status::ErrNoConnection,
                      tr("Unable to retrieve standard folders for unconfigured account"));
    } else {
        enqueueRequest(new CreateStandardFoldersRequest(accountId), action, sources,
                       &ServiceHandler::dispatchCreateStandardFolders, &ServiceHandler::retrievalReady);
    }
}

bool ServiceHandler::dispatchCreateStandardFolders(Request *req)
{
    RetrieveNewMessagesRequest *request = static_cast<RetrieveNewMessagesRequest*>(req);

    if (QMailMessageSource *source = accountSource(request->accountId)) {
        bool success(sourceService.value(source)->usesConcurrentActions()
            ? source->createStandardFolders(request->accountId, request->action)
            : source->createStandardFolders(request->accountId));
        if (!success) {
            qCWarning(lcMessaging) << "Unable to service request to create standard folder for account:" << request->accountId;
        }

        return success;
    } else {
        reportFailure(request->action, QMailServiceAction::Status::ErrFrameworkFault,
                      tr("Unable to locate source for account"), request->accountId);
        return false;
    }
}

class RetrieveMessagesRequest : public Request
{
public:
    RetrieveMessagesRequest(const QMap<QMailAccountId, QMailMessageIdList> &messageLists,
                            QMailRetrievalAction::RetrievalSpecification spec)
        : Request(RetrieveMessagesRequestType)
        , messageLists(messageLists)
        , spec(spec) {}

    QMap<QMailAccountId, QMailMessageIdList> messageLists;
    QMailRetrievalAction::RetrievalSpecification spec;
};

void ServiceHandler::retrieveMessages(quint64 action, const QMailMessageIdList &messageIds,
                                      QMailRetrievalAction::RetrievalSpecification spec)
{
    QMap<QMailAccountId, QMailMessageIdList> messageLists(accountMessages(messageIds));

    QSet<QMailMessageService*> sources(sourceServiceSet(messageLists.keys()));
    if (sources.isEmpty()) {
        reportFailure(action, QMailServiceAction::Status::ErrNoConnection,
                      tr("Unable to retrieve messages for unconfigured account"));
    } else {
        enqueueRequest(new RetrieveMessagesRequest(messageLists, spec), action, sources,
                       &ServiceHandler::dispatchRetrieveMessages, &ServiceHandler::retrievalReady);
    }
}

bool ServiceHandler::dispatchRetrieveMessages(Request *req)
{
    RetrieveMessagesRequest *request = static_cast<RetrieveMessagesRequest*>(req);

    for (auto it = request->messageLists.begin() ; it != request->messageLists.end(); ++it) {
        if (QMailMessageSource *source = accountSource(it.key())) {
            bool success(sourceService.value(source)->usesConcurrentActions()
                                ? source->retrieveMessages(it.value(), request->spec, request->action)
                                : source->retrieveMessages(it.value(), request->spec));

            if (success) {
                // This account is now retrieving
                if (!_retrievalAccountIds.contains(it.key())) {
                    _retrievalAccountIds.insert(it.key());
                }
            } else {
                qCWarning(lcMessaging) << "Unable to service request to retrieve messages for account:" << it.key()
                                       << "with spec" << request->spec;
                return false;
            }
        } else {
            reportFailure(request->action, QMailServiceAction::Status::ErrFrameworkFault,
                          tr("Unable to locate source for account"), it.key());
            return false;
        }
    }

    QMailStore::instance()->setRetrievalInProgress(QMailAccountIdList(_retrievalAccountIds.constBegin(), _retrievalAccountIds.constEnd()));
    return true;
}

class RetrieveMessagePartRequest : public Request
{
public:
    RetrieveMessagePartRequest(QMailAccountId accountId, const QMailMessagePart::Location &partLocation)
        : Request(RetrieveMessagePartRequestType)
        , accountId(accountId)
        , partLocation(partLocation) {}

    QMailAccountId accountId;
    QMailMessagePart::Location partLocation;
};

void ServiceHandler::retrieveMessagePart(quint64 action, const QMailMessagePart::Location &partLocation)
{
    QSet<QMailAccountId> accountIds(messageAccount(partLocation.containingMessageId()));
    QSet<QMailMessageService*> sources(sourceServiceSet(accountIds));
    if (sources.isEmpty()) {
        reportFailure(action, QMailServiceAction::Status::ErrNoConnection,
                      tr("Unable to retrieve message part for unconfigured account"));
    } else {
        enqueueRequest(new RetrieveMessagePartRequest(*accountIds.begin(), partLocation), action, sources,
                       &ServiceHandler::dispatchRetrieveMessagePart, &ServiceHandler::retrievalReady);
    }
}

bool ServiceHandler::dispatchRetrieveMessagePart(Request *req)
{
    RetrieveMessagePartRequest *request = static_cast<RetrieveMessagePartRequest*>(req);

    if (QMailMessageSource *source = accountSource(request->accountId)) {
        bool success(sourceService.value(source)->usesConcurrentActions()
            ? source->retrieveMessagePart(request->partLocation, request->action)
            : source->retrieveMessagePart(request->partLocation));

        if (success) {
           // This account is now retrieving
            setRetrievalInProgress(request->accountId, true);
        } else {
            qCWarning(lcMessaging) << "Unable to service request to retrieve part for message:"
                                   << request->partLocation.containingMessageId();
            return false;
        }
    } else {
        reportFailure(request->action, QMailServiceAction::Status::ErrFrameworkFault,
                      tr("Unable to locate source for account"), request->accountId);
        return false;
    }

    return true;
}

class RetrieveMessageRangeRequest : public Request
{
public:
    RetrieveMessageRangeRequest(QMailAccountId accountId, QMailMessageId messageId, uint minimum)
        : Request(RetrieveMessageRangeRequestType)
        , accountId(accountId)
        , messageId(messageId)
        , minimum(minimum) {}

    QMailAccountId accountId;
    QMailMessageId messageId;
    uint minimum;
};

void ServiceHandler::retrieveMessageRange(quint64 action, const QMailMessageId &messageId, uint minimum)
{
    QSet<QMailAccountId> accountIds(messageAccount(messageId));
    QSet<QMailMessageService*> sources(sourceServiceSet(accountIds));
    if (sources.isEmpty()) {
        reportFailure(action, QMailServiceAction::Status::ErrNoConnection,
                      tr("Unable to retrieve message range for unconfigured account"));
    } else {
        enqueueRequest(new RetrieveMessageRangeRequest(*accountIds.begin(), messageId, minimum), action, sources,
                       &ServiceHandler::dispatchRetrieveMessageRange, &ServiceHandler::retrievalReady);
    }
}

bool ServiceHandler::dispatchRetrieveMessageRange(Request *req)
{
    RetrieveMessageRangeRequest *request = static_cast<RetrieveMessageRangeRequest*>(req);

    if (QMailMessageSource *source = accountSource(request->accountId)) {
        bool success(sourceService.value(source)->usesConcurrentActions()
            ? source->retrieveMessageRange(request->messageId, request->minimum, request->action)
            : source->retrieveMessageRange(request->messageId, request->minimum));

        if (success) {
            // This account is now retrieving
            setRetrievalInProgress(request->accountId, true);
        } else {
            qCWarning(lcMessaging) << "Unable to service request to retrieve range:" << request->minimum
                                   << "for message:" << request->messageId;
            return false;
        }
    } else {
        reportFailure(request->action, QMailServiceAction::Status::ErrFrameworkFault,
                      tr("Unable to locate source for account"), request->accountId);
        return false;
    }

    return true;
}

class RetrieveMessagePartRangeRequest : public Request
{
public:
    RetrieveMessagePartRangeRequest(QMailAccountId accountId,
                                    const QMailMessagePart::Location &partLocation,
                                    uint minimum)
        : Request(RetrieveMessagePartRangeRequestType)
        , accountId(accountId)
        , partLocation(partLocation)
        , minimum(minimum) {}

    QMailAccountId accountId;
    QMailMessagePart::Location partLocation;
    uint minimum;
};

void ServiceHandler::retrieveMessagePartRange(quint64 action, const QMailMessagePart::Location &partLocation, uint minimum)
{
    QSet<QMailAccountId> accountIds(messageAccount(partLocation.containingMessageId()));
    QSet<QMailMessageService*> sources(sourceServiceSet(accountIds));
    if (sources.isEmpty()) {
        reportFailure(action, QMailServiceAction::Status::ErrNoConnection,
                      tr("Unable to retrieve message part range for unconfigured account"));
    } else {
        enqueueRequest(new RetrieveMessagePartRangeRequest(*accountIds.begin(), partLocation, minimum), action, sources,
                       &ServiceHandler::dispatchRetrieveMessagePartRange, &ServiceHandler::retrievalReady);
    }
}

bool ServiceHandler::dispatchRetrieveMessagePartRange(Request *req)
{
    RetrieveMessagePartRangeRequest *request = static_cast<RetrieveMessagePartRangeRequest*>(req);

    if (QMailMessageSource *source = accountSource(request->accountId)) {
        bool success(sourceService.value(source)->usesConcurrentActions()
            ? source->retrieveMessagePartRange(request->partLocation, request->minimum, request->action)
            : source->retrieveMessagePartRange(request->partLocation, request->minimum));

        if (success) {
            // This account is now retrieving
            setRetrievalInProgress(request->accountId, true);
        } else {
            qCWarning(lcMessaging) << "Unable to service request to retrieve range:" << request->minimum
                                   << "for part in message:" << request->partLocation.containingMessageId();
            return false;
        }
    } else {
        reportFailure(request->action, QMailServiceAction::Status::ErrFrameworkFault,
                      tr("Unable to locate source for account"), request->accountId);
        return false;
    }

    return true;
}

class RetrieveAllRequest : public Request
{
public:
    RetrieveAllRequest(QMailAccountId accountId)
        : Request(RetrieveAllRequestType)
        , accountId(accountId) {}

    QMailAccountId accountId;
};

void ServiceHandler::retrieveAll(quint64 action, const QMailAccountId &accountId)
{
    QSet<QMailMessageService*> sources(sourceServiceSet(accountId));
    if (sources.isEmpty()) {
        reportFailure(action, QMailServiceAction::Status::ErrNoConnection, tr("Unable to retrieve all messages for unconfigured account"));
    } else {
        enqueueRequest(new RetrieveAllRequest(accountId), action, sources,
                       &ServiceHandler::dispatchRetrieveAll, &ServiceHandler::retrievalReady);
    }
}

bool ServiceHandler::dispatchRetrieveAll(Request *req)
{
    RetrieveAllRequest *request = static_cast<RetrieveAllRequest*>(req);

    if (QMailMessageSource *source = accountSource(request->accountId)) {
        bool success(sourceService.value(source)->usesConcurrentActions()
            ? source->retrieveAll(request->accountId, request->action)
            : source->retrieveAll(request->accountId));

        if (success) {
            // This account is now retrieving
            setRetrievalInProgress(request->accountId, true);
        } else {
            qCWarning(lcMessaging) << "Unable to service request to retrieve all messages for account:" << request->accountId;
            return false;
        }
    } else {
        reportFailure(request->action, QMailServiceAction::Status::ErrFrameworkFault,
                      tr("Unable to locate source for account"), request->accountId);
        return false;
    }

    return true;
}

class ExportUpdatesRequest : public Request
{
public:
    ExportUpdatesRequest(QMailAccountId accountId)
        : Request(ExportUpdatesRequestType)
        , accountId(accountId) {}

    QMailAccountId accountId;
};

void ServiceHandler::exportUpdates(quint64 action, const QMailAccountId &accountId)
{
    QSet<QMailMessageService*> sources(sourceServiceSet(accountId));
    if (sources.isEmpty()) {
        reportFailure(action, QMailServiceAction::Status::ErrNoConnection, tr("Unable to export updates for unconfigured account"));
    } else {
        enqueueRequest(new ExportUpdatesRequest(accountId), action, sources,
                       &ServiceHandler::dispatchExportUpdates, &ServiceHandler::retrievalReady);
    }
}

bool ServiceHandler::dispatchExportUpdates(Request *req)
{
    RetrieveAllRequest *request = static_cast<RetrieveAllRequest*>(req);

    if (QMailMessageSource *source = accountSource(request->accountId)) {
        bool success(sourceService.value(source)->usesConcurrentActions()
            ? source->exportUpdates(request->accountId, request->action)
            : source->exportUpdates(request->accountId));

        if (!success) {
            qCWarning(lcMessaging) << "Unable to service request to export updates for account:" << request->accountId;
            return false;
        }
    } else {
        reportFailure(request->action, QMailServiceAction::Status::ErrFrameworkFault,
                      tr("Unable to locate source for account"), request->accountId);
        return false;
    }

    return true;
}

class SynchronizeRequest : public Request
{
public:
    SynchronizeRequest(QMailAccountId accountId)
        : Request(SynchronizeRequestType)
        , accountId(accountId) {}

    QMailAccountId accountId;
};

void ServiceHandler::synchronize(quint64 action, const QMailAccountId &accountId)
{
    QSet<QMailMessageService*> sources(sourceServiceSet(accountId));
    if (sources.isEmpty()) {
        reportFailure(action, QMailServiceAction::Status::ErrNoConnection, tr("Unable to synchronize unconfigured account"));
    } else {
        enqueueRequest(new SynchronizeRequest(accountId), action, sources,
                       &ServiceHandler::dispatchSynchronize, &ServiceHandler::retrievalReady);
    }
}

bool ServiceHandler::dispatchSynchronize(Request *req)
{
    SynchronizeRequest *request = static_cast<SynchronizeRequest*>(req);

    if (QMailMessageSource *source = accountSource(request->accountId)) {
        bool success(sourceService.value(source)->usesConcurrentActions()
            ? source->exportUpdates(request->accountId, request->action)
            : source->exportUpdates(request->accountId));

        if (success) {
            // This account is now retrieving
            setRetrievalInProgress(request->accountId, true);
        } else {
            qCWarning(lcMessaging) << "Unable to service request to synchronize account:" << request->accountId;
            return false;
        }
    } else {
        reportFailure(request->action, QMailServiceAction::Status::ErrFrameworkFault,
                      tr("Unable to locate source for account"), request->accountId);
        return false;
    }

    return true;
}

class OnlineDeleteMessagesRequest : public Request
{
public:
    OnlineDeleteMessagesRequest(const QMap<QMailAccountId, QMailMessageIdList> &messageLists)
        : Request(DeleteMessagesRequestType)
        , messageLists(messageLists) {}

    QMap<QMailAccountId, QMailMessageIdList> messageLists;
};

void ServiceHandler::onlineDeleteMessages(quint64 action, const QMailMessageIdList& messageIds,
                                          QMailStore::MessageRemovalOption option)
{
    QSet<QMailMessageService*> sources;

    if (option == QMailStore::NoRemovalRecord) {
        // Delete these records locally without involving the source
        discardMessages(action, messageIds);
    } else {
        QMap<QMailAccountId, QMailMessageIdList> messageLists(accountMessages(messageIds));
        sources = sourceServiceSet(messageLists.keys());
        if (sources.isEmpty()) {
            reportFailure(action, QMailServiceAction::Status::ErrNoConnection, tr("Unable to delete messages for unconfigured account"));
        } else {
            QString description(tr("Deleting messages"));
            enqueueRequest(new OnlineDeleteMessagesRequest(messageLists), action, sources,
                           &ServiceHandler::dispatchOnlineDeleteMessages, &ServiceHandler::storageActionCompleted);
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
        qCWarning(lcMessaging) << "Unable to service request to discard messages";

        reportFailure(action, QMailServiceAction::Status::ErrEnqueueFailed, tr("Unable to discard messages"));
        return;
    }

    emit progressChanged(action, total, total);
    emit messagesDeleted(action, messageIds);
    emit storageActionCompleted(action);
    emit activityChanged(action, QMailServiceAction::Successful);
    QMailStore::instance()->flushIpcNotifications();
}

bool ServiceHandler::dispatchOnlineDeleteMessages(Request *req)
{
    OnlineDeleteMessagesRequest *request = static_cast<OnlineDeleteMessagesRequest*>(req);

    for (auto it = request->messageLists.begin() ; it != request->messageLists.end(); ++it) {
        if (QMailMessageSource *source = accountSource(it.key())) {
            bool success(sourceService.value(source)->usesConcurrentActions()
                ? source->deleteMessages(it.value(), request->action)
                : source->deleteMessages(it.value()));

            if (!success) {
                qCWarning(lcMessaging) << "Unable to service request to delete messages for account:" << it.key();
                return false;
            }
        } else {
            reportFailure(request->action, QMailServiceAction::Status::ErrFrameworkFault,
                          tr("Unable to locate source for account"), it.key());
            return false;
        }
    }

    return true;
}

class CopyToLocalRequest : public Request
{
public:
    CopyToLocalRequest(const QMailMessageIdList &messageIds, QMailFolderId destination)
        : Request(CopyMessagesRequestType)
        , messageIds(messageIds)
        , destination(destination) {}

    QMailMessageIdList messageIds;
    QMailFolderId destination;
};

class OnlineCopyMessagesRequest : public Request
{
public:
    OnlineCopyMessagesRequest(const QMailMessageIdList &messageIds, QMailFolderId destination)
        : Request(CopyMessagesRequestType)
        , messageIds(messageIds)
        , destination(destination) {}

    QMailMessageIdList messageIds;
    QMailFolderId destination;
};

void ServiceHandler::onlineCopyMessages(quint64 action, const QMailMessageIdList& messageIds, const QMailFolderId &destination)
{
    QSet<QMailAccountId> accountIds(folderAccount(destination));

    if (accountIds.isEmpty()) {
        // Copy to a non-account folder
        QSet<QMailMessageService*> sources;
        enqueueRequest(new CopyToLocalRequest(messageIds, destination), action, sources,
                       &ServiceHandler::dispatchCopyToLocal, &ServiceHandler::storageActionCompleted);
    } else {
        QSet<QMailMessageService*> sources(sourceServiceSet(accountIds));
        if (sources.isEmpty()) {
            reportFailure(action, QMailServiceAction::Status::ErrNoConnection,
                          tr("Unable to copy messages to unconfigured account"));
        } else if (sources.count() > 1) {
            reportFailure(action, QMailServiceAction::Status::ErrFrameworkFault,
                          tr("Unable to copy messages to multiple destination accounts!"));
        } else {
            enqueueRequest(new OnlineCopyMessagesRequest(messageIds, destination), action, sources,
                           &ServiceHandler::dispatchOnlineCopyMessages, &ServiceHandler::storageActionCompleted);
        }
    }
}

bool ServiceHandler::dispatchOnlineCopyMessages(Request *req)
{
    OnlineCopyMessagesRequest *request = static_cast<OnlineCopyMessagesRequest*>(req);

    if (request->messageIds.isEmpty())
        return false;

    QSet<QMailAccountId> accountIds(folderAccount(request->destination));

    if (accountIds.count() != 1) {
        reportFailure(request->action, QMailServiceAction::Status::ErrNoConnection,
                      tr("Unable to copy messages to unconfigured account"));
        return false;
    } else {
        QMailAccountId accountId(*accountIds.begin());

        if (QMailMessageSource *source = accountSource(accountId)) {
            bool success(sourceService.value(source)->usesConcurrentActions()
                ? source->copyMessages(request->messageIds, request->destination, request->action)
                : source->copyMessages(request->messageIds, request->destination));

            if (!success) {
                qCWarning(lcMessaging) << "Unable to service request to copy messages to folder:"
                                       << request->destination << "for account:" << accountId;
                return false;
            }
        } else {
            reportFailure(request->action, QMailServiceAction::Status::ErrFrameworkFault,
                          tr("Unable to locate source for account"), accountId);
            return false;
        }
    }

    return true;
}

bool ServiceHandler::dispatchCopyToLocal(Request *req)
{
    CopyToLocalRequest *request = static_cast<CopyToLocalRequest*>(req);

    if (request->messageIds.isEmpty())
        return false;

    uint progress = 0;
    uint total = request->messageIds.count();

    emit progressChanged(request->action, progress, total);

    // Create a copy of each message
    foreach (const QMailMessageId id, request->messageIds) {
        QMailMessage message(id);

        message.setId(QMailMessageId());
        message.setContentIdentifier(QString());
        message.setParentFolderId(request->destination);

        if (!QMailStore::instance()->addMessage(&message)) {
            qCWarning(lcMessaging) << "Unable to service request to copy messages to folder:" << request->destination
                                   << "for account:" << message.parentAccountId();

            reportFailure(request->action, QMailServiceAction::Status::ErrFrameworkFault,
                          tr("Unable to copy messages for account"), message.parentAccountId());
            return false;
        } else {
            emit progressChanged(request->action, ++progress, total);
        }
    }

    emit activityChanged(request->action, QMailServiceAction::Successful);
    return true;
}

class OnlineMoveMessagesRequest : public Request
{
public:
    OnlineMoveMessagesRequest(const QMap<QMailAccountId, QMailMessageIdList> &messageLists, QMailFolderId destination)
        : Request(MoveMessagesRequestType)
        , messageLists(messageLists)
        , destination(destination) {}

    QMap<QMailAccountId, QMailMessageIdList> messageLists;
    QMailFolderId destination;
};

void ServiceHandler::onlineMoveMessages(quint64 action, const QMailMessageIdList& messageIds, const QMailFolderId &destination)
{
    QSet<QMailMessageService*> sources;

    QMap<QMailAccountId, QMailMessageIdList> messageLists(accountMessages(messageIds));
    sources = sourceServiceSet(messageLists.keys());
    if (sources.isEmpty()) {
        reportFailure(action, QMailServiceAction::Status::ErrNoConnection, tr("Unable to move messages for unconfigured account"));
    } else {
        enqueueRequest(new OnlineMoveMessagesRequest(messageLists, destination), action, sources,
                       &ServiceHandler::dispatchOnlineMoveMessages, &ServiceHandler::storageActionCompleted);
    }
}

bool ServiceHandler::dispatchOnlineMoveMessages(Request *req)
{
    OnlineMoveMessagesRequest *request = static_cast<OnlineMoveMessagesRequest*>(req);

    for (auto it = request->messageLists.begin() ; it != request->messageLists.end(); ++it) {
        if (QMailMessageSource *source = accountSource(it.key())) {
            bool success(sourceService.value(source)->usesConcurrentActions()
                ? source->moveMessages(it.value(), request->destination, request->action)
                : source->moveMessages(it.value(), request->destination));

            if (!success) {
                qCWarning(lcMessaging) << "Unable to service request to move messages to folder:"
                                       << request->destination << "for account:" << it.key();
                return false;
            }
        } else {
            reportFailure(request->action, QMailServiceAction::Status::ErrFrameworkFault,
                          tr("Unable to locate source for account"), it.key());
            return false;
        }
    }

    return true;
}

class OnlineFlagMessagesAndMoveToStandardFolderRequest : public Request
{
public:
    OnlineFlagMessagesAndMoveToStandardFolderRequest(const QMap<QMailAccountId, QMailMessageIdList> &messageLists,
                                                     quint64 setMask, quint64 unsetMask)
        : Request(FlagMessagesRequestType)
        , messageLists(messageLists)
        , setMask(setMask)
        , unsetMask(unsetMask) {}

    QMap<QMailAccountId, QMailMessageIdList> messageLists;
    quint64 setMask;
    quint64 unsetMask;
};

void ServiceHandler::onlineFlagMessagesAndMoveToStandardFolder(quint64 action, const QMailMessageIdList& messageIds,
                                                               quint64 setMask, quint64 unsetMask)
{
    QSet<QMailMessageService*> sources;

    QMap<QMailAccountId, QMailMessageIdList> messageLists(accountMessages(messageIds));
    sources = sourceServiceSet(messageLists.keys());
    if (sources.isEmpty()) {
        reportFailure(action, QMailServiceAction::Status::ErrNoConnection, tr("Unable to flag messages for unconfigured account"));
    } else {
        enqueueRequest(new OnlineFlagMessagesAndMoveToStandardFolderRequest(messageLists, setMask, unsetMask), action, sources,
                       &ServiceHandler::dispatchOnlineFlagMessagesAndMoveToStandardFolder, &ServiceHandler::storageActionCompleted);
    }
}

void ServiceHandler::addMessages(quint64 action, const QMailMessageMetaDataList &messages)
{
    bool failure = false;
    QList<QMailMessageMetaData*> list;
    QString scheme;
    if (messages.count()) {
        scheme = messages.first().contentScheme();
    }
    for (const QMailMessageMetaData &m : messages) {
        if (m.contentScheme() != scheme) {
            reportFailure(action,
                          QMailServiceAction::Status::ErrFrameworkFault,
                          tr("Unable to async add messages, "
                             "inconsistent contentscheme"));
        }
    }
    for (const QMailMessageMetaData &m : messages) {
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
                      tr("Unable to async add messages"));
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
    for (const QMailMessageMetaData &m : messages) {
        if (m.contentScheme() != scheme) {
            reportFailure(action,
                          QMailServiceAction::Status::ErrFrameworkFault,
                          tr("Unable to async update messages, "
                             "inconsistent contentscheme"));
        }
    }
    for (const QMailMessageMetaData &m : messages) {
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
            for (QMailMessageMetaData *m : list) {
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

bool ServiceHandler::dispatchOnlineFlagMessagesAndMoveToStandardFolder(Request *req)
{
    OnlineFlagMessagesAndMoveToStandardFolderRequest *request = static_cast<OnlineFlagMessagesAndMoveToStandardFolderRequest*>(req);

    Q_ASSERT(!request->messageLists.empty());

    for (auto it = request->messageLists.begin() ; it != request->messageLists.end(); ++it) {
        if (QMailMessageSource *source = accountSource(it.key())) {
            bool success(sourceService.value(source)->usesConcurrentActions()
                ? source->flagMessages(it.value(), request->setMask, request->unsetMask, request->action)
                : source->flagMessages(it.value(), request->setMask, request->unsetMask));

            if (!success) {
                qCWarning(lcMessaging) << "Unable to service request to flag messages for account:" << it.key();
                return false;
            }
        } else {
            reportFailure(request->action, QMailServiceAction::Status::ErrFrameworkFault,
                          tr("Unable to locate source for account"), it.key());
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
        qCWarning(lcMessaging) << "Unable to service request to delete messages";
        reportFailure(action, QMailServiceAction::Status::ErrFrameworkFault, tr("Could not delete messages"));
        return;
    }

    emit progressChanged(action, total, total);
    emit messagesDeleted(action, messageIds);
    emit storageActionCompleted(action);
    emit activityChanged(action, QMailServiceAction::Successful);
    QMailStore::instance()->flushIpcNotifications();
}

void ServiceHandler::rollBackUpdates(quint64 action, const QMailAccountId &mailAccountId)
{
    QMailDisconnected::rollBackUpdates(mailAccountId);

    emit storageActionCompleted(action);
    emit activityChanged(action, QMailServiceAction::Successful);
    QMailStore::instance()->flushIpcNotifications();
}

void ServiceHandler::moveToStandardFolder(quint64 action, const QMailMessageIdList& ids, quint64 standardFolder)
{
    QMailDisconnected::moveToStandardFolder(ids, static_cast<QMailFolder::StandardFolder>(standardFolder));
    emitMessagesMoved(ids, action);
    emitMessagesFlagged(ids, action);

    emit storageActionCompleted(action);
    emit activityChanged(action, QMailServiceAction::Successful);
    QMailStore::instance()->flushIpcNotifications();
}

void ServiceHandler::moveToFolder(quint64 action, const QMailMessageIdList& ids, const QMailFolderId& folderId)
{
    QMailDisconnected::moveToFolder(ids, folderId);
    emitMessagesMoved(ids, action);

    emit storageActionCompleted(action);
    emit activityChanged(action, QMailServiceAction::Successful);
    QMailStore::instance()->flushIpcNotifications();
}

void ServiceHandler::flagMessages(quint64 action, const QMailMessageIdList& ids, quint64 setMask, quint64 unsetMask)
{
    QMailDisconnected::flagMessages(ids, setMask, unsetMask, "");
    emitMessagesFlagged(ids, action);

    emit storageActionCompleted(action);
    emit activityChanged(action, QMailServiceAction::Successful);
    QMailStore::instance()->flushIpcNotifications();
}

void ServiceHandler::restoreToPreviousFolder(quint64 action, const QMailMessageKey& key)
{
    QMailDisconnected::restoreToPreviousFolder(key);

    emit storageActionCompleted(action);
    emit activityChanged(action, QMailServiceAction::Successful);
    QMailStore::instance()->flushIpcNotifications();
}

class OnlineCreateFolderRequest : public Request
{
public:
    OnlineCreateFolderRequest(const QString &name, QMailAccountId accountId, QMailFolderId parentId)
        : Request(CreateFolderRequestType)
        , name(name)
        , accountId(accountId)
        , parentId(parentId) {}

    QString name;
    QMailAccountId accountId;
    QMailFolderId parentId;
};

void ServiceHandler::onlineCreateFolder(quint64 action, const QString &name, const QMailAccountId &accountId, const QMailFolderId &parentId)
{
    if (accountId.isValid()) {

        QSet<QMailAccountId> accounts;
        if (parentId.isValid()) {
            accounts = folderAccount(parentId);
        } else {
            accounts.insert(accountId);
        }

        QSet<QMailMessageService *> sources(sourceServiceSet(accounts));

        enqueueRequest(new OnlineCreateFolderRequest(name, accountId, parentId), action, sources,
                       &ServiceHandler::dispatchOnlineCreateFolder, &ServiceHandler::storageActionCompleted);
    } else {
        reportFailure(action, QMailServiceAction::Status::ErrInvalidData, tr("Unable to create folder for invalid account"));
    }
}

bool ServiceHandler::dispatchOnlineCreateFolder(Request *req)
{
    OnlineCreateFolderRequest *request = static_cast<OnlineCreateFolderRequest*>(req);

    if (QMailMessageSource *source = accountSource(request->accountId)) {
        bool success(sourceService.value(source)->usesConcurrentActions()
            ? source->createFolder(request->name, request->accountId, request->parentId, request->action)
            : source->createFolder(request->name, request->accountId, request->parentId));
        if (!success) {
            qCWarning(lcMessaging) << "Unable to service request to create folder for account:" << request->accountId;
        }

        return success;
    } else {
        reportFailure(request->action, QMailServiceAction::Status::ErrFrameworkFault,
                      tr("Unable to locate source for account"), request->accountId);
        return false;
    }
}

class OnlineRenameFolderRequest : public Request
{
public:
    OnlineRenameFolderRequest(QMailFolderId folderId, const QString &newFolderName)
        : Request(RenameFolderRequestType)
        , folderId(folderId)
        , newFolderName(newFolderName) {}

    QMailFolderId folderId;
    QString newFolderName;
};

void ServiceHandler::onlineRenameFolder(quint64 action, const QMailFolderId &folderId, const QString &name)
{
    if (folderId.isValid()) {
        QSet<QMailAccountId> accounts = folderAccount(folderId);
        QSet<QMailMessageService *> sources(sourceServiceSet(accounts));

        enqueueRequest(new OnlineRenameFolderRequest(folderId, name), action, sources,
                       &ServiceHandler::dispatchOnlineRenameFolder, &ServiceHandler::storageActionCompleted);
    } else {
        reportFailure(action, QMailServiceAction::Status::ErrInvalidData, tr("Unable to rename invalid folder"));
    }
}

bool ServiceHandler::dispatchOnlineRenameFolder(Request *req)
{
    OnlineRenameFolderRequest *request = static_cast<OnlineRenameFolderRequest*>(req);

    if (QMailMessageSource *source = accountSource(QMailFolder(request->folderId).parentAccountId())) {
        bool success(sourceService.value(source)->usesConcurrentActions()
            ? source->renameFolder(request->folderId, request->newFolderName, request->action)
            : source->renameFolder(request->folderId, request->newFolderName));
        if (!success) {
            qCWarning(lcMessaging) << "Unable to service request to rename folder id:" << request->folderId;
        }

        return success;
    } else {
        reportFailure(request->action, QMailServiceAction::Status::ErrFrameworkFault,
                      tr("Unable to locate source for account"), QMailFolder(request->folderId).parentAccountId());
        return false;
    }
}

class OnlineDeleteFolderRequest : public Request
{
public:
    OnlineDeleteFolderRequest(QMailFolderId folderId)
        : Request(DeleteFolderRequestType)
        , folderId(folderId) {}

    QMailFolderId folderId;
};

void ServiceHandler::onlineDeleteFolder(quint64 action, const QMailFolderId &folderId)
{
    if (folderId.isValid()) {
        QSet<QMailAccountId> accounts = folderAccount(folderId);
        QSet<QMailMessageService *> sources(sourceServiceSet(accounts));

        enqueueRequest(new OnlineDeleteFolderRequest(folderId), action, sources,
                       &ServiceHandler::dispatchOnlineDeleteFolder, &ServiceHandler::storageActionCompleted);
    } else {
        reportFailure(action, QMailServiceAction::Status::ErrInvalidData, tr("Unable to delete invalid folder"));
    }
}

bool ServiceHandler::dispatchOnlineDeleteFolder(Request *req)
{
    OnlineDeleteFolderRequest *request = static_cast<OnlineDeleteFolderRequest*>(req);

    if (QMailMessageSource *source = accountSource(QMailFolder(request->folderId).parentAccountId())) {
        bool success = source->deleteFolder(request->folderId);
        if (!success) {
            qCWarning(lcMessaging) << "Unable to service request to delete folder id:" << request->folderId;
        }

        return success;
    } else {
        reportFailure(request->action, QMailServiceAction::Status::ErrFrameworkFault,
                      tr("Unable to locate source for account"), QMailFolder(request->folderId).parentAccountId());
        return false;
    }
}

class OnlineMoveFolderRequest : public Request
{
public:
    OnlineMoveFolderRequest(QMailFolderId folderId, QMailFolderId newParentId)
        : Request(MoveFolderRequestType)
        , folderId(folderId)
        , newParentId(newParentId) {}

    QMailFolderId folderId;
    QMailFolderId newParentId;
};

void ServiceHandler::onlineMoveFolder(quint64 action, const QMailFolderId &folderId, const QMailFolderId &newParentId)
{
    if (folderId.isValid()) {
        QSet<QMailAccountId> accounts = folderAccount(folderId);
        QSet<QMailMessageService *> sources(sourceServiceSet(accounts));
        enqueueRequest(new OnlineMoveFolderRequest(folderId, newParentId), action, sources,
                       &ServiceHandler::dispatchOnlineMoveFolder, &ServiceHandler::storageActionCompleted);
    } else {
        reportFailure(action, QMailServiceAction::Status::ErrInvalidData, tr("Unable to move invalid folder"));
    }
}

bool ServiceHandler::dispatchOnlineMoveFolder(Request *req)
{
    OnlineMoveFolderRequest *request = static_cast<OnlineMoveFolderRequest*>(req);

    if (QMailMessageSource *source = accountSource(QMailFolder(request->folderId).parentAccountId())) {
        bool success(sourceService.value(source)->usesConcurrentActions()
                     ? source->moveFolder(request->folderId, request->newParentId, request->action)
                     : source->moveFolder(request->folderId, request->newParentId));

        if (!success) {
            qCWarning(lcMessaging) << "Unable to service request to move folder id:" << request->folderId;
        }

        return success;
    } else {
        reportFailure(request->action, QMailServiceAction::Status::ErrFrameworkFault,
                      tr("Unable to locate source for account"), QMailFolder(request->folderId).parentAccountId());
        return false;
    }
}

void ServiceHandler::searchMessages(quint64 action, const QMailMessageKey& filter, const QString& bodyText,
                                    QMailSearchAction::SearchSpecification spec, const QMailMessageSortKey &sort)
{
    searchMessages(action, filter, bodyText, spec, 0, sort, NoLimit);
}

void ServiceHandler::searchMessages(quint64 action, const QMailMessageKey& filter, const QString& bodyText,
                                    QMailSearchAction::SearchSpecification spec, quint64 limit, const QMailMessageSortKey &sort)
{
    searchMessages(action, filter, bodyText, spec, limit, sort, Limit);
}

void ServiceHandler::countMessages(quint64 action, const QMailMessageKey& filter, const QString& bodyText)
{
    searchMessages(action, filter, bodyText, QMailSearchAction::Remote, 0, QMailMessageSortKey(), Count);
}

class SearchMessagesRequest : public Request
{
public:
    SearchMessagesRequest(const QSet<QMailAccountId> &accountIds,
                          const QMailMessageKey &filter,
                          const QString &bodyText,
                          qint64 limit,
                          QMailMessageSortKey sort,
                          SearchType searchType)
        : Request(SearchMessagesRequestType)
        , accountIds(accountIds)
        , filter(filter)
        , bodyText(bodyText)
        , limit(limit)
        , sort(sort)
        , searchType(searchType) {}

    QSet<QMailAccountId> accountIds;
    QMailMessageKey filter;
    QString bodyText;
    qint64 limit;
    QMailMessageSortKey sort;
    SearchType searchType;
};

void ServiceHandler::searchMessages(quint64 action, const QMailMessageKey &filter, const QString &bodyText,
                                    QMailSearchAction::SearchSpecification spec, quint64 limit,
                                    const QMailMessageSortKey &sort, SearchType searchType)
{
    if (spec == QMailSearchAction::Remote) {
        // Find the accounts that we need to search within from the criteria
        QSet<QMailAccountId> searchAccountIds(accountsApplicableTo(filter, QSet<QMailAccountId>(sourceMap.keyBegin(), sourceMap.keyEnd())));

        QSet<QMailMessageService*> sources(sourceServiceSet(searchAccountIds));
        if (sources.isEmpty()) {
            reportFailure(action, QMailServiceAction::Status::ErrNoConnection, tr("Unable to search messages for unconfigured account"));
        } else {
            enqueueRequest(new SearchMessagesRequest(searchAccountIds, filter, bodyText, limit, sort, searchType), action, sources,
                           &ServiceHandler::dispatchSearchMessages, &ServiceHandler::searchCompleted);
        }
    } else {
        // Find the messages that match the filter criteria
        QMailMessageIdList searchIds(QMailStore::instance()->queryMessages(filter, sort));

        // Schedule this search
        mSearches.append(MessageSearch(action, searchIds, bodyText));
        QTimer::singleShot(0, this, SLOT(continueSearch()));
    }
}

bool ServiceHandler::dispatchSearchMessages(Request *req)
{
    SearchMessagesRequest *request = static_cast<SearchMessagesRequest*>(req);

    bool sentSearch = false;

    foreach (const QMailAccountId &accountId, request->accountIds) {
        if (QMailMessageSource *source = accountSource(accountId)) {
            bool success(false);
            bool concurrent(sourceService.value(source)->usesConcurrentActions());

            switch (request->searchType) {
            case NoLimit:
                if (concurrent) {
                    success = source->searchMessages(request->filter, request->bodyText, request->sort, request->action);
                } else {
                    success = source->searchMessages(request->filter, request->bodyText, request->sort);
                }
                break;
            case Limit:
                if (concurrent) {
                    success = source->searchMessages(request->filter, request->bodyText, request->limit, request->sort, request->action);
                } else {
                    success = source->searchMessages(request->filter, request->bodyText, request->limit, request->sort);
                }
                break;
            case Count:
                if (concurrent) {
                    success = source->countMessages(request->filter, request->bodyText, request->action);
                } else {
                    success = source->countMessages(request->filter, request->bodyText);
                }
                break;
            default:
                qCWarning(lcMessaging) << "Unknown SearchType";
                break;
            }

            // only dispatch to appropriate account
            if (success) {
                sentSearch = true; // we've at least sent one
            } else {
                // do it locally instead
                qCWarning(lcMessaging) << "Unable to do remote search, doing it locally instead";
                mSearches.append(MessageSearch(request->action,
                                               QMailStore::instance()->queryMessages(request->filter, request->sort),
                                               request->bodyText));
                QTimer::singleShot(0, this, SLOT(continueSearch()));
            }
        } else {
            reportFailure(request->action, QMailServiceAction::Status::ErrFrameworkFault,
                          tr("Unable to locate source for account"), accountId);
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
    QTimer::singleShot(0, QCoreApplication::instance(), SLOT(quit()));
}

void ServiceHandler::listActions()
{
    QMailActionDataList list;

    for (QMap<quint64, ActionData>::iterator i(mActiveActions.begin()) ; i != mActiveActions.end(); ++i) {
        QMailActionData t(i.key(), i->requestType, i->progressTotal, i->progressCurrent,
                          i->status.errorCode, i->status.text,
                          i->status.accountId, i->status.folderId, i->status.messageId);
        list.append(t);
    }

    emit actionsListed(list);
}

// concurrent actions
void ServiceHandler::onStatusChanged(const QMailServiceAction::Status s, quint64 a)
{
    QMap<quint64, ActionData>::iterator it = mActiveActions.find(a);
    if (it != mActiveActions.end()) {
        it->status = s;
    }
    emit statusChanged(a, s);
}

void ServiceHandler::emitAvailabilityChanged(bool b, quint64 a)
{
    emit availabilityChanged(a, b);
}

void ServiceHandler::emitConnectivityChanged(QMailServiceAction::Connectivity c, quint64 a)
{
    emit connectivityChanged(a, c);
}

void ServiceHandler::emitActivityChanged(QMailServiceAction::Activity act, quint64 a)
{
    emit activityChanged(a, act);
}

void ServiceHandler::onProgressChanged(uint p, uint t, quint64 a)
{
    QMap<quint64, ActionData>::iterator it = mActiveActions.find(a);
    if (it != mActiveActions.end()) {
        it->progressCurrent = p;
        it->progressTotal = t;
    }
    emit progressChanged(a, p, t);
}

void ServiceHandler::onActionCompleted(bool success, quint64 action)
{
   qCDebug(lcMessaging) << "Action completed" << action << "result" << (success ? "success" : "failure");
   QMailMessageService *service = qobject_cast<QMailMessageService*>(sender());
   Q_ASSERT(service);

   handleActionCompleted(success, service, action);
}

void ServiceHandler::handleActionCompleted(bool success, QMailMessageService *service, quint64 action)
{
    qCDebug(lcMessaging) << "Action completed" << action << "result" << (success ? "success" : "failure");
    QMap<quint64, ActionData>::iterator it = mActiveActions.find(action);
    if (it != mActiveActions.end()) {
        ActionData &data(it.value());

        if (!mSentIds.isEmpty() && (data.completion == &ServiceHandler::transmissionReady)) {
            if (accountSource(service->accountId())) {
                // Mark these message as Sent
                quint64 setMask(QMailMessage::Sent);
                quint64 unsetMask(QMailMessage::Outbox | QMailMessage::Draft | QMailMessage::LocalOnly);

                QMailMessageKey idsKey(QMailMessageKey::id(mSentIds));

                bool failed = !QMailStore::instance()->updateMessagesMetaData(idsKey, setMask, true);
                failed = !QMailStore::instance()->updateMessagesMetaData(idsKey, unsetMask, false) || failed;

                if (failed) {
                    qCWarning(lcMessaging) << "Unable to flag messages:" << mSentIds;
                }

                // FWOD messages have already been uploaded to the remote server, don't try to upload twice
                quint64 externalStatus(QMailMessage::TransmitFromExternal | QMailMessage::HasUnresolvedReferences);
                QMailMessageKey externalKey(QMailMessageKey::status(externalStatus, QMailDataComparator::Includes));
                QMailMessageKey sentIdsKey(QMailMessageKey::id(mSentIds));
                QMailMessageIdList sentNonFwodIds = QMailStore::instance()->queryMessages(sentIdsKey & ~externalKey);

                // Move sent messages to sent folder on remote server
                QMap<QMailAccountId, QMailMessageIdList> groupedMessages(accountMessages(sentNonFwodIds));
                if (!groupedMessages.empty()) { // messages are still around
                    enqueueRequest(new OnlineFlagMessagesAndMoveToStandardFolderRequest(groupedMessages, setMask, unsetMask),
                        newLocalActionId(),
                        sourceServiceSet(service->accountId()),
                        &ServiceHandler::dispatchOnlineFlagMessagesAndMoveToStandardFolder,
                        &ServiceHandler::storageActionCompleted);
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
                if (data.completion == &ServiceHandler::retrievalReady) {
                    setRetrievalInProgress(accountId, false);
                } else if (data.completion == &ServiceHandler::transmissionReady) {
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

void ServiceHandler::emitMessagesTransmitted(const QMailMessageIdList& ml, quint64 a)
{
    emit messagesTransmitted(a, ml);
}

void ServiceHandler::emitMessagesFailedTransmission(const QMailMessageIdList& ml, QMailServiceAction::Status::ErrorCode err, quint64 a)
{
    emit messagesFailedTransmission(a, ml, err);
}

void ServiceHandler::emitMessagesDeleted(const QMailMessageIdList& ml, quint64 a)
{
    emit messagesDeleted(a, ml);
}

void ServiceHandler::emitMessagesCopied(const QMailMessageIdList& ml, quint64 a)
{
    emit messagesCopied(a, ml);
}

void ServiceHandler::emitMessagesMoved(const QMailMessageIdList& ml, quint64 a)
{
    emit messagesMoved(a, ml);
}

void ServiceHandler::emitMessagesFlagged(const QMailMessageIdList& ml, quint64 a)
{
    emit messagesFlagged(a, ml);
}

void ServiceHandler::emitMessagesPrepared(const QMailMessageIdList& ml, quint64 a)
{
    Q_UNUSED(ml);
    Q_UNUSED(a);
}

void ServiceHandler::emitMatchingMessageIds(const QMailMessageIdList& ml, quint64 a)
{
    emit matchingMessageIds(a, ml);
}

void ServiceHandler::emitRemainingMessagesCount(uint count, quint64 a)
{
    emit remainingMessagesCount(a, count);
}

void ServiceHandler::emitMessagesCount(uint count, quint64 a)
{
    emit messagesCount(a, count);
}

void ServiceHandler::emitProtocolResponse(const QString &response, const QVariantMap &data, quint64 a)
{
    emit protocolResponse(a, response, data);
}

// end concurrent actions

class ProtocolRequest : public Request
{
public:
    ProtocolRequest(QMailAccountId accountId, const QString &request, const QVariantMap &requestData)
        : Request(ProtocolRequestRequestType)
        , accountId(accountId)
        , request(request)
        , requestData(requestData) {}

    QMailAccountId accountId;
    QString request;
    QVariantMap requestData;
};

void ServiceHandler::protocolRequest(quint64 action, const QMailAccountId &accountId, const QString &request,
                                     const QVariantMap &data)
{
    QSet<QMailMessageService*> sources(sourceServiceSet(accountId));
    if (sources.isEmpty()) {
        reportFailure(action, QMailServiceAction::Status::ErrNoConnection,
                      tr("Unable to forward protocol-specific request for unconfigured account"));
    } else {
        enqueueRequest(new ProtocolRequest(accountId, request, data), action, sources,
                       &ServiceHandler::dispatchProtocolRequest, &ServiceHandler::protocolRequestCompleted);
    }
}

bool ServiceHandler::dispatchProtocolRequest(Request *req)
{
    ProtocolRequest *request = static_cast<ProtocolRequest*>(req);

    if (QMailMessageSource *source = accountSource(request->accountId)) {
        bool success(sourceService.value(source)->usesConcurrentActions()
                     ? source->protocolRequest(request->accountId, request->request, request->requestData, request->action)
                     : source->protocolRequest(request->accountId, request->request, request->requestData));
        if (!success) {
            qCWarning(lcMessaging) << "Unable to service request to forward protocol-specific request to account:" << request->accountId;
            return false;
        }
    } else {
        reportFailure(request->action, QMailServiceAction::Status::ErrFrameworkFault,
                      tr("Unable to locate source for account"), request->accountId);
        return false;
    }

    return true;
}

void ServiceHandler::onMessagesDeleted(const QMailMessageIdList &messageIds)
{
    if (quint64 action = sourceAction(qobject_cast<QMailMessageSource*>(sender())))
        emit messagesDeleted(action, messageIds);
}

void ServiceHandler::onMessagesCopied(const QMailMessageIdList &messageIds)
{
    if (quint64 action = sourceAction(qobject_cast<QMailMessageSource*>(sender())))
        emit messagesCopied(action, messageIds);
}

void ServiceHandler::onMessagesMoved(const QMailMessageIdList &messageIds)
{
    if (quint64 action = sourceAction(qobject_cast<QMailMessageSource*>(sender())))
        emit messagesMoved(action, messageIds);
}

void ServiceHandler::onMessagesFlagged(const QMailMessageIdList &messageIds)
{
    if (quint64 action = sourceAction(qobject_cast<QMailMessageSource*>(sender())))
        emit messagesFlagged(action, messageIds);
}

void ServiceHandler::onMessagesPrepared(const QMailMessageIdList &messageIds)
{
    Q_UNUSED(messageIds)
}

void ServiceHandler::onMatchingMessageIds(const QMailMessageIdList &messageIds)
{
    if (quint64 action = sourceAction(qobject_cast<QMailMessageSource*>(sender())))
        emit matchingMessageIds(action, messageIds);
}

void ServiceHandler::onRemainingMessagesCount(uint count)
{
    if (quint64 action = sourceAction(qobject_cast<QMailMessageSource*>(sender())))
        emit remainingMessagesCount(action, count);
}

void ServiceHandler::onMessagesCount(uint count)
{
    if (quint64 action = sourceAction(qobject_cast<QMailMessageSource*>(sender())))
        emit messagesCount(action, count);
}

void ServiceHandler::onProtocolResponse(const QString &response, const QVariantMap &data)
{
    if (quint64 action = sourceAction(qobject_cast<QMailMessageSource*>(sender())))
        emit protocolResponse(action, response, data);
}

void ServiceHandler::onMessagesTransmitted(const QMailMessageIdList &messageIds)
{
    if (QMailMessageSink *sink = qobject_cast<QMailMessageSink*>(sender())) {
        if (quint64 action = sinkAction(sink)) {
            mSentIds << messageIds;

            emit messagesTransmitted(action, messageIds);
        }
    }
}

void ServiceHandler::onMessagesFailedTransmission(const QMailMessageIdList &messageIds, QMailServiceAction::Status::ErrorCode error)
{
    if (QMailMessageSink *sink = qobject_cast<QMailMessageSink*>(sender())) {
        if (quint64 action = sinkAction(sink)) {
            emit messagesFailedTransmission(action, messageIds, error);
        }
    }
}

void ServiceHandler::onAvailabilityChanged(bool available)
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

void ServiceHandler::onConnectivityChanged(QMailServiceAction::Connectivity connectivity)
{
    if (QMailMessageService *service = qobject_cast<QMailMessageService*>(sender()))
        if (quint64 action = serviceAction(service)) {
            updateAction(action);
            emit connectivityChanged(action, connectivity);
        }
}

void ServiceHandler::onActivityChanged(QMailServiceAction::Activity activity)
{
    if (QMailMessageService *service = qobject_cast<QMailMessageService*>(sender()))
        if (quint64 action = serviceAction(service)) {
            updateAction(action);
            emit activityChanged(action, activity);
        }
}

void ServiceHandler::onStatusChanged(const QMailServiceAction::Status status)
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

void ServiceHandler::onProgressChanged(uint progress, uint total)
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

void ServiceHandler::onActionCompleted(bool success)
{
    if (QMailMessageService *service = qobject_cast<QMailMessageService*>(sender())) {
        if (quint64 action = serviceAction(service)) {
            handleActionCompleted(success, service, action);
            return;
        }
    }

    qCWarning(lcMessaging) << "Would not determine server/action completing";
    // See if there are pending requests
    QTimer::singleShot(0, this, SLOT(dispatchRequest()));
}

void ServiceHandler::reportFailure(quint64 action, QMailServiceAction::Status::ErrorCode code, const QString &text,
                                   const QMailAccountId &accountId, const QMailFolderId &folderId,
                                   const QMailMessageId &messageId)
{
    reportFailure(action, QMailServiceAction::Status(code, text, accountId, folderId, messageId));
}

void ServiceHandler::reportFailure(quint64 action, const QMailServiceAction::Status status)
{
    if (status.errorCode != QMailServiceAction::Status::ErrNoError
        && mActiveActions.contains(action)
        && mActiveActions[action].requestType == TransmitMessagesRequestType) {
        markFailedMessage(status);
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

void ServiceHandler::reportPastFailures()
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
        QMailStore::instance()->setRetrievalInProgress(QMailAccountIdList(_retrievalAccountIds.constBegin(), _retrievalAccountIds.constEnd()));
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
        QMailStore::instance()->setTransmissionInProgress(QMailAccountIdList(_transmissionAccountIds.constBegin(), _transmissionAccountIds.constEnd()));
    }
}

uint qHash(QPointer<QMailMessageService> service)
{
    return qHash(service.data());
}

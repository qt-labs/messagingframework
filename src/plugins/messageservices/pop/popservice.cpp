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

#include "popservice.h"
#include "popconfiguration.h"
#ifndef QMF_NO_WIDGETS
#include "popsettings.h"
#endif
#include <QTimer>
#include <QtPlugin>
#include <QtGlobal>
#include <QCoreApplication>

namespace { const QString serviceKey("pop3"); }

class PopService::Source : public QMailMessageSource
{
    Q_OBJECT

public:
    Source(PopService *service)
        : QMailMessageSource(service),
          _service(service),
          _deleting(false),
          _unavailable(false),
          _mailCheckQueued(false),
          _queuedMailCheckInProgress(false)
    {
        connect(&_service->_client, SIGNAL(allMessagesReceived()), this, SIGNAL(newMessagesAvailable()));
        connect(&_service->_client, SIGNAL(messageActionCompleted(QString)), this, SLOT(messageActionCompleted(QString)));
        connect(&_service->_client, SIGNAL(retrievalCompleted()), this, SLOT(retrievalCompleted()));
        connect(&_intervalTimer, SIGNAL(timeout()), this, SLOT(queueMailCheck()));
    }

    void setIntervalTimer(int interval)
    {
        _intervalTimer.stop();
        if (interval > 0)
            _intervalTimer.start(interval*1000*60); // interval minutes
    }

    virtual QMailStore::MessageRemovalOption messageRemovalOption() const { return QMailStore::CreateRemovalRecord; }

public slots:
    virtual bool retrieveFolderList(const QMailAccountId &accountId, const QMailFolderId &folderId, bool descending);
    virtual bool retrieveMessageList(const QMailAccountId &accountId, const QMailFolderId &folderId, uint minimum, const QMailMessageSortKey &sort);
    virtual bool retrieveMessageLists(const QMailAccountId &accountId, const QMailFolderIdList &folderIds, uint minimum, const QMailMessageSortKey &sort);

    virtual bool retrieveMessages(const QMailMessageIdList &messageIds, QMailRetrievalAction::RetrievalSpecification spec);

    virtual bool retrieveAll(const QMailAccountId &accountId);
    virtual bool exportUpdates(const QMailAccountId &accountId);

    virtual bool synchronize(const QMailAccountId &accountId);

    virtual bool deleteMessages(const QMailMessageIdList &ids);

    void messageActionCompleted(const QString &uid);
    void retrievalCompleted();
    void retrievalTerminated();
    void queueMailCheck();

private:
    PopService *_service;
    bool _deleting;
    bool _unavailable;
    bool _mailCheckQueued;
    bool _queuedMailCheckInProgress;
    QTimer _intervalTimer;
};

bool PopService::Source::retrieveFolderList(const QMailAccountId &accountId, const QMailFolderId &folderId, bool descending)
{
    if (!accountId.isValid()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("No account specified"));
        return false;
    }
    
    if (!_service->_client.findInbox()) { // find/create local inbox
        // Inbox created so first sync, so test connection
        _service->_client.testConnection();
        _unavailable = true;
    } else {
        // Just report success
        _service->updateStatus("");
        QTimer::singleShot(0, this, SLOT(retrievalCompleted()));
    }
    return true;

    Q_UNUSED(descending)
    Q_UNUSED(folderId)
}

bool PopService::Source::retrieveMessageLists(const QMailAccountId &accountId, const QMailFolderIdList &folderIds, uint minimum, const QMailMessageSortKey &sort)
{
    if (folderIds.isEmpty()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("No folders specified"));
        return false;
    }

    return retrieveMessageList(accountId, QMailFolderId(), minimum, sort);
}

bool PopService::Source::retrieveMessageList(const QMailAccountId &accountId, const QMailFolderId &folderId, uint minimum, const QMailMessageSortKey &sort)
{
    if (!accountId.isValid()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("No account specified"));
        return false;
    }

    QMailMessageKey countKey(QMailMessageKey::parentAccountId(accountId));
    countKey &= ~QMailMessageKey::status(QMailMessage::Temporary);
    uint existing = QMailStore::instance()->countMessages(countKey);
    existing = qMin(existing, minimum);
    
    _service->_client.setOperation(QMailRetrievalAction::Auto);
    _service->_client.setAdditional(minimum - existing);

    if (!_service->_client.synchronizationEnabled(folderId)) {
        // Just report success
        _service->updateStatus("");
        QTimer::singleShot(0, this, SLOT(retrievalCompleted()));
        return true;
    }
    
    _service->_client.newConnection();
    _unavailable = true;
    return true;

    Q_UNUSED(folderId)
    Q_UNUSED(sort)
}

bool PopService::Source::retrieveMessages(const QMailMessageIdList &messageIds, QMailRetrievalAction::RetrievalSpecification spec)
{
    if (messageIds.isEmpty()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("No messages to retrieve"));
        return false;
    }

    if (spec == QMailRetrievalAction::Flags) {
        // Just report success
        QTimer::singleShot(0, this, SLOT(retrievalCompleted()));
        return true;
    }

    SelectionMap selectionMap;
    foreach (const QMailMessageId& id, messageIds) {
        QMailMessageMetaData message(id);
        selectionMap.insert(message.serverUid(), id);
    }

    _service->_client.setOperation(spec);
    _service->_client.setSelectedMails(selectionMap);
    _service->_client.newConnection();
    _unavailable = true;
    return true;
}

bool PopService::Source::retrieveAll(const QMailAccountId &accountId)
{
    if (!accountId.isValid()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("No account specified"));
        return false;
    }

    _service->_client.setOperation(QMailRetrievalAction::MetaData);
    _service->_client.newConnection();
    _unavailable = true;
    return true;
}

bool PopService::Source::exportUpdates(const QMailAccountId &accountId)
{
    if (!accountId.isValid()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("No account specified"));
        return false;
    }

    // Just report success
    QTimer::singleShot(0, this, SLOT(retrievalCompleted()));
    return true;
}

bool PopService::Source::synchronize(const QMailAccountId &accountId)
{
    return retrieveAll(accountId);
}

bool PopService::Source::deleteMessages(const QMailMessageIdList &messageIds)
{
    if (messageIds.isEmpty()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("No messages to delete"));
        return false;
    }

    QMailAccountConfiguration accountCfg(_service->accountId());
    PopConfiguration popCfg(accountCfg);
    if (popCfg.canDeleteMail()) {
        // Delete the messages from the server
        SelectionMap selectionMap;
        foreach (const QMailMessageId& id, messageIds) {
            QMailMessageMetaData message(id);
            selectionMap.insert(message.serverUid(), id);
        }

        _deleting = true;
        _service->_client.setDeleteOperation();
        _service->_client.setSelectedMails(selectionMap);
        _service->_client.newConnection();
        _unavailable = true;
        return true;
    }

    // Just delete the local copies
    return QMailMessageSource::deleteMessages(messageIds);
}

void PopService::Source::messageActionCompleted(const QString &uid)
{
    if (_deleting) {
        QMailMessageMetaData metaData(uid, _service->accountId());
        if (metaData.id().isValid()) {
            QMailMessageIdList messageIds;
            messageIds.append(metaData.id());

            emit messagesDeleted(messageIds);
        }
    }
}

void PopService::Source::retrievalCompleted()
{
    _unavailable = false;

    if (_queuedMailCheckInProgress) {
        _queuedMailCheckInProgress = false;
        emit _service->availabilityChanged(true);
    }

    emit _service->actionCompleted(true);

    _deleting = false;
    
    if (_mailCheckQueued) {
        queueMailCheck();
    }
}

void PopService::Source::queueMailCheck()
{
    if (_unavailable) {
        _mailCheckQueued = true;
        return;
    }

    _mailCheckQueued = false;
    _queuedMailCheckInProgress = true;

    emit _service->availabilityChanged(false);
    synchronize(_service->accountId());
}

void PopService::Source::retrievalTerminated()
{
    _unavailable = false;
    if (_queuedMailCheckInProgress) {
        _queuedMailCheckInProgress = false;
        emit _service->availabilityChanged(true);
    }
    
    // Just give up if an error occurs
    _mailCheckQueued = false;
}

PopService::PopService(const QMailAccountId &accountId)
    : QMailMessageService(),
      _client(this),
      _source(new Source(this))
{
    connect(&_client, SIGNAL(progressChanged(uint, uint)), this, SIGNAL(progressChanged(uint, uint)));

    connect(&_client, SIGNAL(errorOccurred(int, QString)), this, SLOT(errorOccurred(int, QString)));
    connect(&_client, SIGNAL(errorOccurred(QMailServiceAction::Status::ErrorCode, QString)), this, SLOT(errorOccurred(QMailServiceAction::Status::ErrorCode, QString)));
    connect(&_client, SIGNAL(updateStatus(QString)), this, SLOT(updateStatus(QString)));
    connect(QMailStore::instance(), SIGNAL(accountsUpdated(const QMailAccountIdList&)), 
            this, SLOT(accountsUpdated(const QMailAccountIdList&)));

    _client.setAccount(accountId);
    QMailAccountConfiguration accountCfg(accountId);
    PopConfiguration popCfg(accountCfg);
    _source->setIntervalTimer(popCfg.checkInterval());
}

PopService::~PopService()
{
    delete _source;
}

QString PopService::service() const
{
    return serviceKey;
}

QMailAccountId PopService::accountId() const
{
    return _client.accountId();
}

bool PopService::hasSource() const
{
    return true;
}

QMailMessageSource &PopService::source() const
{
    return *_source;
}

bool PopService::available() const
{
    return true;
}

bool PopService::cancelOperation(QMailServiceAction::Status::ErrorCode code, const QString &text)
{
    _client.cancelTransfer(code, text);
    _client.closeConnection();
    _source->retrievalTerminated();
    return true;
}

void PopService::errorOccurred(int code, const QString &text)
{
    updateStatus(code, text, _client.accountId());
    _source->retrievalTerminated();
    emit actionCompleted(false);
}

void PopService::errorOccurred(QMailServiceAction::Status::ErrorCode code, const QString &text)
{
    updateStatus(code, text, _client.accountId());
    _source->retrievalTerminated();
    emit actionCompleted(false);
}

void PopService::updateStatus(const QString &text)
{
    updateStatus(QMailServiceAction::Status::ErrNoError, text, _client.accountId());
}

void PopService::accountsUpdated(const QMailAccountIdList &ids)
{
    if (!ids.contains(accountId()))
        return;

    QMailAccount account(accountId());
    bool isEnabled(account.status() & QMailAccount::Enabled);
    if (!isEnabled) {
        // cancel any mail check in progress
        cancelOperation(QMailServiceAction::Status::ErrConfiguration, tr("Account disabled"));
        return;
    }

    // keep the check interval up to date
    QMailAccountConfiguration accountCfg(accountId());
    PopConfiguration popCfg(accountCfg);
    _source->setIntervalTimer(popCfg.checkInterval());
}


class PopConfigurator : public QMailMessageServiceConfigurator
{
public:
    PopConfigurator();
    ~PopConfigurator();

    virtual QString service() const;
    virtual QString displayName() const;

#ifndef QMF_NO_WIDGETS
    virtual QMailMessageServiceEditor *createEditor(QMailMessageServiceFactory::ServiceType type);
#endif
};

PopConfigurator::PopConfigurator()
{
}

PopConfigurator::~PopConfigurator()
{
}

QString PopConfigurator::service() const
{
    return serviceKey;
}

QString PopConfigurator::displayName() const
{
    return QCoreApplication::instance()->translate("QMailMessageService", "POP");
}

#ifndef QMF_NO_WIDGETS
QMailMessageServiceEditor *PopConfigurator::createEditor(QMailMessageServiceFactory::ServiceType type)
{
    if (type == QMailMessageServiceFactory::Source)
        return new PopSettings;

    return 0;
}
#endif

PopServicePlugin::PopServicePlugin()
    : QMailMessageServicePlugin()
{
}

QString PopServicePlugin::key() const
{
    return serviceKey;
}

bool PopServicePlugin::supports(QMailMessageServiceFactory::ServiceType type) const
{
    return (type == QMailMessageServiceFactory::Source);
}

bool PopServicePlugin::supports(QMailMessage::MessageType type) const
{
    return (type == QMailMessage::Email);
}

QMailMessageService *PopServicePlugin::createService(const QMailAccountId &id)
{
    return new PopService(id);
}

QMailMessageServiceConfigurator *PopServicePlugin::createServiceConfigurator()
{
    return new PopConfigurator();
}

#include "popservice.moc"

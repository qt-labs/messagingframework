/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#include "popservice.h"
#include "popconfiguration.h"
#include "popsettings.h"
#include <QTimer>
#include <QtPlugin>

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

    if (folderId.isValid()) {
        // Folders don't make sense for POP
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("No account specified"));
        return false;
    }

    // Just report success
    QTimer::singleShot(0, this, SLOT(retrievalCompleted()));
    return true;

    Q_UNUSED(descending)
}

bool PopService::Source::retrieveMessageList(const QMailAccountId &accountId, const QMailFolderId &folderId, uint minimum, const QMailMessageSortKey &sort)
{
    if (!accountId.isValid()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("No account specified"));
        return false;
    }

    if (folderId.isValid()) {
        // Folders don't make sense for POP
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("No account specified"));
        return false;
    }

    _service->_client.setOperation(QMailRetrievalAction::MetaData);
    _service->_client.newConnection();
    _unavailable = true;
    return true;

    Q_UNUSED(minimum)
    Q_UNUSED(sort)
}

bool PopService::Source::retrieveMessages(const QMailMessageIdList &messageIds, QMailRetrievalAction::RetrievalSpecification spec)
{
    if (messageIds.isEmpty()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("No messages to retrieve"));
        return false;
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

    emit _service->activityChanged(QMailServiceAction::Successful);
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
    return _client.account();
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

bool PopService::cancelOperation()
{
    _client.cancelTransfer();
    _client.closeConnection();
    _source->retrievalTerminated();
    return true;
}

void PopService::errorOccurred(int code, const QString &text)
{
    updateStatus(code, text, _client.account());
    _source->retrievalTerminated();
    emit actionCompleted(false);
}

void PopService::errorOccurred(QMailServiceAction::Status::ErrorCode code, const QString &text)
{
    updateStatus(code, text, _client.account());
    _source->retrievalTerminated();
    emit actionCompleted(false);
}

void PopService::updateStatus(const QString &text)
{
    updateStatus(QMailServiceAction::Status::ErrNoError, text, _client.account());
}


class PopConfigurator : public QMailMessageServiceConfigurator
{
public:
    PopConfigurator();
    ~PopConfigurator();

    virtual QString service() const;
    virtual QString displayName() const;

    virtual QMailMessageServiceEditor *createEditor(QMailMessageServiceFactory::ServiceType type);
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
    return qApp->translate("QMailMessageService", "POP");
}

QMailMessageServiceEditor *PopConfigurator::createEditor(QMailMessageServiceFactory::ServiceType type)
{
    if (type == QMailMessageServiceFactory::Source)
        return new PopSettings;

    return 0;
}

Q_EXPORT_PLUGIN2(pop,PopServicePlugin)

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


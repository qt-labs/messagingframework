/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#include "smtpservice.h"
#include "smtpsettings.h"
#include <QtPlugin>

namespace { const QString serviceKey("smtp"); }

class SmtpService::Sink : public QMailMessageSink
{
    Q_OBJECT

public:
    Sink(SmtpService *service) 
        : QMailMessageSink(service),
          _service(service)
    {
        connect(&_service->_client, SIGNAL(messageTransmitted(QMailMessageId)), this, SLOT(messageTransmitted(QMailMessageId)));
        connect(&_service->_client, SIGNAL(sendCompleted()), this, SLOT(sendCompleted()));
    }

public slots:
    virtual bool transmitMessages(const QMailMessageIdList &ids);

    void messageTransmitted(const QMailMessageId &id);
    void sendCompleted();

private:
    SmtpService *_service;
};

bool SmtpService::Sink::transmitMessages(const QMailMessageIdList &ids)
{
    QMailServiceAction::Status::ErrorCode errorCode = QMailServiceAction::Status::ErrNoError;
    QString errorText;

    if (!ids.isEmpty()) {
        foreach (const QMailMessageId id, ids) {
            QMailMessage message(id);
            if ((errorCode = _service->_client.addMail(message)) != QMailServiceAction::Status::ErrNoError) {
                errorText = tr("Unable to addMail");
                break;
            }
        }
    }

    if (errorCode == QMailServiceAction::Status::ErrNoError) {
        _service->_client.newConnection();
        return true;
    }

    _service->errorOccurred(errorCode, errorText);
    return false;
}

void SmtpService::Sink::messageTransmitted(const QMailMessageId &id)
{
    emit messagesTransmitted(QMailMessageIdList() << id);
}

void SmtpService::Sink::sendCompleted()
{
    emit _service->actionCompleted(true);
    emit _service->activityChanged(QMailServiceAction::Successful);
}


SmtpService::SmtpService(const QMailAccountId &accountId)
    : QMailMessageService(),
      _client(this),
      _sink(new Sink(this))
{
    connect(&_client, SIGNAL(progressChanged(uint, uint)), this, SIGNAL(progressChanged(uint, uint)));

    connect(&_client, SIGNAL(errorOccurred(int, QString)), this, SLOT(errorOccurred(int, QString)));
    connect(&_client, SIGNAL(errorOccurred(QMailServiceAction::Status::ErrorCode, QString)), this, SLOT(errorOccurred(QMailServiceAction::Status::ErrorCode, QString)));
    connect(&_client, SIGNAL(updateStatus(QString)), this, SLOT(updateStatus(QString)));

    _client.setAccount(accountId);
}

SmtpService::~SmtpService()
{
    delete _sink;
}

QString SmtpService::service() const
{
    return serviceKey;
}

QMailAccountId SmtpService::accountId() const
{
    return _client.account();
}

bool SmtpService::hasSink() const
{
    return true;
}

QMailMessageSink &SmtpService::sink() const
{
    return *_sink;
}

bool SmtpService::available() const
{
    return true;
}

bool SmtpService::cancelOperation()
{
    _client.cancelTransfer();
    return true;
}

void SmtpService::errorOccurred(int code, const QString &text)
{
    updateStatus(code, text, _client.account());
    emit actionCompleted(false);
}

void SmtpService::errorOccurred(QMailServiceAction::Status::ErrorCode code, const QString &text)
{
    updateStatus(code, text, _client.account());
    emit actionCompleted(false);
}

void SmtpService::updateStatus(const QString &text)
{
    updateStatus(QMailServiceAction::Status::ErrNoError, text, _client.account());
}


class SmtpConfigurator : public QMailMessageServiceConfigurator
{
public:
    SmtpConfigurator();
    ~SmtpConfigurator();

    virtual QString service() const;
    virtual QString displayName() const;

    virtual QMailMessageServiceEditor *createEditor(QMailMessageServiceFactory::ServiceType type);
};

SmtpConfigurator::SmtpConfigurator()
{
}

SmtpConfigurator::~SmtpConfigurator()
{
}

QString SmtpConfigurator::service() const
{
    return serviceKey;
}

QString SmtpConfigurator::displayName() const
{
    return qApp->translate("QMailMessageService", "SMTP");
}

QMailMessageServiceEditor *SmtpConfigurator::createEditor(QMailMessageServiceFactory::ServiceType type)
{
    if (type == QMailMessageServiceFactory::Sink)
        return new SmtpSettings;

    return 0;
}

Q_EXPORT_PLUGIN2(smtp,SmtpServicePlugin)

SmtpServicePlugin::SmtpServicePlugin()
    : QMailMessageServicePlugin()
{
}

QString SmtpServicePlugin::key() const
{
    return serviceKey;
}

bool SmtpServicePlugin::supports(QMailMessageServiceFactory::ServiceType type) const
{
    return (type == QMailMessageServiceFactory::Sink);
}

bool SmtpServicePlugin::supports(QMailMessage::MessageType type) const
{
    return (type == QMailMessage::Email);
}

QMailMessageService *SmtpServicePlugin::createService(const QMailAccountId &id)
{
    return new SmtpService(id);
}

QMailMessageServiceConfigurator *SmtpServicePlugin::createServiceConfigurator()
{
    return new SmtpConfigurator();
}


#include "smtpservice.moc"


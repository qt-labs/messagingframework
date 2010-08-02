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

#include "smtpservice.h"
#ifndef QMF_NO_MESSAGE_SERVICE_EDITOR
#include "smtpsettings.h"
#endif
#include <QtPlugin>
#include <QTimer>
#include <QCoreApplication>

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
    bool messageQueued = false;
    QMailMessageIdList failedMessages;
    QString errorText;

    if (!ids.isEmpty()) {
        foreach (const QMailMessageId id, ids) {
            QMailMessage message(id);
            if (_service->_client.addMail(message) == QMailServiceAction::Status::ErrNoError) {
                messageQueued = true;
            } else {
                failedMessages << id;
            }
        }
    }
    
    if (failedMessages.count()) {
        emit messagesFailedTransmission(failedMessages, QMailServiceAction::Status::ErrInvalidAddress);
    }

    if (messageQueued) {
        // At least one message could be queued for sending
        _service->_client.newConnection();
    } else {
        // No messages to send, so sending completed successfully
        QTimer::singleShot(0, this, SLOT(sendCompleted()));
    }
    return true;
}

void SmtpService::Sink::messageTransmitted(const QMailMessageId &id)
{
    emit messagesTransmitted(QMailMessageIdList() << id);
}

void SmtpService::Sink::sendCompleted()
{
    emit _service->actionCompleted(true);
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

bool SmtpService::cancelOperation(QMailServiceAction::Status::ErrorCode code, const QString &text)
{
    _client.cancelTransfer(code, text);
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

#ifndef QMF_NO_MESSAGE_SERVICE_EDITOR
    virtual QMailMessageServiceEditor *createEditor(QMailMessageServiceFactory::ServiceType type);
#endif
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
    return QCoreApplication::instance()->translate("QMailMessageService", "SMTP");
}

#ifndef QMF_NO_MESSAGE_SERVICE_EDITOR
QMailMessageServiceEditor *SmtpConfigurator::createEditor(QMailMessageServiceFactory::ServiceType type)
{
    if (type == QMailMessageServiceFactory::Sink)
        return new SmtpSettings;

    return 0;
}
#endif

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


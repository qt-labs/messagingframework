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

#include "smtpservice.h"
#ifndef QMF_NO_WIDGETS
#include "smtpsettings.h"
#endif
#include <QtPlugin>
#include <QTimer>
#include <QCoreApplication>
#include <qmaillog.h>

namespace { const QString serviceKey("smtp"); }

/* TODO: in future, use QNetworkInformation class */
class NetworkStatusMonitor : public QObject
{
    Q_OBJECT
public:
    NetworkStatusMonitor(QObject *parent = nullptr) : QObject(parent) {}
    bool isOnline() const { return true; }
Q_SIGNALS:
    void onlineStateChanged(bool online);
};

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
    bool transmitMessages(const QMailMessageIdList &ids) override;

    void messageTransmitted(const QMailMessageId &id);
    void sendCompleted();

private:
    SmtpService *_service;
};

bool SmtpService::Sink::transmitMessages(const QMailMessageIdList &ids)
{
    QMailMessageIdList failedMessages;

    if (!ids.isEmpty()) {
        foreach (const QMailMessageId id, ids) {
            QMailMessage message(id);
            if (!(_service->_client.addMail(message) == QMailServiceAction::Status::ErrNoError)) {
                failedMessages << id;
            }
        }
    }

    if (failedMessages.count()) {
        emit messagesFailedTransmission(failedMessages, QMailServiceAction::Status::ErrInvalidAddress);
    }

    // Open new connection, even if there's no messages queued for transmission, client might
    // want to test the connection or the smtp server capabilities
    // (e.g. forward without download capable) are not known
     _service->_client.newConnection();
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
      _client(accountId, this),
      _sink(new Sink(this)),
      _capabilityFetcher(nullptr),
      _capabilityFetchTimeout(0)
{
    connect(&_client, SIGNAL(progressChanged(uint, uint)), this, SIGNAL(progressChanged(uint, uint)));

    connect(&_client, SIGNAL(errorOccurred(int, QString)), this, SLOT(errorOccurred(int, QString)));
    connect(&_client, SIGNAL(errorOccurred(QMailServiceAction::Status, QString)), this, SLOT(errorOccurred(QMailServiceAction::Status, QString)));
    connect(&_client, SIGNAL(updateStatus(QString)), this, SLOT(updateStatus(QString)));

    fetchCapabilities();
}

void SmtpService::fetchCapabilities()
{
    QMailAccount account(_client.account());
    if (account.customField("qmf-smtp-capabilities-listed") != "true") {
        if (!_capabilityFetcher) {
            _capabilityFetcher = new SmtpClient(account.id(), this);
            connect(_capabilityFetcher, &SmtpClient::fetchCapabilitiesFinished,
                    this, &SmtpService::onCapabilitiesFetched);
        }
        _capabilityFetcher->fetchCapabilities();
    }
}

void SmtpService::onCapabilitiesFetched()
{
    QMailAccount account(_client.account());
    if (account.customField("qmf-smtp-capabilities-listed") != "true") {
        uint timeout = 1000;
        if (!_capabilityFetchTimeout) {
            _capabilityFetchTimeout = new QTimer(this);
            _capabilityFetchTimeout->setSingleShot(true);
            connect(_capabilityFetchTimeout, &QTimer::timeout,
                    this, &SmtpService::fetchCapabilities);
        } else {
            timeout = _capabilityFetchTimeout->interval() << 2; // * 4
        }
        const uint capabilityCheckTimeoutLimit = 5 * 60 * 1000; // 5 minutes
        if (timeout <= capabilityCheckTimeoutLimit) {
            _capabilityFetchTimeout->setInterval(timeout);
            _capabilityFetchTimeout->start();
            qMailLog(SMTP) << "Could not fetch capabilities...trying again after " << (timeout / 1000) << "seconds";
        } else {
            qMailLog(SMTP) << "Could not fetch capabilities, giving up";
        }
    } else {
        _capabilityFetcher->deleteLater();
        _capabilityFetcher = nullptr;
        delete _capabilityFetchTimeout;
        _capabilityFetchTimeout = nullptr;
    }
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

void SmtpService::errorOccurred(const QMailServiceAction::Status & status, const QString &text)
{
    updateStatus(status.errorCode, text, _client.account(), status.folderId, status.messageId);
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

    QString service() const override;
    QString displayName() const override;

#ifndef QMF_NO_WIDGETS
    QMailMessageServiceEditor *createEditor(QMailMessageServiceFactory::ServiceType type) override;
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

#ifndef QMF_NO_WIDGETS
QMailMessageServiceEditor *SmtpConfigurator::createEditor(QMailMessageServiceFactory::ServiceType type)
{
    if (type == QMailMessageServiceFactory::Sink)
        return new SmtpSettings;

    return 0;
}
#endif

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


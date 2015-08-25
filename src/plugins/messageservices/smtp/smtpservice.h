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

#ifndef SMTPSERVICE_H
#define SMTPSERVICE_H

#include "smtpclient.h"
#include <qmailmessageservice.h>

#include <QPointer>

QT_BEGIN_NAMESPACE

class QNetworkConfigurationManager;

QT_END_NAMESPACE

class SmtpService : public QMailMessageService
{
    Q_OBJECT

public:
    using QMailMessageService::updateStatus;

    SmtpService(const QMailAccountId &accountId);
    ~SmtpService();

    virtual QString service() const;
    virtual QMailAccountId accountId() const;

    virtual bool hasSink() const;
    virtual QMailMessageSink &sink() const;

    virtual bool available() const;

public slots:
    virtual bool cancelOperation(QMailServiceAction::Status::ErrorCode code, const QString &text);

protected slots:
    void errorOccurred(int code, const QString &text);
    void errorOccurred(const QMailServiceAction::Status & status, const QString &text);

    void updateStatus(const QString& text);

private slots:
    void fetchCapabilities();
    void onCapabilityFetchingActivityChanged(QMailServiceAction::Activity activity);
    void onOnlineStateChanged(bool isOnline);
    void onAccountsUpdated(const QMailAccountIdList & accountIds);

private:
    class Sink;
    friend class Sink;

    SmtpClient _client;
    Sink *_sink;
    QMailTransmitAction *_capabilityFetchAction;
    QTimer *_capabilityFetchTimeout;
    QNetworkConfigurationManager *_networkManager;
};

class SmtpServicePlugin : public QMailMessageServicePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.SmtpServicePluginHandlerFactoryInterface")

public:
    SmtpServicePlugin();

    virtual QString key() const;
    virtual bool supports(QMailMessageServiceFactory::ServiceType type) const;
    virtual bool supports(QMailMessage::MessageType type) const;

    virtual QMailMessageService *createService(const QMailAccountId &id);
    virtual QMailMessageServiceConfigurator *createServiceConfigurator();
};


#endif

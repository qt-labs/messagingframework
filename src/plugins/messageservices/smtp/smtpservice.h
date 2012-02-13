/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef SMTPSERVICE_H
#define SMTPSERVICE_H

#include "smtpclient.h"
#include <qmailmessageservice.h>

#include <QPointer>

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
    class QNetworkConfigurationManager *_networkManager;
};


class SmtpServicePlugin : public QMailMessageServicePlugin
{
    Q_OBJECT

public:
    SmtpServicePlugin();

    virtual QString key() const;
    virtual bool supports(QMailMessageServiceFactory::ServiceType type) const;
    virtual bool supports(QMailMessage::MessageType type) const;

    virtual QMailMessageService *createService(const QMailAccountId &id);
    virtual QMailMessageServiceConfigurator *createServiceConfigurator();
};


#endif

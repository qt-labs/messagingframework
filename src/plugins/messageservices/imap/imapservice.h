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

#ifndef IMAPSERVICE_H
#define IMAPSERVICE_H

#include "imapclient.h"
#include "idlenetworksession.h"
#include <qmailmessageservice.h>

class ImapService : public QMailMessageService
{
    Q_OBJECT

public:
    using QMailMessageService::updateStatus;

    ImapService(const QMailAccountId &accountId);
    ~ImapService();

    void enable();
    void disable();

    QString service() const override;
    QMailAccountId accountId() const override;

    bool hasSource() const override;
    QMailMessageSource &source() const override;

    bool available() const override;
    bool pushEmailEstablished();

public slots:
    bool cancelOperation(QMailServiceAction::Status::ErrorCode code, const QString &text) override;
    virtual void restartPushEmail();
    virtual void initiatePushEmail();

protected slots:
    virtual void accountsUpdated(const QMailAccountIdList &ids);
    void errorOccurred(int code, const QString &text);
    void errorOccurred(QMailServiceAction::Status::ErrorCode code, const QString &text);

    void updateStatus(const QString& text);
    // Only used for IMAP IDLE, network session for other request types are managed by the caller.
    void createIdleSession();
    void openIdleSession();
    void closeIdleSession();

private slots:
    void onOnlineStateChanged(bool isOnline);
    void onSessionOpened();
    void onSessionStateChanged(IdleNetworkSession::State status);
    void onSessionError(IdleNetworkSession::Error error);
    void onSessionConnectionTimeout();

private:
    class Source;
    friend class Source;

    bool accountPushEnabled();
    void setPersistentConnectionStatus(bool status);
    void enablePushEmail();
    void disablePushEmail();

    QMailAccountId _accountId;
    ImapClient *_client;
    Source *_source;
    QTimer *_restartPushEmailTimer;
    bool _establishingPushEmail;
    bool _idling;
    int _pushRetry;
    bool _accountWasPushEnabled;
    QStringList _previousPushFolders;
    enum { ThirtySeconds = 30 };
    static QMap<QMailAccountId, int> _initiatePushDelay; // Limit battery consumption
    QTimer *_initiatePushEmailTimer;
    IdleNetworkSession              *_networkSession;          // IDLE network session
    QTimer                          *_networkSessionTimer;
};

class ImapServicePlugin : public QMailMessageServicePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.ImapServicePluginHandlerFactoryInterface")

public:
    ImapServicePlugin();

    QString key() const override;
    bool supports(QMailMessageServiceFactory::ServiceType type) const override;
    bool supports(QMailMessage::MessageType type) const override;

    QMailMessageService *createService(const QMailAccountId &id) override;
    QMailMessageServiceConfigurator *createServiceConfigurator() override;
};


#endif

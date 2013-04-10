/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef IMAPSERVICE_H
#define IMAPSERVICE_H

#include "imapclient.h"
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

    virtual QString service() const;
    virtual QMailAccountId accountId() const;

    virtual bool hasSource() const;
    virtual QMailMessageSource &source() const;

    virtual bool available() const;
    bool pushEmailEstablished();

public slots:
    virtual bool cancelOperation(QMailServiceAction::Status::ErrorCode code, const QString &text);
    virtual void restartPushEmail();
    virtual void initiatePushEmail();

protected slots:
    virtual void accountsUpdated(const QMailAccountIdList &ids);
    void errorOccurred(int code, const QString &text);
    void errorOccurred(QMailServiceAction::Status::ErrorCode code, const QString &text);

    void updateStatus(const QString& text);

private:
    class Source;
    friend class Source;

    QMailAccountId _accountId;
    ImapClient *_client;
    Source *_source;
    QTimer *_restartPushEmailTimer;
    bool _establishingPushEmail;
    int _pushRetry;
    bool _accountWasEnabled;
    bool _accountWasPushEnabled;
    QStringList _previousPushFolders;
    QString _previousConnectionSettings; // Connection related settings
    enum { ThirtySeconds = 30 };
    static QMap<QMailAccountId, int> _initiatePushDelay; // Limit battery consumption
    QTimer *_initiatePushEmailTimer;
};

class ImapServicePlugin : public QMailMessageServicePlugin
{
    Q_OBJECT
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.ImapServicePluginHandlerFactoryInterface")
#endif

public:
    ImapServicePlugin();

    virtual QString key() const;
    virtual bool supports(QMailMessageServiceFactory::ServiceType type) const;
    virtual bool supports(QMailMessage::MessageType type) const;

    virtual QMailMessageService *createService(const QMailAccountId &id);
    virtual QMailMessageServiceConfigurator *createServiceConfigurator();
};


#endif

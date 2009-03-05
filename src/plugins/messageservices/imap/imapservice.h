/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
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

    virtual QString service() const;
    virtual QMailAccountId accountId() const;

    virtual bool hasSource() const;
    virtual QMailMessageSource &source() const;

    virtual bool available() const;

public slots:
    virtual bool cancelOperation();

protected slots:
    void errorOccurred(int code, const QString &text);
    void errorOccurred(QMailServiceAction::Status::ErrorCode code, const QString &text);

    void updateStatus(const QString& text);

private:
    class Source;
    friend class Source;

    ImapClient _client;
    Source *_source;
};

class ImapServicePlugin : public QMailMessageServicePlugin
{
    Q_OBJECT

public:
    ImapServicePlugin();

    virtual QString key() const;
    virtual bool supports(QMailMessageServiceFactory::ServiceType type) const;
    virtual bool supports(QMailMessage::MessageType type) const;

    virtual QMailMessageService *createService(const QMailAccountId &id);
    virtual QMailMessageServiceConfigurator *createServiceConfigurator();
};


#endif

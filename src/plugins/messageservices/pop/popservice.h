/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef POPSERVICE_H
#define POPSERVICE_H

#include "popclient.h"
#include <qmailmessageservice.h>

class PopService : public QMailMessageService
{
    Q_OBJECT

public:
    using QMailMessageService::updateStatus;

    PopService(const QMailAccountId &accountId);
    ~PopService();

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

    PopClient _client;
    Source *_source;
};


class PopServicePlugin : public QMailMessageServicePlugin
{
    Q_OBJECT

public:
    PopServicePlugin();

    virtual QString key() const;
    virtual bool supports(QMailMessageServiceFactory::ServiceType type) const;
    virtual bool supports(QMailMessage::MessageType type) const;

    virtual QMailMessageService *createService(const QMailAccountId &id);
    virtual QMailMessageServiceConfigurator *createServiceConfigurator();
};


#endif

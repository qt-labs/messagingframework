/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef SERVICE_H
#define SERVICE_H

#include <qmailmessageservice.h>

class QtopiamailfileServicePlugin : public QMailMessageServicePlugin
{
    Q_OBJECT

public:
    QtopiamailfileServicePlugin();

    virtual QString key() const;
    virtual bool supports(QMailMessageServiceFactory::ServiceType type) const;
    virtual bool supports(QMailMessage::MessageType type) const;

    virtual QMailMessageService *createService(const QMailAccountId &accountId);
    virtual QMailMessageServiceConfigurator *createServiceConfigurator();
};

#endif

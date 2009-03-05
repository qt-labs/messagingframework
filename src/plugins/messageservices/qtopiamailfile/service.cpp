/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#include "service.h"
#include "settings.h"
#ifndef QMAIL_QTOPIA
#include <QtPlugin>
#endif

namespace { const QString serviceKey("qtopiamailfile"); }


class QtopiamailfileConfigurator : public QMailMessageServiceConfigurator
{
public:
    QtopiamailfileConfigurator();
    ~QtopiamailfileConfigurator();

    virtual QString service() const;
    virtual QString displayName() const;

    virtual QMailMessageServiceEditor *createEditor(QMailMessageServiceFactory::ServiceType type);
};

QtopiamailfileConfigurator::QtopiamailfileConfigurator()
{
}

QtopiamailfileConfigurator::~QtopiamailfileConfigurator()
{
}

QString QtopiamailfileConfigurator::service() const
{
    return serviceKey;
}

QString QtopiamailfileConfigurator::displayName() const
{
    return qApp->translate("QMailMessageService", "Mailfile");
}

QMailMessageServiceEditor *QtopiamailfileConfigurator::createEditor(QMailMessageServiceFactory::ServiceType type)
{
    if (type == QMailMessageServiceFactory::Storage)
        return new QtopiamailfileSettings;

    return 0;
}

#ifdef QMAIL_QTOPIA
QTOPIA_EXPORT_PLUGIN( QtopiamailfileServicePlugin )
#else
Q_EXPORT_PLUGIN2(qtopiamailfile,QtopiamailfileServicePlugin)
#endif

QtopiamailfileServicePlugin::QtopiamailfileServicePlugin()
    : QMailMessageServicePlugin()
{
}

QString QtopiamailfileServicePlugin::key() const
{
    return serviceKey;
}

bool QtopiamailfileServicePlugin::supports(QMailMessageServiceFactory::ServiceType type) const
{
    return (type == QMailMessageServiceFactory::Storage);
}

bool QtopiamailfileServicePlugin::supports(QMailMessage::MessageType) const
{
    return true;
}

QMailMessageService *QtopiamailfileServicePlugin::createService(const QMailAccountId &)
{
    return 0;
}

QMailMessageServiceConfigurator *QtopiamailfileServicePlugin::createServiceConfigurator()
{
    return new QtopiamailfileConfigurator();
}



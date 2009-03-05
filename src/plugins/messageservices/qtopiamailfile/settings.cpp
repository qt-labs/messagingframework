/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#include "settings.h"

#include <QComboBox>

#include <qmailaccount.h>
#include <qmailaccountconfiguration.h>


extern QList<QPair<QString, QString> > storageLocations();


namespace { 

const QString serviceKey("qtopiamailfile"); 

const QList<QPair<QString, QString> > locations(storageLocations());

}

QtopiamailfileSettings::QtopiamailfileSettings()
    : QMailMessageServiceEditor()
{
    setupUi(this);
    setLayoutDirection(qApp->layoutDirection());

    QList<QPair<QString, QString> >::const_iterator it = locations.begin(), end = locations.end();
    for ( ; it != end; ++it)
        locationSelector->addItem((*it).first);
}

void QtopiamailfileSettings::displayConfiguration(const QMailAccount &, const QMailAccountConfiguration &config)
{
    if (!config.services().contains(serviceKey)) {
        // New account
        locationSelector->setCurrentIndex(0);
    } else {
        const QMailAccountConfiguration::ServiceConfiguration &svcCfg(config.serviceConfiguration(serviceKey));
        QString path(svcCfg.value("basePath"));

        QList<QPair<QString, QString> >::const_iterator it = locations.begin(), end = locations.end();
        for (int i = 0; it != end; ++it, ++i)
            if ((*it).second == path) {
                locationSelector->setCurrentIndex(i);
                break;
            }
    }
}

bool QtopiamailfileSettings::updateAccount(QMailAccount *, QMailAccountConfiguration *config)
{
    if (!config->services().contains(serviceKey))
        config->addServiceConfiguration(serviceKey);

    QMailAccountConfiguration::ServiceConfiguration &svcCfg(config->serviceConfiguration(serviceKey));
    svcCfg.setValue("version", "100");
    svcCfg.setValue("servicetype", "storage");

    svcCfg.setValue("basePath", locations[locationSelector->currentIndex()].second);
    return true;
}


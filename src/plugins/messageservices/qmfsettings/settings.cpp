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

#include "settings.h"

#include <QComboBox>

#include <qmailaccount.h>
#include <qmailaccountconfiguration.h>


extern QList<QPair<QString, QString> > storageLocations();


namespace { 

const QString serviceKey("qmfstoragemanager");

const QList<QPair<QString, QString> > locations(storageLocations());

}

QmfSettings::QmfSettings()
    : QMailMessageServiceEditor()
{
    setupUi(this);
    setLayoutDirection(qApp->layoutDirection());

    QList<QPair<QString, QString> >::const_iterator it = locations.begin(), end = locations.end();
    for ( ; it != end; ++it)
        locationSelector->addItem((*it).first);
}

void QmfSettings::displayConfiguration(const QMailAccount &, const QMailAccountConfiguration &config)
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

bool QmfSettings::updateAccount(QMailAccount *, QMailAccountConfiguration *config)
{
    if (!config->services().contains(serviceKey))
        config->addServiceConfiguration(serviceKey);

    QMailAccountConfiguration::ServiceConfiguration &svcCfg(config->serviceConfiguration(serviceKey));
    svcCfg.setValue("version", "101");
    svcCfg.setValue("servicetype", "storage");

    svcCfg.setValue("basePath", locations[locationSelector->currentIndex()].second);
    return true;
}


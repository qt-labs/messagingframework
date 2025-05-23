/****************************************************************************
**
** Copyright (C) 2024 Damien Caliste
** Contact: Damien Caliste <dcaliste@free.fr>
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

#include "plugin.h"
#include "ssomanager.h"

#include <qmaillog.h>
#include <qmailaccountconfiguration.h>

class SSOCredentials: public SSOManager
{
public:
    SSOCredentials(QObject *parent = nullptr);

    bool init(const QMailServiceConfiguration &svcCfg) override;
};

SSOCredentials::SSOCredentials(QObject *parent)
    : SSOManager(parent)
{
}

bool SSOCredentials::init(const QMailServiceConfiguration &svcCfg)
{
    QMailCredentialsInterface::init(svcCfg);

    uint credentialsId = svcCfg.value(QString::fromLatin1("CredentialsId")).toUInt();
    QString method;
    QString mechanism;
    QVariantMap parameters;
    QMailAccountConfiguration config(m_id);
    const QMailAccountConfiguration::ServiceConfiguration &authSrv = config.serviceConfiguration(QLatin1String("auth"));
    if (authSrv.id().isValid()) {
        if (!credentialsId) {
            credentialsId = authSrv.value(QString::fromLatin1("CredentialsId")).toUInt();
        }
        method = authSrv.value(QLatin1String("method"));
        mechanism = authSrv.value(QLatin1String("mechanism"));
        const QMap<QString, QString> values = authSrv.values();
        for (QMap<QString, QString>::ConstIterator it = values.constBegin();
             it != values.constEnd(); it++) {
            const QString prefix = QStringLiteral("%1/%2/").arg(method).arg(mechanism);
            if (it.key().startsWith(prefix)) {
                parameters.insert(it.key().mid(prefix.length()), it.value());
            }
        }
    }
    parameters.insert("UserName", svcCfg.value(QStringLiteral("username")));

    qCDebug(lcMessaging) << "Creating SSO identity for the service" << service()
                         << "from account" << id() << "with creds id" << credentialsId;
    return SSOManager::init(credentialsId, method, mechanism, parameters);
}

SSOPlugin::SSOPlugin(QObject *parent)
    : QMailCredentialsPlugin(parent)
{
}

QString SSOPlugin::key() const
{
    return QString::fromLatin1("sso");
}

QMailCredentialsInterface* SSOPlugin::createCredentialsHandler(QObject* parent)
{
    return new SSOCredentials(parent);
}

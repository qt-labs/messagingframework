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

#include "qmailcredentials.h"

#include <qmaillog.h>
#include <qmailstore.h>
#include <qmailpluginmanager.h>

class PlainCredentialConfiguration: public QMailServiceConfiguration
{
public:
    PlainCredentialConfiguration(QMailAccountConfiguration *config, const QString &service)
        : QMailServiceConfiguration(config, service) {}
    ~PlainCredentialConfiguration() {}

    QString username() const
    {
        const QString key = QLatin1String(service() == QLatin1String("smtp") ? "smtpusername" : "username");
        return isValid() ? value(key) : QString();
    }

    QString password() const
    {
        const QString key = QLatin1String(service() == QLatin1String("smtp") ? "smtppassword" : "password");
        // SMTP, POP and IMAP service configurations are
        // base64 encoding the password in storage,
        // see smtpconfiguration.cpp for instance.
        return isValid() ? decodeValue(value(key)) : QString();
    }
};

class PlainCredentials: public QMailCredentialsInterface
{
public:
    PlainCredentials(QObject* parent = nullptr);
    ~PlainCredentials();

    bool init(const QMailServiceConfiguration &svcCfg) override;

    QMailCredentialsInterface::Status status() const override;
    QString lastError() const override;

    QString username() const override;
    QString password() const override;
    QString accessToken() const override;

private:
    QMailCredentialsInterface::Status m_status = Invalid;
};

PlainCredentials::PlainCredentials(QObject *parent)
    : QMailCredentialsInterface(parent)
{
}

PlainCredentials::~PlainCredentials()
{
}

bool PlainCredentials::init(const QMailServiceConfiguration &svcCfg)
{
    QMailCredentialsInterface::init(svcCfg);
    m_status = Ready;
    emit statusChanged();
    return true;
}

QMailCredentialsInterface::Status PlainCredentials::status() const
{
    return m_status;
}

QString PlainCredentials::lastError() const
{
    return QString();
}

QString PlainCredentials::username() const
{
    QMailAccountConfiguration config(id());
    return PlainCredentialConfiguration(&config, service()).username();
}

QString PlainCredentials::password() const
{
    QMailAccountConfiguration config(id());
    return PlainCredentialConfiguration(&config, service()).password();
}

QString PlainCredentials::accessToken() const
{
    QMailAccountConfiguration config(id());
    QMailServiceConfiguration srv(&config, service());
    return srv.isValid() ? srv.value(QLatin1String("accessToken")) : QString();
}

static QString PLUGIN_KEY = QStringLiteral("messagecredentials");

typedef QMap<QString, QMailCredentialsPlugin*> PluginMap;

static PluginMap initMap(QMailPluginManager& manager)
{
    PluginMap map;

    const QStringList keys = manager.list();
    for (const QString &key : keys) {
        QMailCredentialsPlugin* plugin = qobject_cast<QMailCredentialsPlugin*>(manager.instance(key));
        if (plugin)
            map.insert(plugin->key(), plugin);
    }
    return map;
}

// Return a reference to a map containing all loaded plugin objects
static PluginMap& pluginMap()
{
    static QMailPluginManager pluginManager(PLUGIN_KEY);
    static PluginMap map(initMap(pluginManager));

    return map;
}

QStringList QMailCredentialsFactory::keys()
{
    return pluginMap().keys();
}

QMailCredentialsInterface *QMailCredentialsFactory::createCredentialsHandler(const QString& key,
                                                                             QObject *parent)
{
    PluginMap::ConstIterator it = pluginMap().find(key);
    if (it != pluginMap().end())
        return (*it)->createCredentialsHandler(parent);

    qCWarning(lcMessaging) << "Unknown plugin: " << key;
    return nullptr;
}

QMailCredentialsInterface *QMailCredentialsFactory::defaultCredentialsHandler(QObject *parent)
{
    return new PlainCredentials(parent);
}

QMailCredentialsInterface *QMailCredentialsFactory::getCredentialsHandlerForAccount(const QMailAccountConfiguration &config, QObject *parent)
{
    QMailCredentialsInterface *credentials = nullptr;

    const QMailAccountConfiguration::ServiceConfiguration &auth = config.serviceConfiguration(QLatin1String("auth"));
    if (auth.id().isValid()) {
        const QString plugin = auth.value(QLatin1String("plugin"));
        if (!plugin.isEmpty()) {
            credentials = QMailCredentialsFactory::createCredentialsHandler(plugin, parent);
            if (!credentials) {
                qCWarning(lcMessaging) << "Credential plugin" << plugin
                                       << "is not available for account id: "
                                       << config.id()
                                       << ", account configuration will be used";
            }
        }
    }
    if (!credentials) {
        credentials = QMailCredentialsFactory::defaultCredentialsHandler(parent);
    }
    return credentials;
}

QMailCredentialsPlugin::QMailCredentialsPlugin(QObject* parent)
    : QObject(parent)
{
}

QMailCredentialsPlugin::~QMailCredentialsPlugin()
{
}

class QMailCredentialsInterfacePrivate
{
public:
    QMailCredentialsInterfacePrivate() {}

    QMailAccountId m_id;
    QString m_service;
};

QMailCredentialsInterface::QMailCredentialsInterface(QObject* parent)
    : QObject(parent)
    , d(new QMailCredentialsInterfacePrivate)
{
}

QMailCredentialsInterface::~QMailCredentialsInterface()
{
}

bool QMailCredentialsInterface::init(const QMailServiceConfiguration &svcCfg)
{
    d->m_id = svcCfg.id();
    d->m_service = svcCfg.service();

    return true;
}

QMailAccountId QMailCredentialsInterface::id() const
{
    return d->m_id;
}

QString QMailCredentialsInterface::service() const
{
    return d->m_service;
}

QString QMailCredentialsInterface::username() const
{
    qCWarning(lcMessaging) << "username credential not supported for service:" << service();
    return QString();
}

QString QMailCredentialsInterface::password() const
{
    qCWarning(lcMessaging) << "password credential not supported for service:" << service();
    return QString();
}

QString QMailCredentialsInterface::accessToken() const
{
    qCWarning(lcMessaging) << "access token credential not supported for service:" << service();
    return QString();
}

void QMailCredentialsInterface::invalidate(const QString &source)
{
    if (id().isValid()) {
        qCWarning(lcMessaging) << "Invalidate credentials" << service()
                               << " from account" << id().toULongLong();
        QMailAccountConfiguration config(id());
        QMailAccountConfiguration::ServiceConfiguration &srv = config.serviceConfiguration(service());
        srv.setValue(QLatin1String("CredentialsNeedUpdate"),
                     QLatin1String("true"));
        if (!source.isEmpty()) {
            srv.setValue(QLatin1String("CredentialsNeedUpdateFrom"), source);
        }
        QMailStore::instance()->updateAccountConfiguration(&config);
    }
}

bool QMailCredentialsInterface::isInvalidated()
{
    if (id().isValid()) {
        const QMailAccountConfiguration config(id());
        const QMailAccountConfiguration::ServiceConfiguration &srv = config.serviceConfiguration(service());
        return srv.value(QLatin1String("CredentialsNeedUpdate"))
            .compare(QLatin1String("true"), Qt::CaseInsensitive) == 0;
    } else {
        return false;
    }
}

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

#include "qmailaccountconfiguration.h"
#include "qmailstore.h"


class QMailAccountConfiguration::ConfigurationValues
{
public:
    ConfigurationValues();

    QMap<QString, QString> _values;
    bool _removed;
};

QMailAccountConfiguration::ConfigurationValues::ConfigurationValues()
    : _removed(false)
{
}


class QMailAccountConfiguration::ServiceConfigurationPrivate
{
public:
    ServiceConfigurationPrivate();
    ServiceConfigurationPrivate(QMailAccountConfigurationPrivate *parent, const QString *service, ConfigurationValues *configuration);

    QMailAccountConfigurationPrivate *_parent;
    const QString *_service;
    ConfigurationValues *_configuration;
};

QMailAccountConfiguration::ServiceConfigurationPrivate::ServiceConfigurationPrivate()
    : _parent(0),
      _service(0),
      _configuration(0)
{
}

QMailAccountConfiguration::ServiceConfigurationPrivate::ServiceConfigurationPrivate(QMailAccountConfigurationPrivate *parent, const QString *service, ConfigurationValues *configuration)
    : _parent(parent),
      _service(service),
      _configuration(configuration)
{
}


class QMailAccountConfigurationPrivate : public QSharedData
{
public:
    QMailAccountConfigurationPrivate();
    ~QMailAccountConfigurationPrivate();
    
    QMailAccountConfiguration::ServiceConfiguration &serviceConfiguration(const QString &service) const;

private:
    friend class QMailAccountConfiguration;
    friend class QMailAccountConfiguration::ServiceConfiguration;

    QMailAccountId _id;
    QMap<QString, QMailAccountConfiguration::ConfigurationValues> _services;
    mutable QMap<QString, QMailAccountConfiguration::ServiceConfiguration> _configurations;
    bool _modified;
};


/*!
    \class QMailAccountConfiguration::ServiceConfiguration

    \preliminary
    \brief The ServiceConfiguration class provides access to the configuration parameters 
    of a single named service within an account.
    \ingroup messaginglibrary

    ServiceConfiguration provides an interface mapping the configuration parameters of
    the service as a set of key-value pairs, where all data is maintained in QString form.

    A ServiceConfiguration object cannot be directly constructed, but can be acquired
    from the containing account's QMailAccountConfiguration object. For example:

    \code
    QMailAccountConfiguration config(accountId);
    
    if (config.id().isValid()) {
        qDebug() << "Account" << config.id() << ":";
        foreach (const QString& service, config.services()) {
            QMailAccountConfiguration::ServiceConfiguration &svcCfg(config.serviceConfiguration(service));
            qDebug() << "\tService" << service << ":";
            foreach (const QString &name, svcCfg.values().keys()) {
                qDebug() << "\t\t" << name << ":" << svcCfg.value(name);
            }
        }
    }
    \endcode

    Several predefined sets of key/value pairs are used to control the behavior of retrieval
    services, such as IMAP and POP services, as described in the following table. Other
    retrieval protocols should behave similarly.
    
    \table
    \header \o Desired retrieval behavior \o Required key/value pair settings
    \row \o Headers only \o "downloadAttachments" = "0", "autoDownload" = "0", "maxSize" = "0"
    \row \o Entire message including attachments \o "downloadAttachments" = "1", "autoDownload" = "1", "maxSize" = "0"
    \row \o Message headers, body and when available structure, no attachments \o "downloadAttachments" = "0", "autoDownload" = "1", "maxSize" = "0"
    \endtable
    
    Structure refers to MIME body structure information.
    
    \sa QMailAccountConfiguration::serviceConfiguration()
*/

/*! \internal */
QMailAccountConfiguration::ServiceConfiguration::ServiceConfiguration()
    : d(new QMailAccountConfiguration::ServiceConfigurationPrivate)
{
}

/*! \internal */
QMailAccountConfiguration::ServiceConfiguration::ServiceConfiguration(QMailAccountConfigurationPrivate *parent, const QString *service, QMailAccountConfiguration::ConfigurationValues *configuration)
    : d(new QMailAccountConfiguration::ServiceConfigurationPrivate(parent, service, configuration))
{
}

/*! \internal */
QMailAccountConfiguration::ServiceConfiguration::ServiceConfiguration(const ServiceConfiguration &other)
    : d(new QMailAccountConfiguration::ServiceConfigurationPrivate)
{
    *this = other;
}

/*! \internal */
QMailAccountConfiguration::ServiceConfiguration::~ServiceConfiguration()
{
    delete d;
}

/*! \internal */
const QMailAccountConfiguration::ServiceConfiguration &QMailAccountConfiguration::ServiceConfiguration::operator=(const ServiceConfiguration &other)
{
    d->_parent = other.d->_parent;
    d->_service = other.d->_service;
    d->_configuration = other.d->_configuration;

    return *this;
}

/*!
    Returns the name of the service to which this configuration pertains.
*/
QString QMailAccountConfiguration::ServiceConfiguration::service() const
{
    return *d->_service;
}

/*!
    Returns the identifier of the account to which this configuration pertains.
*/
QMailAccountId QMailAccountConfiguration::ServiceConfiguration::id() const
{
    if (d->_parent)
        return d->_parent->_id;

    return QMailAccountId();
}

/*!
    Returns the value of the parameter named \a name in the service configuration.
*/
QString QMailAccountConfiguration::ServiceConfiguration::value(const QString &name) const
{
    return d->_configuration->_values.value(name);
}

/*!
    Returns the value of the parameter named \a name in the service configuration.
*/
QStringList QMailAccountConfiguration::ServiceConfiguration::listValue(const QString &name) const
{
    const QString value = d->_configuration->_values.value(name);
    return value.startsWith(QStringLiteral("list:\x1F")) && value.length() > 6
        ? value.mid(6).split('\x1F') : QStringList();
}

/*!
    Sets the parameter named \a name to contain the value \a value in the service configuration.
*/
void QMailAccountConfiguration::ServiceConfiguration::setValue(const QString &name, const QString &value)
{
    d->_configuration->_values[name] = value;
    d->_parent->_modified = true;
}

/*!
    Sets the parameter named \a name to contain the list \a list in the service configuration.
*/
void QMailAccountConfiguration::ServiceConfiguration::setValue(const QString &name, const QStringList &list)
{
    d->_configuration->_values[name] = QStringLiteral("list:\x1F") + list.join('\x1F');
    d->_parent->_modified = true;
}

/*!
    Removes the parameter named \a name from the service configuration.
*/
void QMailAccountConfiguration::ServiceConfiguration::removeValue(const QString &name)
{
    d->_configuration->_values.remove(name);
    d->_parent->_modified = true;
}

/*!
    Returns the values of the service configuration parameters.
*/
const QMap<QString, QString> &QMailAccountConfiguration::ServiceConfiguration::values() const
{
    return d->_configuration->_values;
}


QMailAccountConfigurationPrivate::QMailAccountConfigurationPrivate()
    : QSharedData(),
      _modified(false)
{
}

QMailAccountConfigurationPrivate::~QMailAccountConfigurationPrivate()
{
}

QMailAccountConfiguration::ServiceConfiguration &QMailAccountConfigurationPrivate::serviceConfiguration(const QString &service) const
{
    QMap<QString, QMailAccountConfiguration::ServiceConfiguration>::iterator it = _configurations.find(service);
    if (it == _configurations.end()) {
        // Do we have this service?
        QMap<QString, QMailAccountConfiguration::ConfigurationValues>::const_iterator sit = _services.find(service);
        if ((sit != _services.end()) && ((*sit)._removed == false)) {
            // Add a configuration for the service
            QMailAccountConfigurationPrivate *self = const_cast<QMailAccountConfigurationPrivate*>(this);
            QMailAccountConfiguration::ConfigurationValues *config = const_cast<QMailAccountConfiguration::ConfigurationValues*>(&sit.value());

            _configurations.insert(service, QMailAccountConfiguration::ServiceConfiguration(self, &(sit.key()), config));
            it = _configurations.find(service);
        }
    }

    Q_ASSERT(it != _configurations.end() && ((*it).d->_configuration->_removed == false));
    return (*it);
}


/*!
    \class QMailAccountConfiguration

    \preliminary
    \brief The QMailAccountConfiguration class contains the configuration parameters of an account.
    \ingroup messaginglibrary

    QMailAccountConfiguration provides the configuration information for a single account, 
    as retrieved from the mail store.  The configuration is stored as key-value pairs, grouped
    into services, where each service within the account has a different name.  A service
    typically corresponds to a protocol used to implement the account.

    To modify the configuration details, the ServiceConfiguration class must be used. 
    ServiceConfiguration groupings may be added to and removed from the account configuration, 
    but a service configuration may not be modified until it has been added.  A service
    is not stored in the mail store until it has member parameters.

    ServiceConfiguration objects are allocated by, and retained within the QMailAccountConfiguration
    object.
    
    A ServiceConfiguration object cannot be directly constructed, but can be acquired
    from the containing account's QMailAccountConfiguration object.

    \sa QMailAccountConfiguration::serviceConfiguration()
*/

/*!
    Creates an empty configuration object which is not associated with any account.
*/
QMailAccountConfiguration::QMailAccountConfiguration()
    : d(new QMailAccountConfigurationPrivate)
{
}

/*!
    Creates a configuration object which contains the configuration details of the account identified by \a id.
*/
QMailAccountConfiguration::QMailAccountConfiguration(const QMailAccountId &id)
    : d(new QMailAccountConfigurationPrivate)
{
    *this = QMailStore::instance()->accountConfiguration(id);
}

/*! \internal */
QMailAccountConfiguration::QMailAccountConfiguration(const QMailAccountConfiguration& other)
{
    d = other.d;
}

/*! \internal */
const QMailAccountConfiguration &QMailAccountConfiguration::operator=(const QMailAccountConfiguration& other)
{
    if (&other != this)
        d = other.d;

    return *this;
}

/*! \internal */
QMailAccountConfiguration::~QMailAccountConfiguration()
{
}

/*!
    Sets the configuration to pertain to the account identified by \a id.
*/
void QMailAccountConfiguration::setId(const QMailAccountId& id)
{
    d->_id = id;
}

/*!
    Returns the identifier of the account that this configuration pertains to.
*/
QMailAccountId QMailAccountConfiguration::id() const
{
    return d->_id;
}

/*!
    Returns the configuration for the service \a service, which must be present within the account configuration.

    ServiceConfiguration instances are allocated by, and retained within the QMailAccountConfiguration object.
*/
QMailAccountConfiguration::ServiceConfiguration &QMailAccountConfiguration::serviceConfiguration(const QString &service)
{
    return d->serviceConfiguration(service);
}

/*!
    Returns the configuration for the service \a service, which must be present within the account configuration.

    ServiceConfiguration instances are allocated by, and retained within the QMailAccountConfiguration object.

    \sa QMailAccountConfiguration::serviceConfiguration()
*/
const QMailAccountConfiguration::ServiceConfiguration &QMailAccountConfiguration::serviceConfiguration(const QString &service) const
{
    return d->serviceConfiguration(service);
}

/*!
    Adds a configuration for the service named \a service within the account configuration.
    Returns true if successful; otherwise returns false;
*/
bool QMailAccountConfiguration::addServiceConfiguration(const QString &service)
{
    if (d->_services.contains(service))
        return false;

    d->_services.insert(service, ConfigurationValues());
    d->_modified = true;
    return true;
}

/*!
    Removes the configuration for the service named \a service within the account configuration.
    Returns true if successful; otherwise returns false;
*/
bool QMailAccountConfiguration::removeServiceConfiguration(const QString &service)
{
    QMap<QString, ConfigurationValues>::iterator it = d->_services.find(service);
    if (it == d->_services.end())
        return false;

    d->_services.erase(it);
    d->_modified = true;
    return true;
}

/*!
    Returns a list of the services whose configurations are contained within the account configuration.
*/
QStringList QMailAccountConfiguration::services() const
{
    QStringList keys;

    QMap<QString, ConfigurationValues>::const_iterator it = d->_services.begin(), end = d->_services.end();
    for ( ; it != end ; ++it) {
        if (!(*it)._removed)
            keys.append(it.key());
    }

    return keys;
}

/*! \internal */
bool QMailAccountConfiguration::modified() const
{
    return d->_modified;
}

/*! \internal */
void QMailAccountConfiguration::setModified(bool set)
{
    d->_modified = set;
}


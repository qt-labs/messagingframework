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

#include "qmailserviceconfiguration.h"
#include <qmailcodec.h>
#include <QStringList>


/*!
    \class QMailServiceConfiguration
    \ingroup libmessageserver

    \preliminary
    \brief The QMailServiceConfiguration class provides a simple framework for creating
    wrappers classes that simplify service configuration management.

    QMailServiceConfiguration provides a simple interface for manipulating the configuration
    parameters of a single service within an account configuration.  For each specific
    service implemented, a configuration class derived from QMailServiceConfiguration 
    should be implemented to make the configuration easily accessible.

    \sa QMailAccountConfiguration::ServiceConfiguration
*/

/*!
    \enum QMailServiceConfiguration::ServiceType
    
    This enum type is used to describe the type of a service

    \value Unknown          The type of the service is unknown.
    \value Source           The service is a message source.
    \value Sink             The service is a message sink.
    \value SourceAndSink    The service is both a message source and a message sink.
    \value Storage          The service is a content manager.

    \sa QMailMessageSource, QMailMessageSink
*/

/*!
    Creates a configuration object accessing the parameters of the service \a service
    within the account configuration object \a config.
*/
QMailServiceConfiguration::QMailServiceConfiguration(QMailAccountConfiguration *config, const QString &service)
    : _config(config->services().contains(service) ? &config->serviceConfiguration(service) : 0)
{
}

/*!
    Creates a configuration object accessing the parameters of the service \a service
    within the account configuration object \a config.
*/
QMailServiceConfiguration::QMailServiceConfiguration(const QMailAccountConfiguration &config, const QString &service)
    : _config(const_cast<QMailAccountConfiguration::ServiceConfiguration*>(config.services().contains(service) ? &config.serviceConfiguration(service) : 0))
{
}

/*!
    Creates a configuration object accessing the service configuration parameters described by \a svcCfg.
*/
QMailServiceConfiguration::QMailServiceConfiguration(const QMailAccountConfiguration::ServiceConfiguration &svcCfg)
    : _config(const_cast<QMailAccountConfiguration::ServiceConfiguration*>(&svcCfg))
{
}

/*! \internal */
QMailServiceConfiguration::~QMailServiceConfiguration()
{
}

/*!
    Returns the service that this configuration pertains to.
*/
QString QMailServiceConfiguration::service() const
{
    return _config->service();
}

/*!
    Returns the identifier of the account that this configuration pertains to.
*/
QMailAccountId QMailServiceConfiguration::id() const
{
    return _config->id();
}

/*!
    Returns the version of the service configuration.
*/
int QMailServiceConfiguration::version() const
{
    return value("version", "0").toInt();
}

/*!
    Sets the version of the service configuration to \a version.
*/
void QMailServiceConfiguration::setVersion(int version)
{
    setValue("version", QString::number(version));
}

/*!
    Returns the type of the service.
*/
QMailServiceConfiguration::ServiceType QMailServiceConfiguration::type() const
{
    QString svcType(value("servicetype"));

    if (svcType == "source") {
        return Source;
    } else if (svcType == "sink") {
        return Sink;
    } else if (svcType == "source-sink") {
        return SourceAndSink;
    } else if (svcType == "storage") {
        return Storage;
    }
        
    return Unknown;
}

/*!
    Sets the type of the service to \a type.
*/
void QMailServiceConfiguration::setType(ServiceType type)
{
    setValue("servicetype", (type == Source ?  "source" : (type == Sink ? "sink" : (type == SourceAndSink ? "source-sink" : (type == Storage ? "storage" : "unknown")))));
}

/*!
    Returns true if the service is present in the account configuration.
*/
bool QMailServiceConfiguration::isValid() const
{
    return (_config != 0);
}

/*!
    Returns true if no configuration parameters are recorded for the service.
*/
bool QMailServiceConfiguration::isEmpty() const
{
    if (!_config)
        return true;

    return (_config->values().isEmpty());
}

/*!
    Returns the string \a value encoded into base-64 encoded form.
*/
QString QMailServiceConfiguration::encodeValue(const QString &value)
{
    // TODO: Shouldn't this be UTF-8?
    QMailBase64Codec codec(QMailBase64Codec::Text);
    QByteArray encoded(codec.encode(value, "ISO-8859-1"));
    return QString::fromLatin1(encoded.constData(), encoded.length());
}

/*!
    Returns the string \a value decoded from base-64 encoded form.
*/
QString QMailServiceConfiguration::decodeValue(const QString &value)
{
    if (value.isEmpty())
        return QString();

    QByteArray encoded(value.toLatin1());
    QMailBase64Codec codec(QMailBase64Codec::Text);
    return codec.decode(encoded, "ISO-8859-1");
}

/*!
    Returns the value of the configuration parameter \a name, if present.
    Otherwise returns \a defaultValue.
*/
QString QMailServiceConfiguration::value(const QString &name, const QString &defaultValue) const
{
    if (!_config)
        return defaultValue;

    QString result = _config->value(name);
    if (result.isNull() && !defaultValue.isNull())
        return defaultValue;

    return result;
}

/*! 
    Sets the configuration parameter \a name to have the value \a value.
*/
void QMailServiceConfiguration::setValue(const QString &name, const QString &value)
{
    if (!_config) {
        qWarning() << "Attempted to modify uninitialized configuration! (" << name << ":" << value << ")";
    } else {
        _config->setValue(name, value);
    }
}


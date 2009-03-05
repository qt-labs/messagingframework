/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#include "smtpconfiguration.h"


SmtpConfiguration::SmtpConfiguration(const QMailAccountConfiguration &config)
    : QMailServiceConfiguration(config, "smtp")
{
}

SmtpConfiguration::SmtpConfiguration(const QMailAccountConfiguration::ServiceConfiguration &svcCfg)
    : QMailServiceConfiguration(svcCfg)
{
}

QString SmtpConfiguration::userName() const
{
    return value("username");
}

QString SmtpConfiguration::emailAddress() const
{
    return value("address");
}

QString SmtpConfiguration::smtpServer() const
{
    return value("server");
}

int SmtpConfiguration::smtpPort() const
{
    return value("port", "25").toInt();
}

#ifndef QT_NO_OPENSSL

QString SmtpConfiguration::smtpUsername() const
{
    return value("smtpusername");
}

QString SmtpConfiguration::smtpPassword() const
{
    return decodeValue(value("smtppassword"));
}

#endif

int SmtpConfiguration::smtpAuthentication() const
{
    return value("authentication", "0").toInt();
}

int SmtpConfiguration::smtpEncryption() const
{
    return value("encryption", "0").toInt();
}


SmtpConfigurationEditor::SmtpConfigurationEditor(QMailAccountConfiguration *config)
    : SmtpConfiguration(*config)
{
}

void SmtpConfigurationEditor::setUserName(const QString& str)
{
    setValue("username",str);
}

void SmtpConfigurationEditor::setEmailAddress(const QString &str)
{
    setValue("address", str);
}

void SmtpConfigurationEditor::setSmtpServer(const QString &str)
{
    setValue("server", str);
}

void SmtpConfigurationEditor::setSmtpPort(int i)
{
    setValue("port", QString::number(i));
}

#ifndef QT_NO_OPENSSL

void SmtpConfigurationEditor::setSmtpUsername(const QString& str)
{
    setValue("smtpusername", str);
}

void SmtpConfigurationEditor::setSmtpPassword(const QString& str)
{
    setValue("smtppassword", encodeValue(str));
}

#endif

#ifndef QT_NO_OPENSSL

void SmtpConfigurationEditor::setSmtpAuthentication(int t)
{
    setValue("authentication", QString::number(t));
}

#endif

#ifndef QT_NO_OPENSSL

void SmtpConfigurationEditor::setSmtpEncryption(int t)
{
    setValue("encryption", QString::number(t));
}

#endif


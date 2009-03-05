/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#include "popconfiguration.h"


PopConfiguration::PopConfiguration(const QMailAccountConfiguration &config)
    : QMailServiceConfiguration(config, "pop3")
{
}

PopConfiguration::PopConfiguration(const QMailAccountConfiguration::ServiceConfiguration &svcCfg)
    : QMailServiceConfiguration(svcCfg)
{
}

QString PopConfiguration::mailUserName() const
{
    return value("username");
}

QString PopConfiguration::mailPassword() const
{
    return decodeValue(value("password"));
}

QString PopConfiguration::mailServer() const
{
    return value("server");
}

int PopConfiguration::mailPort() const
{
    return value("port", "143").toInt();
}

int PopConfiguration::mailEncryption() const
{
    return value("encryption", "0").toInt();
}

bool PopConfiguration::canDeleteMail() const
{
    return (value("canDelete", "0").toInt() != 0);
}

bool PopConfiguration::isAutoDownload() const
{
    return (value("autoDownload", "0").toInt() != 0);
}

int PopConfiguration::maxMailSize() const
{
    return value("maxSize", "102400").toInt();
}

int PopConfiguration::checkInterval() const
{
    return value("checkInterval", "-1").toInt();
}

bool PopConfiguration::intervalCheckRoamingEnabled() const
{
    return (value("intervalCheckRoamingEnabled", "0").toInt() != 0);
}


PopConfigurationEditor::PopConfigurationEditor(QMailAccountConfiguration *config)
    : PopConfiguration(*config)
{
}

void PopConfigurationEditor::setMailUserName(const QString &str)
{
    setValue("username", str);
}

void PopConfigurationEditor::setMailPassword(const QString &str)
{
    setValue("password", encodeValue(str));
}

void PopConfigurationEditor::setMailServer(const QString &str)
{
    setValue("server", str);
}

void PopConfigurationEditor::setMailPort(int i)
{
    setValue("port", QString::number(i));
}

#ifndef QT_NO_OPENSSL

void PopConfigurationEditor::setMailEncryption(int t)
{
    setValue("encryption", QString::number(t));
}

#endif

void PopConfigurationEditor::setDeleteMail(bool b)
{
    setValue("canDelete", QString::number(b ? 1 : 0));
}

void PopConfigurationEditor::setAutoDownload(bool b)
{
    setValue("autoDownload", QString::number(b ? 1 : 0));
}

void PopConfigurationEditor::setMaxMailSize(int i)
{
    setValue("maxSize", QString::number(i));
}

void PopConfigurationEditor::setCheckInterval(int i)
{
    setValue("checkInterval", QString::number(i));
}

void PopConfigurationEditor::setIntervalCheckRoamingEnabled(bool b)
{
    setValue("intervalCheckRoamingEnabled", QString::number(b ? 1 : 0));
}


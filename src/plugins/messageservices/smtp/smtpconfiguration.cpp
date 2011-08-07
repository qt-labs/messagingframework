/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
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


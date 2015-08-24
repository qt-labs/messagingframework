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
    return (value("canDelete", "1").toInt() != 0);
}

bool PopConfiguration::deleteRetrievedMailsFromServer() const
{
    return (value("deleteRetrievedMailsFromServer", "0").toInt() != 0);
}

bool PopConfiguration::isAutoDownload() const
{
    return (value("autoDownload", "0").toInt() != 0);
}

int PopConfiguration::maxMailSize() const
{
    return value("maxSize", "100").toInt();
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

#ifndef QT_NO_SSL

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


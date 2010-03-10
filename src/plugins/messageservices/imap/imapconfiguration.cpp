/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the either Technology Preview License Agreement or the
** Beta Release License Agreement.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/

#include "imapconfiguration.h"


ImapConfiguration::ImapConfiguration(const QMailAccountConfiguration &config)
    : QMailServiceConfiguration(config, "imap4")
{
}

ImapConfiguration::ImapConfiguration(const QMailAccountConfiguration::ServiceConfiguration &svcCfg)
    : QMailServiceConfiguration(svcCfg)
{
}

QString ImapConfiguration::mailUserName() const
{
    return value("username");
}

QString ImapConfiguration::mailPassword() const
{
    return decodeValue(value("password"));
}

QString ImapConfiguration::mailServer() const
{
    return value("server");
}

int ImapConfiguration::mailPort() const
{
    return value("port", "110").toInt();
}

int ImapConfiguration::mailEncryption() const
{
    return value("encryption", "0").toInt();
}

bool ImapConfiguration::canDeleteMail() const
{
    return (value("canDelete", "1").toInt() != 0);
}

bool ImapConfiguration::isAutoDownload() const
{
    return (value("autoDownload", "0").toInt() != 0);
}

int ImapConfiguration::maxMailSize() const
{
    return value("maxSize", "20").toInt();
}

QString ImapConfiguration::preferredTextSubtype() const
{
    return value("textSubtype");
}

bool ImapConfiguration::pushEnabled() const
{
    return (value("pushEnabled", "0").toInt() != 0);
}

QString ImapConfiguration::baseFolder() const
{
    return value("baseFolder");
}

QString ImapConfiguration::draftsFolder() const
{
    return value("draftsFolder");
}

QString ImapConfiguration::sentFolder() const
{
    return value("sentFolder");
}

QString ImapConfiguration::trashFolder() const
{
    return value("trashFolder");
}

QString ImapConfiguration::junkFolder() const
{
    return value("junkFolder");
}

QStringList ImapConfiguration::pushFolders() const
{
    return value("pushFolders").split(QChar('\x0A'), QString::SkipEmptyParts);
}

int ImapConfiguration::checkInterval() const
{
    return value("checkInterval", "-1").toInt();
}

bool ImapConfiguration::intervalCheckRoamingEnabled() const
{
    return (value("intervalCheckRoamingEnabled", "0").toInt() != 0);
}


ImapConfigurationEditor::ImapConfigurationEditor(QMailAccountConfiguration *config)
    : ImapConfiguration(*config)
{
}

void ImapConfigurationEditor::setMailUserName(const QString &str)
{
    setValue("username", str);
}

void ImapConfigurationEditor::setMailPassword(const QString &str)
{
    setValue("password", encodeValue(str));
}

void ImapConfigurationEditor::setMailServer(const QString &str)
{
    setValue("server", str);
}

void ImapConfigurationEditor::setMailPort(int i)
{
    setValue("port", QString::number(i));
}

#ifndef QT_NO_OPENSSL

void ImapConfigurationEditor::setMailEncryption(int t)
{
    setValue("encryption", QString::number(t));
}

#endif

void ImapConfigurationEditor::setDeleteMail(bool b)
{
    setValue("canDelete", QString::number(b ? 1 : 0));
}

void ImapConfigurationEditor::setAutoDownload(bool b)
{
    setValue("autoDownload", QString::number(b ? 1 : 0));
}

void ImapConfigurationEditor::setMaxMailSize(int i)
{
    setValue("maxSize", QString::number(i));
}

void ImapConfigurationEditor::setPreferredTextSubtype(const QString &str)
{
    setValue("textSubtype", str);
}

void ImapConfigurationEditor::setPushEnabled(bool b)
{
    setValue("pushEnabled", QString::number(b ? 1 : 0));
}

void ImapConfigurationEditor::setBaseFolder(const QString &s)
{
    setValue("baseFolder", s);
}

void ImapConfigurationEditor::setDraftsFolder(const QString &s)
{
    setValue("draftsFolder", s);
}

void ImapConfigurationEditor::setSentFolder(const QString &s)
{
    setValue("sentFolder", s);
}

void ImapConfigurationEditor::setTrashFolder(const QString &s)
{
    setValue("trashFolder", s);
}

void ImapConfigurationEditor::setJunkFolder(const QString &s)
{
    setValue("junkFolder", s);
}

void ImapConfigurationEditor::setPushFolders(const QStringList &s)
{
    setValue("pushFolders", QString("") + s.join(QChar('\x0A'))); // can't setValue to null string
}

void ImapConfigurationEditor::setCheckInterval(int i)
{
    setValue("checkInterval", QString::number(i));
}

void ImapConfigurationEditor::setIntervalCheckRoamingEnabled(bool b)
{
    setValue("intervalCheckRoamingEnabled", QString::number(b ? 1 : 0));
}


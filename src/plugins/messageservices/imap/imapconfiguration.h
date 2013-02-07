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

#ifndef IMAPCONFIGURATION_H
#define IMAPCONFIGURATION_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt Extended API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qmailserviceconfiguration.h>

class PLUGIN_EXPORT ImapConfiguration : public QMailServiceConfiguration
{
public:
    explicit ImapConfiguration(const QMailAccountConfiguration &config);
    explicit ImapConfiguration(const QMailAccountConfiguration::ServiceConfiguration &svcCfg);

    QString mailUserName() const;
    QString mailPassword() const;
    QString mailServer() const;
    int mailPort() const;
    int mailEncryption() const;
    int mailAuthentication() const;

    bool canDeleteMail() const;
    bool downloadAttachments() const;
    bool isAutoDownload() const;
    int maxMailSize() const;

    QString preferredTextSubtype() const;

    bool pushEnabled() const;

    QString baseFolder() const;
    QStringList pushFolders() const;

    int checkInterval() const;
    bool intervalCheckRoamingEnabled() const;

    QStringList capabilities() const;
    void setCapabilities(const QStringList &s);
    bool pushCapable() const;
    void setPushCapable(bool b);

    int timeTillLogout() const;
    void setTimeTillLogout(int milliseconds);

    int searchLimit() const;
    void setSearchLimit(int limit);
};

class PLUGIN_EXPORT ImapConfigurationEditor : public ImapConfiguration
{
public:
    explicit ImapConfigurationEditor(QMailAccountConfiguration *config);

    void setMailUserName(const QString &str);
    void setMailPassword(const QString &str);
    void setMailServer(const QString &str);
    void setMailPort(int i);
#ifndef QT_NO_OPENSSL
    void setMailEncryption(int t);
    void setMailAuthentication(int t);
#endif

    void setDeleteMail(bool b);
    void setAutoDownload(bool autodl);
    void setMaxMailSize(int i);

    void setPreferredTextSubtype(const QString &str);

    void setPushEnabled(bool enabled);

    void setBaseFolder(const QString &s);
    void setPushFolders(const QStringList &s);

    void setCheckInterval(int i);
    void setIntervalCheckRoamingEnabled(bool enabled);
};

#endif

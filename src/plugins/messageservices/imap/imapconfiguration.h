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

class ImapConfiguration : public QMailServiceConfiguration
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

class ImapConfigurationEditor : public ImapConfiguration
{
public:
    explicit ImapConfigurationEditor(QMailAccountConfiguration *config);

    void setMailUserName(const QString &str);
    void setMailPassword(const QString &str);
    void setMailServer(const QString &str);
    void setMailPort(int i);
#ifndef QT_NO_SSL
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

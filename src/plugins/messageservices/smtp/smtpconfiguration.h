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

#ifndef SMTPCONFIGURATION_H
#define SMTPCONFIGURATION_H

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
#include <qmailnamespace.h>

class SmtpConfiguration : public QMailServiceConfiguration
{
public:
    enum AuthType {
        Auth_NONE = QMail::NoMechanism,
#ifndef QT_NO_SSL
        Auth_LOGIN = QMail::LoginMechanism,
        Auth_PLAIN = QMail::PlainMechanism,
#endif
        Auth_CRAMMD5 = QMail::CramMd5Mechanism,
        Auth_INCOMING = 4
    };

    explicit SmtpConfiguration(const QMailAccountConfiguration &config);
    explicit SmtpConfiguration(const QMailAccountConfiguration::ServiceConfiguration &svcCfg);

    QString userName() const;
    QString emailAddress() const;
    QString smtpServer() const;
    int smtpPort() const;
#ifndef QT_NO_SSL
    QString smtpUsername() const;
    QString smtpPassword() const;
#endif
    int smtpAuthentication() const;
    int smtpEncryption() const;
};

class SmtpConfigurationEditor : public SmtpConfiguration
{
public:
    explicit SmtpConfigurationEditor(QMailAccountConfiguration *config);

    void setUserName(const QString &str);
    void setEmailAddress(const QString &str);
    void setSmtpServer(const QString &str);
    void setSmtpPort(int i);
#ifndef QT_NO_SSL
    void setSmtpUsername(const QString& username);
    void setSmtpPassword(const QString& password);
    void setSmtpAuthentication(int t);
    void setSmtpEncryption(int t);
#endif
};

#endif

/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
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

class QTOPIAMAIL_EXPORT SmtpConfiguration : public QMailServiceConfiguration
{
public:
    enum AuthType {
        Auth_NONE = 0,
#ifndef QT_NO_OPENSSL
        Auth_LOGIN = 1,
        Auth_PLAIN = 2,
#endif
        Auth_INCOMING = 3
    };

    explicit SmtpConfiguration(const QMailAccountConfiguration &config);
    explicit SmtpConfiguration(const QMailAccountConfiguration::ServiceConfiguration &svcCfg);

    QString userName() const;
    QString emailAddress() const;
    QString smtpServer() const;
    int smtpPort() const;
#ifndef QT_NO_OPENSSL
    QString smtpUsername() const;
    QString smtpPassword() const;
#endif
    int smtpAuthentication() const;
    int smtpEncryption() const;
};

class QTOPIAMAIL_EXPORT SmtpConfigurationEditor : public SmtpConfiguration
{
public:
    explicit SmtpConfigurationEditor(QMailAccountConfiguration *config);

    void setUserName(const QString &str);
    void setEmailAddress(const QString &str);
    void setSmtpServer(const QString &str);
    void setSmtpPort(int i);
#ifndef QT_NO_OPENSSL
    void setSmtpUsername(const QString& username);
    void setSmtpPassword(const QString& password);
#endif
#ifndef QT_NO_OPENSSL
    void setSmtpAuthentication(int t);
#endif
#ifndef QT_NO_OPENSSL
    void setSmtpEncryption(int t);
#endif
};

#endif

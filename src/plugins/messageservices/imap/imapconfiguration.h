/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
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

class QTOPIAMAIL_EXPORT ImapConfiguration : public QMailServiceConfiguration
{
public:
    explicit ImapConfiguration(const QMailAccountConfiguration &config);
    explicit ImapConfiguration(const QMailAccountConfiguration::ServiceConfiguration &svcCfg);

    QString mailUserName() const;
    QString mailPassword() const;
    QString mailServer() const;
    int mailPort() const;
    int mailEncryption() const;

    bool canDeleteMail() const;
    bool isAutoDownload() const;
    int maxMailSize() const;

    bool pushEnabled() const;
    QString baseFolder() const;

    int checkInterval() const;
    bool intervalCheckRoamingEnabled() const;
};

class QTOPIAMAIL_EXPORT ImapConfigurationEditor : public ImapConfiguration
{
public:
    explicit ImapConfigurationEditor(QMailAccountConfiguration *config);

    void setMailUserName(const QString &str);
    void setMailPassword(const QString &str);
    void setMailServer(const QString &str);
    void setMailPort(int i);
#ifndef QT_NO_OPENSSL
    void setMailEncryption(int t);
#endif

    void setDeleteMail(bool b);
    void setAutoDownload(bool autodl);
    void setMaxMailSize(int i);

    void setPushEnabled(bool enabled);
    void setBaseFolder(const QString &s);

    void setCheckInterval(int i);
    void setIntervalCheckRoamingEnabled(bool enabled);
};

#endif

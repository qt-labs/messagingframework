/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef POPCONFIGURATION_H
#define POPCONFIGURATION_H

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

class PLUGIN_EXPORT PopConfiguration : public QMailServiceConfiguration
{
public:
    explicit PopConfiguration(const QMailAccountConfiguration &config);
    explicit PopConfiguration(const QMailAccountConfiguration::ServiceConfiguration &svcCfg);

    QString mailUserName() const;
    QString mailPassword() const;
    QString mailServer() const;
    int mailPort() const;
    int mailEncryption() const;

    bool canDeleteMail() const;

    bool isAutoDownload() const;
    int maxMailSize() const;

    int checkInterval() const;
    bool intervalCheckRoamingEnabled() const;
};

class PLUGIN_EXPORT PopConfigurationEditor : public PopConfiguration
{
public:
    explicit PopConfigurationEditor(QMailAccountConfiguration *config);

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

    void setCheckInterval(int i);
    void setIntervalCheckRoamingEnabled(bool enabled);
};

#endif

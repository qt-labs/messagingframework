/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef QMAILAUTHENTICATOR_H
#define QMAILAUTHENTICATOR_H

#include <qmailaccountconfiguration.h>

#include <QByteArray>
#include <QStringList>

class MESSAGESERVER_EXPORT QMailAuthenticator
{
public:
    static bool useEncryption(const QMailAccountConfiguration::ServiceConfiguration &svcCfg, const QStringList &capabilities);
    static QByteArray getAuthentication(const QMailAccountConfiguration::ServiceConfiguration &svcCfg, const QStringList &capabilities);
    static QByteArray getResponse(const QMailAccountConfiguration::ServiceConfiguration &svcCfg, const QByteArray &challenge);
};

#endif


/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#include "popauthenticator.h"

#include "popconfiguration.h"

#include <qmailauthenticator.h>
#include <qmailtransport.h>


bool PopAuthenticator::useEncryption(const QMailAccountConfiguration::ServiceConfiguration &svcCfg, const QStringList &capabilities)
{
#ifdef QT_NO_OPENSSL
    return false;
#else
    PopConfiguration popCfg(svcCfg);
    bool useTLS(popCfg.mailEncryption() == QMailTransport::Encrypt_TLS);

    if (!capabilities.contains("STLS")) {
        if (useTLS) {
            qWarning() << "Server does not support TLS - continuing unencrypted";
        }
    } else {
        if (useTLS) {
            return true;
        } else {
            if (!capabilities.contains("USER")) {
                qWarning() << "Server does not support unencrypted USER - using encryption";
                return true;
            }
        }
    }

    return QMailAuthenticator::useEncryption(svcCfg, capabilities);
#endif
}

QList<QByteArray> PopAuthenticator::getAuthentication(const QMailAccountConfiguration::ServiceConfiguration &svcCfg, const QStringList &capabilities)
{
    QList<QByteArray> result;

    QByteArray auth(QMailAuthenticator::getAuthentication(svcCfg, capabilities));
    if (!auth.isEmpty()) {
        result.append(QByteArray("AUTH ") + auth);
    } else {
        // If not handled by the authenticator, fall back to user/pass
        PopConfiguration popCfg(svcCfg);

        result.append(QByteArray("USER ") + popCfg.mailUserName().toAscii());
        result.append(QByteArray("PASS ") + popCfg.mailPassword().toAscii());
    }

    return result;
}

QByteArray PopAuthenticator::getResponse(const QMailAccountConfiguration::ServiceConfiguration &svcCfg, const QByteArray &challenge)
{
    return QMailAuthenticator::getResponse(svcCfg, challenge);
}


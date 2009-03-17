/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#include "imapauthenticator.h"

#include "imapprotocol.h"
#include "imapconfiguration.h"

#include <qmailauthenticator.h>
#include <qmailtransport.h>


bool ImapAuthenticator::useEncryption(const QMailAccountConfiguration::ServiceConfiguration &svcCfg, const QStringList &capabilities)
{
#ifdef QT_NO_OPENSSL
    return false;
#else
    ImapConfiguration imapCfg(svcCfg);
    bool useTLS(imapCfg.mailEncryption() == QMailTransport::Encrypt_TLS);

    if (!capabilities.contains("STARTTLS")) {
        if (useTLS) {
            qWarning() << "Server does not support TLS - continuing unencrypted";
        }
    } else {
        if (useTLS) {
            return true;
        }
    }

    return QMailAuthenticator::useEncryption(svcCfg, capabilities);
#endif
}

QByteArray ImapAuthenticator::getAuthentication(const QMailAccountConfiguration::ServiceConfiguration &svcCfg, const QStringList &capabilities)
{
    QByteArray result(QMailAuthenticator::getAuthentication(svcCfg, capabilities));
    if (!result.isEmpty())
        return QByteArray("AUTHENTICATE ") + result;

    // If not handled by the authenticator, fall back to login
    ImapConfiguration imapCfg(svcCfg);
    return QByteArray("LOGIN") + " " + ImapProtocol::quoteString(imapCfg.mailUserName().toAscii()) 
                               + " " + ImapProtocol::quoteString(imapCfg.mailPassword().toAscii());
}

QByteArray ImapAuthenticator::getResponse(const QMailAccountConfiguration::ServiceConfiguration &svcCfg, const QByteArray &challenge)
{
    return QMailAuthenticator::getResponse(svcCfg, challenge);
}


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

#include "qmailauthenticator.h"
#include "qmailnamespace.h"
#include "qmaillog.h"
#include <qmailserviceconfiguration.h>
#include <qcryptographichash.h>
#include <qbytearray.h>

// cram-md5 helper code

static QByteArray xorArray(const QByteArray input, char c)
{
    QByteArray result;
    for (int i = 0; i < input.size(); ++i) {
        result += (input[i] ^ c);
    }
    return result;
}

static QByteArray cramMd5Response(const QByteArray &nonce, const QByteArray &name, const QByteArray &password)
{
    char opad(0x5c);
    char ipad(0x36);
    QByteArray result = name + " ";
    QCryptographicHash hash(QCryptographicHash::Md5);
    QCryptographicHash hash1(QCryptographicHash::Md5);
    QCryptographicHash hash2(QCryptographicHash::Md5);
    QByteArray passwordPad(password);
    if (passwordPad.size() > 64) {
        hash.addData(passwordPad);
        passwordPad = hash.result();
    }
    while (passwordPad.size() < 64)
        passwordPad.append(char(0));
    hash1.addData(xorArray(passwordPad, ipad));
    hash1.addData(nonce);
    hash2.addData(xorArray(passwordPad, opad));
    hash2.addData(hash1.result());
    result.append(hash2.result().toHex());
    return result;
}

// end cram-md5 helper code


/*!
    \class QMailAuthenticator
    \ingroup libmessageserver

    \preliminary
    \brief The QMailAuthenticator class provides a customization point
    where authentication services can be supplied to the messageserver.

    QMailAuthenticator provides a simple interface for handling authentication
    exchanges between the messageserver and external services.  Protocol plugins
    operating within the messageserver should use the QMailAuthenticator interface
    to request the authentication type they should use, and to resolve any
    challenges required by the external server during the authentication
    process.
*/

/*!
    Returns true if the protocol should immediately switch to using TLS encryption;
    otherwise returns false.
    The use of encryption may be preferred depending on the service whose configuration 
    is described by \a svcCfg, and the service's reported \a capabilities.
*/
bool QMailAuthenticator::useEncryption(const QMailServiceConfiguration &svcCfg, const QStringList &capabilities)
{
    Q_UNUSED(svcCfg)
    Q_UNUSED(capabilities)

    return false;
}

QMail::SaslMechanism QMailAuthenticator::authFromCapabilities(const QStringList &capabilities)
{
    if (capabilities.contains(QLatin1String("CRAM-MD5"), Qt::CaseInsensitive)) {
        return QMail::CramMd5Mechanism;
    } else if (capabilities.contains(QLatin1String("XOAUTH2"), Qt::CaseInsensitive)) {
        // https://developers.google.com/gmail/imap/xoauth2-protocol
        return QMail::XOAuth2Mechanism;
    } else if (capabilities.contains(QLatin1String("PLAIN"), Qt::CaseInsensitive)) {
        // According to RFC3501, IMAP4 servers MUST implement plain auth
        return QMail::PlainMechanism;
    } else if (!capabilities.contains(QLatin1String("PLAIN"), Qt::CaseInsensitive)
               && !capabilities.contains(QLatin1String("LOGINDISABLED"), Qt::CaseInsensitive)) {
        // According to RFC3501, LOGIN should be used as last resort(for retro-compatibility)
        // We should check that plain is not advertised(this can be omitted even if server supports it),
        // and that LOGINDISABLED capability is not advertised.
        return QMail::LoginMechanism;
    } else {
        return QMail::NoMechanism;
    }
}

/*!
    Returns the authentication string that should be used to initiate an authentication
    attempt for the service whose configuration is described by \a svcCfg.  The
    authentication method is the one chosen in the service configuration.
*/
QByteArray QMailAuthenticator::getAuthentication(const QMailServiceConfiguration &svcCfg,
                                                 const QMailCredentialsInterface &credentials)
{
    Q_UNUSED(credentials);

    const QString auth = svcCfg.value(QLatin1String("authentication"));
    if (auth == QString::number(QMail::CramMd5Mechanism)) {
        return "CRAM-MD5";
    }

    // Unknown service type and/or authentication type
    return QByteArray();
}

/*!
    Returns the response string that should be reported as the response to a
    service's \a challenge, for the the service desribed by \a svcCfg.

    If the protocol invoking the challenge-response resolution requires 
    encoding for the challenge-response tokens (such as Base64), the challenge 
    should be decoded before invocation, and the result should be encoded for
    transmission.
*/
QByteArray QMailAuthenticator::getResponse(const QMailServiceConfiguration &svcCfg,
                                           const QByteArray &challenge,
                                           const QMailCredentialsInterface &credentials)
{
    const QString auth = svcCfg.value(QLatin1String("authentication"));
    if (auth == QString::number(QMail::CramMd5Mechanism)) {
        return cramMd5Response(challenge, credentials.username().toUtf8(),
                               credentials.password().toUtf8());
    }

    // Unknown service type and/or authentication type
    return QByteArray();
}

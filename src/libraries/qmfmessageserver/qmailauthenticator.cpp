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

#include "qmailauthenticator.h"
#include "qmailnamespace.h"
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
bool QMailAuthenticator::useEncryption(const QMailAccountConfiguration::ServiceConfiguration &svcCfg, const QStringList &capabilities)
{
    Q_UNUSED(svcCfg)
    Q_UNUSED(capabilities)

    return false;
}

/*!
    Returns the authentication string that should be used to initiate an authentication
    attempt for the service whose configuration is described by \a svcCfg.  The preferred
    authentication method may depend upon the service's reported \a capabilities.
*/
QByteArray QMailAuthenticator::getAuthentication(const QMailAccountConfiguration::ServiceConfiguration &svcCfg, const QStringList &capabilities)
{
    Q_UNUSED(capabilities)

    QMailServiceConfiguration configuration(svcCfg);
    if (configuration.value("authentication") == QString::number(QMail::CramMd5Mechanism))
        return "CRAM-MD5";

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
QByteArray QMailAuthenticator::getResponse(const QMailAccountConfiguration::ServiceConfiguration &svcCfg, const QByteArray &challenge)
{
    QMailServiceConfiguration configuration(svcCfg);
    if (!configuration.value("smtpusername").isEmpty() 
        && (configuration.value("authentication") == QString::number(QMail::CramMd5Mechanism))) {
        // SMTP server CRAM-MD5 authentication
        return cramMd5Response(challenge, configuration.value("smtpusername").toUtf8(), QByteArray::fromBase64(configuration.value("smtppassword").toUtf8()));
    } else if (configuration.value("authentication") == QString::number(QMail::CramMd5Mechanism)) {
        // IMAP/POP server CRAM-MD5 authentication
        return cramMd5Response(challenge, configuration.value("username").toUtf8(), QByteArray::fromBase64(configuration.value("password").toUtf8()));
    }

    // Unknown service type and/or authentication type
    return QByteArray();
}


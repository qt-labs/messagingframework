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

#include "imapauthenticator.h"

#include "imapprotocol.h"
#include "imapconfiguration.h"
#include "imaplog.h"

#include <qmailauthenticator.h>
#include <qmailtransport.h>
#include <qmailnamespace.h>

bool ImapAuthenticator::useEncryption(const ImapConfiguration &svcCfg,
                                      const QStringList &capabilities)
{
    bool useTLS(svcCfg.mailEncryption() == QMailTransport::Encrypt_TLS);

    if (!capabilities.contains("STARTTLS")) {
        if (useTLS) {
            qCWarning(lcIMAP) << "Server does not support TLS - continuing unencrypted";
        }
    } else {
        if (useTLS) {
            return true;
        }
    }

    return QMailAuthenticator::useEncryption(svcCfg, capabilities);
}

QList<QByteArray> ImapAuthenticator::getAuthentication(const ImapConfiguration &svcCfg,
                                                       const QMailCredentialsInterface &credentials)
{
    QList<QByteArray> result;

    QByteArray auth(QMailAuthenticator::getAuthentication(svcCfg, credentials));
    if (!auth.isEmpty()) {
        result.append(QByteArray("AUTHENTICATE ") + auth);
    } else if (svcCfg.mailAuthentication() == QMail::XOAuth2Mechanism) {
        result.append(QByteArray("AUTHENTICATE XOAUTH2"));
        result.append(QString::fromLatin1("user=%1\001auth=Bearer %2\001\001").arg(credentials.username()).arg(credentials.accessToken()).toUtf8().toBase64());
    } else if (svcCfg.mailAuthentication() == QMail::PlainMechanism) {
        QByteArray username(credentials.username().toUtf8());
        QByteArray password(credentials.password().toUtf8());
        result.append(QByteArray("AUTHENTICATE PLAIN"));
        result << QByteArray(username + '\0' + username + '\0' + password).toBase64();
    } else {
        result.append(QByteArray("LOGIN")
                      + ' ' + ImapProtocol::quoteString(credentials.username()).toLatin1()
                      + ' ' + ImapProtocol::quoteString(credentials.password()).toLatin1());
    }
    return result;
}

QByteArray ImapAuthenticator::getResponse(const ImapConfiguration &svcCfg,
                                          const QByteArray &challenge,
                                          const QMailCredentialsInterface &credentials)
{
    return QMailAuthenticator::getResponse(svcCfg, challenge, credentials);
}

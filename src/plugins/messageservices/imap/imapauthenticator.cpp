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

#include <qmailauthenticator.h>
#include <qmailtransport.h>
#include <qmailnamespace.h>

namespace {

QMap<QMailAccountId, QList<QByteArray> > gResponses;

}

bool ImapAuthenticator::useEncryption(const QMailAccountConfiguration::ServiceConfiguration &svcCfg, const QStringList &capabilities)
{
#ifdef QT_NO_SSL
    Q_UNUSED(svcCfg)
    Q_UNUSED(capabilities)
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
    if (imapCfg.mailAuthentication() == QMail::PlainMechanism) {
        QByteArray username(imapCfg.mailUserName().toLatin1());
        QByteArray password(imapCfg.mailPassword().toLatin1());
        return QByteArray("AUTHENTICATE PLAIN ") + QByteArray(username + '\0' + username + '\0' + password).toBase64();
    }

    return QByteArray("LOGIN") + ' ' + ImapProtocol::quoteString(imapCfg.mailUserName().toLatin1())
                               + ' ' + ImapProtocol::quoteString(imapCfg.mailPassword().toLatin1());
}

QByteArray ImapAuthenticator::getResponse(const QMailAccountConfiguration::ServiceConfiguration &svcCfg, const QByteArray &challenge)
{
    return QMailAuthenticator::getResponse(svcCfg, challenge);
}


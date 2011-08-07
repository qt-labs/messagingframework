/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "popauthenticator.h"

#include "popconfiguration.h"

#include <qmailauthenticator.h>
#include <qmailtransport.h>


bool PopAuthenticator::useEncryption(const QMailAccountConfiguration::ServiceConfiguration &svcCfg, const QStringList &capabilities)
{
#ifdef QT_NO_OPENSSL
    Q_UNUSED(svcCfg)
    Q_UNUSED(capabilities)
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


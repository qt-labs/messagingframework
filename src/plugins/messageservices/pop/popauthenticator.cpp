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

#include "popauthenticator.h"

#include "popconfiguration.h"

#include <qmailauthenticator.h>
#include <qmailtransport.h>


bool PopAuthenticator::useEncryption(const QMailAccountConfiguration::ServiceConfiguration &svcCfg, const QStringList &capabilities)
{
#ifdef QT_NO_SSL
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

        result.append(QByteArray("USER ") + popCfg.mailUserName().toLatin1());
        result.append(QByteArray("PASS ") + popCfg.mailPassword().toLatin1());
    }

    return result;
}

QByteArray PopAuthenticator::getResponse(const QMailAccountConfiguration::ServiceConfiguration &svcCfg, const QByteArray &challenge)
{
    return QMailAuthenticator::getResponse(svcCfg, challenge);
}


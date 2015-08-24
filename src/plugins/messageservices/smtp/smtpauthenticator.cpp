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

#include "smtpauthenticator.h"

#include "smtpconfiguration.h"

#include <qmailauthenticator.h>


namespace {

QMap<QMailAccountId, QList<QByteArray> > gResponses;

}

QByteArray SmtpAuthenticator::getAuthentication(const QMailAccountConfiguration::ServiceConfiguration &svcCfg, const QStringList &capabilities)
{
    QByteArray result(QMailAuthenticator::getAuthentication(svcCfg, capabilities));
    if (!result.isEmpty())
        return result.prepend("AUTH ");

#ifndef QT_NO_SSL
    SmtpConfiguration smtpCfg(svcCfg);
    if (smtpCfg.smtpAuthentication() != SmtpConfiguration::Auth_NONE) {
        QMailAccountId id(smtpCfg.id());
        QByteArray username(smtpCfg.smtpUsername().toUtf8());
        QByteArray password(smtpCfg.smtpPassword().toUtf8());

        if (smtpCfg.smtpAuthentication() == SmtpConfiguration::Auth_LOGIN) {
            result = QByteArray("LOGIN");
            gResponses[id] = (QList<QByteArray>() << username << password);
        } else if (smtpCfg.smtpAuthentication() == SmtpConfiguration::Auth_PLAIN) {
            result = QByteArray("PLAIN ") + QByteArray(username + '\0' + username + '\0' + password).toBase64();
            gResponses[id] = (QList<QByteArray>() << QByteArray(username + '\0' + username + '\0' + password));
        }
    }
#endif

    if (!result.isEmpty()) {
        result.prepend("AUTH ");
    }
    return result;
}

QByteArray SmtpAuthenticator::getResponse(const QMailAccountConfiguration::ServiceConfiguration &svcCfg, const QByteArray &challenge)
{
    QByteArray result;

    QMap<QMailAccountId, QList<QByteArray> >::iterator it = gResponses.find(svcCfg.id());
    if (it != gResponses.end()) {
        QList<QByteArray> &responses = it.value();
        result = responses.takeFirst();

        if (responses.isEmpty())
            gResponses.erase(it);
    } else {
        result = QMailAuthenticator::getResponse(svcCfg, challenge);
    }

    return result;
}


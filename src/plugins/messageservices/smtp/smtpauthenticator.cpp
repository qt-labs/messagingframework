/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
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
        return result;

#ifndef QT_NO_OPENSSL
    SmtpConfiguration smtpCfg(svcCfg);
    if (smtpCfg.smtpAuthentication() != SmtpConfiguration::Auth_NONE) {
        QMailAccountId id(smtpCfg.id());
        QByteArray username(smtpCfg.smtpUsername().toAscii());
        QByteArray password(smtpCfg.smtpPassword().toAscii());

        if (smtpCfg.smtpAuthentication() == SmtpConfiguration::Auth_LOGIN) {
            result = QByteArray("LOGIN");
            gResponses[id] = (QList<QByteArray>() << username << password);
        } else if (smtpCfg.smtpAuthentication() == SmtpConfiguration::Auth_PLAIN) {
            result = QByteArray("PLAIN");
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


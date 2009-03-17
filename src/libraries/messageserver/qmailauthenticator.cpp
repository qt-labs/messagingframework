/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#include "qmailauthenticator.h"


/*!
    \class QMailAuthenticator
    \inpublicgroup QtMessagingModule
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
    return false;

    Q_UNUSED(svcCfg)
    Q_UNUSED(capabilities)
}

/*!
    Returns the authentication string that should be used to initiate an authentication
    attempt for the service whose configuration is described by \a svcCfg.  The preferred
    authentication method may depend upon the service's reported \a capabilities.
*/
QByteArray QMailAuthenticator::getAuthentication(const QMailAccountConfiguration::ServiceConfiguration &svcCfg, const QStringList &capabilities)
{
    return QByteArray();

    Q_UNUSED(svcCfg)
    Q_UNUSED(capabilities)
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
    return QByteArray();

    Q_UNUSED(svcCfg)
    Q_UNUSED(challenge)
}


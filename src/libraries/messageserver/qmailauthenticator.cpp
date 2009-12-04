/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Qt Software Information (qt-info@nokia.com)
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the either Technology Preview License Agreement or the
** Beta Release License Agreement.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qmailauthenticator.h"


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


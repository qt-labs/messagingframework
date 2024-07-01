/****************************************************************************
**
** Copyright (C) 2024 Damien Caliste
** Contact: Damien Caliste <dcaliste@free.fr>
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

#include <qmaillog.h>

#include "ssomanager.h"

#include <oauth2data.h>

SSOManager::SSOManager(QObject *parent)
    : QMailCredentialsInterface(parent)
{
}

SSOManager::~SSOManager()
{
    deinit();
}

bool SSOManager::init(uint credentialsId, const QString &method,
                      const QString &mechanism, const QVariantMap &parameters)
{
    if (m_credentialIds != credentialsId) {
        deinit();
    }

    m_credentialIds = credentialsId;
    m_errorMessage.clear();
    if (m_status != Invalid) {
        m_status = Invalid;
        emit statusChanged();
    }
    // Save the username, SSO may return an empty string sometimes.
    m_username = parameters.value(QStringLiteral("UserName")).toString();

    if (!m_identity) {
        m_identity = SignOn::Identity::existingIdentity(credentialsId, this);
        if (m_identity) {
            connect(m_identity, &SignOn::Identity::methodsAvailable,
                    [this, mechanism, parameters] (const QStringList &methods) {
                        if (methods.isEmpty()) {
                            qMailLog(Messaging) << "No method available for credentialsId:" << m_identity->id();
                        } else {
                            if (methods.size() > 1) {
                                qMailLog(Messaging) << "Several methods available for credentialsId:" << m_identity->id();
                            }
                            start(methods.first(), mechanism, parameters);
                        }
                    });
        }
    }
    if (!m_identity) {
        qMailLog(Messaging) << "No such credential id:" << credentialsId;
        m_errorMessage = QLatin1String("invalid SSO credential id.");
        return false;
    }
    if (method.isEmpty()) {
        m_identity->queryAvailableMethods();
        return true;
    } else {
        return start(method, mechanism, parameters);
    }
}

bool SSOManager::start(const QString &method, const QString &mechanism,
                       const QVariantMap &parameters)
{
    if (!m_identity) {
        return false;
    }

    // Session was already started with a different method.
    if (m_session && m_session->name() != method) {
        m_identity->destroySession(m_session);
        m_session = nullptr;
    }

    SignOn::SessionData sessionData(parameters);
    if (!m_session) {
        m_session = m_identity->createSession(method);
        if (m_session) {
            connect(m_session, &SignOn::AuthSession::response,
                    this, &SSOManager::onResponse);
            connect(m_session, &SignOn::AuthSession::error,
                    this, &SSOManager::onError);
        }
    } else if (method == QLatin1String("oauth2")) {
        // A session already exists, but a request for
        // data is again desired. It means that the use
        // of the previous credentials failed. Try to
        // refresh the access token.
        OAuth2PluginNS::OAuth2PluginData oauth2Data
            = sessionData.data<OAuth2PluginNS::OAuth2PluginData>();
        oauth2Data.setForceTokenRefresh(true);
        sessionData = oauth2Data;
    }
    if (m_session) {
        sessionData.setUiPolicy(SignOn::NoUserInteractionPolicy);
        m_session->process(sessionData, mechanism);
        m_status = Fetching;
        emit statusChanged();
    }
    return m_session != nullptr;
}

void SSOManager::deinit()
{
    if (m_identity) {
        if (m_session)
            m_identity->destroySession(m_session);
        m_session = nullptr;
        delete m_identity;
        m_identity = nullptr;
    }
    m_credentialIds = 0;
    m_status = Invalid;
}

QMailCredentialsInterface::Status SSOManager::status() const
{
    return m_status;
}

QString SSOManager::lastError() const
{
    return m_errorMessage;
}

QString SSOManager::username() const
{
    if (m_status == Ready) {
        return !m_sessionData.UserName().isEmpty() ? m_sessionData.UserName() : m_username;
    } else {
        return QString();
    }
}

QString SSOManager::password() const
{
    return m_status == Ready && m_session->name() == QLatin1String("password")
        ? m_sessionData.Secret() : QString();
}

QString SSOManager::accessToken() const
{
    if (m_status == Ready && m_session->name() == QLatin1String("oauth2")) {
        OAuth2PluginNS::OAuth2PluginTokenData oauth2Data
            = m_sessionData.data<OAuth2PluginNS::OAuth2PluginTokenData>();
        return oauth2Data.AccessToken();
    } else {
        return QString();
    }
}

void SSOManager::onResponse(const SignOn::SessionData &sessionData)
{
    m_status = Ready;
    m_sessionData = sessionData;

    emit statusChanged();
}

void SSOManager::onError(const SignOn::Error &code)
{
    m_status = Failed;
    m_errorMessage = QString::fromLatin1("SSO error %1: %2").arg(code.type()).arg(code.message());
    if (code.type() == SignOn::Error::InvalidCredentials
        || code.type() == SignOn::Error:: UserInteraction) {
        invalidate();
    }

    emit statusChanged();
}

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

#ifndef SSOMANAGER_H
#define SSOMANAGER_H

#include <qmailcredentials.h>

#include <QTimer>
#include <SignOn/Identity>
#include <SignOn/SessionData>

class SSOManager: public QMailCredentialsInterface
{
public:
    SSOManager(QObject *parent = nullptr);
    ~SSOManager() override;

    bool init(uint credentialsId,
              const QString &method = QString(),
              const QString &mechanism = QString(),
              const QVariantMap &parameters = QVariantMap());
    void deinit();

    void authSuccessNotice(const QString &source = QString()) override;
    void authFailureNotice(const QString &source = QString()) override;
    bool shouldRetryAuth() const override;

    Status status() const override;
    QString lastError() const override;

    QString username() const override;
    QString password() const override;
    QString accessToken() const override;

private:
    bool start(const QString &method, const QString &mechanism,
               const QVariantMap &parameters);
    void onResponse(const SignOn::SessionData &sessionData);
    void onError(const SignOn::Error &code);
    void onStateChanged(SignOn::AuthSession::AuthSessionState state,
                        const QString &message);
    void onSessionTimeout();

    uint m_credentialIds = 0;
    SignOn::Identity *m_identity = nullptr;
    SignOn::AuthSession *m_session = nullptr;
    SignOn::SessionData m_sessionData;
    Status m_status = Invalid;
    QString m_errorMessage;
    QString m_username;
    QTimer m_timeout;
    bool m_retry = false;
};

#endif

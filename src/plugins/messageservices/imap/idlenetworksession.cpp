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

#include "idlenetworksession.h"

#include <QTimer>

class IdleNetworkSessionPrivate
{
    Q_DISABLE_COPY(IdleNetworkSessionPrivate)
    Q_DECLARE_PUBLIC(IdleNetworkSession)

public:
    IdleNetworkSessionPrivate(IdleNetworkSession *parent);

    void open();
    void close();
    bool isOpen() const;
    IdleNetworkSession::State state() const;
    IdleNetworkSession::Error error() const;

private:
    IdleNetworkSession * const q_ptr;
    IdleNetworkSession::State m_state = IdleNetworkSession::Disconnected;
    IdleNetworkSession::Error m_error = IdleNetworkSession::NoError;
};

IdleNetworkSessionPrivate::IdleNetworkSessionPrivate(IdleNetworkSession *parent)
    : q_ptr(parent)
{
}

void IdleNetworkSessionPrivate::open()
{
    Q_Q(IdleNetworkSession);

    if (m_state == IdleNetworkSession::Connecting
            || m_state == IdleNetworkSession::Connected) {
        return;
    }

    m_state = IdleNetworkSession::Connecting;
    emit q->stateChanged(m_state);

    QTimer::singleShot(500, q, [this, q] {
        if (m_state == IdleNetworkSession::Connecting) {
            m_state = IdleNetworkSession::Connected;
            emit q->stateChanged(m_state);
            emit q->opened();
        }
    });
}

void IdleNetworkSessionPrivate::close()
{
    Q_Q(IdleNetworkSession);

    if (m_state == IdleNetworkSession::Closing
            || m_state == IdleNetworkSession::Disconnected) {
        return;
    }

    m_state = IdleNetworkSession::Closing;
    emit q->stateChanged(m_state);

    QTimer::singleShot(500, q, [this, q] {
        if (m_state == IdleNetworkSession::Closing) {
            m_state = IdleNetworkSession::Disconnected;
            emit q->stateChanged(m_state);
            emit q->closed();
        }
    });
}

bool IdleNetworkSessionPrivate::isOpen() const
{
    return m_state == IdleNetworkSession::Connected;
}

IdleNetworkSession::State IdleNetworkSessionPrivate::state() const
{
    return m_state;
}

IdleNetworkSession::Error IdleNetworkSessionPrivate::error() const
{
    return m_error;
}

//----------------------------------------

IdleNetworkSession::IdleNetworkSession(QObject *parent)
    : QObject(parent)
    , d_ptr(new IdleNetworkSessionPrivate(this))
{
}

IdleNetworkSession::~IdleNetworkSession()
{
}

void IdleNetworkSession::open()
{
    Q_D(IdleNetworkSession);
    d->open();
}

void IdleNetworkSession::close()
{
    Q_D(IdleNetworkSession);
    d->close();
}

bool IdleNetworkSession::isOpen() const
{
    Q_D(const IdleNetworkSession);
    return d->isOpen();
}

IdleNetworkSession::State IdleNetworkSession::state() const
{
    Q_D(const IdleNetworkSession);
    return d->state();
}

IdleNetworkSession::Error IdleNetworkSession::error() const
{
    Q_D(const IdleNetworkSession);
    return d->error();
}

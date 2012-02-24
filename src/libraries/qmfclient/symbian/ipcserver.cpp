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

#include "ipcserver.h"

#include <QQueue>
#include <private/qobject_p.h>
#include "qmfipcchannel.h"
#include "ipcsocket.h"

class SymbianIpcServerPrivate : public QObjectPrivate, public SymbianQMFIPCChannelIncomingConnectionObserver
{
    Q_DECLARE_PUBLIC(SymbianIpcServer)

public:
    SymbianIpcServerPrivate();
    ~SymbianIpcServerPrivate();

public: // From SymbianQMFIPCChannelIncomingConnectionObserver
    virtual void NewConnection(quintptr connectionId);

public: // Data
    QString m_channelName;
    quintptr m_connectionId;
    int m_maxPendingConnections;
    QQueue<SymbianIpcSocket*> m_pendingConnections;
    mutable SymbianQMFIPCChannel m_channel;
};

SymbianIpcServerPrivate::SymbianIpcServerPrivate()
    : m_channelName(),
      m_connectionId(0),
      m_maxPendingConnections(30)
{
    m_channel.connect();
}

SymbianIpcServerPrivate::~SymbianIpcServerPrivate()
{
}

void SymbianIpcServerPrivate::NewConnection(quintptr connectionId)
{
    Q_Q(SymbianIpcServer);
    q->incomingConnection(connectionId);
}

SymbianIpcServer::SymbianIpcServer(QObject *parent)
    : QObject(*new SymbianIpcServerPrivate, parent)
{
}

SymbianIpcServer::~SymbianIpcServer()
{
    Q_D(SymbianIpcServer);
    d->m_channel.destroyChannel(d->m_channelName);
    qDeleteAll(d->m_pendingConnections);
    d->m_pendingConnections.clear();
}

bool SymbianIpcServer::listen(const QString &name)
{
    Q_D(SymbianIpcServer);
    if (!d->m_channelName.isEmpty()) {
        d->m_channel.destroyChannel(d->m_channelName);
    }
    d->m_channelName = name;
    if (d->m_channel.createChannel(name)) {
        return d->m_channel.waitForIncomingConnection(d);
    }

    return false;
}

bool SymbianIpcServer::hasPendingConnections() const
{
    Q_D(const SymbianIpcServer);
    return !(d->m_pendingConnections.isEmpty());
}

SymbianIpcSocket *SymbianIpcServer::nextPendingConnection()
{
    Q_D(SymbianIpcServer);

    if (d->m_pendingConnections.isEmpty()) {
        return 0;
    }

    return d->m_pendingConnections.dequeue();
}

void SymbianIpcServer::incomingConnection(quintptr socketDescriptor)
{
    Q_D(SymbianIpcServer);

    SymbianIpcSocket *socket = new SymbianIpcSocket(this);
    socket->setSocketDescriptor(socketDescriptor);
    d->m_pendingConnections.enqueue(socket);

    emit newConnection();

    d->m_channel.waitForIncomingConnection(d);
}

// End of File

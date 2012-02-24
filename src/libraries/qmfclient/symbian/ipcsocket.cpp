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

#include "ipcsocket.h"

#include <QEventLoop>
#include <QTimer>
#include "private/qiodevice_p.h"
#include "qmfipcchannel.h"

class SymbianIpcSocketPrivate : public QIODevicePrivate, public SymbianQMFIPCChannelObserver
{
    Q_DECLARE_PUBLIC(SymbianIpcSocket)

public:
    SymbianIpcSocketPrivate();
    SymbianIpcSocketPrivate(const SymbianIpcSocketPrivate& other); // Not used
    ~SymbianIpcSocketPrivate();

public: // From SymbianQMFIPCChannelDataObserver
    virtual void Connected(quintptr connectionId);
    virtual void ConnectionRefused();
    virtual void Disconnected();
    virtual void DataAvailable();

public: // Data
    SymbianIpcSocket::OpenMode m_openMode;
    SymbianIpcSocket::SymbianIpcSocketError m_error;
    QString m_channelName;
    quintptr m_connectionId;
    SymbianIpcSocket::SymbianIpcSocketState m_state;
    mutable SymbianQMFIPCChannel m_channel;
};

SymbianIpcSocketPrivate::SymbianIpcSocketPrivate()
    : QIODevicePrivate(),
      m_openMode(QIODevice::NotOpen),
      m_error(SymbianIpcSocket::UnknownSocketError),
      m_channelName(),
      m_connectionId(0),
      m_state(SymbianIpcSocket::UnconnectedState)
{
    m_channel.connect();
}

SymbianIpcSocketPrivate::SymbianIpcSocketPrivate(const SymbianIpcSocketPrivate& /*other*/)
{
    // Not used
}

SymbianIpcSocketPrivate::~SymbianIpcSocketPrivate()
{
}

void SymbianIpcSocketPrivate::Connected(quintptr connectionId)
{
    Q_Q(SymbianIpcSocket);
    m_connectionId = connectionId;
    m_state = SymbianIpcSocket::ConnectedState;
    emit q->stateChanged(m_state);
    emit q->connected();
    q->setSocketDescriptor(m_connectionId, SymbianIpcSocket::ConnectedState, m_openMode);
}

void SymbianIpcSocketPrivate::ConnectionRefused()
{
    Q_Q(SymbianIpcSocket);

    SymbianIpcSocket::SymbianIpcSocketState currentState = m_state;

    m_error = SymbianIpcSocket::ServerNotFoundError;
    m_state = SymbianIpcSocket::UnconnectedState;

    if (currentState != m_state) {
        emit q->stateChanged(m_state);
        if (m_state == SymbianIpcSocket::UnconnectedState) {
            emit q->disconnected();
        }
    }
    emit q->error(m_error);
}

void SymbianIpcSocketPrivate::Disconnected()
{
    Q_Q(SymbianIpcSocket);

    m_state = SymbianIpcSocket::UnconnectedState;
    emit q->stateChanged(m_state);
    emit q->disconnected();
}

void SymbianIpcSocketPrivate::DataAvailable()
{
    Q_Q(SymbianIpcSocket);
    emit q->readyRead();
}

SymbianIpcSocket::SymbianIpcSocket(QObject *parent)
    : QIODevice(*new SymbianIpcSocketPrivate, parent)
{
}

SymbianIpcSocket::~SymbianIpcSocket()
{
}

void SymbianIpcSocket::connectToServer(const QString &name, OpenMode openMode)
{
    Q_D(SymbianIpcSocket);
    if (state() == ConnectedState || state() == ConnectingState) {
        return;
    }

    d->m_openMode = openMode;
    d->m_channel.connectClientToChannel(name, d);
    d->m_state = SymbianIpcSocket::ConnectingState;
    d->m_channelName = name;
    emit stateChanged(d->m_state);
}

bool SymbianIpcSocket::flush()
{
    return true;
}

bool SymbianIpcSocket::setSocketDescriptor(quintptr socketDescriptor,
                                           SymbianIpcSocketState socketState,
                                           OpenMode openMode)
{
    Q_D(SymbianIpcSocket);
    QIODevice::open(openMode);
    d->m_state = socketState;
    if (d->m_connectionId == 0) {
        d->m_channel.connectServerToChannel(socketDescriptor, d);
        d->m_connectionId = socketDescriptor;
    }
    d->m_channel.startWaitingForData();
    return true;
}

SymbianIpcSocket::SymbianIpcSocketState SymbianIpcSocket::state() const
{
    Q_D(const SymbianIpcSocket);
    return d->m_state;
}

bool SymbianIpcSocket::isSequential() const
{
    return true;
}

qint64 SymbianIpcSocket::bytesAvailable() const
{
    Q_D(const SymbianIpcSocket);
    qint64 bytes = d->m_channel.dataSize() + QIODevice::bytesAvailable();
    return bytes;
}

bool SymbianIpcSocket::waitForConnected(int msecs)
{
    Q_D(SymbianIpcSocket);
    if (state() != ConnectingState) {
        return (state() == ConnectedState);
    }

    QEventLoop eventLoop;
    QObject::connect(this, SIGNAL(connected()), &eventLoop, SLOT(quit()));
    QObject::connect(this, SIGNAL(disconnected()), &eventLoop, SLOT(quit()));
    QTimer::singleShot(msecs, &eventLoop, SLOT(quit()));
    eventLoop.exec();

    return (state() == ConnectedState);
}

qint64 SymbianIpcSocket::readData(char *data, qint64 maxlen)
{
    Q_D(const SymbianIpcSocket);
    return d->m_channel.ReadData(data, maxlen);
}

qint64 SymbianIpcSocket::writeData(const char *data, qint64 len)
{
    Q_D(const SymbianIpcSocket);
    if (d->m_channel.SendDataL(data, len)) {
        return len;
    }
    return 0;
}

// End of File

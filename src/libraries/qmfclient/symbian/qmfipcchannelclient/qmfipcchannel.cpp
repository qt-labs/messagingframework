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

#include "qmfipcchannel.h"
#include "qmfipcchannelsession.h"
#include "qmfipcchannelclientservercommon.h"
#include <QString>
#include <QThreadStorage>
#include <qglobal.h>

class SymbianQMFIPCChannelPrivate : public CActive, public QSharedData
{
public:
    SymbianQMFIPCChannelPrivate();
    SymbianQMFIPCChannelPrivate(const SymbianQMFIPCChannelPrivate &other);
    ~SymbianQMFIPCChannelPrivate();

    SymbianQMFIPCChannelPrivate& operator=(const SymbianQMFIPCChannelPrivate& other);

protected: // From CActive
    void RunL();
    void DoCancel();

public: // Data
    friend class SymbianQMFIPCChannel;
    bool m_connected;
    RQMFIPCChannelSession m_session;
    SymbianQMFIPCChannelObserver* m_channelObserver;
    SymbianQMFIPCChannelIncomingConnectionObserver* m_incomingConnectionObserver;
    HBufC* m_currentChannel;
    TPckgBuf<TInt> m_incomingMessageSizePckgBuf;
    TPckgBuf<TUint> m_connectionIdPckgBuf;
    RBuf8 m_incomingData;
};

SymbianQMFIPCChannelPrivate::SymbianQMFIPCChannelPrivate()
    : CActive(EPriorityStandard),
      m_connected(false)
{
    CActiveScheduler::Add(this);
    if (m_session.Connect() == KErrNone) {
        m_connected = true;
    }
}

SymbianQMFIPCChannelPrivate::SymbianQMFIPCChannelPrivate(const SymbianQMFIPCChannelPrivate &other)
    : QSharedData(other),
      CActive(EPriorityStandard),
      m_session(other.m_session),
      m_connected(other.m_connected)
{
}

SymbianQMFIPCChannelPrivate::~SymbianQMFIPCChannelPrivate()
{
    Cancel();
    m_session.Close();
    m_incomingData.Close();
    if (m_currentChannel) {
        delete m_currentChannel;
    }
}

SymbianQMFIPCChannelPrivate& SymbianQMFIPCChannelPrivate::operator=(const SymbianQMFIPCChannelPrivate& other)
{
    if (this != &other) {
        m_session = other.m_session;
    }

    return *this;
}

void SymbianQMFIPCChannelPrivate::RunL()
{
    switch (iStatus.Int()) {
    case EQMFIPCChannelRequestNewChannelConnection:
        m_incomingConnectionObserver->NewConnection(m_connectionIdPckgBuf());
        if (!IsActive()) {
            // Continue waiting for new incoming connections
            m_session.WaitForIncomingConnection(m_connectionIdPckgBuf, iStatus);
            SetActive();
        }
        break;
    case EQMFIPCChannelRequestChannelNotFound:
        m_channelObserver->ConnectionRefused();
        break;
    case EQMFIPCChannelRequestChannelConnected:
        m_channelObserver->Connected(m_connectionIdPckgBuf());
        if (!IsActive()) {
            // => First message can be handled
            m_session.ListenChannel(m_incomingMessageSizePckgBuf, iStatus);
            SetActive();
        }
        break;
    case EQMFIPCChannelRequestDataAvailable:
        m_incomingData.Close();
        TRAPD(err, m_incomingData.CreateL(m_incomingMessageSizePckgBuf()));
        if (err == KErrNone) {
            m_session.ReceiveData(m_incomingData);
            m_channelObserver->DataAvailable();
        }
        break;
    case KErrCancel:
        break;
    case KErrNotReady:
    default:
        break;
    }
}

void SymbianQMFIPCChannelPrivate::DoCancel()
{
    m_session.Cancel();
}

SymbianQMFIPCChannel::SymbianQMFIPCChannel()
{
    d = new SymbianQMFIPCChannelPrivate;
}

SymbianQMFIPCChannel::SymbianQMFIPCChannel(const SymbianQMFIPCChannel& other)
    : d(other.d)
{
}

SymbianQMFIPCChannel::~SymbianQMFIPCChannel()
{
}

SymbianQMFIPCChannel& SymbianQMFIPCChannel::operator=(const SymbianQMFIPCChannel& other)
{
    if (this != &other) {
        d = other.d;
    }

    return *this;
}

bool SymbianQMFIPCChannel::connect()
{
    if (!d->m_connected) {
        if (d->m_session.Connect() == KErrNone) {
            d->m_connected = true;
        }
    }
    return d->m_connected;
}

bool SymbianQMFIPCChannel::createChannel(QString name)
{
    if (!d->m_connected) {
        return false;
    }

    TPtrC16 stringPtr(KNullDesC);
    stringPtr.Set(reinterpret_cast<const TUint16*>(name.utf16()));
    return d->m_session.CreateChannel(stringPtr);
}

bool SymbianQMFIPCChannel::destroyChannel(QString name)
{
    if (!d->m_connected) {
        return false;
    }

    TPtrC16 stringPtr(KNullDesC);
    stringPtr.Set(reinterpret_cast<const TUint16*>(name.utf16()));
    return d->m_session.DestroyChannel(stringPtr);
}

bool SymbianQMFIPCChannel::waitForIncomingConnection(SymbianQMFIPCChannelIncomingConnectionObserver* observer)
{
    if (!d->m_connected) {
        return false;
    }

    bool retVal = d->m_session.WaitForIncomingConnection(d->m_connectionIdPckgBuf,
                                                         d->iStatus);
    d->SetActive();
    d->m_incomingConnectionObserver = observer;

    return retVal;
}

bool SymbianQMFIPCChannel::connectClientToChannel(QString name, SymbianQMFIPCChannelObserver* observer)
{
    bool retVal = false;

    if (!d->m_connected) {
        return false;
    }

    TPtrC16 stringPtr(KNullDesC);
    stringPtr.Set(reinterpret_cast<const TUint16*>(name.utf16()));
    if (d->m_currentChannel) {
        delete d->m_currentChannel;
        d->m_currentChannel = NULL;
    }
    d->m_currentChannel = stringPtr.Alloc();
    if (d->m_currentChannel) {
        bool retVal = d->m_session.ConnectClientToChannel(*d->m_currentChannel,
                                                          d->m_connectionIdPckgBuf,
                                                          d->iStatus);
        d->SetActive();
        d->m_channelObserver = observer;
    }

    return retVal;
}

bool SymbianQMFIPCChannel::connectServerToChannel(quintptr socketDescriptor,
                                                  SymbianQMFIPCChannelObserver* observer)
{
    bool retVal = false;

    if (!d->m_connected) {
        return false;
    }

    d->m_connectionIdPckgBuf = socketDescriptor;
    d->m_channelObserver = observer;
    return d->m_session.ConnectServerToChannel(socketDescriptor);
}

bool SymbianQMFIPCChannel::SendDataL(const char *data, qint64 len)
{
    if (!d->m_connected) {
        return false;
    }

    TPtrC8 stringPtr((const TUint8 *)data, len);
    return d->m_session.SendData(stringPtr);
}

bool SymbianQMFIPCChannel::startWaitingForData()
{
    if (!d->IsActive()) {
        d->m_session.ListenChannel(d->m_incomingMessageSizePckgBuf, d->iStatus);
        d->SetActive();
        return true;
    }
    return false;
}

qint64 SymbianQMFIPCChannel::ReadData(char *data, qint64 maxlen)
{
    quint64 dataLength = d->m_incomingData.Length();

    if (dataLength > maxlen) {
        dataLength = maxlen;
    }

    char* dataPtr = (char*)d->m_incomingData.Ptr();
    for (int i=0; i < dataLength; i++) {
        *(data+i) = *(dataPtr+i);
    }

    if (d->m_incomingData.Length() > dataLength) {
        d->m_incomingData.Delete(0, dataLength);
    } else {
        d->m_incomingData.Close();
    }

    if (d->m_incomingData.Length() == 0) {
        // All message data was read from the buffer
        if (!d->IsActive()) {
            // => Next message can be handled
            d->m_session.ListenChannel(d->m_incomingMessageSizePckgBuf, d->iStatus);
            d->SetActive();
        }
    }

    return dataLength;
}

qint64 SymbianQMFIPCChannel::dataSize() const
{
    return d->m_incomingData.Size();
}

// End of File

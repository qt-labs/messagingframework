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

#include "qmfipcchannelserversession.h"

#include "qmfipcchannelserver.h"
#include "qmfipcchannelclientservercommon.h"

CQMFIPCChannelServerSession* CQMFIPCChannelServerSession::NewL(CQMFIPCChannelServer& aServer)
{
    CQMFIPCChannelServerSession* pSelf = CQMFIPCChannelServerSession::NewLC(aServer);
    CleanupStack::Pop(pSelf);
    return pSelf;
}

CQMFIPCChannelServerSession* CQMFIPCChannelServerSession::NewLC(CQMFIPCChannelServer& aServer)
{
    CQMFIPCChannelServerSession* pSelf = new (ELeave) CQMFIPCChannelServerSession(aServer);
    CleanupStack::PushL(pSelf);
    pSelf->ConstructL();
    return pSelf;
}

void CQMFIPCChannelServerSession::ConstructL()
{
    iServer.IncrementSessions();
}

CQMFIPCChannelServerSession::CQMFIPCChannelServerSession(CQMFIPCChannelServer& aServer)
    : iServer(aServer)
{
}

CQMFIPCChannelServerSession::~CQMFIPCChannelServerSession()
{
    if (ipChannelName) {
        delete ipChannelName;
    }
    iClientsToConnect.Reset();
    iMessagesToDeliver.ResetAndDestroy();
    iServer.DisconnectSessionFromChannel(this);
    iServer.DecrementSessions();
}

void CQMFIPCChannelServerSession::ServiceL(const RMessage2& aMessage)
{
    switch (aMessage.Function()) {
    case EQMFIPCChannelServerRequestCreateChannel:
        CreateChannelL(aMessage);
        break;
    case EQMFIPCChannelServerRequestWaitForIncomingConnection:
        WaitForIncomingConnectionL(aMessage);
        break;
    case EQMFIPCChannelServerRequestDestroyChannel:
        DestroyChannelL(aMessage);
        break;
    case EQMFIPCChannelServerRequestChannelExists:
        ChannelExistsL(aMessage);
        break;
    case EQMFIPCChannelServerRequestConnectClientToChannel:
        ConnectClientToChannelL(aMessage);
        break;
    case EQMFIPCChannelServerRequestConnectServerToChannel:
        ConnectServerToChannelL(aMessage);
        break;
    case EQMFIPCChannelServerRequestListenChannel:
        ListenChannelL(aMessage);
        break;
    case EQMFIPCChannelServerRequestDisconnectFromChannel:
        DisconnectFromChannelL(aMessage);
        break;
    case EQMFIPCChannelServerRequestSendData:
        SendDataL(aMessage);
        break;
    case EQMFIPCChannelServerRequestReceiveData:
        ReceiveDataL(aMessage);
        break;
    case EQMFIPCChannelServerRequestCancel:
        CancelL(aMessage);
        break;
    default:
        PanicClient(aMessage, EBadRequest);
    }
}

void CQMFIPCChannelServerSession::CreateChannelL(const RMessage2& aMessage)
{
    RBuf channelName;
    channelName.CreateL(aMessage.GetDesLengthL(1));
    CleanupClosePushL(channelName);
    aMessage.ReadL(1, channelName);

    iServer.ActivateChannel(channelName, this);

    CleanupStack::PopAndDestroy(&channelName);
    TPckg<TBool> resultPckg(ETrue);
    aMessage.WriteL(0, resultPckg);

    iServerSession = ETrue;

    aMessage.Complete(KErrNone);
}

void CQMFIPCChannelServerSession::WaitForIncomingConnectionL(const RMessage2& aMessage)
{
    if (iClientsToConnect.Count() > 0) {
        TPckg<TUint> connectionIdPckg(iClientsToConnect[0]->iConnectionId);
        aMessage.WriteL(0, connectionIdPckg);
        aMessage.Complete(EQMFIPCChannelRequestNewChannelConnection);
        iClientsToConnect.Remove(0);
        iWaitingForConnection = EFalse;
    } else {
        iWaitingForConnection = ETrue;
        iMessage = aMessage;
    }
}

void CQMFIPCChannelServerSession::DestroyChannelL(const RMessage2& aMessage)
{
    RBuf channelName;
    channelName.CreateL(aMessage.GetDesLengthL(1));
    CleanupClosePushL(channelName);
    aMessage.ReadL(1, channelName);

    iServer.DeactivateChannel(channelName);

    CleanupStack::PopAndDestroy(&channelName);
    TPckg<TBool> resultPckg(ETrue);
    aMessage.WriteL(0, resultPckg);

    aMessage.Complete(KErrNone);
}

void CQMFIPCChannelServerSession::ChannelExistsL(const RMessage2& aMessage)
{
    RBuf channelName;
    channelName.CreateL(aMessage.GetDesLengthL(1));
    CleanupClosePushL(channelName);
    aMessage.ReadL(1, channelName);

    TBool exists = iServer.ChannelExists(channelName);
    CleanupStack::PopAndDestroy(&channelName);
    TPckg<TBool> resultPckg(exists);
    aMessage.WriteL(0, resultPckg);

    aMessage.Complete(KErrNone);
}

void CQMFIPCChannelServerSession::ConnectClientToChannelL(const RMessage2& aMessage)
{
    RBuf channelName;
    channelName.CreateL(aMessage.GetDesLengthL(1));
    CleanupClosePushL(channelName);
    aMessage.ReadL(1, channelName);

    if (iServer.ConnectClientSessionToChannel(channelName, this)) {
        if (ipChannelName) {
            delete ipChannelName;
        }
        ipChannelName = channelName.AllocL();

        iWaitingForConnected = ETrue;
        iClientChannelSession = ETrue;
        iMessage = aMessage;
    } else {
        aMessage.Complete(EQMFIPCChannelRequestChannelNotFound);
    }

    CleanupStack::PopAndDestroy(&channelName);
}

void CQMFIPCChannelServerSession::ConnectServerToChannelL(const RMessage2& aMessage)
{
    TUint connectionId;
    TPckg<TUint> connectionIdPckg(connectionId);
    aMessage.ReadL(1, connectionIdPckg);
    TBool retVal = ETrue;
    if (iServer.ConnectServerSessionToChannel(connectionId, this)) {
        iServerChannelSession = ETrue;
        retVal = EFalse;
    }
    TPckg<TBool> retValPckg(retVal);
    aMessage.WriteL(0, retValPckg);
    aMessage.Complete(KErrNone);
}

void CQMFIPCChannelServerSession::ListenChannelL(const RMessage2& aMessage)
{
    iMessage = aMessage;
    if (iMessagesToDeliver.Count() > 0) {
        TPckgBuf<TInt> lengthPckg(iMessagesToDeliver[0]->Length());
        TRAPD(err, iMessage.WriteL(0, lengthPckg));
        if (err == KErrNone) {
            iWaitingForMessage = EFalse;
            iMessage.Complete(EQMFIPCChannelRequestDataAvailable);
        } else {
            PanicClient(iMessage, EBadDescriptor);
        }
    } else {
        iWaitingForMessage = ETrue;
    }
}

void CQMFIPCChannelServerSession::DisconnectFromChannelL(const RMessage2& aMessage)
{
    iServer.DisconnectSessionFromChannel(this);
    if (ipChannelName) {
        delete ipChannelName;
        ipChannelName = NULL;
    }
    if (iWaitingForMessage) {
        iMessage.Complete(EQMFIPCChannelRequestChannelDisconnected);
    }

    aMessage.Complete(KErrNone);
}

void CQMFIPCChannelServerSession::SendDataL(const RMessage2& aMessage)
{
    TBool retVal = EFalse;

    if (ipChannelName) {
        RBuf8 message;
        message.CreateL(aMessage.GetDesLengthL(1));
        CleanupClosePushL(message);
        aMessage.ReadL(1, message);

        if (ipSession) {
            ipSession->NewMessage(message);
        }
        CleanupStack::PopAndDestroy(&message);
        retVal = ETrue;
    }

    TPckg<TBool> retValPckg(retVal);
    aMessage.WriteL(0, retValPckg);
    aMessage.Complete(KErrNone);
}

void CQMFIPCChannelServerSession::ReceiveDataL(const RMessage2& aMessage)
{
    TBool retVal = EFalse;

    if (iMessagesToDeliver.Count() > 0) {
        aMessage.WriteL(1, *iMessagesToDeliver[0]);
        delete iMessagesToDeliver[0];
        iMessagesToDeliver.Remove(0);
        retVal = ETrue;
        TPckg<TBool> retValPckg(retVal);
        aMessage.WriteL(0, retValPckg);
        aMessage.Complete(KErrNone);
    } else {
        PanicClient(aMessage, EBadRequest);
    }
}

void CQMFIPCChannelServerSession::CancelL(const RMessage2& aMessage)
{
    iServer.DisconnectSessionFromChannel(this);
    if (iWaitingForMessage || iWaitingForConnected || iWaitingForConnection) {
        iMessage.Complete(EQMFIPCChannelRequestCanceled);
    }
    aMessage.Complete(KErrNone);
}

void CQMFIPCChannelServerSession::NewMessage(const TDesC8& aMessage)
{
    HBufC8* pNewMessage = aMessage.Alloc();
    if (pNewMessage) {
        if (iMessagesToDeliver.Append(pNewMessage) != KErrNone) {
            delete pNewMessage;
            pNewMessage = NULL;
        }
    }

    if (iWaitingForMessage && pNewMessage) {
        if (iMessagesToDeliver.Count() > 0) {
            TPckgBuf<TInt> lengthPckg(iMessagesToDeliver[0]->Length());
            TRAPD(err, iMessage.WriteL(0, lengthPckg));
            if (err == KErrNone) {
                iWaitingForMessage = EFalse;
                iMessage.Complete(EQMFIPCChannelRequestDataAvailable);
            } else {
                PanicClient(iMessage, EBadDescriptor);
            }
        }
    }
}

void CQMFIPCChannelServerSession::ClientSessionConnected(CQMFIPCChannelServerSession* apSession)
{
    if (iServerSession) {
        if (iWaitingForConnection) {
            TPckg<TUint> connectionIdPckg(apSession->iConnectionId);
            iMessage.WriteL(0, connectionIdPckg);
            iMessage.Complete(EQMFIPCChannelRequestNewChannelConnection);
            iWaitingForConnection = EFalse;
        } else {
            iClientsToConnect.Append(apSession);
        }
    }
}

void CQMFIPCChannelServerSession::ServerSessionConnected(TUint aConnectionId)
{
    if (iClientChannelSession && iWaitingForConnected) {
        TPckg<TUint> connectionIdPckg(aConnectionId);
        iMessage.WriteL(0, connectionIdPckg);
        iMessage.Complete(EQMFIPCChannelRequestChannelConnected);
        iWaitingForConnected = EFalse;
    }
}

void CQMFIPCChannelServerSession::PanicClient(const RMessagePtr2& aMessage, TInt aPanic ) const
{
    aMessage.Panic(KQMFIPCChannelServer, aPanic);
}

// End of File

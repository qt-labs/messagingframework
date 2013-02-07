/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
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

#include "qmfipcchannelsession.h"
#include "qmfipcchannelclientservercommon.h"

#include <f32file.h>

static TInt StartServer();
static TInt CreateServerProcess();

RQMFIPCChannelSession::RQMFIPCChannelSession()
    : RSessionBase()
{
}

TInt RQMFIPCChannelSession::Connect()
{
    TInt retVal = ::StartServer();
    if (retVal == KErrNone) {
        retVal = CreateSession(KQMFIPCChannelServerName, Version(), KDefaultMessageSlots);
        if (retVal == KErrServerTerminated) {
            retVal = ::StartServer();
            if (retVal == KErrNone) {
                retVal = CreateSession(KQMFIPCChannelServerName, Version(), KDefaultMessageSlots);
            }
        }
    }
    return retVal;
}

TVersion RQMFIPCChannelSession::Version() const
{
    return(TVersion(KQMFIPCChannelServerMajorVersionNumber,
                    KQMFIPCChannelServerMinorVersionNumber,
                    KQMFIPCChannelServerBuildVersionNumber));
}

TBool RQMFIPCChannelSession::CreateChannel(const TDesC& aChannelName)
{
    TPckgBuf<TBool> resultPckgBuf(EFalse);
    TInt retVal = SendReceive(EQMFIPCChannelServerRequestCreateChannel, TIpcArgs(&resultPckgBuf, &aChannelName));
    if (retVal < KErrNone) {
        return EFalse;
    }
    return resultPckgBuf();
}

TBool RQMFIPCChannelSession::WaitForIncomingConnection(TPckgBuf<TUint>& aNewConnectionIdPckgBuf,
                                                       TRequestStatus& aStatus)
{
    SendReceive(EQMFIPCChannelServerRequestWaitForIncomingConnection, TIpcArgs(&aNewConnectionIdPckgBuf), aStatus);
    return ETrue;
}

TBool RQMFIPCChannelSession::DestroyChannel(const TDesC& aChannelName)
{
    TPckgBuf<TBool> resultPckgBuf(EFalse);
    TInt retVal = SendReceive(EQMFIPCChannelServerRequestDestroyChannel, TIpcArgs(&resultPckgBuf, &aChannelName));
    if (retVal < KErrNone) {
        return EFalse;
    }
    return resultPckgBuf();
}

TBool RQMFIPCChannelSession::ChannelExists(const TDesC& aChannelName)
{
    TPckgBuf<TBool> resultPckgBuf(EFalse);
    TInt retVal = SendReceive(EQMFIPCChannelServerRequestCreateChannel, TIpcArgs(&resultPckgBuf, &aChannelName));
    if (retVal < KErrNone) {
        return EFalse;
    }
    return resultPckgBuf();
}

TBool RQMFIPCChannelSession::ConnectClientToChannel(const TDesC& aChannelName,
                                                    TPckgBuf<TUint>& aNewConnectionIdPckgBuf,
                                                    TRequestStatus& aStatus)
{
    SendReceive(EQMFIPCChannelServerRequestConnectClientToChannel,
                TIpcArgs(&aNewConnectionIdPckgBuf, &aChannelName),
                aStatus);
    return ETrue;
}

TBool RQMFIPCChannelSession::ConnectServerToChannel(TUint aConnectionId)
{
    TPckgBuf<TBool> resultPckgBuf(EFalse);
    TPckg<TUint> connectionIdPckg(aConnectionId);
    TInt retVal = SendReceive(EQMFIPCChannelServerRequestConnectServerToChannel,
                              TIpcArgs(&resultPckgBuf, &connectionIdPckg));
    if (retVal != KErrNone) {
        return EFalse;
    }
    return resultPckgBuf();
}

TBool RQMFIPCChannelSession::ListenChannel(TPckgBuf<TInt>& aIncomingMessageSizePckgBug,
                                           TRequestStatus& aStatus)
{
    SendReceive(EQMFIPCChannelServerRequestListenChannel,
                TIpcArgs(&aIncomingMessageSizePckgBug),
                aStatus);
    return ETrue;
}

TBool RQMFIPCChannelSession::DisconnectFromChannel()
{
    TPckgBuf<TBool> resultPckgBuf(EFalse);
    TInt retVal = SendReceive(EQMFIPCChannelServerRequestDisconnectFromChannel, TIpcArgs(&resultPckgBuf));
    if (retVal != KErrNone) {
        return EFalse;
    }
    return resultPckgBuf();
}

TBool RQMFIPCChannelSession::SendData(const TDesC8& aData)
{
    TPckgBuf<TBool> resultPckgBuf(EFalse);
    TInt retVal = SendReceive(EQMFIPCChannelServerRequestSendData, TIpcArgs(&resultPckgBuf, &aData));
    if (retVal != KErrNone) {
        return EFalse;
    }
    return resultPckgBuf();
}

TBool RQMFIPCChannelSession::ReceiveData(RBuf8& aData)
{
    TPckgBuf<TBool> resultPckgBuf(EFalse);
    TInt retVal = SendReceive(EQMFIPCChannelServerRequestReceiveData, TIpcArgs(&resultPckgBuf, &aData));
    if (retVal != KErrNone) {
        return EFalse;
    }
    return resultPckgBuf();
}

void RQMFIPCChannelSession::Cancel()
{
    SendReceive(EQMFIPCChannelServerRequestCancel);
}

static TInt StartServer()
{
    TInt retVal;

    TFindServer findQMFIPCChannelServer(KQMFIPCChannelServerName);
    TFullName name;

    retVal = findQMFIPCChannelServer.Next(name);
    if (retVal == KErrNone) {
        // Server is already running
        return KErrNone;
    }

    RSemaphore semaphore;
    retVal = semaphore.CreateGlobal(KQMFIPCChannelServerSemaphoreName, 0);
    if (retVal == KErrAlreadyExists) {
        retVal = semaphore.OpenGlobal(KQMFIPCChannelServerSemaphoreName);
        if (retVal != KErrNone) {
            return  retVal;
        }
    } else {
        if (retVal != KErrNone) {
            return  retVal;
        }
        retVal = CreateServerProcess();
        if (retVal != KErrNone ) {
            return  retVal;
        }
    }

    semaphore.Wait();
    semaphore.Close();

    return KErrNone;
}

static TInt CreateServerProcess()
{
    TInt retVal;

    const TUidType serverUid(KNullUid, KNullUid, KServerUid3);

    RProcess server;
    retVal = server.Create(KQMFIPCChannelServerFilename, KNullDesC, serverUid);
    if (retVal != KErrNone) {
        return  retVal;
    }
    server.Resume();
    server.Close();

    return  KErrNone;
}

// End of File

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

#ifndef QMFIPCCHANNELSERVER_H
#define QMFIPCCHANNELSERVER_H

#include "qmfipcchannelclientservercommon.h"
#include <e32base.h>

class CQMFIPCChannelServerSession;

struct TIPCChannel
{
    HBufC* channelName;
    CQMFIPCChannelServerSession* ipServerSession;
    TBool active;
    RPointerArray<CQMFIPCChannelServerSession> iConnectedClientSessions;
};

class CQMFIPCChannelServer : public CServer2
{
public :
    static CQMFIPCChannelServer* NewL();
    static CQMFIPCChannelServer* NewLC();
    virtual ~CQMFIPCChannelServer();

    static TInt ThreadFunction(TAny* aStarted);
    void IncrementSessions();
    void DecrementSessions();

    void ActivateChannel(const TDesC& aChannelName, CQMFIPCChannelServerSession* apSession);
    void DeactivateChannel(const TDesC& aChannelName);
    TBool ChannelExists(const TDesC& aChannelName);
    TBool ConnectClientSessionToChannel(const TDesC& aChannelName, CQMFIPCChannelServerSession* apSession);
    TBool ConnectServerSessionToChannel(TUint aConnectionId, CQMFIPCChannelServerSession* apSession);
    void DisconnectSessionFromChannel(CQMFIPCChannelServerSession* apSession);

protected: // From CActive
    TInt RunError(TInt aError);

protected: // CPeriodic callback
    static TInt PeriodicTimerCallBack(TAny* aAny);

private: // Constructors and destructors
    CQMFIPCChannelServer(TInt aPriority);
    void ConstructL();

private:
    static void PanicClient(const RMessage2& aMessage, TQMFIPCChannelServerPanic aReason);
    static void PanicServer(TQMFIPCChannelServerPanic aPanic);
    static void ThreadFunctionL();
    TUint NewConnectionId();

private: // From CServer2
    CSession2* NewSessionL(const TVersion& aVersion, const RMessage2& aMessage) const;

private: // Data
    TInt iSessionCount;
    TInt iConnectionIdCounter;
    CPeriodic* ipShutdownTimer;
    RArray<TIPCChannel> iIPCChannels;
};

#endif // QMFIPCCHANNELSERVER_H

// End of File

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

#ifndef QMFIPCCHANNELSERVERSESSION_H
#define QMFIPCCHANNELSERVERSESSION_H

#include <e32base.h>

class CQMFIPCChannelServer;

class CQMFIPCChannelServerSession : public CSession2
{
public:
    static CQMFIPCChannelServerSession* NewL(CQMFIPCChannelServer& aServer);
    static CQMFIPCChannelServerSession* NewLC(CQMFIPCChannelServer& aServer);
    virtual ~CQMFIPCChannelServerSession();

    void ServiceL( const RMessage2& aMessage );

    void NewMessage(const TDesC8& aMessage);
    void ClientSessionConnected(CQMFIPCChannelServerSession* apSession);
    void ServerSessionConnected(TUint aConnectionId);

private:
    CQMFIPCChannelServerSession(CQMFIPCChannelServer& aServer);
    void ConstructL();
    void PanicClient(const RMessagePtr2& aMessage, TInt aPanic) const;

    void CreateChannelL(const RMessage2& aMessage);
    void WaitForIncomingConnectionL(const RMessage2& aMessage);
    void DestroyChannelL(const RMessage2& aMessage);
    void ChannelExistsL(const RMessage2& aMessage);
    void ConnectClientToChannelL(const RMessage2& aMessage);
    void ConnectServerToChannelL(const RMessage2& aMessage);
    void ListenChannelL(const RMessage2& aMessage);
    void DisconnectFromChannelL(const RMessage2& aMessage);
    void SendDataL(const RMessage2& aMessage);
    void ReceiveDataL(const RMessage2& aMessage);
    void CancelL(const RMessage2& aMessage);

public: // Data
    TBool iServerSession;
    TBool iServerChannelSession;
    TBool iClientChannelSession;
    TUint iConnectionId;
    HBufC* ipChannelName;
    CQMFIPCChannelServerSession* ipSession;

private: // Data
    TBool iWaitingForMessage;
    TBool iWaitingForConnected;
    TBool iWaitingForConnection;
    RMessage2 iMessage;
    CQMFIPCChannelServer& iServer;
    RPointerArray<HBufC8> iMessagesToDeliver;
    RPointerArray<CQMFIPCChannelServerSession> iClientsToConnect;
};

#endif // QMFIPCCHANNELSERVERSESSION_H

// End of File

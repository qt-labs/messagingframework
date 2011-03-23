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

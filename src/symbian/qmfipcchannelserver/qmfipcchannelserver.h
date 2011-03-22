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

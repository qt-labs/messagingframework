#include "qmfipcchannelserver.h"
#include "qmfipcchannelserversession.h"
#include <e32svr.h>

const TInt KShutdownInterval(30000000); // 30 seconds

CQMFIPCChannelServer* CQMFIPCChannelServer::NewL()
{
    CQMFIPCChannelServer* pSelf = CQMFIPCChannelServer::NewLC();
    CleanupStack::Pop(pSelf);
    return pSelf;
}
CQMFIPCChannelServer* CQMFIPCChannelServer::NewLC()
{
    CQMFIPCChannelServer* pSelf = new (ELeave) CQMFIPCChannelServer(EPriorityNormal);
    CleanupStack::PushL(pSelf);
    pSelf->ConstructL();
    return pSelf;
}

void CQMFIPCChannelServer::ConstructL()
{
    StartL(KQMFIPCChannelServerName);
}

CQMFIPCChannelServer::CQMFIPCChannelServer(TInt aPriority)
    : CServer2(aPriority)
{
}

CQMFIPCChannelServer::~CQMFIPCChannelServer()
{
    if (ipShutdownTimer) {
        delete ipShutdownTimer;
        ipShutdownTimer = NULL;
    }

    for (int i=iIPCChannels.Count()-1; i >= 0; i--) {
        delete iIPCChannels[i].channelName;
        iIPCChannels[i].iConnectedClientSessions.Close();
        iIPCChannels.Remove(i);
    }
}

CSession2* CQMFIPCChannelServer::NewSessionL(const TVersion& aVersion, const RMessage2& /*aMessage*/) const
{
    // Check version
    if (!User::QueryVersionSupported(TVersion(KQMFIPCChannelServerMajorVersionNumber,
                                              KQMFIPCChannelServerMinorVersionNumber,
                                              KQMFIPCChannelServerBuildVersionNumber),
                                     aVersion)) {
        User::Leave(KErrNotSupported);
    }

    return CQMFIPCChannelServerSession::NewL(*const_cast<CQMFIPCChannelServer*> ( this ));
}

void CQMFIPCChannelServer::IncrementSessions()
{
    if (ipShutdownTimer && ipShutdownTimer->IsActive()) {
        ipShutdownTimer->Cancel();
    }
    iSessionCount++;
}

void CQMFIPCChannelServer::DecrementSessions()
{
    iSessionCount--;
    if (iSessionCount <= 0) {
        if (!ipShutdownTimer) {
            ipShutdownTimer = CPeriodic::New(EPriorityIdle);
        }
        if (ipShutdownTimer) {
            ipShutdownTimer->Start(KShutdownInterval, KShutdownInterval, TCallBack(PeriodicTimerCallBack, this));
        } else {
            CActiveScheduler::Stop();
        }
    }
}

void CQMFIPCChannelServer::ActivateChannel(const TDesC& aChannelName, CQMFIPCChannelServerSession* apSession)
{
    TBool channelFound = EFalse;
    for (int i=0; i < iIPCChannels.Count(); i++) {
        if (iIPCChannels[i].channelName->Compare(aChannelName) == 0) {
            iIPCChannels[i].active = ETrue;
            iIPCChannels[i].ipServerSession = apSession;
            channelFound = ETrue;
            break;
        }
    }
    if (!channelFound) {
        TIPCChannel newChannel;
        newChannel.active = ETrue;
        newChannel.ipServerSession = apSession;
        newChannel.channelName = aChannelName.Alloc();
        iIPCChannels.Append(newChannel);
    }
}

void CQMFIPCChannelServer::DeactivateChannel(const TDesC& aChannelName)
{
    for (int i=0; i < iIPCChannels.Count(); i++) {
        if (iIPCChannels[i].channelName->Compare(aChannelName) == 0) {
            iIPCChannels[i].active = EFalse;
            if (iIPCChannels[i].iConnectedClientSessions.Count() == 0) {
                delete iIPCChannels[i].channelName;
                iIPCChannels[i].iConnectedClientSessions.Close();
                iIPCChannels.Remove(i);
            }
        }
    }
}

TBool CQMFIPCChannelServer::ChannelExists(const TDesC& aChannelName)
{
    for (int i=0; i < iIPCChannels.Count(); i++) {
        if (iIPCChannels[i].channelName->Compare(aChannelName) == 0) {
            if (iIPCChannels[i].active) {
                return ETrue;
            }
        }
    }
    return EFalse;
}

TBool CQMFIPCChannelServer::ConnectClientSessionToChannel(const TDesC& aChannelName,
                                                         CQMFIPCChannelServerSession* aSession)
{
    for (int i=0; i < iIPCChannels.Count(); i++) {
        if (iIPCChannels[i].channelName->Compare(aChannelName) == 0) {
            aSession->iConnectionId = NewConnectionId();
            iIPCChannels[i].iConnectedClientSessions.Append(aSession);
            iIPCChannels[i].ipServerSession->ClientSessionConnected(aSession);
            return ETrue;
        }
    }
    return EFalse;
}

TBool CQMFIPCChannelServer::ConnectServerSessionToChannel(TUint aConnectionId,
                                                          CQMFIPCChannelServerSession* apSession)
{
    for (int i=0; i < iIPCChannels.Count(); i++) {
        for (int j=0; j < iIPCChannels[i].iConnectedClientSessions.Count(); j++) {
            if (iIPCChannels[i].iConnectedClientSessions[j]->iConnectionId == aConnectionId) {
                // Connect server session to client session and vice versa
                iIPCChannels[i].iConnectedClientSessions[j]->ipSession = apSession;
                apSession->ipSession = iIPCChannels[i].iConnectedClientSessions[j];
                // Set channel name to server session
                if (apSession->ipChannelName) {
                    delete apSession->ipChannelName;
                    apSession->ipChannelName = NULL;
                }
                apSession->ipChannelName = iIPCChannels[i].iConnectedClientSessions[j]->ipChannelName->Alloc();
                // Notify client that server session is connected
                // => Channel is ready to be used
                iIPCChannels[i].iConnectedClientSessions[j]->ServerSessionConnected(aConnectionId);
                return ETrue;
            }
        }
    }
    return EFalse;
}

void CQMFIPCChannelServer::DisconnectSessionFromChannel(CQMFIPCChannelServerSession* apSession)
{
    for (int i=0; i < iIPCChannels.Count(); i++) {
        for (int j=0; j < iIPCChannels[i].iConnectedClientSessions.Count(); j++) {
            if (iIPCChannels[i].iConnectedClientSessions[j] == apSession) {
                if (apSession->ipSession) {
                    apSession->ipSession->ipSession = NULL;
                }
                apSession->ipSession = NULL;
                iIPCChannels[i].iConnectedClientSessions.Remove(j);
                if (!iIPCChannels[i].active && (iIPCChannels[i].iConnectedClientSessions.Count() == 0)) {
                    delete iIPCChannels[i].channelName;
                    iIPCChannels[i].iConnectedClientSessions.Close();
                    iIPCChannels.Remove(i);
                }
                return;
            } else if (iIPCChannels[i].iConnectedClientSessions[j]->ipSession == apSession) {
                iIPCChannels[i].iConnectedClientSessions[j]->ipSession = NULL;
                apSession->ipSession = NULL;
                return;
            }
        }
    }
}

TUint CQMFIPCChannelServer::NewConnectionId()
{
    iConnectionIdCounter++;
    if (iConnectionIdCounter == KMaxTInt) {
        iConnectionIdCounter = 1;
    }
    return iConnectionIdCounter;
}

TInt CQMFIPCChannelServer::PeriodicTimerCallBack(TAny* /*aAny*/)
{
    CActiveScheduler::Stop();
    return KErrNone;
}

TInt CQMFIPCChannelServer::RunError(TInt aError)
{
    if (aError == KErrBadDescriptor) {
        PanicClient( Message(), EBadDescriptor );
    } else {
        Message().Complete(aError);
    }

    ReStart();

    return KErrNone;
}

void CQMFIPCChannelServer::PanicClient(const RMessage2& aMessage, TQMFIPCChannelServerPanic aPanic)
{
    aMessage.Panic(KQMFIPCChannelServer, aPanic);
}

void CQMFIPCChannelServer::PanicServer(TQMFIPCChannelServerPanic aPanic)
{
    User::Panic(KQMFIPCChannelServer, aPanic);
}

void CQMFIPCChannelServer::ThreadFunctionL()
{
    CActiveScheduler* pActiveScheduler = new (ELeave) CActiveScheduler;
    CleanupStack::PushL(pActiveScheduler);
    CActiveScheduler::Install(pActiveScheduler);

    CQMFIPCChannelServer* pMessageDataServer = CQMFIPCChannelServer::NewLC();

    RSemaphore semaphore;
    User::LeaveIfError(semaphore.OpenGlobal(KQMFIPCChannelServerSemaphoreName));
    semaphore.Signal(KMaxTInt); // Signal all waiting semaphore instances
    semaphore.Close();

    CActiveScheduler::Start();

    CleanupStack::PopAndDestroy(pMessageDataServer);
    CleanupStack::PopAndDestroy(pActiveScheduler);
}

TInt CQMFIPCChannelServer::ThreadFunction(TAny* /*aNone*/)
{
    CTrapCleanup* pCleanupStack = CTrapCleanup::New();
    if (!(pCleanupStack)) {
        PanicServer(ECreateTrapCleanup);
    }

    TRAPD(err, ThreadFunctionL());
    if (err != KErrNone) {
        PanicServer(ESrvCreateServer);
    }

    delete pCleanupStack;
    pCleanupStack = NULL;

    return KErrNone;
}

TInt E32Main()
{
    return CQMFIPCChannelServer::ThreadFunction(NULL);
}

// End of File

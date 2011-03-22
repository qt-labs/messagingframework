#include "qmfdataserver.h"
#include "qmfdataserversession.h"
#include <e32svr.h>

static _LIT_SECURITY_POLICY_C4(KQMFDataServerSecurityPolicy, ECapabilityReadUserData,
                                                             ECapabilityWriteUserData,
                                                             ECapabilityReadDeviceData,
                                                             ECapabilityWriteDeviceData);
const TInt KShutdownInterval(30000000); // 30 seconds

CQMFDataServer* CQMFDataServer::NewL()
{
    CQMFDataServer* pSelf = CQMFDataServer::NewLC();
    CleanupStack::Pop(pSelf);
    return pSelf;
}
CQMFDataServer* CQMFDataServer::NewLC()
{
    CQMFDataServer* pSelf = new (ELeave) CQMFDataServer(EPriorityNormal);
    CleanupStack::PushL(pSelf);
    pSelf->ConstructL();
    return pSelf;
}

void CQMFDataServer::ConstructL()
{
    StartL(KQMFDataServerName);
}

CQMFDataServer::CQMFDataServer(TInt aPriority)
    : CServer2(aPriority)
{
    if (iFs.Connect() == KErrNone) {
        iFs.ShareProtected();
        iFs.PrivatePath(iPrivatePath);
    }
}

CQMFDataServer::~CQMFDataServer()
{
    iFs.Close();
    if (ipShutdownTimer) {
        delete ipShutdownTimer;
        ipShutdownTimer = NULL;
    }
}

CSession2* CQMFDataServer::NewSessionL(const TVersion& aVersion, const RMessage2& aMessage) const
{
    // Check capabilities
    if (!KQMFDataServerSecurityPolicy().CheckPolicy(aMessage)) {
        User::Leave(KErrPermissionDenied);
    }

    // Check version
    if (!User::QueryVersionSupported(TVersion(KQMFDataServerMajorVersionNumber,
                                              KQMFDataServerMinorVersionNumber,
                                              KQMFDataServerBuildVersionNumber),
                                     aVersion)) {
        User::Leave(KErrNotSupported);
    }

    return CQMFDataServerSession::NewL(*const_cast<CQMFDataServer*> ( this ));
}

void CQMFDataServer::IncrementSessions()
{
    if (ipShutdownTimer && ipShutdownTimer->IsActive()) {
        ipShutdownTimer->Cancel();
    }
    iSessionCount++;
}

void CQMFDataServer::DecrementSessions()
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

TInt CQMFDataServer::PeriodicTimerCallBack(TAny* /*aAny*/)
{
    CActiveScheduler::Stop();
    return KErrNone;
}

RFs& CQMFDataServer::fileSession()
{
    return iFs;
}

TDesC& CQMFDataServer::privatePath()
{
    return iPrivatePath;
}

TInt CQMFDataServer::RunError(TInt aError)
{
    if (aError == KErrBadDescriptor) {
        PanicClient( Message(), EBadDescriptor );
    } else {
        Message().Complete(aError);
    }

    ReStart();

    return KErrNone;
}

void CQMFDataServer::PanicClient(const RMessage2& aMessage, TQMFDataServerPanic aPanic)
{
    aMessage.Panic(KQMFDataServer, aPanic);
}

void CQMFDataServer::PanicServer(TQMFDataServerPanic aPanic)
{
    User::Panic(KQMFDataServer, aPanic);
}

void CQMFDataServer::ThreadFunctionL()
{
    CActiveScheduler* pActiveScheduler = new (ELeave) CActiveScheduler;
    CleanupStack::PushL(pActiveScheduler);
    CActiveScheduler::Install(pActiveScheduler);

    CQMFDataServer* pMessageDataServer = CQMFDataServer::NewLC();

    RSemaphore semaphore;
    User::LeaveIfError(semaphore.OpenGlobal(KQMFDataServerSemaphoreName));
    semaphore.Signal();
    semaphore.Close();

    CActiveScheduler::Start();

    CleanupStack::PopAndDestroy(pMessageDataServer);
    CleanupStack::PopAndDestroy(pActiveScheduler);
}

TInt CQMFDataServer::ThreadFunction(TAny* /*aNone*/)
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
    return CQMFDataServer::ThreadFunction(NULL);
}

// End of File

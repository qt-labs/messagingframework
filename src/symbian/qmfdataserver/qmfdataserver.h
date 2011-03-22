#ifndef QMFDATASERVER_H
#define QMFDATASERVER_H

#include "qmfdataclientservercommon.h"
#include <e32base.h>
#include <f32file.h>

class CQMFDataServer : public CServer2
{
public :
    static CQMFDataServer* NewL();
    static CQMFDataServer* NewLC();
    virtual ~CQMFDataServer();

    static TInt ThreadFunction( TAny* aStarted );
    void IncrementSessions();
    void DecrementSessions();
    RFs& fileSession();
    TDesC& privatePath();

protected: // From CActive
    TInt RunError(TInt aError);

protected: // CPeriodic callback
    static TInt PeriodicTimerCallBack(TAny* aAny);

private: // Constructors and destructors
    CQMFDataServer(TInt aPriority);
    void ConstructL();

private:
    static void PanicClient(const RMessage2& aMessage, TQMFDataServerPanic aReason);
    static void PanicServer(TQMFDataServerPanic aPanic);
    static void ThreadFunctionL();
    void SendTimeToSessions();

private: // From CServer2
    CSession2* NewSessionL(const TVersion& aVersion, const RMessage2& aMessage) const;

private: // Data
    TInt iSessionCount;
    RFs iFs;
    CPeriodic* ipShutdownTimer;
    TBuf<KMaxPath> iPrivatePath;
};

#endif // QMFDATASERVER_H

// End of File

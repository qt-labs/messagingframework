#ifndef QMFIPCCHANNELSESSION_H
#define QMFIPCCHANNELSESSION_H

#include <e32base.h>

// Constants
// Number of message slots to reserve for this client server session.
// Only one asynchronous request can be outstanding
// and one synchronous request in progress.
const TUint KDefaultMessageSlots = 2;

const TUid KServerUid3 = { 0x2003A67B }; // Server UID

_LIT(KQMFIPCChannelServerFilename, "QMFIPCChannelServer");

#ifdef __WINS__
static const TUint KServerMinHeapSize =  0x1000;  //  4K
static const TUint KServerMaxHeapSize = 0x10000;  // 64K
#endif

class RQMFIPCChannelSession : public RSessionBase
{
public:
    RQMFIPCChannelSession();

    TInt Connect();
    TVersion Version() const;

    TBool CreateChannel(const TDesC& aChannelName);
    TBool WaitForIncomingConnection(TPckgBuf<TUint>& aNewConnectionIdPckgBuf,
                                    TRequestStatus& aStatus);
    TBool DestroyChannel(const TDesC& aChannelName);
    TBool ChannelExists(const TDesC& aChannelName);
    TBool ConnectClientToChannel(const TDesC& aChannelName,
                                 TPckgBuf<TUint>& aNewConnectionIdPckgBuf,
                                 TRequestStatus& aStatus);
    TBool ConnectServerToChannel(TUint aConnectionId);
    TBool ListenChannel(TPckgBuf<TInt>& aIncomingMessageSizePckgBug,
                        TRequestStatus& aStatus);
    TBool DisconnectFromChannel();
    TBool SendData(const TDesC8& aData);
    TBool ReceiveData(RBuf8& aData);

    void Cancel();
};

#endif // QMFIPCCHANNELSESSION

// End of File

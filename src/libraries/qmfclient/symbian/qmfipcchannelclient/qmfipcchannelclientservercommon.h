#ifndef QMFIPCCHANNELCLIENTSERVERCOMMON_H
#define QMFIPCCHANNELCLIENTSERVERCOMMON_H

#include <e32base.h>

_LIT(KQMFIPCChannelServer, "QMFIPCChannelServer");

enum TQMFIPCChannelServerPanic
{
    EBadRequest = 1,
    EBadDescriptor = 2,
    ESrvCreateServer = 3,
    EMainSchedulerError = 4,
    ECreateTrapCleanup = 5,
    ESrvSessCreateTimer = 6,
    EReqAlreadyPending = 7
};


// Constants
_LIT(KQMFIPCChannelServerName,"QMFIPCChannelServer");
_LIT(KQMFIPCChannelServerSemaphoreName, "QMFIPCChannelServerSemaphore");

const TUint KQMFIPCChannelServerMajorVersionNumber=0;
const TUint KQMFIPCChannelServerMinorVersionNumber=1;
const TUint KQMFIPCChannelServerBuildVersionNumber=1;

enum TQMFIPCChannelServerRequest
{
    EQMFIPCChannelServerRequestCreateChannel,
    EQMFIPCChannelServerRequestWaitForIncomingConnection,
    EQMFIPCChannelServerRequestDestroyChannel,
    EQMFIPCChannelServerRequestChannelExists,
    EQMFIPCChannelServerRequestConnectClientToChannel,
    EQMFIPCChannelServerRequestConnectServerToChannel,
    EQMFIPCChannelServerRequestListenChannel,
    EQMFIPCChannelServerRequestDisconnectFromChannel,
    EQMFIPCChannelServerRequestSendData,
    EQMFIPCChannelServerRequestReceiveData,
    EQMFIPCChannelServerRequestCancel
};

enum TQMFIPCChannelServerRequestComplete
{
    EQMFIPCChannelRequestNewChannelConnection = 1,
    EQMFIPCChannelRequestChannelConnected,
    EQMFIPCChannelRequestChannelNotFound,
    EQMFIPCChannelRequestDataAvailable,
    EQMFIPCChannelRequestChannelDisconnected,
    EQMFIPCChannelRequestCanceled
};


#endif // QMFIPCCHANNELCLIENTSERVERCOMMON_H

// End of File

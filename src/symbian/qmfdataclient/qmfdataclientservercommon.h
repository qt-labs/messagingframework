#ifndef QMFDATACLIENTSERVERCOMMON_H
#define QMFDATACLIENTSERVERCOMMON_H

#include <e32base.h>

_LIT(KQMFDataServer, "QMFDataServer");

enum TQMFDataServerPanic
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
_LIT(KQMFDataServerName,"QMFDataServer");
_LIT(KQMFDataServerSemaphoreName, "QMFDataServerSemaphore");

const TUint KQMFDataServerMajorVersionNumber=0;
const TUint KQMFDataServerMinorVersionNumber=1;
const TUint KQMFDataServerBuildVersionNumber=1;

enum TQMFDataServerRequest
{
    EQMFDataServerRequestCreateDatabase,
    EQMFDataServerRequestOpenFile,
    EQMFDataServerRequestOpenOrCreateFile,
    EQMFDataServerRequestFileExists,
    EQMFDataServerRequestRenameFile,
    EQMFDataServerRequestDirExists,
    EQMFDataServerRequestMakeDir,
    EQMFDataServerRequestRemoveFile,
    EQMFDataServerRequestRemoveDir,
    EQMFDataServerRequestRemovePath,
    EQMFDataServerRequestDirListingSize,
    EQMFDataServerRequestDirListing
};

#endif // QMFDATACLIENTSERVERCOMMON_H

// End of File

#include "qmfdatasession.h"
#include "qmfdataclientservercommon.h"

#include <f32file.h>

static TInt StartServer();
static TInt CreateServerProcess();

RQMFDataSession::RQMFDataSession()
    : RSessionBase(),
      iTimeBuffer(NULL, 0, 0)
{
}

TInt RQMFDataSession::Connect()
{
    TInt retVal = ::StartServer();
    if (retVal == KErrNone) {
        retVal = CreateSession(KQMFDataServerName, Version(), KDefaultMessageSlots);
        if (retVal == KErrServerTerminated) {
            retVal = ::StartServer();
            if (retVal == KErrNone) {
                retVal = CreateSession(KQMFDataServerName, Version(), KDefaultMessageSlots);
            }
        }
    }
    return retVal;
}

TVersion RQMFDataSession::Version() const
{
    return(TVersion(KQMFDataServerMajorVersionNumber,
                    KQMFDataServerMinorVersionNumber,
                    KQMFDataServerBuildVersionNumber));
}

TBool RQMFDataSession::CreateDatabase(const TDesC& aDatabaseName)
{
    TPckgBuf<TBool> resultPckgBuf(EFalse);
    TInt retVal = SendReceive(EQMFDataServerRequestCreateDatabase, TIpcArgs(&resultPckgBuf, &aDatabaseName));
    if (retVal < KErrNone) {
        return EFalse;
    }
    return resultPckgBuf();
}

TBool RQMFDataSession::OpenFile(RFile& aFile, const TDesC& aFilePath)
{
    TPckgBuf<TInt> fileHandleBuf;
    TInt fileServerHandle = SendReceive(EQMFDataServerRequestOpenFile, TIpcArgs(&fileHandleBuf, &aFilePath));
    if (fileServerHandle < KErrNone) {
        return EFalse;
    }
    TInt fileHandle = fileHandleBuf();
    if (aFile.AdoptFromServer(fileServerHandle, fileHandle) != KErrNone) {
        return EFalse;
    }
    return ETrue;
}

TBool RQMFDataSession::OpenOrCreateFile(RFile& aFile, const TDesC& aFilePath)
{
    TPckgBuf<TInt> fileHandleBuf;
    TInt fileServerHandle = SendReceive(EQMFDataServerRequestOpenOrCreateFile, TIpcArgs(&fileHandleBuf, &aFilePath));
    if (fileServerHandle < KErrNone) {
        return EFalse;
    }
    TInt fileHandle = fileHandleBuf();
    if (aFile.AdoptFromServer(fileServerHandle, fileHandle) != KErrNone) {
        return EFalse;
    }
    RFs fs;
    return ETrue;
}

TBool RQMFDataSession::FileExists(const TDesC& aFilePath)
{
    TPckgBuf<TBool> resultPckgBuf(EFalse);
    TInt retVal = SendReceive(EQMFDataServerRequestFileExists, TIpcArgs(&resultPckgBuf, &aFilePath));
    if (retVal < KErrNone) {
        return EFalse;
    }
    return resultPckgBuf();
}

TBool RQMFDataSession::RemoveFile(const TDesC& aFilePath)
{
    TPckgBuf<TBool> resultPckgBuf(EFalse);
    TInt retVal = SendReceive(EQMFDataServerRequestRemoveFile, TIpcArgs(&resultPckgBuf, &aFilePath));
    if (retVal < KErrNone) {
        return EFalse;
    }
    return resultPckgBuf();
}

TBool RQMFDataSession::RenameFile(const TDesC& aOldFilePath, const TDesC& aNewFilePath)
{
    TPckgBuf<TBool> resultPckgBuf(EFalse);
    TInt retVal = SendReceive(EQMFDataServerRequestRenameFile, TIpcArgs(&resultPckgBuf,
                                                                        &aOldFilePath,
                                                                        &aNewFilePath));
    if (retVal < KErrNone) {
        return EFalse;
    }
    return resultPckgBuf();
}

TBool RQMFDataSession::DirectoryExists(const TDesC& aPath)
{
    TPckgBuf<TBool> resultPckgBuf;
    TInt retVal = SendReceive(EQMFDataServerRequestDirExists, TIpcArgs(&resultPckgBuf, &aPath));
    if (retVal < KErrNone) {
        return EFalse;
    }
    return resultPckgBuf();
}

TBool RQMFDataSession::MakeDirectory(const TDesC& aPath)
{
    TPckgBuf<TBool> resultPckgBuf;
    TInt retVal = SendReceive(EQMFDataServerRequestMakeDir, TIpcArgs(&resultPckgBuf, &aPath));
    if (retVal < KErrNone) {
        return EFalse;
    }
    return resultPckgBuf();
}

TBool RQMFDataSession::RemoveDirectory(const TDesC& aPath)
{
    TPckgBuf<TBool> resultPckgBuf;
    TInt retVal = SendReceive(EQMFDataServerRequestRemoveDir, TIpcArgs(&resultPckgBuf, &aPath));
    if (retVal < KErrNone) {
        return EFalse;
    }
    return resultPckgBuf();
}

TBool RQMFDataSession::RemovePath(const TDesC& aPath)
{
    TPckgBuf<TBool> resultPckgBuf;
    TInt retVal = SendReceive(EQMFDataServerRequestRemovePath, TIpcArgs(&resultPckgBuf, &aPath));
    if (retVal < KErrNone) {
        return EFalse;
    }
    return resultPckgBuf();
}

TBool RQMFDataSession::DirectoryListing(const TDesC& aPath, RBuf& aDirectoryListing)
{
    TPckgBuf<TInt> resultPckgBuf;
    TInt retVal = SendReceive(EQMFDataServerRequestDirListingSize, TIpcArgs(&resultPckgBuf, &aPath));
    if (retVal < KErrNone) {
        return EFalse;
    }
    TRAPD(err, aDirectoryListing.CreateL(resultPckgBuf()));
    if (err != KErrNone) {
        return EFalse;
    }
    retVal = SendReceive(EQMFDataServerRequestDirListing, TIpcArgs(&aDirectoryListing));
    if (retVal < KErrNone) {
        aDirectoryListing.Close();
        return EFalse;
    }
    return ETrue;
}

static TInt StartServer()
{
    TInt retVal;

    TFindServer findQMFDataServer(KQMFDataServerName);
    TFullName name;

    retVal = findQMFDataServer.Next(name);
    if (retVal == KErrNone) {
        // Server is already running
        return KErrNone;
    }

    RSemaphore semaphore;
    retVal = semaphore.CreateGlobal(KQMFDataServerSemaphoreName, 0);
    if (retVal != KErrNone) {
        return  retVal;
    }

    retVal = CreateServerProcess();
    if (retVal != KErrNone ) {
        return  retVal;
    }

    semaphore.Wait();
    semaphore.Close();

    return KErrNone;
}

static TInt CreateServerProcess()
{
    TInt retVal;

    const TUidType serverUid(KNullUid, KNullUid, KServerUid3);

    RProcess server;
    retVal = server.Create(KQMFDataServerFilename, KNullDesC, serverUid);
    if (retVal != KErrNone) {
        return  retVal;
    }
    server.Resume();
    server.Close();

    return  KErrNone;
}

// End of File

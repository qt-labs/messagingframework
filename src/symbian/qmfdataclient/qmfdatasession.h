#ifndef QMFDATASESSION_H
#define QMFDATASESSION_H

#include <e32base.h>

// Constants
// Number of message slots to reserve for this client server session.
// In this example we can have one asynchronous request outstanding
// and one synchronous request in progress.
const TUint KDefaultMessageSlots = 2;

const TUid KServerUid3 = { 0x2003A67A }; // Server UID

_LIT(KQMFDataServerFilename, "QMFDataServer");

#ifdef __WINS__
static const TUint KServerMinHeapSize =  0x1000;  //  4K
static const TUint KServerMaxHeapSize = 0x10000;  // 64K
#endif

class RFile;

class RQMFDataSession : public RSessionBase
{
public:
    RQMFDataSession();
    TInt Connect();
    TVersion Version() const;
    TBool CreateDatabase(const TDesC& aDatabaseName);
    TBool OpenFile(RFile& aFile, const TDesC& aFilePath);
    TBool OpenOrCreateFile(RFile& aFile, const TDesC& aFilePath);
    TBool FileExists(const TDesC& aFilePath);
    TBool RemoveFile(const TDesC& aFilePath);
    TBool RenameFile(const TDesC& aOldFilePath, const TDesC& aNewFilePath);
    TBool DirectoryExists(const TDesC& aPath);
    TBool MakeDirectory(const TDesC& aPath);
    TBool RemoveDirectory(const TDesC& aPath);
    TBool RemovePath(const TDesC& aPath);
    TBool DirectoryListing(const TDesC& aPath, RBuf& aDirectoryListing);

private: // Data
    TPtr8 iTimeBuffer;
};

#endif // QMFDATASESSION

// End of File

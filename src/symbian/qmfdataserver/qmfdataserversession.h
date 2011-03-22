#ifndef QMFDATASERVERSESSION_H
#define QMFDATASERVERSESSION_H

#include <e32base.h>

class CQMFDataServer;

class CQMFDataServerSession : public CSession2
{
public:
    static CQMFDataServerSession* NewL(CQMFDataServer& aServer);
    static CQMFDataServerSession* NewLC(CQMFDataServer& aServer);
    virtual ~CQMFDataServerSession();

    void ServiceL( const RMessage2& aMessage );

private:
    CQMFDataServerSession(CQMFDataServer& aServer);
    void ConstructL();
    void PanicClient(const RMessagePtr2& aMessage, TInt aPanic) const;
    TBool CreatePrivateDirectory();

    void CreateDatabaseL(const RMessage2& aMessage);
    void OpenFileL(const RMessage2& aMessage);
    void OpenOrCreateFileL(const RMessage2& aMessage);
    void FileExistsL(const RMessage2& aMessage);
    void RemoveFileL(const RMessage2& aMessage);
    void RenameFileL(const RMessage2& aMessage);
    void DirExistsL(const RMessage2& aMessage);
    void MakeDirL(const RMessage2& aMessage);
    void RemoveDirL(const RMessage2& aMessage);
    void RemovePathL(const RMessage2& aMessage);
    void DirectoryListingSizeL(const RMessage2& aMessage);
    void DirectoryListingL(const RMessage2& aMessage);

private: // Data
    RBuf iDirectoryListing;
    RMessage2 iMessage;
    CQMFDataServer& iServer;
};

#endif // QMFDATASERVERSESSION_H

// End of File

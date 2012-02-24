/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qmfdataserversession.h"
#include "qmfdataserver.h"
#include "qmfdataclientservercommon.h"
#include <e32svr.h>
#include <sqldb.h>
#include <bautils.h>

_LIT(KQMFSecureDbNamePrefix, "[2003A67A]");

CQMFDataServerSession* CQMFDataServerSession::NewL(CQMFDataServer& aServer)
{
    CQMFDataServerSession* pSelf = CQMFDataServerSession::NewLC(aServer);
    CleanupStack::Pop(pSelf);
    return pSelf;
}

CQMFDataServerSession* CQMFDataServerSession::NewLC(CQMFDataServer& aServer)
{
    CQMFDataServerSession* pSelf = new (ELeave) CQMFDataServerSession(aServer);
    CleanupStack::PushL(pSelf);
    pSelf->ConstructL();
    return pSelf;
}

void CQMFDataServerSession::ConstructL()
{
    iServer.IncrementSessions();
}

CQMFDataServerSession::CQMFDataServerSession(CQMFDataServer& aServer)
    : iServer(aServer)
{
}

CQMFDataServerSession::~CQMFDataServerSession()
{
    iServer.DecrementSessions();
}

void CQMFDataServerSession::ServiceL(const RMessage2& aMessage)
{
    switch (aMessage.Function()) {
    case EQMFDataServerRequestCreateDatabase:
        CreateDatabaseL(aMessage);
        break;
    case EQMFDataServerRequestOpenFile:
        OpenFileL(aMessage);
        break;
    case EQMFDataServerRequestOpenOrCreateFile:
        OpenOrCreateFileL(aMessage);
        break;
    case EQMFDataServerRequestFileExists:
        FileExistsL(aMessage);
        break;
    case EQMFDataServerRequestRemoveFile:
        RemoveFileL(aMessage);
        break;
    case EQMFDataServerRequestRenameFile:
        RenameFileL(aMessage);
        break;
    case EQMFDataServerRequestDirExists:
        DirExistsL(aMessage);
        break;
    case EQMFDataServerRequestMakeDir:
        MakeDirL(aMessage);
        break;
    case EQMFDataServerRequestRemoveDir:
        RemoveDirL(aMessage);
        break;
    case EQMFDataServerRequestRemovePath:
        RemovePathL(aMessage);
        break;
    case EQMFDataServerRequestDirListingSize:
        DirectoryListingSizeL(aMessage);
        break;
    case EQMFDataServerRequestDirListing:
        DirectoryListingL(aMessage);
        break;
    default:
        PanicClient(aMessage, EBadRequest);
    }
}

TBool CQMFDataServerSession::CreatePrivateDirectory()
{
    TBool result = EFalse;

    TFileName processFileName = RProcess().FileName();
    TDriveUnit drive(processFileName);
    if (drive.operator TInt() == EDriveZ) {
        drive = EDriveC;
    }
    TChar driveChar;
    iServer.fileSession().DriveToChar(drive, driveChar);
    TPath privateDirPathIncludingDrive;
    privateDirPathIncludingDrive.Append(driveChar);
    privateDirPathIncludingDrive.Append(':');
    privateDirPathIncludingDrive.Append(iServer.privatePath());
    TInt err = iServer.fileSession().MkDir(privateDirPathIncludingDrive);
    if (err == KErrNone) {
        result = ETrue;
    }

    return result;
}

void CQMFDataServerSession::CreateDatabaseL(const RMessage2& aMessage)
{
    TSecurityPolicy defaultPolicy;
    RSqlSecurityPolicy securityPolicy;
    TInt retVal = securityPolicy.Create(defaultPolicy);
    if (retVal == KErrNone) {
        CleanupClosePushL(securityPolicy);

        TSecurityPolicy schemaPolicy(ECapabilityReadUserData, ECapabilityWriteUserData, ECapabilityReadDeviceData, ECapabilityWriteDeviceData);
        retVal = securityPolicy.SetDbPolicy(RSqlSecurityPolicy::ESchemaPolicy, schemaPolicy);
        if (retVal == KErrNone) {
            RBuf databaseName;
            databaseName.CreateL(aMessage.GetDesLengthL(1));
            CleanupClosePushL(databaseName);
            aMessage.ReadL(1, databaseName);
            RBuf secureDatabaseName;
            secureDatabaseName.CreateL(aMessage.GetDesLengthL(1)+KQMFSecureDbNamePrefix().Length());
            CleanupClosePushL(secureDatabaseName);
            secureDatabaseName.Append(KQMFSecureDbNamePrefix);
            secureDatabaseName.Append(databaseName);
            RSqlDatabase database;
            database.CreateL(secureDatabaseName, securityPolicy);
            CleanupStack::PopAndDestroy(&secureDatabaseName);
            CleanupStack::PopAndDestroy(&databaseName);
        }

        CleanupStack::PopAndDestroy(&securityPolicy);
    }

    aMessage.Complete(KErrNone);
}

void CQMFDataServerSession::OpenFileL(const RMessage2& aMessage)
{
    RBuf filePath;
    filePath.CreateL(aMessage.GetDesLengthL(1));
    CleanupClosePushL(filePath);
    aMessage.ReadL(1, filePath);

    RBuf privateFilePath;
    privateFilePath.CreateL(aMessage.GetDesLengthL(1)+iServer.privatePath().Length());
    privateFilePath.Append(iServer.privatePath());
    privateFilePath.Append(filePath);
    CleanupStack::PopAndDestroy(&filePath);
    CleanupClosePushL(privateFilePath);

    RFile file;
    User::LeaveIfError(file.Open(iServer.fileSession(), privateFilePath, EFileShareAny));
    CleanupClosePushL(file);
    User::LeaveIfError(file.TransferToClient(aMessage, 0));
    ASSERT(aMessage.IsNull());
    CleanupStack::PopAndDestroy(&file);

    CleanupStack::PopAndDestroy(&privateFilePath);
}

void CQMFDataServerSession::OpenOrCreateFileL(const RMessage2& aMessage)
{
    RBuf filePath;
    filePath.CreateL(aMessage.GetDesLengthL(1));
    CleanupClosePushL(filePath);
    aMessage.ReadL(1, filePath);

    RBuf privateFilePath;
    privateFilePath.CreateL(aMessage.GetDesLengthL(1)+iServer.privatePath().Length());
    privateFilePath.Append(iServer.privatePath());
    privateFilePath.Append(filePath);
    CleanupStack::PopAndDestroy(&filePath);
    CleanupClosePushL(privateFilePath);

    RFile file;
    if (file.Open(iServer.fileSession(), privateFilePath, EFileShareAny) != KErrNone) {
        User::LeaveIfError(file.Replace(iServer.fileSession(), privateFilePath, EFileShareAny));
    }
    CleanupClosePushL(file);
    User::LeaveIfError(file.TransferToClient(aMessage, 0));
    ASSERT(aMessage.IsNull());
    CleanupStack::PopAndDestroy(&file);

    CleanupStack::PopAndDestroy(&privateFilePath);
}

void CQMFDataServerSession::FileExistsL(const RMessage2& aMessage)
{
    RBuf filePath;
    filePath.CreateL(aMessage.GetDesLengthL(1));
    CleanupClosePushL(filePath);
    aMessage.ReadL(1, filePath);

    RBuf privateFilePath;
    privateFilePath.CreateL(aMessage.GetDesLengthL(1)+iServer.privatePath().Length());
    privateFilePath.Append(iServer.privatePath());
    privateFilePath.Append(filePath);
    CleanupStack::PopAndDestroy(&filePath);
    CleanupClosePushL(privateFilePath);

    TBool exists = BaflUtils::FileExists(iServer.fileSession(), privateFilePath);
    TPckg<TBool> resultPckg(exists);
    aMessage.WriteL(0, resultPckg);

    CleanupStack::PopAndDestroy(&privateFilePath);

    aMessage.Complete(KErrNone);
}

void CQMFDataServerSession::RemoveFileL(const RMessage2& aMessage)
{
    RBuf filePath;
    filePath.CreateL(aMessage.GetDesLengthL(1));
    CleanupClosePushL(filePath);
    aMessage.ReadL(1, filePath);

    RBuf privateFilePath;
    privateFilePath.CreateL(aMessage.GetDesLengthL(1)+iServer.privatePath().Length());
    privateFilePath.Append(iServer.privatePath());
    privateFilePath.Append(filePath);
    CleanupStack::PopAndDestroy(&filePath);
    CleanupClosePushL(privateFilePath);

    TBool result = false;
    if (iServer.fileSession().Delete(privateFilePath) == KErrNone) {
        result = true;
    }
    TPckg<TBool> resultPckg(result);
    aMessage.WriteL(0, resultPckg);

    CleanupStack::PopAndDestroy(&privateFilePath);

    aMessage.Complete(KErrNone);
}

void CQMFDataServerSession::RenameFileL(const RMessage2& aMessage)
{
    RBuf filePath;
    filePath.CreateL(aMessage.GetDesLengthL(1));
    CleanupClosePushL(filePath);
    aMessage.ReadL(1, filePath);

    RBuf privateFilePath1;
    privateFilePath1.CreateL(aMessage.GetDesLengthL(1)+iServer.privatePath().Length());
    privateFilePath1.Append(iServer.privatePath());
    privateFilePath1.Append(filePath);
    CleanupStack::PopAndDestroy(&filePath);
    CleanupClosePushL(privateFilePath1);

    filePath.CreateL(aMessage.GetDesLengthL(2));
    CleanupClosePushL(filePath);
    aMessage.ReadL(1, filePath);

    RBuf privateFilePath2;
    privateFilePath2.CreateL(aMessage.GetDesLengthL(2)+iServer.privatePath().Length());
    privateFilePath2.Append(iServer.privatePath());
    privateFilePath2.Append(filePath);
    CleanupStack::PopAndDestroy(&filePath);
    CleanupClosePushL(privateFilePath2);

    TBool result = false;
    if (iServer.fileSession().Rename(privateFilePath1, privateFilePath2) == KErrNone) {
        result = true;
    }
    TPckg<TBool> resultPckg(result);
    aMessage.WriteL(0, resultPckg);

    CleanupStack::PopAndDestroy(&privateFilePath2);
    CleanupStack::PopAndDestroy(&privateFilePath1);

    aMessage.Complete(KErrNone);
}

void CQMFDataServerSession::DirExistsL(const RMessage2& aMessage)
{
    RBuf dirPath;
    dirPath.CreateL(aMessage.GetDesLengthL(1));
    CleanupClosePushL(dirPath);
    aMessage.ReadL(1, dirPath);

    RBuf privateDirPath;
    privateDirPath.CreateL(aMessage.GetDesLengthL(1)+iServer.privatePath().Length());
    privateDirPath.Append(iServer.privatePath());
    privateDirPath.Append(dirPath);
    CleanupStack::PopAndDestroy(&dirPath);
    CleanupClosePushL(privateDirPath);

    TBool exists = BaflUtils::FolderExists(iServer.fileSession(), privateDirPath);
    TPckg<TBool> resultPckg(exists);
    aMessage.WriteL(0, resultPckg);

    CleanupStack::PopAndDestroy(&privateDirPath);

    aMessage.Complete(KErrNone);
}


void CQMFDataServerSession::MakeDirL(const RMessage2& aMessage)
{
    RBuf dirPath;
    dirPath.CreateL(aMessage.GetDesLengthL(1));
    CleanupClosePushL(dirPath);
    aMessage.ReadL(1, dirPath);

    RBuf privateDirPath;
    privateDirPath.CreateL(aMessage.GetDesLengthL(1)+iServer.privatePath().Length());
    privateDirPath.Append(iServer.privatePath());
    privateDirPath.Append(dirPath);
    CleanupStack::PopAndDestroy(&dirPath);
    CleanupClosePushL(privateDirPath);

    TBool result = EFalse;
    if (iServer.fileSession().MkDir(privateDirPath) == KErrNone) {
        result = ETrue;
    } else {
        if (CreatePrivateDirectory()) {
            if (iServer.fileSession().MkDir(privateDirPath) == KErrNone) {
                result = ETrue;
            }
        }
    }
    TPckg<TBool> resultPckg(result);
    aMessage.WriteL(0, resultPckg);

    CleanupStack::PopAndDestroy(&privateDirPath);

    aMessage.Complete(KErrNone);
}

void CQMFDataServerSession::RemoveDirL(const RMessage2& aMessage)
{
    RBuf dirPath;
    dirPath.CreateL(aMessage.GetDesLengthL(1));
    CleanupClosePushL(dirPath);
    aMessage.ReadL(1, dirPath);

    RBuf privateDirPath;
    privateDirPath.CreateL(aMessage.GetDesLengthL(1)+iServer.privatePath().Length());
    privateDirPath.Append(iServer.privatePath());
    privateDirPath.Append(dirPath);
    CleanupStack::PopAndDestroy(&dirPath);
    CleanupClosePushL(privateDirPath);

    TBool result = EFalse;
    if (iServer.fileSession().RmDir(privateDirPath) == KErrNone) {
        result = ETrue;
    }
    TPckg<TBool> resultPckg(result);
    aMessage.WriteL(0, resultPckg);

    CleanupStack::PopAndDestroy(&privateDirPath);

    aMessage.Complete(KErrNone);
}

void CQMFDataServerSession::RemovePathL(const RMessage2& aMessage)
{
    RBuf dirPath;
    dirPath.CreateL(aMessage.GetDesLengthL(1));
    CleanupClosePushL(dirPath);
    aMessage.ReadL(1, dirPath);

    RBuf privateDirPath;
    privateDirPath.CreateL(aMessage.GetDesLengthL(1)+iServer.privatePath().Length());
    privateDirPath.Append(iServer.privatePath());
    privateDirPath.Append(dirPath);
    CleanupStack::PopAndDestroy(&dirPath);
    CleanupClosePushL(privateDirPath);

    TBool result = EFalse;
    if (iServer.fileSession().RmDir(privateDirPath) == KErrNone) {
        result = ETrue;
    }
    TPckg<TBool> resultPckg(result);
    aMessage.WriteL(0, resultPckg);

    CleanupStack::PopAndDestroy(&privateDirPath);

    aMessage.Complete(KErrNone);
}

void CQMFDataServerSession::DirectoryListingSizeL(const RMessage2& aMessage)
{
    RBuf dirPath;
    dirPath.CreateL(aMessage.GetDesLengthL(1));
    CleanupClosePushL(dirPath);
    aMessage.ReadL(1, dirPath);

    RBuf privateDirPath;
    privateDirPath.CreateL(aMessage.GetDesLengthL(1)+iServer.privatePath().Length()+3);
    privateDirPath.Append(iServer.privatePath());
    privateDirPath.Append(dirPath);
    privateDirPath.Append(_L("*.*"));
    CleanupStack::PopAndDestroy(&dirPath);
    CleanupClosePushL(privateDirPath);

    CDir* pDirectoryListing;
    User::LeaveIfError(iServer.fileSession().GetDir(privateDirPath, KEntryAttMaskSupported,
                                                    ESortNone, pDirectoryListing));
    CleanupStack::PushL(pDirectoryListing);

    User::LeaveIfError(iDirectoryListing.Create(512));
    CleanupClosePushL(iDirectoryListing);
    for (TInt i=0; i < pDirectoryListing->Count(); i++) {
        if (iDirectoryListing.Length()+(*pDirectoryListing)[i].iName.Length()+1 > iDirectoryListing.MaxLength()) {
            User::LeaveIfError(iDirectoryListing.ReAlloc(iDirectoryListing.MaxLength()*2));
        }
        iDirectoryListing.Append((*pDirectoryListing)[i].iName);
        iDirectoryListing.Append(_L("/"));
    }
    CleanupStack::Pop(&iDirectoryListing);
    CleanupStack::PopAndDestroy(pDirectoryListing);
    CleanupStack::PopAndDestroy(&privateDirPath);

    TPckg<TInt> resultPckg(iDirectoryListing.Length());
    aMessage.WriteL(0, resultPckg);

    aMessage.Complete(KErrNone);
}

void CQMFDataServerSession::DirectoryListingL(const RMessage2& aMessage)
{
    aMessage.WriteL(0, iDirectoryListing);
    aMessage.Complete(KErrNone);
    iDirectoryListing.Close();
}

void CQMFDataServerSession::PanicClient(const RMessagePtr2& aMessage, TInt aPanic ) const
{
    aMessage.Panic(KQMFDataServer, aPanic);
}

// End of File

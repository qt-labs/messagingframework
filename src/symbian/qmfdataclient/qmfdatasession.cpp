/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

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

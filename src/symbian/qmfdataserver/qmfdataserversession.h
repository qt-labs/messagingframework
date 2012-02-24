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

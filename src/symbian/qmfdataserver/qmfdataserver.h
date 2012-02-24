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

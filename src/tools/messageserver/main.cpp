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

#include "messageserver.h"
#include <QCoreApplication>
#include <QDebug>
#include <qmailnamespace.h>
#include <qmaillog.h>
#include <qloggers.h>
#include <signal.h>

#if !defined(NO_SHUTDOWN_SIGNAL_HANDLING) && defined(Q_OS_UNIX) && !defined(Q_OS_SYMBIAN)

static void shutdown(int n)
{
    qMailLog(Messaging) << "Received signal" << n << ", shutting down.";
    QCoreApplication::exit();
}
#endif

#if defined(Q_OS_UNIX) && !defined(Q_OS_SYMBIAN)

static void recreateLoggers(int n)
{
    qMailLoggersRecreate();
    qMailLog(Messaging) << "Received signal" << n << ", logs recreated.";
}
#endif

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    // This is ~/.config/Nokia/Messageserver.conf
    qMailLoggersRecreate("Nokia", "Messageserver", "Msgsrv");

    if(QMail::fileLock("messageserver-instance.lock") == -1)
        qFatal("Could not get messageserver lock. Messageserver might already be running!");

    MessageServer server;

#if !defined(NO_SHUTDOWN_SIGNAL_HANDLING) && defined(Q_OS_UNIX) && !defined(Q_OS_SYMBIAN)
    signal(SIGINT, shutdown);
    signal(SIGTERM, shutdown);
#endif

#if defined(Q_OS_UNIX) && !defined(Q_OS_SYMBIAN)
    signal(SIGHUP,recreateLoggers);
#endif

    int exitCode = app.exec();

    return exitCode;
}


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

#include "messageserver.h"
#include <QCoreApplication>
#include <qmailnamespace.h>
#include <qmaillog.h>
#include <qloggers.h>
#include <signal.h>

#if !defined(NO_SHUTDOWN_SIGNAL_HANDLING) && defined(Q_OS_UNIX)

static void shutdown(int n)
{
    qMailLog(Messaging) << "Received signal" << n << ", shutting down.";
    QCoreApplication::exit();
}
#endif

#if defined(Q_OS_UNIX)

static void recreateLoggers(int n)
{
    qMailLoggersRecreate("QtProject", "Messageserver", "Msgsrv");
    qMailLog(Messaging) << "Received signal" << n << ", logs recreated.";
}
#endif

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    // This is ~/.config/QtProject/Messageserver.conf
    qMailLoggersRecreate("QtProject", "Messageserver", "Msgsrv");

    if(QMail::fileLock("messageserver-instance.lock") == -1)
        qFatal("Could not get messageserver lock. Messageserver might already be running!");

    MessageServer server;

#if !defined(NO_SHUTDOWN_SIGNAL_HANDLING) && defined(Q_OS_UNIX)
    signal(SIGINT, shutdown);
    signal(SIGTERM, shutdown);
#endif

#if defined(Q_OS_UNIX)
    signal(SIGHUP,recreateLoggers);
#endif

    int exitCode = app.exec();

    return exitCode;
}


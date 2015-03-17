/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
#include <stdlib.h>
#ifdef USE_HTML_PARSER
#include <QtGui>
#endif

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
#ifdef USE_HTML_PARSER
    // Need for html parsing by <QTextdocument> in qmailmessage.cpp, but don't need real UI
    setenv("QT_QPA_PLATFORM", "minimal", 1);
    QGuiApplication app(argc, argv);
#else
    QCoreApplication app(argc, argv);
#endif

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


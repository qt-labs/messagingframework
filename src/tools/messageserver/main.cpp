/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#include "messageserver.h"
#ifdef QMAIL_QTOPIA
#include <QtopiaApplication>

#ifdef SINGLE_EXEC
QTOPIA_ADD_APPLICATION(QTOPIA_TARGET,messageserver)
#define MAIN_FUNC main_messageserver
#else
#define MAIN_FUNC main
#endif

QSXE_APP_KEY
int MAIN_FUNC(int argc, char** argv)
{
    QSXE_SET_APP_KEY(argv[0])

    QtopiaApplication app(argc, argv);

    MessageServer server;

    app.registerRunningTask("daemon");
    return app.exec();
}

#else //QT VERSION

#include <QApplication>
#include <QDebug>
#include <qmailnamespace.h>

int main(int argc, char** argv)
{

    if(QMail::fileLock("messageserver-instance.lock") == -1)
        qFatal("Could not get messageserver lock. Messageserver might already be running!");

    QApplication app(argc, argv);

    MessageServer server;

    int exitCode = app.exec();

    return exitCode;
}

#endif


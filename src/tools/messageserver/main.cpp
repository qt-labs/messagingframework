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


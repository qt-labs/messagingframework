/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#include <QApplication>
#include <qmailnamespace.h>
#include "qtmailwindow.h"
#include <QIcon>

int main(int argc, char** argv)
{
    if(QMail::fileLock("qtmail-instance.lock") == -1)
        qFatal("Qtmail already running!\n");

    QApplication app(argc,argv);
    app.setWindowIcon(QIcon(":icon/qtmail"));

    QTMailWindow appWindow;
    appWindow.show();

    return app.exec();
}


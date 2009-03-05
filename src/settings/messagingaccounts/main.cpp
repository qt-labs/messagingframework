/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#include "accountsettings.h"
#include <QApplication>
#include <qmailnamespace.h>
#include <QIcon>

int main(int argc, char** argv)
{
    if(QMail::fileLock("messagingsettings-instance.lock") == -1)
        qFatal("MessagingSettings already running!\n");

    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":icon/messagingaccounts"));
    AccountSettings settings;
    settings.show();
    return app.exec();
}


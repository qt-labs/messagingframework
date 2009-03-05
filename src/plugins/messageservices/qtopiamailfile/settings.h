/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef SETTINGS_H
#define SETTINGS_H

#include "ui_settings.h"
#include <qmailmessageservice.h>

class QtopiamailfileSettings : public QMailMessageServiceEditor, private Ui::QtopiamailfileSettings
{
    Q_OBJECT

public:
    QtopiamailfileSettings();

    void displayConfiguration(const QMailAccount &account, const QMailAccountConfiguration &config);
    bool updateAccount(QMailAccount *account, QMailAccountConfiguration *config);
};

#endif


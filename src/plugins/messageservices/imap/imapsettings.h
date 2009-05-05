/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef IMAPSETTINGS_H
#define IMAPSETTINGS_H

#include "ui_imapsettings.h"
#include <qmailmessageservice.h>

class ImapSettings : public QMailMessageServiceEditor, private Ui::ImapSettings
{
    Q_OBJECT

public:
    ImapSettings();

    void displayConfiguration(const QMailAccount &account, const QMailAccountConfiguration &config);
    bool updateAccount(QMailAccount *account, QMailAccountConfiguration *config);

private slots:
    void intervalCheckChanged(int enabled);

private:
    bool warningEmitted;
};

#endif


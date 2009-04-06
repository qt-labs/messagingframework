/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef POPSETTINGS_H
#define POPSETTINGS_H

#include "ui_popsettings.h"
#include <qmailmessageservice.h>

class PopSettings : public QMailMessageServiceEditor, private Ui::PopSettings
{
    Q_OBJECT

public:
    PopSettings();

    void displayConfiguration(const QMailAccount &account, const QMailAccountConfiguration &config);
    bool updateAccount(QMailAccount *account, QMailAccountConfiguration *config);

private slots:
    void pushCheckChanged(int enabled);
    void intervalCheckChanged(int enabled);

private:
    bool warningEmitted;
};

#endif


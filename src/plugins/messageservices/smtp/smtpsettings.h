/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef SMTPSETTINGS_H
#define SMTPSETTINGS_H

#include "ui_smtpsettings.h"
#include <qmailmessageservice.h>

class QMailAccount;
class QMailAccountConfiguration;


class SmtpSettings : public QMailMessageServiceEditor, private Ui::SmtpSettings
{
    Q_OBJECT

public:
    SmtpSettings();

    virtual void displayConfiguration(const QMailAccount &account, const QMailAccountConfiguration &config);
    virtual bool updateAccount(QMailAccount *account, QMailAccountConfiguration *config);

private slots:
    void sigPressed();
    void emailModified();
    void authChanged(int index);

private:
    bool addressModified;
    QString signature;
};

#endif


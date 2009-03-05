/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#include "mmseditaccount.h"
#include <qmailaccount.h>
#include <qmailmessageservice.h>
#include <QVBoxLayout>

MmsEditAccount::MmsEditAccount(QWidget *parent)
    : QDialog(parent)
{
    setObjectName("mms-account");

    if (QMailMessageServiceConfigurator *service = QMailMessageServiceFactory::createServiceConfigurator("mms")) {
        editor = service->createEditor(QMailMessageServiceFactory::Source);
        if (editor) {
            QVBoxLayout *layout = new QVBoxLayout(this);
            layout->addWidget(editor);
        }
    }
}

void MmsEditAccount::setAccount(QMailAccount *acct, QMailAccountConfiguration *conf)
{
    account = acct;
    config = conf;

    if (editor) {
        editor->displayConfiguration(*account, *config);
    }
}

void MmsEditAccount::accept()
{
    if (editor) {
        editor->updateAccount(account, config);
    }

    QDialog::accept();
}


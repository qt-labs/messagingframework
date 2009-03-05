/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef MMSEDITACCOUNT_H
#define MMSEDITACCOUNT_H

#include <QDialog>

class QMailAccount;
class QMailAccountConfiguration;
class QMailMessageServiceEditor;

class MmsEditAccount : public QDialog
{
    Q_OBJECT

public:
    MmsEditAccount(QWidget *parent=0);

    void setAccount(QMailAccount *in, QMailAccountConfiguration* config);

protected slots:
    void accept();

private:
    QMailAccount *account;
    QMailAccountConfiguration *config;
    QMailMessageServiceEditor *editor;
};

#endif

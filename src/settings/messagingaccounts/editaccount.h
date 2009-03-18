/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef EDITACCOUNT_H
#define EDITACCOUNT_H

#include <QDialog>
#include <QMap>
#include <QValidator>
#include <qmailmessageservice.h>

class QMailAccount;
class QMailAccountConfiguration;
class QMailMessageServiceConfigurator;
class QMailMessageServiceEditor;
class QTabWidget;
class QCheckBox;
class QComboBox;
class QLineEdit;
class QVBoxLayout;

class EditAccount : public QDialog
{
    Q_OBJECT

public:
    EditAccount(QWidget* parent = 0, const char* name = 0, Qt::WFlags fl = 0);

    void setAccount(QMailAccount *in, QMailAccountConfiguration* config);

protected slots:
    void accept();
    void tabChanged(int index);
    void selectionChanged(int index);

private:
    static QMailMessageServiceFactory::ServiceType serviceType(int type);

    void selectService(int type, int index);
    void setServices(int type, const QStringList &services);

    QMailAccount *account;
    QMailAccountConfiguration *config;

    QLineEdit* accountNameInput;
    QCheckBox* enabledCheckbox;

    enum { Incoming = 0, Outgoing = 1, Storage = 2, ServiceTypesCount = 3 };

    QVBoxLayout* context[ServiceTypesCount];
    QComboBox* selector[ServiceTypesCount];
    QMailMessageServiceEditor* editor[ServiceTypesCount];
    QStringList extantKeys[ServiceTypesCount];
    QStringList availableKeys[ServiceTypesCount];

    bool effectingConstraints;

    QMap<QString, QMailMessageServiceConfigurator*> serviceMap;
};

#endif

/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Qt Software Information (qt-info@nokia.com)
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the either Technology Preview License Agreement or the
** Beta Release License Agreement.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
** $QT_END_LICENSE$
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

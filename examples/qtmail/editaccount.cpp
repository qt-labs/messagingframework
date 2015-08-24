/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "editaccount.h"
#include <QLineEdit>
#include <QCheckBox>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QComboBox>
#include <QFormLayout>
#include <QPushButton>
#include <QMessageBox>

EditAccount::EditAccount(QWidget* parent, const char* name, Qt::WindowFlags fl)
    : QDialog(parent, fl),
      account(0),
      accountNameInput(new QLineEdit),
      enabledCheckbox(new QCheckBox(tr("Enabled"))),
      effectingConstraints(false)
{
    setObjectName(name);
    setWindowTitle("Edit account");
    enabledCheckbox->setChecked(true);

    QTabWidget* tabWidget = new QTabWidget;
    connect(tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));

    QStringList header;
    header.append(tr("Incoming"));
    header.append(tr("Outgoing"));
    header.append(tr("Storage"));

    for (int i = 0; i < ServiceTypesCount; ++i) {
        context[i] = new QVBoxLayout;
#ifndef QMF_NO_MESSAGE_SERVICE_EDITOR
        editor[i] = 0;
#endif
        selector[i] = new QComboBox;

        QFormLayout* formLayout = new QFormLayout;
        formLayout->addRow(tr("Type"), selector[i]);
        context[i]->addLayout(formLayout);

        QWidget* tab = new QWidget;
        tab->setLayout(context[i]);

        tabWidget->addTab(tab, header[i]);
    }

    QFrame* separator = new QFrame;
    separator->setFrameStyle(QFrame::HLine);

    QFormLayout* formLayout = new QFormLayout;
    formLayout->setMargin(6);
    formLayout->setSpacing(4);
    formLayout->addRow(tr("Name"), accountNameInput);
    formLayout->addWidget(enabledCheckbox);

    QVBoxLayout* mainlayout = new QVBoxLayout(this);
    mainlayout->setMargin(0);
    mainlayout->setSpacing(4);
    mainlayout->addLayout(formLayout);
    mainlayout->addWidget(separator);
    mainlayout->addWidget(tabWidget);
    mainlayout->setStretchFactor(tabWidget,1);

    // Find all the email services available to us
    foreach (const QString &key, QMailMessageServiceFactory::keys(QMailMessageServiceFactory::Any)) {
        if (QMailMessageServiceFactory::supports(key, QMailMessage::Email)) {
            if (QMailMessageServiceConfigurator *configurator = QMailMessageServiceFactory::createServiceConfigurator(key)) {
                serviceMap.insert(key, configurator);

                for (int i = 0; i < ServiceTypesCount; ++i)
                    if (QMailMessageServiceFactory::supports(key, serviceType(i)))
                        extantKeys[i].append(key);
            }
        }
    }

    for (int i = 0; i < ServiceTypesCount; ++i) {
        setServices(i, extantKeys[i]);
        connect(selector[i], SIGNAL(currentIndexChanged(int)), this, SLOT(selectionChanged(int)));
    }

    accountNameInput->setFocus();

    tabChanged(0);

    QHBoxLayout* bottomLayout = new QHBoxLayout;
    bottomLayout->addStretch();
    QPushButton* okButton = new QPushButton("Ok");
    QPushButton* cancelButton = new QPushButton("Cancel");
    connect(okButton,SIGNAL(clicked()),this,SLOT(accept()));
    connect(cancelButton,SIGNAL(clicked()),this,SLOT(reject()));
    bottomLayout->addWidget(okButton);
    bottomLayout->addWidget(cancelButton);
    bottomLayout->setSpacing(4);
    bottomLayout->setMargin(6);
    mainlayout->addLayout(bottomLayout);
}

void EditAccount::setAccount(QMailAccount *in, QMailAccountConfiguration* conf)
{
    account = in;
    config = conf;

    if (account->id().isValid()) {
        accountNameInput->setText( account->name() );
        enabledCheckbox->setChecked( account->status() & QMailAccount::Enabled );

        int selection[3] = { -1, -1, 0 };

        foreach (const QString &service, config->services()) {
            int index = 0;
            if (account->status() & QMailAccount::MessageSource) {
                if ((index = availableKeys[Incoming].indexOf(service)) != -1)
                    selection[Incoming] = index + 1;
            }
            if (account->status() & QMailAccount::MessageSink) {
                if ((index = availableKeys[Outgoing].indexOf(service)) != -1)
                    selection[Outgoing] = index + 1;
            }
            if ((index = availableKeys[Storage].indexOf(service)) != -1)
                selection[Storage] = index;
        }

        for (int i = 0; i < ServiceTypesCount; ++i) {
            if (selection[i] != -1) {
                selector[i]->setCurrentIndex(selection[i]);
                if (selection[i] == 0)
                    selectService(i, 0);
            }
        }
    } else {
        setWindowTitle( tr("Create new account", "translation not longer than English") );

        // We have a default selection for Storage
        selectService(Storage, 0);
    }
}

void EditAccount::tabChanged(int index)
{
    // Change the name to select the relevant help page
    setObjectName(index == 0 ? "email-account-in" : (index == 1 ? "email-account-out" : "email-account-storage"));
}

void EditAccount::selectionChanged(int index)
{
    if (index != -1)
        if (const QObject *origin = sender())
            for (int i = 0; i < ServiceTypesCount; ++i)
                if (origin == selector[i]) {
                    selectService(i, index);
                    break;
                }
}

void EditAccount::accept()
{
    QString name(accountNameInput->text());
    if (name.trimmed().isEmpty()) {
        if (QMessageBox::question(0,
                                 tr("Empty account name"),
                                 tr("Do you want to quit and discard any changes?"),
                                 QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            reject();
        }
        return;
    }

    account->setName(name);
    account->setMessageType(QMailMessage::Email);
    account->setStatus(QMailAccount::Enabled, enabledCheckbox->isChecked());

    QStringList currentServices;

#ifndef QMF_NO_MESSAGE_SERVICE_EDITOR
    // Currently, we permit only one service of each type to be configured
    if (editor[Incoming]) {
        account->setStatus(QMailAccount::MessageSource, true);

        editor[Incoming]->updateAccount(account, config);
        currentServices.append((availableKeys[Incoming])[selector[Incoming]->currentIndex() - 1]);
    } else
#endif
    {
        account->setStatus(QMailAccount::MessageSource, false);
        account->setStatus(QMailAccount::CanRetrieve, false);
    }

#ifndef QMF_NO_MESSAGE_SERVICE_EDITOR
    if (editor[Outgoing]) {
        account->setStatus(QMailAccount::MessageSink, true);

        editor[Outgoing]->updateAccount(account, config);
        currentServices.append((availableKeys[Outgoing])[selector[Outgoing]->currentIndex() - 1]);
    } else
#endif
    {
        account->setStatus(QMailAccount::MessageSink, false);
        account->setStatus(QMailAccount::CanTransmit, false);
    }

#ifndef QMF_NO_MESSAGE_SERVICE_EDITOR
    if (editor[Storage]) {
        editor[Storage]->updateAccount(account, config);
        currentServices.append((availableKeys[Storage])[selector[Storage]->currentIndex()]);
    }
#endif

    foreach (const QString& service, config->services()) {
        if (!currentServices.contains(service)) 
            config->removeServiceConfiguration(service);
    }

    QDialog::accept();
}

QMailMessageServiceFactory::ServiceType EditAccount::serviceType(int type)
{
    if (type == Incoming)
        return QMailMessageServiceFactory::Source;
    if (type == Outgoing)
        return QMailMessageServiceFactory::Sink;

    return QMailMessageServiceFactory::Storage;
}

void EditAccount::selectService(int type, int index)
{
    const int offset = (type == Storage ? 0 : 1);
    const int mapIndex = index - offset;

    if (mapIndex >= availableKeys[type].count()) {
        qWarning() << "Invalid service index!";
        return;
    }

#ifndef QMF_NO_MESSAGE_SERVICE_EDITOR
    if (editor[type]) {
        // Save the current settings in case this service is reverted to
        editor[type]->updateAccount(account, config);

        editor[type]->hide();
        context[type]->removeWidget(editor[type]);
        delete editor[type];
        editor[type] = 0;
    }
#endif

    if ((index == 0) && (type != Storage)) {
        // Any previous constraints are now invalid
        for (int i = 0; i < ServiceTypesCount; ++i) {
            if (i != type) {
                // Return the selection options to the full configured set 
                QStringList permissibleServices = extantKeys[i];
                if (!permissibleServices.isEmpty() && !effectingConstraints) {
                    effectingConstraints = true;
                    setServices(i, permissibleServices);
                    effectingConstraints = false;
                }
            }
        }
    } else {
        if (QMailMessageServiceConfigurator *service = serviceMap[(availableKeys[type])[mapIndex]]) {
#ifndef QMF_NO_MESSAGE_SERVICE_EDITOR
            editor[type] = service->createEditor(serviceType(type));
#endif

            // Does this service constrains the other types allowable?
            for (int i = 0; i < ServiceTypesCount; ++i) {
                if (i != type) {
                    QStringList permissibleServices = service->serviceConstraints(serviceType(i));
                    if (!permissibleServices.isEmpty() && !effectingConstraints) {
                        effectingConstraints = true;
                        setServices(i, permissibleServices);
                        effectingConstraints = false;
                    }
                }
            }
        }
    }

#ifndef QMF_NO_MESSAGE_SERVICE_EDITOR
    if (editor[type]) {
        editor[type]->displayConfiguration(*account, *config);
        context[type]->addWidget(editor[type]);
    }
#endif
}

void EditAccount::setServices(int type, const QStringList &services)
{
    const int offset = (type == Storage ? 0 : 1);

    QString selection;
    if (selector[type]->currentIndex() > 0)
        selection = (availableKeys[type])[selector[type]->currentIndex() - offset];

    selector[type]->clear();
    if (type != Storage) {
        selector[type]->addItem(tr("None"));
    }

    // Set the containers to hold the new values
    availableKeys[type].clear();
    foreach (const QString &key, services) {
        if (QMailMessageServiceConfigurator *service = serviceMap[key]) {
            availableKeys[type].append(key);
            selector[type]->addItem(service->displayName());
        }
    }

    int index = availableKeys[type].indexOf(selection);
    if (index != -1) {
        // Restore the previous selection
        selector[type]->setCurrentIndex(index + offset);
    } else {
        selector[type]->setCurrentIndex(0);
    }
}


/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Nokia Corporation (qt-info@nokia.com)
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

#include "accountsettings.h"
#include "statusdisplay.h"
#include <qmaillog.h>
#include <QAction>
#include <QLayout>
#include <QMouseEvent>
#include <qmailaccount.h>
#include <qmailaccountlistmodel.h>
#include <qmailstore.h>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QTextDocument>
#include <QTimer>
#include <qmailipc.h>
#include <QItemDelegate>
#include <QListView>
#include <QMenuBar>
#include <QLineEdit>
#include <qmailserviceconfiguration.h>
#include <qmailmessageservice.h>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>

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

EditAccount::EditAccount(QWidget* parent, const char* name, Qt::WFlags fl)
    : QDialog(parent, fl),
      account(0),
      accountNameInput(new QLineEdit),
      enabledCheckbox(new QCheckBox(tr("Enabled"))),
      effectingConstraints(false)
{
    setObjectName(name);

    QTabWidget* tabWidget = new QTabWidget;
    connect(tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));

    QStringList header;
    header.append(tr("Incoming"));
    header.append(tr("Outgoing"));
    header.append(tr("Storage"));

    for (int i = 0; i < ServiceTypesCount; ++i) {
        context[i] = new QVBoxLayout;
        editor[i] = 0;
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
        if (QMessageBox::warning(0, 
                                 tr("Empty account name"), 
                                 tr("Do you want to quit and discard any changes?"), 
                                 QMessageBox::Yes, 
                                 QMessageBox::No|QMessageBox::Default|QMessageBox::Escape) == QMessageBox::Yes) {
            reject();
        }
        return;
    }

    account->setName(name);
    account->setMessageType(QMailMessage::Email);
    account->setStatus(QMailAccount::Enabled, enabledCheckbox->isChecked());

    QStringList currentServices;

    // Currently, we permit only one service of each type to be configured
    if (editor[Incoming]) {
        account->setStatus(QMailAccount::MessageSource, true);

        editor[Incoming]->updateAccount(account, config);
        currentServices.append((availableKeys[Incoming])[selector[Incoming]->currentIndex() - 1]);
    } else {
        account->setStatus(QMailAccount::MessageSource, false);
        account->setStatus(QMailAccount::CanRetrieve, false);
    }

    if (editor[Outgoing]) {
        account->setStatus(QMailAccount::MessageSink, true);

        editor[Outgoing]->updateAccount(account, config);
        currentServices.append((availableKeys[Outgoing])[selector[Outgoing]->currentIndex() - 1]);
    } else {
        account->setStatus(QMailAccount::MessageSink, false);
        account->setStatus(QMailAccount::CanTransmit, false);
    }

    if (editor[Storage]) {
        editor[Storage]->updateAccount(account, config);
        currentServices.append((availableKeys[Storage])[selector[Storage]->currentIndex()]);
    }

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

    if (editor[type]) {
        // Save the current settings in case this service is reverted to
        editor[type]->updateAccount(account, config);

        editor[type]->hide();
        context[type]->removeWidget(editor[type]);
        delete editor[type];
        editor[type] = 0;
    }

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
            editor[type] = service->createEditor(serviceType(type));

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

    if (editor[type]) {
        editor[type]->displayConfiguration(*account, *config);
        context[type]->addWidget(editor[type]);
    }
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

AccountSettings::AccountSettings(QWidget *parent, Qt::WFlags flags)
    : QDialog(parent, flags),
      preExisting(false),
      deleteBatchSize(0),
      deleteProgress(0)
{
    setWindowTitle(tr("Messaging Accounts"));
    QVBoxLayout *vb = new QVBoxLayout(this);
    vb->setContentsMargins(0, 0, 0, 0);

    QMenuBar* menu = new QMenuBar(this);
    context = menu->addMenu("File");
    vb->addWidget(menu);

    accountModel = new QMailAccountListModel();
    accountModel->setKey(QMailAccountKey::status(QMailAccount::UserEditable, QMailDataComparator::Includes));
    accountModel->setSortKey(QMailAccountSortKey::id(Qt::AscendingOrder));
    accountView = new QListView(this);

    accountView->setModel(accountModel);

    if (accountModel->rowCount())
        accountView->setCurrentIndex(accountModel->index(0, 0));
    else //no accounts so automatically add
        QTimer::singleShot(0,this,SLOT(addAccount()));
    vb->addWidget(accountView);

    addAccountAction = new QAction( QIcon(":icon/new"), tr("Add account..."), this );
    connect(addAccountAction, SIGNAL(triggered()), this, SLOT(addAccount()));
    context->addAction( addAccountAction );
    removeAccountAction = new QAction( QIcon(":icon/trash"), tr("Remove account"), this );
    connect(removeAccountAction, SIGNAL(triggered()), this, SLOT(removeAccount()));
    context->addAction(removeAccountAction);
    resetAccountAction = new QAction( tr("Reset account"), this );
    connect(resetAccountAction, SIGNAL(triggered()), this, SLOT(resetAccount()));
    context->addAction( resetAccountAction );

    QAction* exitAction = new QAction(tr("Close"), this );
    connect(exitAction, SIGNAL(triggered()), this, SLOT(close()));
    context->addAction(exitAction);

    statusDisplay = new StatusDisplay(this);
    statusDisplay->setVisible(false);
    vb->addWidget(statusDisplay);
    connect(accountView, SIGNAL(activated(QModelIndex)),
	    this, SLOT(accountSelected(QModelIndex)) );
    connect(context, SIGNAL(aboutToShow()), 
	    this, SLOT(updateActions()) );

    retrievalAction = new QMailRetrievalAction(this);
    connect(retrievalAction, SIGNAL(activityChanged(QMailServiceAction::Activity)), 
            this, SLOT(activityChanged(QMailServiceAction::Activity)));
    connect(retrievalAction, SIGNAL(progressChanged(uint, uint)), 
            this, SLOT(displayProgress(uint, uint)));

    transmitAction = new QMailTransmitAction(this);
    connect(transmitAction, SIGNAL(activityChanged(QMailServiceAction::Activity)), 
            this, SLOT(activityChanged(QMailServiceAction::Activity)));
    connect(transmitAction, SIGNAL(progressChanged(uint, uint)), 
            this, SLOT(displayProgress(uint, uint)));
}

void AccountSettings::addAccount()
{
    QMailAccount newAccount;
    editAccount(&newAccount);
}

void AccountSettings::showEvent(QShowEvent* e)
{
    accountModel->setSynchronizeEnabled(true);
    QDialog::showEvent(e);
}

void AccountSettings::hideEvent(QHideEvent* e)
{
    accountModel->setSynchronizeEnabled(false);
    QDialog::hideEvent(e);
}

void AccountSettings::removeAccount()
{
    QModelIndex index = accountView->currentIndex();
    if (!index.isValid())
      return;

    QMailAccount account(accountModel->idFromIndex(index));

    QString message = tr("Delete account:\n%1").arg(Qt::escape(account.name()));
    if (QMessageBox::warning( this, tr("Email"), message, tr("Yes"), tr("No"), 0, 0, 1 ) == 0) {
        // We could simply delete the account since QMailStore::deleteAccount
        // will remove all folders and messages, but for now we will remove the
        // messages manually so we can give progress indication (eventually, we
        // might add progress notification to QMailStore)

        // Number of messages required before we use a progress indicator
        static const int MinimumForProgressIndicator = 20;

        // Maximum messages processed per batch operation
        static const int MaxBatchSize = 50;
        static const int BatchMinimumForProgressIndicator = 2 * MaxBatchSize + 1;

        // Remove the messages and folders from this account (not just in the Inbox)
        QMailMessageKey messageKey(QMailMessageKey::parentAccountId(account.id()));
        const int count = QMailStore::instance()->countMessages(messageKey);
        if (count >= MinimumForProgressIndicator) {
            deleteMessageIds = QMailStore::instance()->queryMessages(messageKey);
            deleteProgress = 0;

            deleteBatchSize = 1;
            if (count >= BatchMinimumForProgressIndicator) {
                // Process this list in batches of roughly equivalent size
                int batchCount = (count / MaxBatchSize) + (count % MaxBatchSize ? 1 : 0);
                deleteBatchSize = ((count / batchCount) + (count % batchCount ? 1 : 0));
            }

            statusDisplay->displayProgress(0, count);
            statusDisplay->displayStatus(tr("Deleting messages"));
        } else {
            // No progress indication is required - allow the messages to be removed in account deletion
            deleteMessageIds = QMailMessageIdList();
        }

        deleteAccountId = account.id();
        QTimer::singleShot(0, this, SLOT(deleteMessages()));
    }
}

void AccountSettings::resetAccount()
{
    QModelIndex index = accountView->currentIndex();
    if (!index.isValid())
      return;

    QMailAccount account(accountModel->idFromIndex(index));

    QString message = tr("Reset account:\n%1").arg(Qt::escape(account.name()));
    if (QMessageBox::warning( this, tr("Email"), message, tr("Yes"), tr("No"), 0, 0, 1 ) == 0) {
        // Load the existing configuration
        QMailAccountConfiguration config(account.id());

        // Delete the account
        QMailStore::instance()->removeAccount(account.id());

        // Add the same account back
        QMailStore::instance()->addAccount(&account, &config);
        accountView->setCurrentIndex(accountModel->index(accountModel->rowCount() - 1, 0));

        QTimer::singleShot(0, this, SLOT(testConfiguration()));
    }
}

void AccountSettings::deleteMessages()
{
    if (!deleteMessageIds.isEmpty()) {
        // Process the next batch
        QMailMessageIdList batch(deleteMessageIds.mid(0, deleteBatchSize));
        deleteMessageIds = deleteMessageIds.mid(deleteBatchSize);

        QMailStore::instance()->removeMessages(QMailMessageKey::id(batch), QMailStore::CreateRemovalRecord);
        deleteProgress += batch.count();
        
        statusDisplay->displayProgress(deleteProgress, deleteProgress + deleteMessageIds.count());
        QTimer::singleShot(0, this, SLOT(deleteMessages()));
    } else {
        // Remove the account now
        QMailStore::instance()->removeAccount(deleteAccountId);

        statusDisplay->setVisible(false);
    }
}

void AccountSettings::accountSelected(QModelIndex index)
{
    if (!index.isValid())
      return;

    QMailAccount account(accountModel->idFromIndex(index));

    if (account.messageType() != QMailMessage::Sms)
        editAccount(&account);
}

void AccountSettings::updateActions()
{
    QModelIndex index = accountView->currentIndex();
    if (!index.isValid())
        return;

    QMailAccount account(accountModel->idFromIndex(index));
    removeAccountAction->setVisible(account.status() & QMailAccount::UserRemovable);
}

void AccountSettings::editAccount(QMailAccount *account)
{
    QMailAccountConfiguration config;
    if (account->id().isValid()) {
        config = QMailAccountConfiguration(account->id());
    } else {
        account->setStatus(QMailAccount::UserEditable, true);
        account->setStatus(QMailAccount::UserRemovable, true);
    }

    QDialog *editAccountView;
    bool wasPreferred(account->status() & QMailAccount::PreferredSender);

    EditAccount *e = new EditAccount(this, "EditAccount");
    e->setAccount(account, &config);
    editAccountView = e;

    editAccountView->setMinimumSize(QSize(400,400));
    int ret = editAccountView->exec();

    delete editAccountView;

    if (ret == QDialog::Accepted) {
        QMailAccountId previousPreferredId;
        if ((account->status() & QMailAccount::PreferredSender) && !wasPreferred) {
            // This account is now preferred - see if there is a predecessor that must be deselected
            QMailAccountKey preferredKey(QMailAccountKey::status(QMailAccount::PreferredSender, QMailDataComparator::Includes));
            QMailAccountKey typeKey(QMailAccountKey::messageType(account->messageType()));

            QMailAccountIdList previousIds = QMailStore::instance()->queryAccounts(preferredKey & typeKey);
            if (!previousIds.isEmpty())
                previousPreferredId = previousIds.first();
        }

        preExisting = account->id().isValid();
        if (preExisting) {
            QMailStore::instance()->updateAccount(account, &config);
        } else {
            QMailStore::instance()->addAccount(account, &config);
            accountView->setCurrentIndex(accountModel->index(accountModel->rowCount() - 1, 0));
        }

        if ((account->status() & QMailAccount::PreferredSender) && !wasPreferred) {
            if (previousPreferredId.isValid()) {
                QMailAccount previousAccount(previousPreferredId);
                previousAccount.setStatus(QMailAccount::PreferredSender, false);
                QMailStore::instance()->updateAccount(&previousAccount);

                QMessageBox::warning(this,
                                     tr("New default account"),
                                     tr("<qt>Your previous default mail account has been unchecked</qt>"),
                                     QMessageBox::Ok);
            }
        }

        QTimer::singleShot(0, this, SLOT(testConfiguration()));
    }
}

void AccountSettings::testConfiguration()
{
    QModelIndex index(accountView->currentIndex());
    if (index.isValid()) {
        QMailAccountId id(accountModel->idFromIndex(index));
        
        QMailAccount account(id);
        if (account.status() & (QMailAccount::MessageSource | QMailAccount::MessageSink)) {
            // See if the user wants to test the configuration for this account
            if (QMessageBox::question(this,
                                      preExisting ? tr("Account Modified") : tr("Account Added"),
                                      tr("Do you wish to test the configuration for this account?"),
                                      QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
                statusDisplay->setVisible(true);
                statusDisplay->displayStatus(tr("Testing configuration..."));

                if (account.status() & QMailAccount::MessageSource) {
                    retrievalAction->retrieveFolderList(id, QMailFolderId(), true);
                } else if (account.status() & QMailAccount::MessageSink) {
                    transmitAction->transmitMessages(id);
                }
            }
        }
    }
}

void AccountSettings::activityChanged(QMailServiceAction::Activity activity)
{
    if (sender() == static_cast<QObject*>(retrievalAction)) {
        const QMailServiceAction::Status status(retrievalAction->status());
        if (status.accountId.isValid()) {
            QMailAccount account(status.accountId);

            if (activity == QMailServiceAction::Successful) {
                if (account.status() & QMailAccount::MessageSink) {
                    transmitAction->transmitMessages(account.id());
                } else {
                    statusDisplay->displayStatus(tr("Configuration tested."));
                }
            } else if (activity == QMailServiceAction::Failed) {
                QString caption(tr("Retrieve Failure"));
                QString action(tr("%1 - Error retrieving folders: %2", "%1: account name, %2: error text"));

                action = action.arg(account.name()).arg(status.text);

                qMailLog(Messaging) << "retrieveFolders failed:" << action;
                statusDisplay->setVisible(false);
                QMessageBox::warning(0, caption, action, QMessageBox::Ok);
            }
        }
    } else if (sender() == static_cast<QObject*>(transmitAction)) {
        if (activity == QMailServiceAction::Successful) {
            statusDisplay->displayStatus(tr("Configuration tested."));
        } else if (activity == QMailServiceAction::Failed) {
            const QMailServiceAction::Status status(transmitAction->status());
            QMailAccount account(status.accountId);

            QString caption(tr("Transmission Failure"));
            QString action(tr("%1 - Error testing connection: %2", "%1: account name, %2: error text"));

            action = action.arg(account.name()).arg(status.text);

            qMailLog(Messaging) << "transmitMessages failed:" << action;
            statusDisplay->setVisible(false);
            QMessageBox::warning(0, caption, action, QMessageBox::Ok);
        }
    }
}

void AccountSettings::displayProgress(uint value, uint range)
{
    if (statusDisplay->isVisible()) {
        statusDisplay->displayProgress(value, range);
    }
}

#include <accountsettings.moc>

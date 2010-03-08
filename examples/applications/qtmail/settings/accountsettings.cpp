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
#include "editaccount.h"
#include "../statusdisplay.h"
#include <qmailaccountlistmodel.h>
#include <qmaillog.h>
#include <QVBoxLayout>
#include <QMenuBar>
#include <QListView>
#include <QTimer>
#include <QMessageBox>
#include <QRegExp>
#include <QToolButton>

AccountSettings::AccountSettings(QWidget *parent, Qt::WFlags flags)
    : QDialog(parent, flags),
      preExisting(false),
      deleteBatchSize(0),
      deleteProgress(0)
{
    setWindowTitle(tr("Accounts"));
    QVBoxLayout *vb = new QVBoxLayout(this);
    vb->setContentsMargins(0, 0, 0, 0);

    QMenuBar* menu = new QMenuBar(this);
    context = menu->addMenu("File");
    vb->addWidget(menu);

    accountModel = new QMailAccountListModel();
    accountModel->setKey(QMailAccountKey::status(QMailAccount::UserEditable, QMailDataComparator::Includes));
    accountModel->setSortKey(QMailAccountSortKey::id(Qt::AscendingOrder));
    connect(accountModel,SIGNAL(rowsInserted(QModelIndex,int,int)),this,SLOT(updateActions()));
    connect(accountModel,SIGNAL(rowsRemoved(QModelIndex,int,int)),this,SLOT(updateActions()));
    accountView = new QListView(this);

    accountView->setModel(accountModel);

    if (accountModel->rowCount())
        accountView->setCurrentIndex(accountModel->index(0, 0));
    else //no accounts so automatically add
        QTimer::singleShot(0,this,SLOT(addAccount()));
    vb->addWidget(accountView);

    addAccountAction = new QAction( QIcon(":icon/add"), tr("Add"), this );
    connect(addAccountAction, SIGNAL(triggered()), this, SLOT(addAccount()));
    context->addAction( addAccountAction );

    editAccountAction = new QAction(QIcon(":icon/settings"),tr("Edit"), this);
    connect(editAccountAction,SIGNAL(triggered()),this,SLOT(editCurrentAccount()));
    context->addAction( editAccountAction );

    removeAccountAction = new QAction( QIcon(":icon/erase"), tr("Remove"), this );
    connect(removeAccountAction, SIGNAL(triggered()), this, SLOT(removeAccount()));
    context->addAction(removeAccountAction);

    resetAccountAction = new QAction( tr("Reset"), this );
    connect(resetAccountAction, SIGNAL(triggered()), this, SLOT(resetAccount()));
    context->addAction( resetAccountAction );

    QAction* exitAction = new QAction(tr("Close"), this );
    connect(exitAction, SIGNAL(triggered()), this, SLOT(close()));
    context->addAction(exitAction);

    statusDisplay = new StatusDisplay(this);
    statusDisplay->setVisible(false);

    QWidget *buttonHolder = new QWidget(this);
    QHBoxLayout* buttonsLayout = new QHBoxLayout(buttonHolder);

    QToolButton* addButton = new QToolButton(this);
    addButton->setDefaultAction(addAccountAction);
    buttonsLayout->addWidget(addButton);

    QToolButton* editButton = new QToolButton(this);
    editButton->setDefaultAction(editAccountAction);
    buttonsLayout->addWidget(editButton);

    QToolButton* removeButton = new QToolButton(this);
    removeButton->setDefaultAction(removeAccountAction);
    buttonsLayout->addWidget(removeButton);

    buttonsLayout->addStretch();

    vb->addWidget(buttonHolder);

    vb->addWidget(statusDisplay);
    connect(accountView, SIGNAL(activated(QModelIndex)),
	    this, SLOT(editCurrentAccount()));
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

    QString message = tr("Remove %1?").arg(account.name());
    if (QMessageBox::question( this, tr("Remove account"), message, QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
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
            statusDisplay->displayStatus(tr("Removing messages"));
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

    QString message = tr("Reset %1?").arg(account.name());
    if (QMessageBox::question( this, tr("Reset account"), message, QMessageBox::Yes | QMessageBox::No,QMessageBox::No) == QMessageBox::Yes) {
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

void AccountSettings::updateActions()
{
    bool haveAccounts(accountModel->rowCount() > 0);
    bool removable(true);
    bool editable(true);

    QModelIndex index = accountView->currentIndex();
    if (index.isValid())
    {
        QMailAccount account(accountModel->idFromIndex(index));
        removable = account.status() & QMailAccount::UserRemovable;
        editable = account.status() & QMailAccount::UserEditable;
    }

    removeAccountAction->setEnabled( removable && haveAccounts);
    editAccountAction->setEnabled( editable && haveAccounts);
    resetAccountAction->setEnabled( editable && haveAccounts);
}

void AccountSettings::editCurrentAccount()
{
    if(!accountModel->rowCount())
        return;

    QModelIndex index = accountView->currentIndex();
    if (!index.isValid())
        return;

    QMailAccount account(accountModel->idFromIndex(index));
    //if (account.messageType() != QMailMessage::Sms)
    editAccount(&account);
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


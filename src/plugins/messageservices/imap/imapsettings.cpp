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

#include "imapsettings.h"
#include "imapconfiguration.h"
#include <emailfoldermodel.h>
#include <qmailaccount.h>
#include <qmailaccountconfiguration.h>
#include <qmailtransport.h>
#include <selectfolder.h>
#include <QLineEdit>
#include <QMessageBox>

namespace {

const QString serviceKey("imap4"); 

class PortValidator : public QValidator
{
public:
    PortValidator(QWidget *parent = 0, const char *name = 0);

    QValidator::State validate(QString &str, int &) const;
};

PortValidator::PortValidator(QWidget *parent, const char *name)
    : QValidator(parent) 
{
    setObjectName(name);
}

QValidator::State PortValidator::validate(QString &str, int &) const
{
    // allow empty strings, as it's a bit awkward to edit otherwise
    if ( str.isEmpty() )
        return QValidator::Acceptable;

    bool ok = false;
    int i = str.toInt(&ok);
    if ( !ok )
        return QValidator::Invalid;

    if ( i <= 0 || i >= 65536 )
        return QValidator::Invalid;

    return QValidator::Acceptable;
}

}


ImapSettings::ImapSettings()
    : QMailMessageServiceEditor(),
      warningEmitted(false)
{
    setupUi(this);
    setLayoutDirection(qApp->layoutDirection());

    connect(intervalCheckBox, SIGNAL(stateChanged(int)), this, SLOT(intervalCheckChanged(int)));

    const QString uncapitalised("email noautocapitalization");

    // These fields should not be autocapitalised
    mailPortInput->setValidator(new PortValidator(this));

    mailPasswInput->setEchoMode(QLineEdit::PasswordEchoOnEdit);

    // This functionality is not currently used:
    mailboxButton->hide();

#ifdef QT_NO_OPENSSL
    encryptionIncoming->hide();
    lblEncryptionIncoming->hide();
#endif

    connect(draftsButton, SIGNAL(clicked()), this, SLOT(selectFolder()));
    connect(sentButton, SIGNAL(clicked()), this, SLOT(selectFolder()));
    connect(trashButton, SIGNAL(clicked()), this, SLOT(selectFolder()));
}

void ImapSettings::intervalCheckChanged(int enabled)
{
    intervalPeriod->setEnabled(enabled);
    roamingCheckBox->setEnabled(enabled);
}

void ImapSettings::selectFolder()
{
    AccountFolderModel model(accountId, this);
    model.init();

    // The account itself is not a selectable folder
    QList<QMailMessageSet*> invalidItems;
    invalidItems.append(model.itemFromIndex(model.indexFromAccountId(accountId)));

    SelectFolderDialog selectFolderDialog(&model);
    selectFolderDialog.setInvalidSelections(invalidItems);
    selectFolderDialog.exec();

    if (selectFolderDialog.result() == QDialog::Accepted) {
        QMailFolder folder(model.folderIdFromIndex(model.indexFromItem(selectFolderDialog.selectedItem())));

        if (sender() == static_cast<QObject*>(draftsButton)) {
            imapDraftsDir->setText(folder.path());
        } else if (sender() == static_cast<QObject*>(sentButton)) {
            imapSentDir->setText(folder.path());
        } else if (sender() == static_cast<QObject*>(trashButton)) {
            imapTrashDir->setText(folder.path());
        }
    }
}

void ImapSettings::displayConfiguration(const QMailAccount &account, const QMailAccountConfiguration &config)
{
    accountId = account.id();
    bool hasFolders(false);
    if (accountId.isValid()) {
        hasFolders = (QMailStore::instance()->countFolders(QMailFolderKey::parentAccountId(accountId)) > 0);
    }

    // Only allow the base folder to be specified before retrieval occurs
    baseFolderLabel->setEnabled(!hasFolders);
    imapBaseDir->setEnabled(!hasFolders);

    // Only allow the other folders to be specified after we have a folder listing
    draftsFolderLabel->setEnabled(hasFolders);
    draftsButton->setEnabled(hasFolders);
    imapDraftsDir->setEnabled(hasFolders);
    imapDraftsDir->setReadOnly(true);

    sentFolderLabel->setEnabled(hasFolders);
    sentButton->setEnabled(hasFolders);
    imapSentDir->setEnabled(hasFolders);
    imapSentDir->setReadOnly(true);

    trashFolderLabel->setEnabled(hasFolders);
    trashButton->setEnabled(hasFolders);
    imapTrashDir->setEnabled(hasFolders);
    imapTrashDir->setReadOnly(true);

    if (!config.services().contains(serviceKey)) {
        // New account
        mailUserInput->setText("");
        mailPasswInput->setText("");
        mailServerInput->setText("");
        mailPortInput->setText("143");
#ifndef QT_NO_OPENSSL
        encryptionIncoming->setCurrentIndex(0);
#endif
        preferHtml->setChecked(true);
        pushCheckBox->setChecked(false);
        intervalCheckBox->setChecked(false);
        roamingCheckBox->setChecked(false);
    } else {
        ImapConfiguration imapConfig(config);

        mailUserInput->setText(imapConfig.mailUserName());
        mailPasswInput->setText(imapConfig.mailPassword());
        mailServerInput->setText(imapConfig.mailServer());
        mailPortInput->setText(QString::number(imapConfig.mailPort()));
#ifndef QT_NO_OPENSSL
        encryptionIncoming->setCurrentIndex(static_cast<int>(imapConfig.mailEncryption()));
#endif
        deleteCheckBox->setChecked(imapConfig.canDeleteMail());
        maxSize->setValue(imapConfig.maxMailSize());
        thresholdCheckBox->setChecked(imapConfig.maxMailSize() != -1);
        preferHtml->setChecked(imapConfig.preferredTextSubtype() == "html");
        pushCheckBox->setChecked(imapConfig.pushEnabled());
        intervalCheckBox->setChecked(imapConfig.checkInterval() > 0);
        intervalPeriod->setValue(qAbs(imapConfig.checkInterval()));
        roamingCheckBox->setChecked(!imapConfig.intervalCheckRoamingEnabled());
        imapBaseDir->setText(imapConfig.baseFolder());
        imapDraftsDir->setText(imapConfig.draftsFolder());
        imapSentDir->setText(imapConfig.sentFolder());
        imapTrashDir->setText(imapConfig.trashFolder());
    }
}

bool ImapSettings::updateAccount(QMailAccount *account, QMailAccountConfiguration *config)
{
    bool result;
    int port = mailPortInput->text().toInt(&result);
    if ( (!result) ) {
        // should only happen when the string is empty, since we use a validator.
        port = -1;
    }

    if (!config->services().contains(serviceKey))
        config->addServiceConfiguration(serviceKey);

    ImapConfigurationEditor imapConfig(config);

    imapConfig.setVersion(100);
    imapConfig.setType(QMailServiceConfiguration::Source);

    imapConfig.setMailUserName(mailUserInput->text());
    imapConfig.setMailPassword(mailPasswInput->text());
    imapConfig.setMailServer(mailServerInput->text());
    imapConfig.setMailPort(port == -1 ? 143 : port);
#ifndef QT_NO_OPENSSL
    imapConfig.setMailEncryption(static_cast<QMailTransport::EncryptType>(encryptionIncoming->currentIndex()));
#endif
    imapConfig.setDeleteMail(deleteCheckBox->isChecked());
    imapConfig.setMaxMailSize(thresholdCheckBox->isChecked() ? maxSize->value() : -1);
    imapConfig.setPreferredTextSubtype(preferHtml->isChecked() ? "html" : "plain");
    imapConfig.setAutoDownload(false);
    imapConfig.setPushEnabled(pushCheckBox->isChecked());
    imapConfig.setCheckInterval(intervalPeriod->value() * (intervalCheckBox->isChecked() ? 1 : -1));
    imapConfig.setIntervalCheckRoamingEnabled(!roamingCheckBox->isChecked());
    imapConfig.setBaseFolder(imapBaseDir->text());
    imapConfig.setDraftsFolder(imapDraftsDir->text());
    imapConfig.setSentFolder(imapSentDir->text());
    imapConfig.setTrashFolder(imapTrashDir->text());

    // Do we have a configuration we can use?
    if (!imapConfig.mailServer().isEmpty() && !imapConfig.mailUserName().isEmpty())
        account->setStatus(QMailAccount::CanRetrieve, true);

    return true;
}


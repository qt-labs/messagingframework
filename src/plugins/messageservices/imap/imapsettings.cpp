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

#include "imapsettings.h"
#include "imapconfiguration.h"
#include <emailfoldermodel.h>
#include <qmailaccount.h>
#include <qmailaccountconfiguration.h>
#include <qmailtransport.h>
#include <qmailnamespace.h>
#include <selectfolder.h>
#include <QLineEdit>
#include <QMessageBox>

namespace {

const QString serviceKey("imap4");

class PortValidator : public QValidator
{
public:
    PortValidator(QWidget *parent = Q_NULLPTR, const char *name = Q_NULLPTR);

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

class PushFolderList : public QObject {
    Q_OBJECT
    
public:
    PushFolderList(QWidget *parent, QGridLayout *parentLayout);
    void setAccountId(const QMailAccountId &id);
    void addRow(const QString &s);
    void populate(const QStringList &pushFolderNames);
    QStringList folderNames();
                                             
public slots:
    void setHasFolders(bool hasFolders);
    void setPushEnabled(int pushEnabled);
    void selectFolder();

private:
    QWidget *_parent;
    QGridLayout *_parentLayout;
    QMailAccountId _accountId;
    bool _hasFolders;
    bool _pushEnabled;
    int _startRow;
    int _items;
    QList<QWidget*> _widgets;
    QList<QHBoxLayout*> _layouts;
    QList<QLineEdit*> _dirTexts;
    QList<QToolButton*> _clearButtons;
    QList<QToolButton*> _dirButtons;
};

PushFolderList::PushFolderList(QWidget *parent, QGridLayout *parentLayout)
    :QObject(parent),
     _parent(parent),
     _parentLayout(parentLayout),
     _hasFolders(false),
     _pushEnabled(false),
     _startRow(parentLayout->rowCount()),
     _items(0)
{
}

void PushFolderList::setAccountId(const QMailAccountId &id)
{
    _accountId = id;
}

void PushFolderList::setHasFolders(bool hasFolders)
{
    _hasFolders = hasFolders;
    foreach(QWidget *widget, _widgets)
        widget->setEnabled(_hasFolders && _pushEnabled);
}

void PushFolderList::setPushEnabled(int pushEnabled)
{
    _pushEnabled = (pushEnabled != Qt::Unchecked);
    foreach(QWidget *widget, _widgets)
        widget->setEnabled(_hasFolders && _pushEnabled);
}

void PushFolderList::addRow(const QString &s)
{
    QIcon clearIcon(":icon/clear_left");
    QLabel *pushLabel = new QLabel(tr("Push folder"), _parent);
    QHBoxLayout *layout = new QHBoxLayout();
    QLineEdit *pushDir = new QLineEdit(_parent);
    QToolButton *clearPushEmailButton = new QToolButton(_parent);
    QToolButton *choosePushDirButton = new QToolButton(_parent);
    pushDir->setReadOnly(true);
    pushDir->setFocusPolicy(Qt::NoFocus);
    pushDir->setText(s);
    clearPushEmailButton->setIcon(clearIcon);
    clearPushEmailButton->setEnabled(!s.isEmpty());
    choosePushDirButton->setText(tr("..."));
    pushLabel->setEnabled(_hasFolders && _pushEnabled);
    pushDir->setEnabled(_hasFolders && _pushEnabled);
    clearPushEmailButton->setEnabled(_hasFolders && _pushEnabled);
    choosePushDirButton->setEnabled(_hasFolders && _pushEnabled);
    connect(clearPushEmailButton, SIGNAL(clicked()), pushDir, SLOT(clear()));
    connect(choosePushDirButton, SIGNAL(clicked()), this, SLOT(selectFolder()));
    _dirTexts.append(pushDir);
    _clearButtons.append(clearPushEmailButton);
    _dirButtons.append(choosePushDirButton);
    _layouts.append(layout);
    _widgets.append(pushLabel);
    _widgets.append(pushDir);
    _widgets.append(clearPushEmailButton);
    _widgets.append(choosePushDirButton);
    layout->addWidget(pushDir);
    layout->addWidget(clearPushEmailButton);
    layout->addWidget(choosePushDirButton);
    _parentLayout->addWidget(pushLabel, _startRow + _items, 0);
    _parentLayout->addLayout(layout, _startRow + _items, 1);
    ++_items;
}

void PushFolderList::populate(const QStringList &pushFolderNames)
{
    _items = 0;
    foreach(QWidget *widget, _widgets) {
        _parentLayout->removeWidget(widget);
        delete widget;
    }
    foreach(QHBoxLayout *layout, _layouts) {
        _parentLayout->removeItem(layout);
        delete layout;
    }
    _widgets.clear();
    _layouts.clear();
    _dirTexts.clear();
    _clearButtons.clear();
    _dirButtons.clear();
    QStringList folderNames(pushFolderNames);
    folderNames.append("");
    foreach(const QString &s, folderNames) {
        addRow(s);
    }
}

void PushFolderList::selectFolder()
{
    AccountFolderModel model(_accountId, _parent);
    model.init();

    // The account itself is not a selectable folder
    QList<QMailMessageSet*> invalidItems;
    invalidItems.append(model.itemFromIndex(model.indexFromAccountId(_accountId)));

    SelectFolderDialog selectFolderDialog(&model);
    selectFolderDialog.setInvalidSelections(invalidItems);
    selectFolderDialog.exec();

    if (selectFolderDialog.result() == QDialog::Accepted) {
        QMailFolder folder(model.folderIdFromIndex(model.indexFromItem(selectFolderDialog.selectedItem())));

        int index(_dirButtons.indexOf(static_cast<QToolButton*>(sender())));
        if (index != -1) {
            _dirTexts.at(index)->setText(folder.path());
            _clearButtons.at(index)->setEnabled(true);

            if (index + 1 == _dirTexts.count()) {
                addRow("");
            }
        }
        
    }
}

QStringList PushFolderList::folderNames()
{
    QStringList result;
    foreach(QLineEdit* edit, _dirTexts) {
        if (!edit->text().isEmpty())
            result.append(edit->text());
    }
    result.removeDuplicates();
    return result;
}

ImapSettings::ImapSettings()
    : QMailMessageServiceEditor(),
      warningEmitted(false),
      pushFolderList(0)
{
    setupUi(this);
    setLayoutDirection(qApp->layoutDirection());

    connect(intervalCheckBox, SIGNAL(stateChanged(int)), this, SLOT(intervalCheckChanged(int)));

    const QString uncapitalised("email noautocapitalization");

    // These fields should not be autocapitalised
    mailPortInput->setValidator(new PortValidator(this));

    mailPasswInput->setEchoMode(QLineEdit::Password);

    // This functionality is not currently used:
    mailboxButton->hide();

#ifdef QT_NO_SSL
    encryptionIncoming->hide();
    lblEncryptionIncoming->hide();
#endif

    connect(draftsButton, SIGNAL(clicked()), this, SLOT(selectFolder()));
    connect(sentButton, SIGNAL(clicked()), this, SLOT(selectFolder()));
    connect(trashButton, SIGNAL(clicked()), this, SLOT(selectFolder()));
    connect(junkButton, SIGNAL(clicked()), this, SLOT(selectFolder()));

    QIcon clearIcon(":icon/clear_left");

    clearBaseButton->setIcon(clearIcon);
    connect(clearBaseButton, SIGNAL(clicked()), imapBaseDir, SLOT(clear()));

    clearDraftsButton->setIcon(clearIcon);
    connect(clearDraftsButton, SIGNAL(clicked()), imapDraftsDir, SLOT(clear()));

    clearSentButton->setIcon(clearIcon);
    connect(clearSentButton, SIGNAL(clicked()), imapSentDir, SLOT(clear()));

    clearTrashButton->setIcon(clearIcon);
    connect(clearTrashButton, SIGNAL(clicked()), imapTrashDir, SLOT(clear()));

    clearJunkButton->setIcon(clearIcon);
    connect(clearJunkButton, SIGNAL(clicked()), imapJunkDir, SLOT(clear()));

    QGridLayout *gridLayout = findChild<QGridLayout *>("gridlayout1");
    if (gridLayout) {
        pushFolderList = new PushFolderList(this, gridLayout);
        connect(pushCheckBox, SIGNAL(stateChanged(int)), pushFolderList, SLOT(setPushEnabled(int)));
    } else {
        qWarning() << "Gridlayout not found";
    }
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
            clearDraftsButton->setEnabled(true);
        } else if (sender() == static_cast<QObject*>(sentButton)) {
            imapSentDir->setText(folder.path());
            clearSentButton->setEnabled(true);
        } else if (sender() == static_cast<QObject*>(trashButton)) {
            imapTrashDir->setText(folder.path());
            clearTrashButton->setEnabled(true);
        } else if (sender() == static_cast<QObject*>(junkButton)) {
            imapJunkDir->setText(folder.path());
            clearJunkButton->setEnabled(true);
        }
    }
}

void ImapSettings::displayConfiguration(const QMailAccount &account, const QMailAccountConfiguration &config)
{
    accountId = account.id();
    QStringList pushFolders;
    bool hasFolders(false);
    if (accountId.isValid()) {
        hasFolders = (QMailStore::instance()->countFolders(QMailFolderKey::parentAccountId(accountId)) > 0);
        pushFolderList->setAccountId(accountId);
    }

    // Only allow the base folder to be specified before retrieval occurs
    baseFolderLabel->setEnabled(!hasFolders);
    imapBaseDir->setEnabled(!hasFolders);

    // Only allow the other folders to be specified after we have a folder listing
    draftsFolderLabel->setEnabled(hasFolders);
    draftsButton->setEnabled(hasFolders);
    imapDraftsDir->setEnabled(hasFolders);

    sentFolderLabel->setEnabled(hasFolders);
    sentButton->setEnabled(hasFolders);
    imapSentDir->setEnabled(hasFolders);

    trashFolderLabel->setEnabled(hasFolders);
    trashButton->setEnabled(hasFolders);
    imapTrashDir->setEnabled(hasFolders);

    junkFolderLabel->setEnabled(hasFolders);
    junkButton->setEnabled(hasFolders);
    imapJunkDir->setEnabled(hasFolders);

    pushCheckBox->setEnabled(hasFolders);
    
    if (!config.services().contains(serviceKey)) {
        // New account
        mailUserInput->setText("");
        mailPasswInput->setText("");
        mailServerInput->setText("");
        mailPortInput->setText("143");
#ifndef QT_NO_SSL
        encryptionIncoming->setCurrentIndex(0);
#endif
        preferHtml->setChecked(true);
        pushCheckBox->setChecked(false);
        intervalCheckBox->setChecked(false);
        roamingCheckBox->setChecked(false);
        pushFolders << "INBOX";
    } else {
        ImapConfiguration imapConfig(config);

        mailUserInput->setText(imapConfig.mailUserName());
        mailPasswInput->setText(imapConfig.mailPassword());
        mailServerInput->setText(imapConfig.mailServer());
        mailPortInput->setText(QString::number(imapConfig.mailPort()));
#ifndef QT_NO_SSL
        encryptionIncoming->setCurrentIndex(static_cast<int>(imapConfig.mailEncryption()));
        authentication->setCurrentIndex(imapConfig.mailAuthentication());
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
        clearBaseButton->setEnabled(!imapBaseDir->text().isEmpty());

        QMailFolderId draftFolderId = account.standardFolder(QMailFolder::DraftsFolder);
        imapDraftsDir->setText(draftFolderId.isValid() ? QMailFolder(draftFolderId).path() : "");
        clearDraftsButton->setEnabled(!imapDraftsDir->text().isEmpty());

        QMailFolderId sentFolderId = account.standardFolder(QMailFolder::SentFolder);
        imapSentDir->setText(sentFolderId.isValid() ? QMailFolder(sentFolderId).path() : "");
        clearSentButton->setEnabled(!imapSentDir->text().isEmpty());

        QMailFolderId trashFolderId = account.standardFolder(QMailFolder::TrashFolder);
        imapTrashDir->setText(trashFolderId.isValid() ? QMailFolder(trashFolderId).path() : "");
        clearTrashButton->setEnabled(!imapTrashDir->text().isEmpty());

        QMailFolderId junkFolderId = account.standardFolder(QMailFolder::JunkFolder);
        imapJunkDir->setText(junkFolderId.isValid() ? QMailFolder(junkFolderId).path() : "");
        clearJunkButton->setEnabled(!imapJunkDir->text().isEmpty());

        pushFolders = imapConfig.pushFolders();
    }
        
    if (pushFolderList) {
        pushFolderList->setHasFolders(hasFolders);
        pushFolderList->setPushEnabled(pushCheckBox->checkState());
        pushFolderList->populate(pushFolders);
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
#ifndef QT_NO_SSL
    imapConfig.setMailEncryption(static_cast<QMailTransport::EncryptType>(encryptionIncoming->currentIndex()));
    int index(authentication->currentIndex());
    Q_ASSERT(index >= 0);
    imapConfig.setMailAuthentication(index);
#endif
    imapConfig.setDeleteMail(deleteCheckBox->isChecked());
    imapConfig.setMaxMailSize(thresholdCheckBox->isChecked() ? maxSize->value() : -1);
    imapConfig.setPreferredTextSubtype(preferHtml->isChecked() ? "html" : "plain");
    imapConfig.setAutoDownload(false);
    imapConfig.setPushEnabled(pushCheckBox->isChecked());
    imapConfig.setCheckInterval(intervalPeriod->value() * (intervalCheckBox->isChecked() ? 1 : -1));
    imapConfig.setIntervalCheckRoamingEnabled(!roamingCheckBox->isChecked());
    imapConfig.setBaseFolder(imapBaseDir->text());

    setStandardFolder(account, QMailFolder::DraftsFolder, imapDraftsDir->text());
    setStandardFolder(account, QMailFolder::SentFolder, imapSentDir->text());
    setStandardFolder(account, QMailFolder::TrashFolder, imapTrashDir->text());
    setStandardFolder(account, QMailFolder::JunkFolder, imapJunkDir->text());

    if (pushFolderList)
        imapConfig.setPushFolders(pushFolderList->folderNames());

    account->setStatus(QMailAccount::CanCreateFolders, true);
    // Do we have a configuration we can use?
    if (!imapConfig.mailServer().isEmpty() && !imapConfig.mailUserName().isEmpty())
        account->setStatus(QMailAccount::CanRetrieve, true);

    return true;
}

void ImapSettings::setStandardFolder(QMailAccount *account, QMailFolder::StandardFolder folderType, const QString &path)
{
    QMailFolderIdList folders(
            QMailStore::instance()->queryFolders(QMailFolderKey::path(path)
                                                 & QMailFolderKey::parentAccountId(account->id()))
            );

    Q_ASSERT(folders.count() <= 1);
    if (folders.count() == 0) {
        // remove standard folder
        account->setStandardFolder(folderType, QMailFolderId());
        return;
    }
    if (folders.count() != 1)
        return;

    QMailFolder folder(folders.first());

    if (folderType == QMailFolder::DraftsFolder)
        folder.setStatus(QMailFolder::Drafts | QMailFolder::OutboxFolder, true);
    else if (folderType == QMailFolder::SentFolder)
        folder.setStatus(QMailFolder::Sent | QMailFolder::OutboxFolder, true);
    else if (folderType == QMailFolder::InboxFolder)
        folder.setStatus(QMailFolder::Incoming, true);
    else if (folderType == QMailFolder::JunkFolder)
        folder.setStatus(QMailFolder::Junk | QMailFolder::Incoming, true);
    else if (folderType == QMailFolder::TrashFolder)
        folder.setStatus(QMailFolder::Trash | QMailFolder::Incoming, true);
    else if (folderType == QMailFolder::OutboxFolder)
        folder.setStatus(QMailFolder::Outgoing, true);
    else
        qWarning() << "Unable to set unsupported folder type";

    QMailStore::instance()->updateFolder(&folder);

    account->setStandardFolder(folderType, folder.id());
}

#include "imapsettings.moc"


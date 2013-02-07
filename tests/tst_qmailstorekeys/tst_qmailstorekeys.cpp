/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QObject>
#include <QTest>
#include <qmailmessagelistmodel.h>
#include <qmailmessagethreadedmodel.h>
#include <qmailstore.h>
#include <QSettings>
#include <qmailnamespace.h>
#include "locks_p.h"

class tst_QMailStoreKeys : public QObject
{
    Q_OBJECT

public:
    tst_QMailStoreKeys();
    virtual ~tst_QMailStoreKeys();

private slots:
    void initTestCase();
    void cleanupTestCase();

    void simpleKeys();

    void accountId();
    void accountName();
    void accountMessageType();
    void accountFromAddress();
    void accountStatus();
    void accountCustomField();

    void folderId();
    void folderPath();
    void folderParentFolderId();
    void folderParentAccountId();
    void folderDisplayName();
    void folderStatus();
    void folderAncestorFolderIds();
    void folderCustomField();

    void messageId();
    void messageType();
    void messageParentFolderId();
    void messageSender();
    void messageRecipients();
    void messageSubject();
    void messageTimeStamp();
    void messageReceptionTimeStamp();
    void messageStatus();
    void messageConversation();
    void messageServerUid();
    void messageSize();
    void messageParentAccountId();
    void messageAncestorFolderIds();
    void messageContentType();
    void messagePreviousParentFolderId();
    void messageInResponseTo();
    void messageResponseType();
    void messageCustom();

    void listModel();
    void threadedModel();

private:
    // We only want to compare sets, disregarding ordering
    const QSet<QMailAccountId> accountSet(const QMailAccountKey &key) const
    {
        return QMailStore::instance()->queryAccounts(key).toSet();
    }

    QSet<QMailAccountId> accountSet() const 
    { 
        return QSet<QMailAccountId>(); 
    }

    const QSet<QMailFolderId> folderSet(const QMailFolderKey &key) const
    {
        return QMailStore::instance()->queryFolders(key).toSet();
    }

    QSet<QMailFolderId> folderSet() const 
    { 
        return QSet<QMailFolderId>(); 
    }

    const QSet<QMailMessageId> messageSet(const QMailMessageKey &key) const
    {
        return QMailStore::instance()->queryMessages(key).toSet();
    }

    QSet<QMailMessageId> messageSet() const 
    { 
        return QSet<QMailMessageId>(); 
    }

    QMailAccountId accountId1, accountId2, accountId3, accountId4;
    QMailFolderId inboxId1, inboxId2, savedId1, savedId2, archivedId1, archivedId2;
    QMailMessageId smsMessage, inboxMessage1, archivedMessage1, inboxMessage2, savedMessage2;

    QSet<QMailAccountId> noAccounts, verifiedAccounts, unverifiedAccounts, allAccounts;
    QSet<QMailFolderId> noFolders, allFolders, standardFolders;
    QSet<QMailMessageId> noMessages, allMessages, allEmailMessages;
};

QTEST_MAIN(tst_QMailStoreKeys)

#include "tst_qmailstorekeys.moc"

tst_QMailStoreKeys::tst_QMailStoreKeys()
{
}

tst_QMailStoreKeys::~tst_QMailStoreKeys()
{
}

void tst_QMailStoreKeys::initTestCase()
{
    // Instantiate the store to initialise the values of the status flags and create the standard folders
    QMailStore::instance();

    // Create the data set we will test our keys upon

    standardFolders << QMailFolderId(QMailFolder::LocalStorageFolderId);

    {
        QMailAccount account;
        account.setName("Account 1");
        account.setMessageType(QMailMessage::Email);
        account.setFromAddress(QMailAddress("Account 1", "account1@example.org"));
        account.setStatus(QMailAccount::SynchronizationEnabled, false);
        account.setStatus(QMailAccount::Synchronized, false);
        account.setStatus(QMailAccount::MessageSource, true);
        account.setStatus(QMailAccount::CanRetrieve, true);
        account.setStatus(QMailAccount::MessageSink, true);
        account.setStatus(QMailAccount::CanTransmit, true);
        account.setCustomField("verified", "true");
        account.setCustomField("question", "What is your dog's name?");
        account.setCustomField("answer", "Fido");

        QMailAccountConfiguration config;
        config.addServiceConfiguration("imap4");
        if (QMailAccountConfiguration::ServiceConfiguration *svcCfg = &config.serviceConfiguration("imap4")) {
            svcCfg->setValue("server", "mail.example.org");
            svcCfg->setValue("username", "account1");
        }
        config.addServiceConfiguration("smtp");
        if (QMailAccountConfiguration::ServiceConfiguration *svcCfg = &config.serviceConfiguration("smtp")) {
            svcCfg->setValue("server", "mail.example.org");
            svcCfg->setValue("username", "account1");
        }

        QVERIFY(QMailStore::instance()->addAccount(&account, &config));
        accountId1 = account.id();
        allAccounts << account.id();
        verifiedAccounts << account.id();
    }

    {
        QMailAccount account;
        account.setName("Account 2");
        account.setMessageType(QMailMessage::Instant);
        account.setFromAddress(QMailAddress("Account 2", "account2@example.org"));
        account.setStatus(QMailAccount::SynchronizationEnabled, true);
        account.setStatus(QMailAccount::Synchronized, false);
        account.setStatus(QMailAccount::MessageSource, true);
        account.setStatus(QMailAccount::CanRetrieve, true);
        account.setCustomField("verified", "true");
        account.setCustomField("question", "What is your dog's name?");
        account.setCustomField("answer", "Lassie");

        QMailAccountConfiguration config;
        config.addServiceConfiguration("imap4");
        if (QMailAccountConfiguration::ServiceConfiguration *svcCfg = &config.serviceConfiguration("imap4")) {
            svcCfg->setValue("server", "imap.example.org");
            svcCfg->setValue("username", "account2");
        }

        QVERIFY(QMailStore::instance()->addAccount(&account, &config));
        accountId2 = account.id();
        allAccounts << account.id();
        verifiedAccounts << account.id();
    }

    {
        QMailAccount account;
        account.setName("Account 3");
        account.setMessageType(QMailMessage::None);
        account.setFromAddress(QMailAddress("Account 3", "account3@test"));
        account.setCustomField("verified", "false");

        QVERIFY(QMailStore::instance()->addAccount(&account, 0));
        accountId3 = account.id();
        allAccounts << account.id();
        unverifiedAccounts << account.id();
    }

    {
        QMailAccount account;
        account.setName("Account 4");
        account.setMessageType(QMailMessage::None);
        account.setFromAddress(QMailAddress("Account 4", "account4@test"));

        QVERIFY(QMailStore::instance()->addAccount(&account, 0));
        accountId4 = account.id();
        allAccounts << account.id();
        unverifiedAccounts << account.id();
    }

    {
        QMailFolder folder;
        folder.setPath("Inbox");
        folder.setDisplayName("Inbox");
        folder.setParentFolderId(QMailFolderId());
        folder.setParentAccountId(accountId1);
        folder.setStatus(QMailFolder::SynchronizationEnabled, true);
        folder.setStatus(QMailFolder::Synchronized, false);
        folder.setCustomField("uidValidity", "abcdefg");
        folder.setCustomField("uidNext", "1");

        QVERIFY(QMailStore::instance()->addFolder(&folder));
        inboxId1 = folder.id();
        allFolders << folder.id();
    }

    {
        QMailFolder folder;
        folder.setPath("Inbox/Saved");
        folder.setDisplayName("Saved");
        folder.setParentFolderId(inboxId1);
        folder.setParentAccountId(accountId1);
        folder.setStatus(QMailFolder::SynchronizationEnabled, true);
        folder.setStatus(QMailFolder::Synchronized, true);
        folder.setCustomField("uidValidity", "hijklmnop");
        folder.setCustomField("uidNext", "11");

        QVERIFY(QMailStore::instance()->addFolder(&folder));
        savedId1 = folder.id();
        allFolders << folder.id();
    }

    {
        QMailFolder folder;
        folder.setPath("Inbox/Saved/Archived");
        folder.setDisplayName("Archived");
        folder.setParentFolderId(savedId1);
        folder.setParentAccountId(accountId1);
        folder.setStatus(QMailFolder::SynchronizationEnabled, false);
        folder.setStatus(QMailFolder::Synchronized, false);
        folder.setCustomField("archived", "true");
        folder.setCustomField("uidNext", "111");

        QVERIFY(QMailStore::instance()->addFolder(&folder));
        archivedId1 = folder.id();
        allFolders << folder.id();
    }

    {
        QMailFolder folder;
        folder.setPath("Inbox");
        folder.setDisplayName("Inbox");
        folder.setParentFolderId(QMailFolderId());
        folder.setParentAccountId(accountId2);
        folder.setStatus(QMailFolder::SynchronizationEnabled, false);
        folder.setStatus(QMailFolder::Synchronized, false);
        folder.setCustomField("uidValidity", "qrstuv");
        folder.setCustomField("uidNext", "1");

        QVERIFY(QMailStore::instance()->addFolder(&folder));
        inboxId2 = folder.id();
        allFolders << folder.id();
    }

    {
        QMailFolder folder;
        folder.setPath("Inbox/Saved");
        folder.setDisplayName("Saved");
        folder.setParentFolderId(inboxId2);
        folder.setParentAccountId(accountId2);
        folder.setStatus(QMailFolder::SynchronizationEnabled, false);
        folder.setStatus(QMailFolder::Synchronized, true);
        folder.setCustomField("uidValidity", "wxyz");
        folder.setCustomField("uidNext", "11");

        QVERIFY(QMailStore::instance()->addFolder(&folder));
        savedId2 = folder.id();
        allFolders << folder.id();
    }

    {
        QMailFolder folder;
        folder.setPath("Inbox/Saved/Archived");
        folder.setDisplayName("Archived");
        folder.setParentFolderId(savedId2);
        folder.setParentAccountId(accountId2);
        folder.setStatus(QMailFolder::SynchronizationEnabled, false);
        folder.setStatus(QMailFolder::Synchronized, false);
        folder.setCustomField("archived", "true");
        folder.setCustomField("uidNext", "111");

        QVERIFY(QMailStore::instance()->addFolder(&folder));
        archivedId2 = folder.id();
        allFolders << folder.id();
    }

    {
        QMailMessage message;
        message.setMessageType(QMailMessage::Sms);
        message.setParentAccountId(accountId4);
        message.setParentFolderId(QMailFolder::LocalStorageFolderId);
        message.setFrom(QMailAddress("0404404040"));
        message.setTo(QMailAddress("0404040404"));
        message.setSubject("Where are you?");
        message.setDate(QMailTimeStamp(QDateTime(QDate::currentDate())));
        message.setReceivedDate(QMailTimeStamp(QDateTime(QDate::currentDate())));
        message.setStatus(QMailMessage::Incoming, true);
        message.setStatus(QMailMessage::New, false);
        message.setStatus(QMailMessage::Read, true);
        message.setServerUid("sim:12345");
        message.setSize(1 * 1024);
        message.setContent(QMailMessage::PlainTextContent);
        message.setCustomField("present", "true");
        message.setCustomField("todo", "true");

        QVERIFY(QMailStore::instance()->addMessage(&message));
        smsMessage = message.id();
        allMessages << message.id();
    }

    {
        QMailMessage message;
        message.setMessageType(QMailMessage::Email);
        message.setParentAccountId(accountId1);
        message.setParentFolderId(inboxId1);
        message.setFrom(QMailAddress("account2@example.org"));
        message.setTo(QMailAddress("account1@example.org"));
        message.setSubject("inboxMessage1");
        message.setDate(QMailTimeStamp(QDateTime(QDate::currentDate())));
        message.setReceivedDate(QMailTimeStamp(QDateTime(QDate::currentDate())));
        message.setStatus(QMailMessage::Incoming, true);
        message.setStatus(QMailMessage::New, true);
        message.setStatus(QMailMessage::Read, false);
        message.setServerUid("inboxMessage1");
        message.setSize(5 * 1024);
        message.setContent(QMailMessage::PlainTextContent);
        message.setCustomField("present", "true");

        QVERIFY(QMailStore::instance()->addMessage(&message));
        inboxMessage1 = message.id();
        allMessages << message.id();
        allEmailMessages << message.id();
    }

    {
        QMailMessage message;
        message.setMessageType(QMailMessage::Email);
        message.setParentAccountId(accountId1);
        message.setParentFolderId(inboxId1);
        message.setFrom(QMailAddress("account1@example.org"));
        message.setTo(QMailAddress("fred@example.net"));
        message.setSubject("archivedMessage1");
        message.setDate(QMailTimeStamp(QDateTime(QDate::currentDate().addDays(-1))));
        message.setReceivedDate(QMailTimeStamp(QDateTime(QDate::currentDate().addDays(-1))));
        message.setStatus(QMailMessage::Outgoing, true);
        message.setStatus(QMailMessage::New, false);
        message.setStatus(QMailMessage::Sent, true);
        message.setServerUid("archivedMessage1");
        message.setSize(15 * 1024);
        message.setContent(QMailMessage::VideoContent);
        message.setCustomField("present", "true");

        QVERIFY(QMailStore::instance()->addMessage(&message));

        message.setPreviousParentFolderId(message.parentFolderId());
        message.setParentFolderId(archivedId1);
        QVERIFY(QMailStore::instance()->updateMessage(&message));

        archivedMessage1 = message.id();
        allMessages << message.id();
        allEmailMessages << message.id();
    }

    {
        QMailMessage message;
        message.setMessageType(QMailMessage::Email);
        message.setParentAccountId(accountId2);
        message.setParentFolderId(inboxId2);
        message.setFrom(QMailAddress("account1@example.org"));
        message.setTo(QMailAddress("account2@example.org"));
        message.setSubject("Fwd:inboxMessage2");
        message.setDate(QMailTimeStamp(QDateTime(QDate::currentDate())));
        message.setReceivedDate(QMailTimeStamp(QDateTime(QDate::currentDate())));
        message.setStatus(QMailMessage::Incoming, true);
        message.setStatus(QMailMessage::New, true);
        message.setStatus(QMailMessage::Read, true);
        message.setServerUid("inboxMessage2");
        message.setSize(5 * 1024);
        message.setContent(QMailMessage::HtmlContent);
        message.setInResponseTo(inboxMessage1);
        message.setResponseType(QMailMessage::Forward);
        message.setCustomField("present", "true");
        message.setCustomField("todo", "false");

        QVERIFY(QMailStore::instance()->addMessage(&message));
        inboxMessage2 = message.id();
        allMessages << message.id();
        allEmailMessages << message.id();
    }

    {
        QMailMessage message;
        message.setMessageType(QMailMessage::Email);
        message.setParentAccountId(accountId2);
        message.setParentFolderId(inboxId2);
        message.setFrom(QMailAddress("fred@example.net"));
        message.setTo(QMailAddressList() << QMailAddress("account2@example.org") << QMailAddress("testing@test"));
        message.setSubject("Re:savedMessage2");
        message.setDate(QMailTimeStamp(QDateTime(QDate::currentDate().addDays(-7))));
        message.setReceivedDate(QMailTimeStamp(QDateTime(QDate::currentDate().addDays(-7))));
        message.setStatus(QMailMessage::Incoming, true);
        message.setStatus(QMailMessage::New, false);
        message.setStatus(QMailMessage::Read, false);
        message.setServerUid("savedMessage2");
        message.setSize(5 * 1024);
        message.setContent(QMailMessage::HtmlContent);
        message.setInResponseTo(archivedMessage1);
        message.setResponseType(QMailMessage::Reply);
        message.setCustomField("present", "true");

        QVERIFY(QMailStore::instance()->addMessage(&message));

        message.setPreviousParentFolderId(message.parentFolderId());
        message.setParentFolderId(savedId2);
        QVERIFY(QMailStore::instance()->updateMessage(&message));

        savedMessage2 = message.id();
        allMessages << message.id();
        allEmailMessages << message.id();
    }
}

void tst_QMailStoreKeys::cleanupTestCase()
{
    QMailStore::instance()->clearContent();
}

using namespace QMailDataComparator;

void tst_QMailStoreKeys::simpleKeys()
{
    // Empty key
    QCOMPARE(accountSet(QMailAccountKey()), allAccounts);
    QCOMPARE(accountSet(~QMailAccountKey()), noAccounts);
    QCOMPARE(folderSet(QMailFolderKey()), standardFolders + allFolders);
    QCOMPARE(folderSet(~QMailFolderKey()), noFolders);
    QCOMPARE(messageSet(QMailMessageKey()), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey()), noMessages);

    // Non-matching key
    QCOMPARE(accountSet(QMailAccountKey::nonMatchingKey()), noAccounts);
    QCOMPARE(accountSet(~QMailAccountKey::nonMatchingKey()), allAccounts);
    QCOMPARE(folderSet(QMailFolderKey::nonMatchingKey()), noFolders);
    QCOMPARE(folderSet(~QMailFolderKey::nonMatchingKey()), standardFolders + allFolders);
    QCOMPARE(messageSet(QMailMessageKey::nonMatchingKey()), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::nonMatchingKey()), allMessages);

    // Combined
    QCOMPARE(accountSet(QMailAccountKey() & QMailAccountKey::nonMatchingKey()), noAccounts);
    QCOMPARE(accountSet(QMailAccountKey() | QMailAccountKey::nonMatchingKey()), allAccounts);
    QCOMPARE(folderSet(QMailFolderKey() & QMailFolderKey::nonMatchingKey()), noFolders);
    QCOMPARE(folderSet(QMailFolderKey() | QMailFolderKey::nonMatchingKey()), standardFolders + allFolders);
    QCOMPARE(messageSet(QMailMessageKey() & QMailMessageKey::nonMatchingKey()), noMessages);
    QCOMPARE(messageSet(QMailMessageKey() | QMailMessageKey::nonMatchingKey()), allMessages);

    // Combinations with standard keys
    QMailAccountKey accountKey(QMailAccountKey::id(accountId1));
    QSet<QMailAccountId> accountResult(accountSet() << accountId1);

    QMailFolderKey folderKey(QMailFolderKey::id(inboxId1));
    QSet<QMailFolderId> folderResult(folderSet() << inboxId1);

    QMailMessageKey messageKey(QMailMessageKey::id(smsMessage));
    QSet<QMailMessageId> messageResult(messageSet() << smsMessage);

    QCOMPARE(accountSet(accountKey), accountResult);
    QCOMPARE(folderSet(folderKey), folderResult);
    QCOMPARE(messageSet(messageKey), messageResult);
    
    // Empty & standard = standard
    QCOMPARE(accountSet(QMailAccountKey() & accountKey), accountResult);
    QCOMPARE(accountSet(accountKey & QMailAccountKey()), accountResult);
    QCOMPARE(folderSet(QMailFolderKey() & folderKey), folderResult);
    QCOMPARE(folderSet(folderKey & QMailFolderKey()), folderResult);
    QCOMPARE(messageSet(QMailMessageKey() & messageKey), messageResult);
    QCOMPARE(messageSet(messageKey & QMailMessageKey()), messageResult);

    // Empty | standard = standard
    QCOMPARE(accountSet(QMailAccountKey() | accountKey), accountResult);
    QCOMPARE(accountSet(accountKey | QMailAccountKey()), accountResult);
    QCOMPARE(folderSet(QMailFolderKey() | folderKey), folderResult);
    QCOMPARE(folderSet(folderKey | QMailFolderKey()), folderResult);
    QCOMPARE(messageSet(QMailMessageKey() | messageKey), messageResult);
    QCOMPARE(messageSet(messageKey | QMailMessageKey()), messageResult);

    // Non-matching & standard = non-matching
    QCOMPARE(accountSet(QMailAccountKey::nonMatchingKey() & accountKey), noAccounts);
    QCOMPARE(accountSet(accountKey & QMailAccountKey::nonMatchingKey()), noAccounts);
    QCOMPARE(folderSet(QMailFolderKey::nonMatchingKey() & folderKey), noFolders);
    QCOMPARE(folderSet(folderKey & QMailFolderKey::nonMatchingKey()), noFolders);
    QCOMPARE(messageSet(QMailMessageKey::nonMatchingKey() & messageKey), noMessages);
    QCOMPARE(messageSet(messageKey & QMailMessageKey::nonMatchingKey()), noMessages);

    // Non-matching | standard = standard
    QCOMPARE(accountSet(QMailAccountKey::nonMatchingKey() | accountKey), accountResult);
    QCOMPARE(accountSet(accountKey |QMailAccountKey::nonMatchingKey()), accountResult);
    QCOMPARE(folderSet(QMailFolderKey::nonMatchingKey() | folderKey), folderResult);
    QCOMPARE(folderSet(folderKey | QMailFolderKey::nonMatchingKey()), folderResult);
    QCOMPARE(messageSet(QMailMessageKey::nonMatchingKey() | messageKey), messageResult);
    QCOMPARE(messageSet(messageKey | QMailMessageKey::nonMatchingKey()), messageResult);
}

void tst_QMailStoreKeys::accountId()
{
    // ID equality
    QCOMPARE(accountSet(QMailAccountKey::id(accountId1, Equal)), accountSet() << accountId1);
    QCOMPARE(accountSet(~QMailAccountKey::id(accountId1, Equal)), accountSet() << accountId2 << accountId3 << accountId4);
    QCOMPARE(accountSet(QMailAccountKey::id(accountId2, Equal)), accountSet() << accountId2);
    QCOMPARE(accountSet(~QMailAccountKey::id(accountId2, Equal)), accountSet() << accountId1 << accountId3 << accountId4);
    QCOMPARE(accountSet(QMailAccountKey::id(QMailAccountId(), Equal)), noAccounts);
    QCOMPARE(accountSet(~QMailAccountKey::id(QMailAccountId(), Equal)), allAccounts);

    // ID inequality
    QCOMPARE(accountSet(QMailAccountKey::id(accountId1, NotEqual)), accountSet() << accountId2 << accountId3 << accountId4);
    QCOMPARE(accountSet(~QMailAccountKey::id(accountId1, NotEqual)), accountSet() << accountId1);
    QCOMPARE(accountSet(QMailAccountKey::id(accountId2, NotEqual)), accountSet() << accountId1 << accountId3 << accountId4);
    QCOMPARE(accountSet(~QMailAccountKey::id(accountId2, NotEqual)), accountSet() << accountId2);
    QCOMPARE(accountSet(QMailAccountKey::id(QMailAccountId(), NotEqual)), allAccounts);
    QCOMPARE(accountSet(~QMailAccountKey::id(QMailAccountId(), NotEqual)), noAccounts);

    // List inclusion
    QCOMPARE(accountSet(QMailAccountKey::id(allAccounts.toList())), allAccounts);
    QCOMPARE(accountSet(~QMailAccountKey::id(allAccounts.toList())), noAccounts);
    QCOMPARE(accountSet(QMailAccountKey::id(QMailAccountIdList() << accountId1)), accountSet() << accountId1);
    QCOMPARE(accountSet(~QMailAccountKey::id(QMailAccountIdList() << accountId1)), accountSet() << accountId2 << accountId3 << accountId4);
    QCOMPARE(accountSet(QMailAccountKey::id(QMailAccountIdList() << accountId2)), accountSet() << accountId2);
    QCOMPARE(accountSet(~QMailAccountKey::id(QMailAccountIdList() << accountId2)), accountSet() << accountId1 << accountId3 << accountId4);
    QCOMPARE(accountSet(QMailAccountKey::id(QMailAccountIdList() << accountId1 << accountId2)), accountSet() << accountId1 << accountId2);
    QCOMPARE(accountSet(~QMailAccountKey::id(QMailAccountIdList() << accountId1 << accountId2)), accountSet() << accountId3 << accountId4);

    // List exclusion
    QCOMPARE(accountSet(QMailAccountKey::id(allAccounts.toList(), Excludes)), noAccounts);
    QCOMPARE(accountSet(~QMailAccountKey::id(allAccounts.toList(), Excludes)), allAccounts);
    QCOMPARE(accountSet(QMailAccountKey::id(QMailAccountIdList() << accountId1, Excludes)), accountSet() << accountId2 << accountId3 << accountId4);
    QCOMPARE(accountSet(~QMailAccountKey::id(QMailAccountIdList() << accountId1, Excludes)), accountSet() << accountId1);
    QCOMPARE(accountSet(QMailAccountKey::id(QMailAccountIdList() << accountId2, Excludes)), accountSet() << accountId1 << accountId3 << accountId4);
    QCOMPARE(accountSet(~QMailAccountKey::id(QMailAccountIdList() << accountId2, Excludes)), accountSet() << accountId2);
    QCOMPARE(accountSet(QMailAccountKey::id(QMailAccountIdList() << accountId1 << accountId2, Excludes)), accountSet() << accountId3 << accountId4);
    QCOMPARE(accountSet(~QMailAccountKey::id(QMailAccountIdList() << accountId1 << accountId2, Excludes)), accountSet() << accountId1 << accountId2);

    // Key matching
    QCOMPARE(accountSet(QMailAccountKey::id(accountId1, Equal)), accountSet() << accountId1);
    QCOMPARE(accountSet(~QMailAccountKey::id(accountId1, Equal)), accountSet() << accountId2 << accountId3 << accountId4);
    QCOMPARE(accountSet(QMailAccountKey::id(QMailAccountKey::id(accountId1, Equal))), accountSet() << accountId1);
    QCOMPARE(accountSet(QMailAccountKey::id(~QMailAccountKey::id(accountId1, Equal))), accountSet() << accountId2 << accountId3 << accountId4);
    QCOMPARE(accountSet(~QMailAccountKey::id(QMailAccountKey::id(accountId1, Equal))), accountSet() << accountId2 << accountId3 << accountId4);
    QCOMPARE(accountSet(~QMailAccountKey::id(~QMailAccountKey::id(accountId1, Equal))), accountSet() << accountId1);

    QString name("Account 1");
    QCOMPARE(accountSet(QMailAccountKey::name(name, Equal)), accountSet() << accountId1);
    QCOMPARE(accountSet(~QMailAccountKey::name(name, Equal)), accountSet() << accountId2 << accountId3 << accountId4);
    QCOMPARE(accountSet(QMailAccountKey::id(QMailAccountKey::name(name, Equal))), accountSet() << accountId1);
    QCOMPARE(accountSet(QMailAccountKey::id(~QMailAccountKey::name(name, Equal))), accountSet() << accountId2 << accountId3 << accountId4);
    QCOMPARE(accountSet(~QMailAccountKey::id(QMailAccountKey::name(name, Equal))), accountSet() << accountId2 << accountId3 << accountId4);
    QCOMPARE(accountSet(~QMailAccountKey::id(~QMailAccountKey::name(name, Equal))), accountSet() << accountId1);

    QCOMPARE(accountSet(QMailAccountKey::messageType(QMailMessage::Instant, Equal)), accountSet() << accountId2);
    QCOMPARE(accountSet(~QMailAccountKey::messageType(QMailMessage::Instant, Equal)), accountSet() << accountId1 << accountId3 << accountId4);
    QCOMPARE(accountSet(QMailAccountKey::id(QMailAccountKey::messageType(QMailMessage::Instant, Equal))), accountSet() << accountId2);
    QCOMPARE(accountSet(QMailAccountKey::id(~QMailAccountKey::messageType(QMailMessage::Instant, Equal))), accountSet() << accountId1 << accountId3 << accountId4);
    QCOMPARE(accountSet(~QMailAccountKey::id(QMailAccountKey::messageType(QMailMessage::Instant, Equal))), accountSet() << accountId1 << accountId3 << accountId4);
    QCOMPARE(accountSet(~QMailAccountKey::id(~QMailAccountKey::messageType(QMailMessage::Instant, Equal))), accountSet() << accountId2);
}

void tst_QMailStoreKeys::accountName()
{
    QString name1("Account 1"), name2("Account 2"), sub("count");

    // Equality
    QCOMPARE(accountSet(QMailAccountKey::name(name1, Equal)), accountSet() << accountId1);
    QCOMPARE(accountSet(~QMailAccountKey::name(name1, Equal)), accountSet() << accountId2 << accountId3 << accountId4);
    QCOMPARE(accountSet(QMailAccountKey::name(name2, Equal)), accountSet() << accountId2);
    QCOMPARE(accountSet(~QMailAccountKey::name(name2, Equal)), accountSet() << accountId1 << accountId3 << accountId4);
    QCOMPARE(accountSet(QMailAccountKey::name(QString(""), Equal)), noAccounts);
    QCOMPARE(accountSet(~QMailAccountKey::name(QString(""), Equal)), allAccounts);
    QCOMPARE(accountSet(QMailAccountKey::name(QString(), Equal)), noAccounts);
    QCOMPARE(accountSet(~QMailAccountKey::name(QString(), Equal)), allAccounts);

    // Inequality
    QCOMPARE(accountSet(QMailAccountKey::name(name1, NotEqual)), accountSet() << accountId2 << accountId3 << accountId4);
    QCOMPARE(accountSet(~QMailAccountKey::name(name1, NotEqual)), accountSet() << accountId1);
    QCOMPARE(accountSet(QMailAccountKey::name(name2, NotEqual)), accountSet() << accountId1 << accountId3 << accountId4);
    QCOMPARE(accountSet(~QMailAccountKey::name(name2, NotEqual)), accountSet() << accountId2);
    QCOMPARE(accountSet(QMailAccountKey::name(QString(""), NotEqual)), allAccounts);
    QCOMPARE(accountSet(~QMailAccountKey::name(QString(""), NotEqual)), noAccounts);
    QCOMPARE(accountSet(QMailAccountKey::name(QString(), NotEqual)), allAccounts);
    QCOMPARE(accountSet(~QMailAccountKey::name(QString(), NotEqual)), noAccounts);

    // List inclusion
    QCOMPARE(accountSet(QMailAccountKey::name(QStringList())), noAccounts);
    QCOMPARE(accountSet(~QMailAccountKey::name(QStringList())), allAccounts);
    QCOMPARE(accountSet(QMailAccountKey::name(QStringList() << name1)), accountSet() << accountId1);
    QCOMPARE(accountSet(~QMailAccountKey::name(QStringList() << name1)), accountSet() << accountId2 << accountId3 << accountId4);
    QCOMPARE(accountSet(QMailAccountKey::name(QStringList() << name1 << name2)), accountSet() << accountId1 << accountId2);
    QCOMPARE(accountSet(~QMailAccountKey::name(QStringList() << name1 << name2)), accountSet() << accountId3 << accountId4);

    // List exclusion
    QCOMPARE(accountSet(QMailAccountKey::name(QStringList(), Excludes)), allAccounts);
    QCOMPARE(accountSet(~QMailAccountKey::name(QStringList(), Excludes)), noAccounts);
    QCOMPARE(accountSet(QMailAccountKey::name(QStringList() << name1, Excludes)), accountSet() << accountId2 << accountId3 << accountId4);
    QCOMPARE(accountSet(~QMailAccountKey::name(QStringList() << name1, Excludes)), accountSet() << accountId1);
    QCOMPARE(accountSet(QMailAccountKey::name(QStringList() << name1 << name2, Excludes)), accountSet() << accountId3 << accountId4);
    QCOMPARE(accountSet(~QMailAccountKey::name(QStringList() << name1 << name2, Excludes)), accountSet() << accountId1 << accountId2);

    // Inclusion
    QCOMPARE(accountSet(QMailAccountKey::name(sub, Includes)), allAccounts);
    QCOMPARE(accountSet(~QMailAccountKey::name(sub, Includes)), noAccounts);
    QCOMPARE(accountSet(QMailAccountKey::name(QString("1"), Includes)), accountSet() << accountId1);
    QCOMPARE(accountSet(~QMailAccountKey::name(QString("1"), Includes)), accountSet() << accountId2 << accountId3 << accountId4);
    QCOMPARE(accountSet(QMailAccountKey::name(QString(""), Includes)), allAccounts);
    QCOMPARE(accountSet(~QMailAccountKey::name(QString(""), Includes)), noAccounts);
    QCOMPARE(accountSet(QMailAccountKey::name(QString(), Includes)), allAccounts);
    QCOMPARE(accountSet(~QMailAccountKey::name(QString(), Includes)), noAccounts);

    // Exclusion
    QCOMPARE(accountSet(QMailAccountKey::name(sub, Excludes)), noAccounts);
    QCOMPARE(accountSet(~QMailAccountKey::name(sub, Excludes)), allAccounts);
    QCOMPARE(accountSet(QMailAccountKey::name(QString("1"), Excludes)), accountSet() << accountId2 << accountId3 << accountId4);
    QCOMPARE(accountSet(~QMailAccountKey::name(QString("1"), Excludes)), accountSet() << accountId1);
    QCOMPARE(accountSet(QMailAccountKey::name(QString(""), Excludes)), noAccounts);
    QCOMPARE(accountSet(~QMailAccountKey::name(QString(""), Excludes)), allAccounts);
    QCOMPARE(accountSet(QMailAccountKey::name(QString(), Excludes)), noAccounts);
    QCOMPARE(accountSet(~QMailAccountKey::name(QString(), Excludes)), allAccounts);
}

void tst_QMailStoreKeys::accountMessageType()
{
    // MessageType equality
    QCOMPARE(accountSet(QMailAccountKey::messageType(QMailMessage::Sms, Equal)), noAccounts);
    QCOMPARE(accountSet(~QMailAccountKey::messageType(QMailMessage::Sms, Equal)), allAccounts);
    QCOMPARE(accountSet(QMailAccountKey::messageType(QMailMessage::Email, Equal)), accountSet() << accountId1);
    QCOMPARE(accountSet(~QMailAccountKey::messageType(QMailMessage::Email, Equal)), accountSet() << accountId2 << accountId3 << accountId4);
    QCOMPARE(accountSet(QMailAccountKey::messageType(QMailMessage::Instant, Equal)), accountSet() << accountId2);
    QCOMPARE(accountSet(~QMailAccountKey::messageType(QMailMessage::Instant, Equal)), accountSet() << accountId1 << accountId3 << accountId4);

    // MessageType inequality
    QCOMPARE(accountSet(QMailAccountKey::messageType(QMailMessage::Sms, NotEqual)), allAccounts);
    QCOMPARE(accountSet(~QMailAccountKey::messageType(QMailMessage::Sms, NotEqual)), noAccounts);
    QCOMPARE(accountSet(QMailAccountKey::messageType(QMailMessage::Email, NotEqual)), accountSet() << accountId2 << accountId3 << accountId4);
    QCOMPARE(accountSet(~QMailAccountKey::messageType(QMailMessage::Email, NotEqual)), accountSet() << accountId1);
    QCOMPARE(accountSet(QMailAccountKey::messageType(QMailMessage::Instant, NotEqual)), accountSet() << accountId1 << accountId3 << accountId4);
    QCOMPARE(accountSet(~QMailAccountKey::messageType(QMailMessage::Instant, NotEqual)), accountSet() << accountId2);

    // Bitwise exclusion
    QCOMPARE(accountSet(QMailAccountKey::messageType(QMailMessage::Sms | QMailMessage::Mms, Excludes)), allAccounts);
    QCOMPARE(accountSet(~QMailAccountKey::messageType(QMailMessage::Sms | QMailMessage::Mms, Excludes)), noAccounts);
    QCOMPARE(accountSet(QMailAccountKey::messageType(QMailMessage::Sms | QMailMessage::Email, Excludes)), accountSet() << accountId2 << accountId3 << accountId4);
    QCOMPARE(accountSet(~QMailAccountKey::messageType(QMailMessage::Sms | QMailMessage::Email, Excludes)), accountSet() << accountId1);
    QCOMPARE(accountSet(QMailAccountKey::messageType(QMailMessage::Sms | QMailMessage::Instant, Excludes)), accountSet() << accountId1 << accountId3 << accountId4);
    QCOMPARE(accountSet(~QMailAccountKey::messageType(QMailMessage::Sms | QMailMessage::Instant, Excludes)), accountSet() << accountId2);
}

void tst_QMailStoreKeys::accountFromAddress()
{
    QString from1("account1@example.org"), from2("account2@example.org"), sub1("ccount"), sub2("example");

    // Equality
    QCOMPARE(accountSet(QMailAccountKey::fromAddress(from1, Equal)), accountSet() << accountId1);
    QCOMPARE(accountSet(~QMailAccountKey::fromAddress(from1, Equal)), accountSet() << accountId2 << accountId3 << accountId4);
    QCOMPARE(accountSet(QMailAccountKey::fromAddress(from2, Equal)), accountSet() << accountId2);
    QCOMPARE(accountSet(~QMailAccountKey::fromAddress(from2, Equal)), accountSet() << accountId1 << accountId3 << accountId4);
    QCOMPARE(accountSet(QMailAccountKey::fromAddress(QString(""), Equal)), noAccounts);
    QCOMPARE(accountSet(~QMailAccountKey::fromAddress(QString(""), Equal)), allAccounts);
    QCOMPARE(accountSet(QMailAccountKey::fromAddress(QString(), Equal)), noAccounts);
    QCOMPARE(accountSet(~QMailAccountKey::fromAddress(QString(), Equal)), allAccounts);

    // Inequality
    QCOMPARE(accountSet(QMailAccountKey::fromAddress(from1, NotEqual)), accountSet() << accountId2 << accountId3 << accountId4);
    QCOMPARE(accountSet(~QMailAccountKey::fromAddress(from1, NotEqual)), accountSet() << accountId1);
    QCOMPARE(accountSet(QMailAccountKey::fromAddress(from2, NotEqual)), accountSet() << accountId1 << accountId3 << accountId4);
    QCOMPARE(accountSet(~QMailAccountKey::fromAddress(from2, NotEqual)), accountSet() << accountId2);
    QCOMPARE(accountSet(QMailAccountKey::fromAddress(QString(""), NotEqual)), allAccounts);
    QCOMPARE(accountSet(~QMailAccountKey::fromAddress(QString(""), NotEqual)), noAccounts);
    QCOMPARE(accountSet(QMailAccountKey::fromAddress(QString(), NotEqual)), allAccounts);
    QCOMPARE(accountSet(~QMailAccountKey::fromAddress(QString(), NotEqual)), noAccounts);

    // List inclusion - not possible due to pattern matching

    // String Inclusion
    QCOMPARE(accountSet(QMailAccountKey::fromAddress(sub1, Includes)), allAccounts);
    QCOMPARE(accountSet(~QMailAccountKey::fromAddress(sub1, Includes)), noAccounts);
    QCOMPARE(accountSet(QMailAccountKey::fromAddress(sub2, Includes)), accountSet() << accountId1 << accountId2);
    QCOMPARE(accountSet(~QMailAccountKey::fromAddress(sub2, Includes)), accountSet() << accountId3 << accountId4);
    QCOMPARE(accountSet(QMailAccountKey::fromAddress(QString("1"), Includes)), accountSet() << accountId1);
    QCOMPARE(accountSet(~QMailAccountKey::fromAddress(QString("1"), Includes)), accountSet() << accountId2 << accountId3 << accountId4);
    QCOMPARE(accountSet(QMailAccountKey::fromAddress(QString(""), Includes)), allAccounts);
    QCOMPARE(accountSet(~QMailAccountKey::fromAddress(QString(""), Includes)), noAccounts);
    QCOMPARE(accountSet(QMailAccountKey::fromAddress(QString(), Includes)), allAccounts);
    QCOMPARE(accountSet(~QMailAccountKey::fromAddress(QString(), Includes)), noAccounts);

    // String Exclusion
    QCOMPARE(accountSet(QMailAccountKey::fromAddress(sub1, Excludes)), noAccounts);
    QCOMPARE(accountSet(~QMailAccountKey::fromAddress(sub1, Excludes)), allAccounts);
    QCOMPARE(accountSet(QMailAccountKey::fromAddress(sub2, Excludes)), accountSet() << accountId3 << accountId4);
    QCOMPARE(accountSet(~QMailAccountKey::fromAddress(sub2, Excludes)), accountSet() << accountId1 << accountId2);
    QCOMPARE(accountSet(QMailAccountKey::fromAddress(QString("1"), Excludes)), accountSet() << accountId2 << accountId3 << accountId4);
    QCOMPARE(accountSet(~QMailAccountKey::fromAddress(QString("1"), Excludes)), accountSet() << accountId1);
    QCOMPARE(accountSet(QMailAccountKey::fromAddress(QString(""), Excludes)), noAccounts);
    QCOMPARE(accountSet(~QMailAccountKey::fromAddress(QString(""), Excludes)), allAccounts);
    QCOMPARE(accountSet(QMailAccountKey::fromAddress(QString(), Excludes)), noAccounts);
    QCOMPARE(accountSet(~QMailAccountKey::fromAddress(QString(), Excludes)), allAccounts);
}

void tst_QMailStoreKeys::accountStatus()
{
    quint64 accountStatus1(QMailAccount(accountId1).status());
    quint64 accountStatus2(QMailAccount(accountId2).status());
    quint64 allSet(QMailAccount::SynchronizationEnabled | QMailAccount::Synchronized | QMailAccount::MessageSource | QMailAccount::CanRetrieve | QMailAccount::MessageSink | QMailAccount::CanTransmit);
    quint64 sourceSet(QMailAccount::MessageSource | QMailAccount::CanRetrieve);
    quint64 sinkSet(QMailAccount::MessageSink | QMailAccount::CanTransmit);

    // Status equality
    QCOMPARE(accountSet(QMailAccountKey::status(QMailAccount::Enabled, Equal)), noAccounts);
    QCOMPARE(accountSet(~QMailAccountKey::status(QMailAccount::Enabled, Equal)), allAccounts);
    QCOMPARE(accountSet(QMailAccountKey::status(allSet, Equal)), noAccounts);
    QCOMPARE(accountSet(~QMailAccountKey::status(allSet, Equal)), allAccounts);
    QCOMPARE(accountSet(QMailAccountKey::status(accountStatus1, Equal)), accountSet() << accountId1);
    QCOMPARE(accountSet(~QMailAccountKey::status(accountStatus1, Equal)), accountSet() << accountId2 << accountId3 << accountId4);
    QCOMPARE(accountSet(QMailAccountKey::status(accountStatus2, Equal)), accountSet() << accountId2);
    QCOMPARE(accountSet(~QMailAccountKey::status(accountStatus2, Equal)), accountSet() << accountId1 << accountId3 << accountId4);

    // Status inequality
    QCOMPARE(accountSet(QMailAccountKey::status(QMailAccount::Enabled, NotEqual)), allAccounts);
    QCOMPARE(accountSet(~QMailAccountKey::status(QMailAccount::Enabled, NotEqual)), noAccounts);
    QCOMPARE(accountSet(QMailAccountKey::status(allSet, NotEqual)), allAccounts);
    QCOMPARE(accountSet(~QMailAccountKey::status(allSet, NotEqual)), noAccounts);
    QCOMPARE(accountSet(QMailAccountKey::status(accountStatus1, NotEqual)), accountSet() << accountId2 << accountId3 << accountId4);
    QCOMPARE(accountSet(~QMailAccountKey::status(accountStatus1, NotEqual)), accountSet() << accountId1);
    QCOMPARE(accountSet(QMailAccountKey::status(accountStatus2, NotEqual)), accountSet() << accountId1 << accountId3 << accountId4);
    QCOMPARE(accountSet(~QMailAccountKey::status(accountStatus2, NotEqual)), accountSet() << accountId2);

    // Bitwise exclusion
    QCOMPARE(accountSet(QMailAccountKey::status(allSet, Excludes)), accountSet() << accountId3 << accountId4);
    QCOMPARE(accountSet(~QMailAccountKey::status(allSet, Excludes)), accountSet() << accountId1 << accountId2);
    QCOMPARE(accountSet(QMailAccountKey::status(sourceSet, Excludes)), accountSet() << accountId3 << accountId4);
    QCOMPARE(accountSet(~QMailAccountKey::status(sourceSet, Excludes)), accountSet() << accountId1 << accountId2);
    QCOMPARE(accountSet(QMailAccountKey::status(sinkSet, Excludes)), accountSet() << accountId2 << accountId3 << accountId4);
    QCOMPARE(accountSet(~QMailAccountKey::status(sinkSet, Excludes)), accountSet() << accountId1);
    QCOMPARE(accountSet(QMailAccountKey::status(QMailAccount::SynchronizationEnabled | QMailAccount::CanTransmit, Excludes)), accountSet() << accountId3 << accountId4);
    QCOMPARE(accountSet(~QMailAccountKey::status(QMailAccount::SynchronizationEnabled | QMailAccount::CanTransmit, Excludes)), accountSet() << accountId1 << accountId2);
}

void tst_QMailStoreKeys::accountCustomField()
{
    // Test for existence
    QCOMPARE(accountSet(QMailAccountKey::customField("verified")), accountSet() << accountId1 << accountId2 << accountId3);
    QCOMPARE(accountSet(~QMailAccountKey::customField("verified")), accountSet() << accountId4);
    QCOMPARE(accountSet(QMailAccountKey::customField("question")), verifiedAccounts);
    QCOMPARE(accountSet(~QMailAccountKey::customField("question")), unverifiedAccounts);
    QCOMPARE(accountSet(QMailAccountKey::customField("answer")), verifiedAccounts);
    QCOMPARE(accountSet(~QMailAccountKey::customField("answer")), unverifiedAccounts);
    QCOMPARE(accountSet(QMailAccountKey::customField("bicycle")), noAccounts);
    QCOMPARE(accountSet(~QMailAccountKey::customField("bicycle")), allAccounts);
    QCOMPARE(accountSet(QMailAccountKey::customField(QString(""))), noAccounts);
    QCOMPARE(accountSet(~QMailAccountKey::customField(QString(""))), allAccounts);
    QCOMPARE(accountSet(QMailAccountKey::customField(QString())), noAccounts);
    QCOMPARE(accountSet(~QMailAccountKey::customField(QString())), allAccounts);

    // Test for non-existence
    QCOMPARE(accountSet(QMailAccountKey::customField("verified", Absent)), accountSet() << accountId4);
    QCOMPARE(accountSet(~QMailAccountKey::customField("verified", Absent)), accountSet() << accountId1 << accountId2 << accountId3);
    QCOMPARE(accountSet(QMailAccountKey::customField("question", Absent)), unverifiedAccounts);
    QCOMPARE(accountSet(~QMailAccountKey::customField("question", Absent)), verifiedAccounts);
    QCOMPARE(accountSet(QMailAccountKey::customField("answer", Absent)), unverifiedAccounts);
    QCOMPARE(accountSet(~QMailAccountKey::customField("answer", Absent)), verifiedAccounts);
    QCOMPARE(accountSet(QMailAccountKey::customField("bicycle", Absent)), allAccounts);
    QCOMPARE(accountSet(~QMailAccountKey::customField("bicycle", Absent)), noAccounts);
    QCOMPARE(accountSet(QMailAccountKey::customField(QString(""), Absent)), allAccounts);
    QCOMPARE(accountSet(~QMailAccountKey::customField(QString(""), Absent)), noAccounts);
    QCOMPARE(accountSet(QMailAccountKey::customField(QString(), Absent)), allAccounts);
    QCOMPARE(accountSet(~QMailAccountKey::customField(QString(), Absent)), noAccounts);
}

void tst_QMailStoreKeys::folderId()
{
    // ID equality
    QCOMPARE(folderSet(QMailFolderKey::id(inboxId1, Equal)), folderSet() << inboxId1);
    QCOMPARE(folderSet(~QMailFolderKey::id(inboxId1, Equal)), standardFolders + folderSet() << savedId1 << archivedId1 << inboxId2 << savedId2 << archivedId2);
    QCOMPARE(folderSet(QMailFolderKey::id(archivedId2, Equal)), folderSet() << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::id(archivedId2, Equal)), standardFolders + folderSet() << inboxId1 << savedId1 << archivedId1 << inboxId2 << savedId2);
    QCOMPARE(folderSet(QMailFolderKey::id(QMailFolderId(), Equal)), noFolders);
    QCOMPARE(folderSet(~QMailFolderKey::id(QMailFolderId(), Equal)), standardFolders + allFolders);

    // ID inequality
    QCOMPARE(folderSet(QMailFolderKey::id(inboxId1, NotEqual)), standardFolders + folderSet() << savedId1 << archivedId1 << inboxId2 << savedId2 << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::id(inboxId1, NotEqual)), folderSet() << inboxId1);
    QCOMPARE(folderSet(QMailFolderKey::id(archivedId2, NotEqual)), standardFolders + folderSet() << inboxId1 << savedId1 << archivedId1 << inboxId2 << savedId2);
    QCOMPARE(folderSet(~QMailFolderKey::id(archivedId2, NotEqual)), folderSet() << archivedId2);
    QCOMPARE(folderSet(QMailFolderKey::id(QMailFolderId(), NotEqual)), standardFolders + allFolders);
    QCOMPARE(folderSet(~QMailFolderKey::id(QMailFolderId(), NotEqual)), noFolders);

    // List inclusion
    QCOMPARE(folderSet(QMailFolderKey::id(allFolders.toList())), allFolders);
    QCOMPARE(folderSet(~QMailFolderKey::id(allFolders.toList())), standardFolders);
    QCOMPARE(folderSet(QMailFolderKey::id(QMailFolderIdList() << inboxId1)), folderSet() << inboxId1);
    QCOMPARE(folderSet(~QMailFolderKey::id(QMailFolderIdList() << inboxId1)), standardFolders + folderSet() << savedId1 << archivedId1 << inboxId2 << savedId2 << archivedId2);
    QCOMPARE(folderSet(QMailFolderKey::id(QMailFolderIdList() << archivedId2)), folderSet() << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::id(QMailFolderIdList() << archivedId2)), standardFolders + folderSet() << inboxId1 << savedId1 << archivedId1 << inboxId2 << savedId2);
    QCOMPARE(folderSet(QMailFolderKey::id(QMailFolderIdList() << inboxId1 << archivedId2)), folderSet() << inboxId1 << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::id(QMailFolderIdList() << inboxId1 << archivedId2)), standardFolders + folderSet() << savedId1 << archivedId1 << inboxId2 << savedId2);

    // List exclusion
    QCOMPARE(folderSet(QMailFolderKey::id(allFolders.toList(), Excludes)), standardFolders);
    QCOMPARE(folderSet(~QMailFolderKey::id(allFolders.toList(), Excludes)), allFolders);
    QCOMPARE(folderSet(QMailFolderKey::id(QMailFolderIdList() << inboxId1, Excludes)), standardFolders + folderSet() << savedId1 << archivedId1 << inboxId2 << savedId2 << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::id(QMailFolderIdList() << inboxId1, Excludes)), folderSet() << inboxId1);
    QCOMPARE(folderSet(QMailFolderKey::id(QMailFolderIdList() << archivedId2, Excludes)), standardFolders + folderSet() << inboxId1 << savedId1 << archivedId1 << inboxId2 << savedId2);
    QCOMPARE(folderSet(~QMailFolderKey::id(QMailFolderIdList() << archivedId2, Excludes)), folderSet() << archivedId2);
    QCOMPARE(folderSet(QMailFolderKey::id(QMailFolderIdList() << inboxId1 << archivedId2, Excludes)), standardFolders + folderSet() << savedId1 << archivedId1 << inboxId2 << savedId2);
    QCOMPARE(folderSet(~QMailFolderKey::id(QMailFolderIdList() << inboxId1 << archivedId2, Excludes)), folderSet() << inboxId1 << archivedId2);

    // Key matching
    QCOMPARE(folderSet(QMailFolderKey::id(inboxId1, Equal)), folderSet() << inboxId1);
    QCOMPARE(folderSet(~QMailFolderKey::id(inboxId1, Equal)), standardFolders + folderSet() << savedId1 << archivedId1 << inboxId2 << savedId2 << archivedId2);
    QCOMPARE(folderSet(QMailFolderKey::id(QMailFolderKey::id(inboxId1, Equal))), folderSet() << inboxId1);
    QCOMPARE(folderSet(QMailFolderKey::id(~QMailFolderKey::id(inboxId1, Equal))), standardFolders + folderSet() << savedId1 << archivedId1 << inboxId2 << savedId2 << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::id(QMailFolderKey::id(inboxId1, Equal))), standardFolders + folderSet() << savedId1 << archivedId1 << inboxId2 << savedId2 << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::id(~QMailFolderKey::id(inboxId1, Equal))), folderSet() << inboxId1);

    QString path("Inbox/Saved");
    QCOMPARE(folderSet(QMailFolderKey::path(path, Equal)), folderSet() << savedId1 << savedId2);
    QCOMPARE(folderSet(~QMailFolderKey::path(path, Equal)), standardFolders + folderSet() << inboxId1 << archivedId1 << inboxId2 << archivedId2);
    QCOMPARE(folderSet(QMailFolderKey::id(QMailFolderKey::path(path, Equal))), folderSet() << savedId1 << savedId2);
    QCOMPARE(folderSet(QMailFolderKey::id(~QMailFolderKey::path(path, Equal))), standardFolders + folderSet() << inboxId1 << archivedId1 << inboxId2 << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::id(QMailFolderKey::path(path, Equal))), standardFolders + folderSet() << inboxId1 << archivedId1 << inboxId2 << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::id(~QMailFolderKey::path(path, Equal))), folderSet() << savedId1 << savedId2);

    QCOMPARE(folderSet(QMailFolderKey::status(QMailFolder::Synchronized, Includes)), folderSet() << savedId1 << savedId2);
    QCOMPARE(folderSet(~QMailFolderKey::status(QMailFolder::Synchronized, Includes)), standardFolders + folderSet() << inboxId1 << archivedId1 << inboxId2 << archivedId2);
    QCOMPARE(folderSet(QMailFolderKey::id(QMailFolderKey::status(QMailFolder::Synchronized, Includes))), folderSet() << savedId1 << savedId2);
    QCOMPARE(folderSet(QMailFolderKey::id(~QMailFolderKey::status(QMailFolder::Synchronized, Includes))), standardFolders + folderSet() << inboxId1 << archivedId1 << inboxId2 << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::id(QMailFolderKey::status(QMailFolder::Synchronized, Includes))), standardFolders + folderSet() << inboxId1 << archivedId1 << inboxId2 << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::id(~QMailFolderKey::status(QMailFolder::Synchronized, Includes))), folderSet() << savedId1 << savedId2);
}

void tst_QMailStoreKeys::folderPath()
{
    QString path1("Inbox/Saved"), path2("Inbox/Saved/Archived"), sub("Saved");

    // Equality
    QCOMPARE(folderSet(QMailFolderKey::path(QString("Local Storage"), Equal)), folderSet() << QMailFolder::LocalStorageFolderId);
    QCOMPARE(folderSet(~QMailFolderKey::path(QString("Local Storage"), Equal)), allFolders);
    QCOMPARE(folderSet(QMailFolderKey::path(path1, Equal)), folderSet() << savedId1 << savedId2);
    QCOMPARE(folderSet(~QMailFolderKey::path(path1, Equal)), standardFolders + folderSet() << inboxId1 << archivedId1 << inboxId2 << archivedId2);
    QCOMPARE(folderSet(QMailFolderKey::path(path2, Equal)), folderSet() << archivedId1 << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::path(path2, Equal)), standardFolders + folderSet() << inboxId1 << savedId1 << inboxId2 << savedId2);
    QCOMPARE(folderSet(QMailFolderKey::path(QString(""), Equal)), noFolders);
    QCOMPARE(folderSet(~QMailFolderKey::path(QString(""), Equal)), standardFolders + allFolders);
    QCOMPARE(folderSet(QMailFolderKey::path(QString(), Equal)), noFolders);
    QCOMPARE(folderSet(~QMailFolderKey::path(QString(), Equal)), standardFolders + allFolders);

    // Inequality
    QCOMPARE(folderSet(QMailFolderKey::path(QString("Local Storage"), NotEqual)), allFolders);
    QCOMPARE(folderSet(~QMailFolderKey::path(QString("Local Storage"), NotEqual)), folderSet() << QMailFolder::LocalStorageFolderId);
    QCOMPARE(folderSet(QMailFolderKey::path(path1, NotEqual)), standardFolders + folderSet() << inboxId1 << archivedId1 << inboxId2 << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::path(path1, NotEqual)), folderSet() << savedId1 << savedId2);
    QCOMPARE(folderSet(QMailFolderKey::path(path2, NotEqual)), standardFolders + folderSet() << inboxId1 << savedId1 << inboxId2 << savedId2);
    QCOMPARE(folderSet(~QMailFolderKey::path(path2, NotEqual)), folderSet() << archivedId1 << archivedId2);
    QCOMPARE(folderSet(QMailFolderKey::path(QString(""), NotEqual)), standardFolders + allFolders);
    QCOMPARE(folderSet(~QMailFolderKey::path(QString(""), NotEqual)), noFolders);
    QCOMPARE(folderSet(QMailFolderKey::path(QString(), NotEqual)), standardFolders + allFolders);
    QCOMPARE(folderSet(~QMailFolderKey::path(QString(), NotEqual)), noFolders);

    // List inclusion
    QCOMPARE(folderSet(QMailFolderKey::path(QStringList())), noFolders);
    QCOMPARE(folderSet(~QMailFolderKey::path(QStringList())), standardFolders + allFolders);
    QCOMPARE(folderSet(QMailFolderKey::path(QStringList() << path1)), folderSet() << savedId1 << savedId2);
    QCOMPARE(folderSet(~QMailFolderKey::path(QStringList() << path1)), standardFolders + folderSet() << inboxId1 << archivedId1 << inboxId2 << archivedId2);
    QCOMPARE(folderSet(QMailFolderKey::path(QStringList() << path1 << path2)), folderSet() << savedId1 << archivedId1 << savedId2 << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::path(QStringList() << path1 << path2)), standardFolders + folderSet() << inboxId1 << inboxId2);

    // List exclusion
    QCOMPARE(folderSet(QMailFolderKey::path(QStringList(), Excludes)), standardFolders + allFolders);
    QCOMPARE(folderSet(~QMailFolderKey::path(QStringList(), Excludes)), noFolders);
    QCOMPARE(folderSet(QMailFolderKey::path(QStringList() << path1, Excludes)), standardFolders + folderSet() << inboxId1 << archivedId1 << inboxId2 << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::path(QStringList() << path1, Excludes)), folderSet() << savedId1 << savedId2);
    QCOMPARE(folderSet(QMailFolderKey::path(QStringList() << path1 << path2, Excludes)), standardFolders + folderSet() << inboxId1 << inboxId2);
    QCOMPARE(folderSet(~QMailFolderKey::path(QStringList() << path1 << path2, Excludes)), folderSet() << savedId1 << archivedId1 << savedId2 << archivedId2);

    // Inclusion
    QCOMPARE(folderSet(QMailFolderKey::path(sub, Includes)), folderSet() << savedId1 << archivedId1 << savedId2 << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::path(sub, Includes)), standardFolders + folderSet() << inboxId1 << inboxId2);
    QCOMPARE(folderSet(QMailFolderKey::path(QString("ocal"), Includes)), folderSet() << QMailFolder::LocalStorageFolderId);
    QCOMPARE(folderSet(~QMailFolderKey::path(QString("ocal"), Includes)), allFolders);
    QCOMPARE(folderSet(QMailFolderKey::path(QString(""), Includes)), standardFolders + allFolders);
    QCOMPARE(folderSet(~QMailFolderKey::path(QString(""), Includes)), noFolders);
    QCOMPARE(folderSet(QMailFolderKey::path(QString(), Includes)), standardFolders + allFolders);
    QCOMPARE(folderSet(~QMailFolderKey::path(QString(), Includes)), noFolders);

    // Exclusion
    QCOMPARE(folderSet(QMailFolderKey::path(sub, Excludes)), standardFolders + folderSet() << inboxId1 << inboxId2);
    QCOMPARE(folderSet(~QMailFolderKey::path(sub, Excludes)), folderSet() << savedId1 << archivedId1 << savedId2 << archivedId2);
    QCOMPARE(folderSet(QMailFolderKey::path(QString("ocal"), Excludes)), allFolders);
    QCOMPARE(folderSet(~QMailFolderKey::path(QString("ocal"), Excludes)), folderSet() << QMailFolder::LocalStorageFolderId);
    QCOMPARE(folderSet(QMailFolderKey::path(QString(""), Excludes)), noFolders);
    QCOMPARE(folderSet(~QMailFolderKey::path(QString(""), Excludes)), standardFolders + allFolders);
    QCOMPARE(folderSet(QMailFolderKey::path(QString(), Excludes)), noFolders);
    QCOMPARE(folderSet(~QMailFolderKey::path(QString(), Excludes)), standardFolders + allFolders);
}

void tst_QMailStoreKeys::folderParentFolderId()
{
    // ID equality
    QCOMPARE(folderSet(QMailFolderKey::parentFolderId(inboxId1, Equal)), folderSet() << savedId1);
    QCOMPARE(folderSet(~QMailFolderKey::parentFolderId(inboxId1, Equal)), standardFolders + folderSet() << inboxId1 << archivedId1 << inboxId2 << savedId2 << archivedId2);
    QCOMPARE(folderSet(QMailFolderKey::parentFolderId(savedId2, Equal)), folderSet() << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::parentFolderId(savedId2, Equal)), standardFolders + folderSet() << inboxId1 << savedId1 << archivedId1 << inboxId2 << savedId2);
    QCOMPARE(folderSet(QMailFolderKey::parentFolderId(QMailFolderId(), Equal)), standardFolders + folderSet() << inboxId1 << inboxId2);
    QCOMPARE(folderSet(~QMailFolderKey::parentFolderId(QMailFolderId(), Equal)), folderSet() << savedId1 << archivedId1 << savedId2 << archivedId2);

    // ID inequality
    QCOMPARE(folderSet(QMailFolderKey::parentFolderId(inboxId1, NotEqual)), standardFolders + folderSet() << inboxId1 << archivedId1 << inboxId2 << savedId2 << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::parentFolderId(inboxId1, NotEqual)), folderSet() << savedId1);
    QCOMPARE(folderSet(QMailFolderKey::parentFolderId(savedId2, NotEqual)), standardFolders + folderSet() << inboxId1 << savedId1 << archivedId1 << inboxId2 << savedId2);
    QCOMPARE(folderSet(~QMailFolderKey::parentFolderId(savedId2, NotEqual)), folderSet() << archivedId2);
    QCOMPARE(folderSet(QMailFolderKey::parentFolderId(QMailFolderId(), NotEqual)), folderSet() << savedId1 << archivedId1 << savedId2 << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::parentFolderId(QMailFolderId(), NotEqual)), standardFolders + folderSet() << inboxId1 << inboxId2);

    // List inclusion
    QCOMPARE(folderSet(QMailFolderKey::parentFolderId(QMailFolderIdList())), noFolders);
    QCOMPARE(folderSet(~QMailFolderKey::parentFolderId(QMailFolderIdList())), standardFolders + allFolders);
    QCOMPARE(folderSet(QMailFolderKey::parentFolderId(QMailFolderIdList() << inboxId1)), folderSet() << savedId1);
    QCOMPARE(folderSet(~QMailFolderKey::parentFolderId(QMailFolderIdList() << inboxId1)), standardFolders + folderSet() << inboxId1 << archivedId1 << inboxId2 << savedId2 << archivedId2);
    QCOMPARE(folderSet(QMailFolderKey::parentFolderId(QMailFolderIdList() << inboxId1 << savedId1)), folderSet() << savedId1 << archivedId1);
    QCOMPARE(folderSet(~QMailFolderKey::parentFolderId(QMailFolderIdList() << inboxId1 << savedId1)), standardFolders + folderSet() << inboxId1 << inboxId2 << savedId2 << archivedId2);

    // List exclusion
    QCOMPARE(folderSet(QMailFolderKey::parentFolderId(QMailFolderIdList(), Excludes)), standardFolders + allFolders);
    QCOMPARE(folderSet(~QMailFolderKey::parentFolderId(QMailFolderIdList(), Excludes)), noFolders);
    QCOMPARE(folderSet(QMailFolderKey::parentFolderId(QMailFolderIdList() << inboxId1, Excludes)), standardFolders + folderSet() << inboxId1 << archivedId1 << inboxId2 << savedId2 << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::parentFolderId(QMailFolderIdList() << inboxId1, Excludes)), folderSet() << savedId1);
    QCOMPARE(folderSet(QMailFolderKey::parentFolderId(QMailFolderIdList() << inboxId1 << savedId1, Excludes)), standardFolders + folderSet() << inboxId1 << inboxId2 << savedId2 << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::parentFolderId(QMailFolderIdList() << inboxId1 << savedId1, Excludes)), folderSet() << savedId1 << archivedId1);

    // Key matching
    QCOMPARE(folderSet(QMailFolderKey::id(inboxId1, Equal)), folderSet() << inboxId1);
    QCOMPARE(folderSet(~QMailFolderKey::id(inboxId1, Equal)), standardFolders + folderSet() << savedId1 << archivedId1 << inboxId2 << savedId2 << archivedId2);
    QCOMPARE(folderSet(QMailFolderKey::parentFolderId(QMailFolderKey::id(inboxId1, Equal))), folderSet() << savedId1);
    QCOMPARE(folderSet(QMailFolderKey::parentFolderId(~QMailFolderKey::id(inboxId1, Equal))), folderSet() << archivedId1 << savedId2 << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::parentFolderId(QMailFolderKey::id(inboxId1, Equal))), standardFolders + folderSet() << inboxId1 << archivedId1 << inboxId2 << savedId2 << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::parentFolderId(~QMailFolderKey::id(inboxId1, Equal))), standardFolders + folderSet() << inboxId1 << savedId1 << inboxId2);

    QString path("Inbox/Saved");
    QCOMPARE(folderSet(QMailFolderKey::path(path, Equal)), folderSet() << savedId1 << savedId2);
    QCOMPARE(folderSet(~QMailFolderKey::path(path, Equal)), standardFolders + folderSet() << inboxId1 << archivedId1 << inboxId2 << archivedId2);
    QCOMPARE(folderSet(QMailFolderKey::parentFolderId(QMailFolderKey::path(path, Equal))), folderSet() << archivedId1 << archivedId2);
    QCOMPARE(folderSet(QMailFolderKey::parentFolderId(~QMailFolderKey::path(path, Equal))), folderSet() << savedId1 << savedId2);
    QCOMPARE(folderSet(~QMailFolderKey::parentFolderId(QMailFolderKey::path(path, Equal))), standardFolders + folderSet() << inboxId1 << savedId1 << inboxId2 << savedId2);
    QCOMPARE(folderSet(~QMailFolderKey::parentFolderId(~QMailFolderKey::path(path, Equal))), standardFolders + folderSet() << inboxId1 << archivedId1 << inboxId2 << archivedId2);

    QCOMPARE(folderSet(QMailFolderKey::status(QMailFolder::Synchronized, Includes)), folderSet() << savedId1 << savedId2);
    QCOMPARE(folderSet(~QMailFolderKey::status(QMailFolder::Synchronized, Includes)), standardFolders + folderSet() << inboxId1 << archivedId1 << inboxId2 << archivedId2);
    QCOMPARE(folderSet(QMailFolderKey::parentFolderId(QMailFolderKey::status(QMailFolder::Synchronized, Includes))), folderSet() << archivedId1 << archivedId2);
    QCOMPARE(folderSet(QMailFolderKey::parentFolderId(~QMailFolderKey::status(QMailFolder::Synchronized, Includes))), folderSet() << savedId1 << savedId2);
    QCOMPARE(folderSet(~QMailFolderKey::parentFolderId(QMailFolderKey::status(QMailFolder::Synchronized, Includes))), standardFolders + folderSet() << inboxId1 << savedId1 << inboxId2 << savedId2);
    QCOMPARE(folderSet(~QMailFolderKey::parentFolderId(~QMailFolderKey::status(QMailFolder::Synchronized, Includes))), standardFolders + folderSet() << inboxId1 << archivedId1 << inboxId2 << archivedId2);
}

void tst_QMailStoreKeys::folderParentAccountId()
{
    // ID equality
    QCOMPARE(folderSet(QMailFolderKey::parentAccountId(accountId1, Equal)), folderSet() << inboxId1 << savedId1 << archivedId1);
    QCOMPARE(folderSet(~QMailFolderKey::parentAccountId(accountId1, Equal)), standardFolders + folderSet() << inboxId2 << savedId2 << archivedId2);
    QCOMPARE(folderSet(QMailFolderKey::parentAccountId(accountId4, Equal)), noFolders);
    QCOMPARE(folderSet(~QMailFolderKey::parentAccountId(accountId4, Equal)), standardFolders + allFolders);
    QCOMPARE(folderSet(QMailFolderKey::parentAccountId(QMailAccountId(), Equal)), standardFolders);
    QCOMPARE(folderSet(~QMailFolderKey::parentAccountId(QMailAccountId(), Equal)), allFolders);

    // ID inequality
    QCOMPARE(folderSet(QMailFolderKey::parentAccountId(accountId1, NotEqual)), standardFolders + folderSet() << inboxId2 << savedId2 << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::parentAccountId(accountId1, NotEqual)), folderSet() << inboxId1 << savedId1 << archivedId1);
    QCOMPARE(folderSet(QMailFolderKey::parentAccountId(accountId4, NotEqual)), standardFolders + allFolders);
    QCOMPARE(folderSet(~QMailFolderKey::parentAccountId(accountId4, NotEqual)), noFolders);
    QCOMPARE(folderSet(QMailFolderKey::parentAccountId(QMailAccountId(), NotEqual)), allFolders);
    QCOMPARE(folderSet(~QMailFolderKey::parentAccountId(QMailAccountId(), NotEqual)), standardFolders);

    // List inclusion
    QCOMPARE(folderSet(QMailFolderKey::parentAccountId(QMailAccountIdList())), noFolders);
    QCOMPARE(folderSet(~QMailFolderKey::parentAccountId(QMailAccountIdList())), standardFolders + allFolders);
    QCOMPARE(folderSet(QMailFolderKey::parentAccountId(QMailAccountIdList() << accountId1)), folderSet() << inboxId1 << savedId1 << archivedId1);
    QCOMPARE(folderSet(~QMailFolderKey::parentAccountId(QMailAccountIdList() << accountId1)), standardFolders + folderSet() << inboxId2 << savedId2 << archivedId2);
    QCOMPARE(folderSet(QMailFolderKey::parentAccountId(QMailAccountIdList() << accountId1 << accountId2)), allFolders);
    QCOMPARE(folderSet(~QMailFolderKey::parentAccountId(QMailAccountIdList() << accountId1 << accountId2)), standardFolders);

    // List exclusion
    QCOMPARE(folderSet(QMailFolderKey::parentAccountId(QMailAccountIdList(), Excludes)), standardFolders + allFolders);
    QCOMPARE(folderSet(~QMailFolderKey::parentAccountId(QMailAccountIdList(), Excludes)), noFolders);
    QCOMPARE(folderSet(QMailFolderKey::parentAccountId(QMailAccountIdList() << accountId1, Excludes)), standardFolders + folderSet() << inboxId2 << savedId2 << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::parentAccountId(QMailAccountIdList() << accountId1, Excludes)), folderSet() << inboxId1 << savedId1 << archivedId1);
    QCOMPARE(folderSet(QMailFolderKey::parentAccountId(QMailAccountIdList() << accountId1 << accountId2, Excludes)), standardFolders);
    QCOMPARE(folderSet(~QMailFolderKey::parentAccountId(QMailAccountIdList() << accountId1 << accountId2, Excludes)), allFolders);

    // Key matching
    QCOMPARE(accountSet(QMailAccountKey::id(accountId1, Equal)), accountSet() << accountId1);
    QCOMPARE(accountSet(~QMailAccountKey::id(accountId1, Equal)), accountSet() << accountId2 << accountId3 << accountId4);
    QCOMPARE(folderSet(QMailFolderKey::parentAccountId(QMailAccountKey::id(accountId1, Equal))), folderSet() << inboxId1 << savedId1 << archivedId1);
    QCOMPARE(folderSet(QMailFolderKey::parentAccountId(~QMailAccountKey::id(accountId1, Equal))), folderSet() << inboxId2 << savedId2 << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::parentAccountId(QMailAccountKey::id(accountId1, Equal))), standardFolders + folderSet() << inboxId2 << savedId2 << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::parentAccountId(~QMailAccountKey::id(accountId1, Equal))), standardFolders + folderSet() << inboxId1 << savedId1 << archivedId1);

    QString name("Account 1");
    QCOMPARE(accountSet(QMailAccountKey::name(name, Equal)), accountSet() << accountId1);
    QCOMPARE(accountSet(~QMailAccountKey::name(name, Equal)), accountSet() << accountId2 << accountId3 << accountId4);
    QCOMPARE(folderSet(QMailFolderKey::parentAccountId(QMailAccountKey::name(name, Equal))), folderSet() << inboxId1 << savedId1 << archivedId1);
    QCOMPARE(folderSet(QMailFolderKey::parentAccountId(~QMailAccountKey::name(name, Equal))), folderSet() << inboxId2 << savedId2 << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::parentAccountId(QMailAccountKey::name(name, Equal))), standardFolders + folderSet() << inboxId2 << savedId2 << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::parentAccountId(~QMailAccountKey::name(name, Equal))), standardFolders + folderSet() << inboxId1 << savedId1 << archivedId1);

    QCOMPARE(accountSet(QMailAccountKey::messageType(QMailMessage::Instant, Equal)), accountSet() << accountId2);
    QCOMPARE(accountSet(~QMailAccountKey::messageType(QMailMessage::Instant, Equal)), accountSet() << accountId1 << accountId3 << accountId4);
    QCOMPARE(folderSet(QMailFolderKey::parentAccountId(QMailAccountKey::messageType(QMailMessage::Instant, Equal))), folderSet() << inboxId2 << savedId2 << archivedId2);
    QCOMPARE(folderSet(QMailFolderKey::parentAccountId(~QMailAccountKey::messageType(QMailMessage::Instant, Equal))), folderSet() << inboxId1 << savedId1 << archivedId1);
    QCOMPARE(folderSet(~QMailFolderKey::parentAccountId(QMailAccountKey::messageType(QMailMessage::Instant, Equal))), standardFolders + folderSet() << inboxId1 << savedId1 << archivedId1);
    QCOMPARE(folderSet(~QMailFolderKey::parentAccountId(~QMailAccountKey::messageType(QMailMessage::Instant, Equal))), standardFolders + folderSet() << inboxId2 << savedId2 << archivedId2);
}

void tst_QMailStoreKeys::folderDisplayName()
{
    // Note: the standard folders do not have display names pre-configured
    QString name1("Saved"), name2("Archived"), sub("box");

    // Equality
    QCOMPARE(folderSet(QMailFolderKey::displayName(name1, Equal)), folderSet() << savedId1 << savedId2);
    QCOMPARE(folderSet(~QMailFolderKey::displayName(name1, Equal)), folderSet() << inboxId1 << archivedId1 << inboxId2 << archivedId2);
    QCOMPARE(folderSet(QMailFolderKey::displayName(name2, Equal)), folderSet() << archivedId1 << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::displayName(name2, Equal)), folderSet() << inboxId1 << savedId1 << inboxId2 << savedId2);
    QCOMPARE(folderSet(QMailFolderKey::displayName(QString(""), Equal)), noFolders);
    QCOMPARE(folderSet(~QMailFolderKey::displayName(QString(""), Equal)), allFolders);
    QCOMPARE(folderSet(QMailFolderKey::displayName(QString(), Equal)), noFolders);
    QCOMPARE(folderSet(~QMailFolderKey::displayName(QString(), Equal)), allFolders);

    // Inequality
    QCOMPARE(folderSet(QMailFolderKey::displayName(name1, NotEqual)), folderSet() << inboxId1 << archivedId1 << inboxId2 << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::displayName(name1, NotEqual)), folderSet() << savedId1 << savedId2);
    QCOMPARE(folderSet(QMailFolderKey::displayName(name2, NotEqual)), folderSet() << inboxId1 << savedId1 << inboxId2 << savedId2);
    QCOMPARE(folderSet(~QMailFolderKey::displayName(name2, NotEqual)), folderSet() << archivedId1 << archivedId2);
    QCOMPARE(folderSet(QMailFolderKey::displayName(QString(""), NotEqual)), allFolders);
    QCOMPARE(folderSet(~QMailFolderKey::displayName(QString(""), NotEqual)), noFolders);
    QCOMPARE(folderSet(QMailFolderKey::displayName(QString(), NotEqual)), allFolders);
    QCOMPARE(folderSet(~QMailFolderKey::displayName(QString(), NotEqual)), noFolders);

    // List inclusion
    QCOMPARE(folderSet(QMailFolderKey::displayName(QStringList())), noFolders);
    QCOMPARE(folderSet(~QMailFolderKey::displayName(QStringList())), standardFolders + allFolders);
    QCOMPARE(folderSet(QMailFolderKey::displayName(QStringList() << name1)), folderSet() << savedId1 << savedId2);
    QCOMPARE(folderSet(~QMailFolderKey::displayName(QStringList() << name1)), folderSet() << inboxId1 << archivedId1 << inboxId2 << archivedId2);
    QCOMPARE(folderSet(QMailFolderKey::displayName(QStringList() << name1 << name2)), folderSet() << savedId1 << archivedId1 << savedId2 << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::displayName(QStringList() << name1 << name2)), folderSet() << inboxId1 << inboxId2);

    // List exclusion
    QCOMPARE(folderSet(QMailFolderKey::displayName(QStringList(), Excludes)), standardFolders + allFolders);
    QCOMPARE(folderSet(~QMailFolderKey::displayName(QStringList(), Excludes)), noFolders);
    QCOMPARE(folderSet(QMailFolderKey::displayName(QStringList() << name1, Excludes)), folderSet() << inboxId1 << archivedId1 << inboxId2 << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::displayName(QStringList() << name1, Excludes)), folderSet() << savedId1 << savedId2);
    QCOMPARE(folderSet(QMailFolderKey::displayName(QStringList() << name1 << name2, Excludes)), folderSet() << inboxId1 << inboxId2);
    QCOMPARE(folderSet(~QMailFolderKey::displayName(QStringList() << name1 << name2, Excludes)), folderSet() << savedId1 << archivedId1 << savedId2 << archivedId2);

    // Inclusion
    QCOMPARE(folderSet(QMailFolderKey::displayName(sub, Includes)), folderSet() << inboxId1 << inboxId2);
    QCOMPARE(folderSet(~QMailFolderKey::displayName(sub, Includes)), folderSet() << savedId1 << archivedId1 << savedId2 << archivedId2);
    QCOMPARE(folderSet(QMailFolderKey::displayName(QString("Outb"), Includes)), noFolders);
    QCOMPARE(folderSet(~QMailFolderKey::displayName(QString("Outb"), Includes)), allFolders);
    QCOMPARE(folderSet(QMailFolderKey::displayName(QString(""), Includes)), allFolders);
    QCOMPARE(folderSet(~QMailFolderKey::displayName(QString(""), Includes)), noFolders);
    QCOMPARE(folderSet(QMailFolderKey::displayName(QString(), Includes)), allFolders);
    QCOMPARE(folderSet(~QMailFolderKey::displayName(QString(), Includes)), noFolders);

    // Exclusion
    QCOMPARE(folderSet(QMailFolderKey::displayName(sub, Excludes)), folderSet() << savedId1 << archivedId1 << savedId2 << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::displayName(sub, Excludes)), folderSet() << inboxId1 << inboxId2);
    QCOMPARE(folderSet(QMailFolderKey::displayName(QString("Outb"), Excludes)), allFolders);
    QCOMPARE(folderSet(~QMailFolderKey::displayName(QString("Outb"), Excludes)), noFolders);
    QCOMPARE(folderSet(QMailFolderKey::displayName(QString(""), Excludes)), noFolders);
    QCOMPARE(folderSet(~QMailFolderKey::displayName(QString(""), Excludes)), allFolders);
    QCOMPARE(folderSet(QMailFolderKey::displayName(QString(), Excludes)), noFolders);
    QCOMPARE(folderSet(~QMailFolderKey::displayName(QString(), Excludes)), allFolders);
}

void tst_QMailStoreKeys::folderStatus()
{
    quint64 inboxStatus1(QMailFolder(inboxId1).status());
    quint64 inboxStatus2(QMailFolder(inboxId2).status());
    quint64 allSet(QMailFolder::SynchronizationEnabled | QMailFolder::Synchronized);

    // Status equality
    QCOMPARE(folderSet(QMailFolderKey::status(inboxStatus1, Equal)), folderSet() << inboxId1);
    QCOMPARE(folderSet(~QMailFolderKey::status(inboxStatus1, Equal)), standardFolders + folderSet() << savedId1 << archivedId1 << inboxId2 << savedId2 << archivedId2);
    QCOMPARE(folderSet(QMailFolderKey::status(inboxStatus2, Equal)), folderSet() << archivedId1 << inboxId2 << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::status(inboxStatus2, Equal)), standardFolders + folderSet() << inboxId1 << savedId1 << savedId2);
    QCOMPARE(folderSet(QMailFolderKey::status(allSet, Equal)), folderSet() << savedId1);
    QCOMPARE(folderSet(~QMailFolderKey::status(allSet, Equal)), standardFolders + folderSet() << inboxId1 << archivedId1 << inboxId2 << savedId2 << archivedId2);

    // Status inequality
    QCOMPARE(folderSet(QMailFolderKey::status(inboxStatus1, NotEqual)), standardFolders + folderSet() << savedId1 << archivedId1 << inboxId2 << savedId2 << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::status(inboxStatus1, NotEqual)), folderSet() << inboxId1);
    QCOMPARE(folderSet(QMailFolderKey::status(inboxStatus2, NotEqual)), standardFolders + folderSet() << inboxId1 << savedId1 << savedId2);
    QCOMPARE(folderSet(~QMailFolderKey::status(inboxStatus2, NotEqual)), folderSet() << archivedId1 << inboxId2 << archivedId2);
    QCOMPARE(folderSet(QMailFolderKey::status(allSet, NotEqual)), standardFolders + folderSet() << inboxId1 << archivedId1 << inboxId2 << savedId2 << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::status(allSet, NotEqual)), folderSet() << savedId1);

    // Bitwise inclusion
    QCOMPARE(folderSet(QMailFolderKey::status(allSet, Includes)), folderSet() << inboxId1 << savedId1 << savedId2);
    QCOMPARE(folderSet(~QMailFolderKey::status(allSet, Includes)), standardFolders + folderSet() << archivedId1 << inboxId2 << archivedId2);
    QCOMPARE(folderSet(QMailFolderKey::status(QMailFolder::SynchronizationEnabled, Includes)), folderSet() << inboxId1 << savedId1);
    QCOMPARE(folderSet(~QMailFolderKey::status(QMailFolder::SynchronizationEnabled, Includes)), standardFolders + folderSet() << archivedId1 << inboxId2 << savedId2 << archivedId2);
    QCOMPARE(folderSet(QMailFolderKey::status(QMailFolder::Synchronized, Includes)), folderSet() << savedId1 << savedId2);
    QCOMPARE(folderSet(~QMailFolderKey::status(QMailFolder::Synchronized, Includes)), standardFolders + folderSet() << inboxId1 << archivedId1 << inboxId2 << archivedId2);

    // Bitwise exclusion
    QCOMPARE(folderSet(QMailFolderKey::status(allSet, Excludes)), standardFolders + folderSet() << archivedId1 << inboxId2 << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::status(allSet, Excludes)), folderSet() << inboxId1 << savedId1 << savedId2);
    QCOMPARE(folderSet(QMailFolderKey::status(QMailFolder::SynchronizationEnabled, Excludes)), standardFolders + folderSet() << archivedId1 << inboxId2 << savedId2 << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::status(QMailFolder::SynchronizationEnabled, Excludes)), folderSet() << inboxId1 << savedId1);
    QCOMPARE(folderSet(QMailFolderKey::status(QMailFolder::Synchronized, Excludes)), standardFolders + folderSet() << inboxId1 << archivedId1 << inboxId2 << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::status(QMailFolder::Synchronized, Excludes)), folderSet() << savedId1 << savedId2);
}

void tst_QMailStoreKeys::folderAncestorFolderIds()
{
    // ID inclusion
    QCOMPARE(folderSet(QMailFolderKey::ancestorFolderIds(inboxId1, Includes)), folderSet() << savedId1 << archivedId1);
    QCOMPARE(folderSet(~QMailFolderKey::ancestorFolderIds(inboxId1, Includes)), standardFolders + folderSet() << inboxId1 << inboxId2 << savedId2 << archivedId2);
    QCOMPARE(folderSet(QMailFolderKey::ancestorFolderIds(savedId2, Includes)), folderSet() << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::ancestorFolderIds(savedId2, Includes)), standardFolders + folderSet() << inboxId1 << savedId1 << archivedId1 << inboxId2 << savedId2);
    QCOMPARE(folderSet(QMailFolderKey::ancestorFolderIds(QMailFolderId(QMailFolder::LocalStorageFolderId), Includes)), noFolders);
    QCOMPARE(folderSet(~QMailFolderKey::ancestorFolderIds(QMailFolderId(QMailFolder::LocalStorageFolderId), Includes)), standardFolders + allFolders);

    // ID exclusion
    QCOMPARE(folderSet(QMailFolderKey::ancestorFolderIds(inboxId1, Excludes)), standardFolders + folderSet() << inboxId1 << inboxId2 << savedId2 << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::ancestorFolderIds(inboxId1, Excludes)), folderSet() << savedId1 << archivedId1);
    QCOMPARE(folderSet(QMailFolderKey::ancestorFolderIds(savedId2, Excludes)), standardFolders + folderSet() << inboxId1 << savedId1 << archivedId1 << inboxId2 << savedId2);
    QCOMPARE(folderSet(~QMailFolderKey::ancestorFolderIds(savedId2, Excludes)), folderSet() << archivedId2);
    QCOMPARE(folderSet(QMailFolderKey::ancestorFolderIds(QMailFolderId(QMailFolder::LocalStorageFolderId), Excludes)), standardFolders + allFolders);
    QCOMPARE(folderSet(~QMailFolderKey::ancestorFolderIds(QMailFolderId(QMailFolder::LocalStorageFolderId), Excludes)), noFolders);

    // List inclusion
    QCOMPARE(folderSet(QMailFolderKey::ancestorFolderIds(QMailFolderIdList())), noFolders);
    QCOMPARE(folderSet(~QMailFolderKey::ancestorFolderIds(QMailFolderIdList())), standardFolders + allFolders);
    QCOMPARE(folderSet(QMailFolderKey::ancestorFolderIds(QMailFolderIdList() << inboxId1)), folderSet() << savedId1 << archivedId1);
    QCOMPARE(folderSet(~QMailFolderKey::ancestorFolderIds(QMailFolderIdList() << inboxId1)), standardFolders + folderSet() << inboxId1 << inboxId2 << savedId2 << archivedId2);
    QCOMPARE(folderSet(QMailFolderKey::ancestorFolderIds(QMailFolderIdList() << inboxId1 << savedId2)), folderSet() << savedId1 << archivedId1 << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::ancestorFolderIds(QMailFolderIdList() << inboxId1 << savedId2)), standardFolders + folderSet() << inboxId1 << inboxId2 << savedId2);

    // List exclusion
    QCOMPARE(folderSet(QMailFolderKey::ancestorFolderIds(QMailFolderIdList(), Excludes)), standardFolders + allFolders);
    QCOMPARE(folderSet(~QMailFolderKey::ancestorFolderIds(QMailFolderIdList(), Excludes)), noFolders);
    QCOMPARE(folderSet(QMailFolderKey::ancestorFolderIds(QMailFolderIdList() << inboxId1, Excludes)), standardFolders + folderSet() << inboxId1 << inboxId2 << savedId2 << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::ancestorFolderIds(QMailFolderIdList() << inboxId1, Excludes)), folderSet() << savedId1 << archivedId1);
    QCOMPARE(folderSet(QMailFolderKey::ancestorFolderIds(QMailFolderIdList() << inboxId1 << savedId2, Excludes)), standardFolders + folderSet() << inboxId1 << inboxId2 << savedId2);
    QCOMPARE(folderSet(~QMailFolderKey::ancestorFolderIds(QMailFolderIdList() << inboxId1 << savedId2, Excludes)), folderSet() << savedId1 << archivedId1 << archivedId2);

    // Key matching
    QCOMPARE(folderSet(QMailFolderKey::id(inboxId1, Equal)), folderSet() << inboxId1);
    QCOMPARE(folderSet(~QMailFolderKey::id(inboxId1, Equal)), standardFolders + folderSet() << savedId1 << archivedId1 << inboxId2 << savedId2 << archivedId2);
    QCOMPARE(folderSet(QMailFolderKey::ancestorFolderIds(QMailFolderKey::id(inboxId1, Equal), Includes)), folderSet() << savedId1 << archivedId1);
    QCOMPARE(folderSet(QMailFolderKey::ancestorFolderIds(~QMailFolderKey::id(inboxId1, Equal), Includes)), folderSet() << archivedId1 << savedId2 << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::ancestorFolderIds(QMailFolderKey::id(inboxId1, Equal), Includes)), standardFolders + folderSet() << inboxId1 << inboxId2 << savedId2 << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::ancestorFolderIds(~QMailFolderKey::id(inboxId1, Equal), Includes)), standardFolders + folderSet() << inboxId1 << savedId1 << inboxId2);

    QString path("Inbox/Saved");
    QCOMPARE(folderSet(QMailFolderKey::path(path, Equal)), folderSet() << savedId1 << savedId2);
    QCOMPARE(folderSet(~QMailFolderKey::path(path, Equal)), standardFolders + folderSet() << inboxId1 << archivedId1 << inboxId2 << archivedId2);
    QCOMPARE(folderSet(QMailFolderKey::ancestorFolderIds(QMailFolderKey::path(path, Equal))), folderSet() << archivedId1 << archivedId2);
    QCOMPARE(folderSet(QMailFolderKey::ancestorFolderIds(~QMailFolderKey::path(path, Equal))), folderSet() << savedId1 << archivedId1 << savedId2 << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::ancestorFolderIds(QMailFolderKey::path(path, Equal))), standardFolders + folderSet() << inboxId1 << savedId1 << inboxId2 << savedId2);
    QCOMPARE(folderSet(~QMailFolderKey::ancestorFolderIds(~QMailFolderKey::path(path, Equal))), standardFolders + folderSet() << inboxId1 << inboxId2);

    QCOMPARE(folderSet(QMailFolderKey::parentAccountId(accountId2, Equal)), folderSet() << inboxId2 << savedId2 << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::parentAccountId(accountId2, Equal)), standardFolders + folderSet() << inboxId1 << savedId1 << archivedId1);
    QCOMPARE(folderSet(QMailFolderKey::ancestorFolderIds(QMailFolderKey::parentAccountId(accountId2, Equal))), folderSet() << savedId2 << archivedId2);
    QCOMPARE(folderSet(QMailFolderKey::ancestorFolderIds(~QMailFolderKey::parentAccountId(accountId2, Equal))), folderSet() << savedId1 << archivedId1);
    QCOMPARE(folderSet(~QMailFolderKey::ancestorFolderIds(QMailFolderKey::parentAccountId(accountId2, Equal))), standardFolders + folderSet() << inboxId1 << savedId1 << archivedId1 << inboxId2);
    QCOMPARE(folderSet(~QMailFolderKey::ancestorFolderIds(~QMailFolderKey::parentAccountId(accountId2, Equal))), standardFolders + folderSet() << inboxId1 << inboxId2 << savedId2 << archivedId2);
}

void tst_QMailStoreKeys::folderCustomField()
{
    // Test for existence
    QCOMPARE(folderSet(QMailFolderKey::customField("uidNext")), allFolders);
    QCOMPARE(folderSet(~QMailFolderKey::customField("uidNext")), standardFolders);
    QCOMPARE(folderSet(QMailFolderKey::customField("uidValidity")), folderSet() << inboxId1 << savedId1 << inboxId2 << savedId2);
    QCOMPARE(folderSet(~QMailFolderKey::customField("uidValidity")), standardFolders + folderSet() << archivedId1 << archivedId2);
    QCOMPARE(folderSet(QMailFolderKey::customField("archived")), folderSet() << archivedId1 << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::customField("archived")), standardFolders + folderSet() << inboxId1 << savedId1 << inboxId2 << savedId2);
    QCOMPARE(folderSet(QMailFolderKey::customField("bicycle")), noFolders);
    QCOMPARE(folderSet(~QMailFolderKey::customField("bicycle")), standardFolders + allFolders);
    QCOMPARE(folderSet(QMailFolderKey::customField(QString(""))), noFolders);
    QCOMPARE(folderSet(~QMailFolderKey::customField(QString(""))), standardFolders + allFolders);
    QCOMPARE(folderSet(QMailFolderKey::customField(QString())), noFolders);
    QCOMPARE(folderSet(~QMailFolderKey::customField(QString())), standardFolders + allFolders);

    // Test for non-existence
    QCOMPARE(folderSet(QMailFolderKey::customField("uidNext", Absent)), standardFolders);
    QCOMPARE(folderSet(~QMailFolderKey::customField("uidNext", Absent)), allFolders);
    QCOMPARE(folderSet(QMailFolderKey::customField("uidValidity", Absent)), standardFolders + folderSet() << archivedId1 << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::customField("uidValidity", Absent)), folderSet() << inboxId1 << savedId1 << inboxId2 << savedId2);
    QCOMPARE(folderSet(QMailFolderKey::customField("archived", Absent)), standardFolders + folderSet() << inboxId1 << savedId1 << inboxId2 << savedId2);
    QCOMPARE(folderSet(~QMailFolderKey::customField("archived", Absent)), folderSet() << archivedId1 << archivedId2);
    QCOMPARE(folderSet(QMailFolderKey::customField("bicycle", Absent)), standardFolders + allFolders);
    QCOMPARE(folderSet(~QMailFolderKey::customField("bicycle", Absent)), noFolders);
    QCOMPARE(folderSet(QMailFolderKey::customField(QString(""), Absent)), standardFolders + allFolders);
    QCOMPARE(folderSet(~QMailFolderKey::customField(QString(""), Absent)), noFolders);
    QCOMPARE(folderSet(QMailFolderKey::customField(QString(), Absent)), standardFolders + allFolders);
    QCOMPARE(folderSet(~QMailFolderKey::customField(QString(), Absent)), noFolders);

    // Test for content equality
    QCOMPARE(folderSet(QMailFolderKey::customField("uidValidity", "abcdefg")), folderSet() << inboxId1);
    QCOMPARE(folderSet(~QMailFolderKey::customField("uidValidity", "abcdefg")), folderSet() << savedId1 << inboxId2 << savedId2);
    QCOMPARE(folderSet(QMailFolderKey::customField("uidValidity", "wxyz")), folderSet() << savedId2);
    QCOMPARE(folderSet(~QMailFolderKey::customField("uidValidity", "wxyz")), folderSet() << inboxId1 << savedId1 << inboxId2);
    QCOMPARE(folderSet(QMailFolderKey::customField("uidValidity", "bicycle")), noFolders);
    QCOMPARE(folderSet(~QMailFolderKey::customField("uidValidity", "bicycle")), folderSet() << inboxId1 << savedId1 << inboxId2 << savedId2);
    QCOMPARE(folderSet(QMailFolderKey::customField("uidValidity", QString(""))), noFolders);
    QCOMPARE(folderSet(~QMailFolderKey::customField("uidValidity", QString(""))), folderSet() << inboxId1 << savedId1 << inboxId2 << savedId2);
    QCOMPARE(folderSet(QMailFolderKey::customField("uidValidity", QString())), noFolders);
    QCOMPARE(folderSet(~QMailFolderKey::customField("uidValidity", QString())), folderSet() << inboxId1 << savedId1 << inboxId2 << savedId2);

    // Test for content inequality
    QCOMPARE(folderSet(QMailFolderKey::customField("uidValidity", "abcdefg", NotEqual)), folderSet() << savedId1 << inboxId2 << savedId2);
    QCOMPARE(folderSet(~QMailFolderKey::customField("uidValidity", "abcdefg", NotEqual)), folderSet() << inboxId1);
    QCOMPARE(folderSet(QMailFolderKey::customField("uidValidity", "wxyz", NotEqual)), folderSet() << inboxId1 << savedId1 << inboxId2);
    QCOMPARE(folderSet(~QMailFolderKey::customField("uidValidity", "wxyz", NotEqual)), folderSet() << savedId2);
    QCOMPARE(folderSet(QMailFolderKey::customField("uidValidity", "bicycle", NotEqual)), folderSet() << inboxId1 << savedId1 << inboxId2 << savedId2);
    QCOMPARE(folderSet(~QMailFolderKey::customField("uidValidity", "bicycle", NotEqual)), noFolders);
    QCOMPARE(folderSet(QMailFolderKey::customField("uidValidity", QString(""), NotEqual)), folderSet() << inboxId1 << savedId1 << inboxId2 << savedId2);
    QCOMPARE(folderSet(~QMailFolderKey::customField("uidValidity", QString(""), NotEqual)), noFolders);
    QCOMPARE(folderSet(QMailFolderKey::customField("uidValidity", QString(), NotEqual)), folderSet() << inboxId1 << savedId1 << inboxId2 << savedId2);
    QCOMPARE(folderSet(~QMailFolderKey::customField("uidValidity", QString(), NotEqual)), noFolders);

    // Test for partial matches
    QCOMPARE(folderSet(QMailFolderKey::customField("uidValidity", "stu", Includes)), folderSet() << inboxId2);
    QCOMPARE(folderSet(~QMailFolderKey::customField("uidValidity", "stu", Includes)), folderSet() << inboxId1 << savedId1 << savedId2);
    QCOMPARE(folderSet(QMailFolderKey::customField("uidNext", "11", Includes)), folderSet() << savedId1 << archivedId1 << savedId2 << archivedId2); 
    QCOMPARE(folderSet(~QMailFolderKey::customField("uidNext", "11", Includes)), folderSet() << inboxId1 << inboxId2); 
    QCOMPARE(folderSet(QMailFolderKey::customField("uidNext", "bicycle", Includes)), noFolders);
    QCOMPARE(folderSet(~QMailFolderKey::customField("uidNext", "bicycle", Includes)), folderSet() << inboxId1 << savedId1 << archivedId1 << inboxId2 << savedId2 << archivedId2);
    QCOMPARE(folderSet(QMailFolderKey::customField("uidNext", QString(""), Includes)), folderSet() << inboxId1 << savedId1 << archivedId1 << inboxId2 << savedId2 << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::customField("uidNext", QString(""), Includes)), noFolders);
    QCOMPARE(folderSet(QMailFolderKey::customField("uidNext", QString(), Includes)), folderSet() << inboxId1 << savedId1 << archivedId1 << inboxId2 << savedId2 << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::customField("uidNext", QString(), Includes)), noFolders);

    // Test for partial match exclusion
    QCOMPARE(folderSet(QMailFolderKey::customField("uidValidity", "stu", Excludes)), folderSet() << inboxId1 << savedId1 << savedId2);
    QCOMPARE(folderSet(~QMailFolderKey::customField("uidValidity", "stu", Excludes)), folderSet() << inboxId2);
    QCOMPARE(folderSet(QMailFolderKey::customField("uidNext", "11", Excludes)), folderSet() << inboxId1 << inboxId2); 
    QCOMPARE(folderSet(~QMailFolderKey::customField("uidNext", "11", Excludes)), folderSet() << savedId1 << archivedId1 << savedId2 << archivedId2); 
    QCOMPARE(folderSet(QMailFolderKey::customField("uidNext", "bicycle", Excludes)), folderSet() << inboxId1 << savedId1 << archivedId1 << inboxId2 << savedId2 << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::customField("uidNext", "bicycle", Excludes)), noFolders);
    QCOMPARE(folderSet(QMailFolderKey::customField("uidNext", QString(""), Excludes)), noFolders);
    QCOMPARE(folderSet(~QMailFolderKey::customField("uidNext", QString(""), Excludes)), folderSet() << inboxId1 << savedId1 << archivedId1 << inboxId2 << savedId2 << archivedId2);
    QCOMPARE(folderSet(QMailFolderKey::customField("uidNext", QString(), Excludes)), noFolders);
    QCOMPARE(folderSet(~QMailFolderKey::customField("uidNext", QString(), Excludes)), folderSet() << inboxId1 << savedId1 << archivedId1 << inboxId2 << savedId2 << archivedId2);
}

void tst_QMailStoreKeys::messageId()
{
    // ID equality
    QCOMPARE(messageSet(QMailMessageKey::id(smsMessage, Equal)), messageSet() << smsMessage);
    QCOMPARE(messageSet(~QMailMessageKey::id(smsMessage, Equal)), allEmailMessages);
    QCOMPARE(messageSet(QMailMessageKey::id(inboxMessage1, Equal)), messageSet() << inboxMessage1);
    QCOMPARE(messageSet(~QMailMessageKey::id(inboxMessage1, Equal)), messageSet() << smsMessage << archivedMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::id(QMailMessageId(), Equal)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::id(QMailMessageId(), Equal)), allMessages);

    // ID inequality
    QCOMPARE(messageSet(QMailMessageKey::id(smsMessage, NotEqual)), allEmailMessages);
    QCOMPARE(messageSet(~QMailMessageKey::id(smsMessage, NotEqual)), messageSet() << smsMessage);
    QCOMPARE(messageSet(QMailMessageKey::id(inboxMessage1, NotEqual)), messageSet() << smsMessage << archivedMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::id(inboxMessage1, NotEqual)), messageSet() << inboxMessage1);
    QCOMPARE(messageSet(QMailMessageKey::id(QMailMessageId(), NotEqual)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::id(QMailMessageId(), NotEqual)), noMessages);

    // List inclusion
    QCOMPARE(messageSet(QMailMessageKey::id(allMessages.toList())), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::id(allMessages.toList())), noMessages);
    QCOMPARE(messageSet(QMailMessageKey::id(QMailMessageIdList() << smsMessage)), messageSet() << smsMessage);
    QCOMPARE(messageSet(~QMailMessageKey::id(QMailMessageIdList() << smsMessage)), allEmailMessages);
    QCOMPARE(messageSet(QMailMessageKey::id(QMailMessageIdList() << inboxMessage1)), messageSet() << inboxMessage1);
    QCOMPARE(messageSet(~QMailMessageKey::id(QMailMessageIdList() << inboxMessage1)), messageSet() << smsMessage << archivedMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::id(QMailMessageIdList() << smsMessage << inboxMessage1)), messageSet() << smsMessage << inboxMessage1);
    QCOMPARE(messageSet(~QMailMessageKey::id(QMailMessageIdList() << smsMessage << inboxMessage1)), messageSet() << archivedMessage1 << inboxMessage2 << savedMessage2);

    // List Exclusion
    QCOMPARE(messageSet(QMailMessageKey::id(allMessages.toList(), Excludes)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::id(allMessages.toList(), Excludes)), allMessages);
    QCOMPARE(messageSet(QMailMessageKey::id(QMailMessageIdList() << smsMessage, Excludes)), allEmailMessages);
    QCOMPARE(messageSet(~QMailMessageKey::id(QMailMessageIdList() << smsMessage, Excludes)), messageSet() << smsMessage);
    QCOMPARE(messageSet(QMailMessageKey::id(QMailMessageIdList() << inboxMessage1, Excludes)), messageSet() << smsMessage << archivedMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::id(QMailMessageIdList() << inboxMessage1, Excludes)), messageSet() << inboxMessage1);
    QCOMPARE(messageSet(QMailMessageKey::id(QMailMessageIdList() << smsMessage << inboxMessage1, Excludes)), messageSet() << archivedMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::id(QMailMessageIdList() << smsMessage << inboxMessage1, Excludes)), messageSet() << smsMessage << inboxMessage1);

    // Key matching
    QCOMPARE(messageSet(QMailMessageKey::id(smsMessage, Equal)), messageSet() << smsMessage);
    QCOMPARE(messageSet(~QMailMessageKey::id(smsMessage, Equal)), allEmailMessages);
    QCOMPARE(messageSet(QMailMessageKey::id(QMailMessageKey::id(smsMessage, Equal))), messageSet() << smsMessage);
    QCOMPARE(messageSet(QMailMessageKey::id(~QMailMessageKey::id(smsMessage, Equal))), allEmailMessages);
    QCOMPARE(messageSet(~QMailMessageKey::id(QMailMessageKey::id(smsMessage, Equal))), allEmailMessages);
    QCOMPARE(messageSet(~QMailMessageKey::id(~QMailMessageKey::id(smsMessage, Equal))), messageSet() << smsMessage);

    QString subject("Where are you?");
    QCOMPARE(messageSet(QMailMessageKey::subject(subject, Equal)), messageSet() << smsMessage);
    QCOMPARE(messageSet(~QMailMessageKey::subject(subject, Equal)), allEmailMessages);
    QCOMPARE(messageSet(QMailMessageKey::id(QMailMessageKey::subject(subject, Equal))), messageSet() << smsMessage);
    QCOMPARE(messageSet(QMailMessageKey::id(~QMailMessageKey::subject(subject, Equal))), allEmailMessages);
    QCOMPARE(messageSet(~QMailMessageKey::id(QMailMessageKey::subject(subject, Equal))), allEmailMessages);
    QCOMPARE(messageSet(~QMailMessageKey::id(~QMailMessageKey::subject(subject, Equal))), messageSet() << smsMessage);

    QCOMPARE(messageSet(QMailMessageKey::messageType(QMailMessage::Sms, Equal)), messageSet() << smsMessage);
    QCOMPARE(messageSet(~QMailMessageKey::messageType(QMailMessage::Sms, Equal)), allEmailMessages);
    QCOMPARE(messageSet(QMailMessageKey::id(QMailMessageKey::messageType(QMailMessage::Sms, Equal))), messageSet() << smsMessage);
    QCOMPARE(messageSet(QMailMessageKey::id(~QMailMessageKey::messageType(QMailMessage::Sms, Equal))), allEmailMessages);
    QCOMPARE(messageSet(~QMailMessageKey::id(QMailMessageKey::messageType(QMailMessage::Sms, Equal))), allEmailMessages);
    QCOMPARE(messageSet(~QMailMessageKey::id(~QMailMessageKey::messageType(QMailMessage::Sms, Equal))), messageSet() << smsMessage);
}

void tst_QMailStoreKeys::messageType()
{
    // MessageType equality
    QCOMPARE(messageSet(QMailMessageKey::messageType(QMailMessage::Sms, Equal)), messageSet() << smsMessage);
    QCOMPARE(messageSet(~QMailMessageKey::messageType(QMailMessage::Sms, Equal)), allEmailMessages);
    QCOMPARE(messageSet(QMailMessageKey::messageType(QMailMessage::Email, Equal)), allEmailMessages);
    QCOMPARE(messageSet(~QMailMessageKey::messageType(QMailMessage::Email, Equal)), messageSet() << smsMessage);
    QCOMPARE(messageSet(QMailMessageKey::messageType(QMailMessage::Instant, Equal)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::messageType(QMailMessage::Instant, Equal)), allMessages);

    // Type inequality
    QCOMPARE(messageSet(QMailMessageKey::messageType(QMailMessage::Sms, NotEqual)), allEmailMessages);
    QCOMPARE(messageSet(~QMailMessageKey::messageType(QMailMessage::Sms, NotEqual)), messageSet() << smsMessage);
    QCOMPARE(messageSet(QMailMessageKey::messageType(QMailMessage::Email, NotEqual)), messageSet() << smsMessage);
    QCOMPARE(messageSet(~QMailMessageKey::messageType(QMailMessage::Email, NotEqual)), allEmailMessages);
    QCOMPARE(messageSet(QMailMessageKey::messageType(QMailMessage::Instant, NotEqual)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::messageType(QMailMessage::Instant, NotEqual)), noMessages);

    // Bitwise inclusion
    QCOMPARE(messageSet(QMailMessageKey::messageType(QMailMessage::Sms | QMailMessage::Mms, Includes)), messageSet() << smsMessage);
    QCOMPARE(messageSet(~QMailMessageKey::messageType(QMailMessage::Sms | QMailMessage::Mms, Includes)), allEmailMessages);
    QCOMPARE(messageSet(QMailMessageKey::messageType(QMailMessage::Sms | QMailMessage::Email, Includes)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::messageType(QMailMessage::Sms | QMailMessage::Email, Includes)), noMessages);
    QCOMPARE(messageSet(QMailMessageKey::messageType(QMailMessage::Email | QMailMessage::Instant, Includes)), allEmailMessages);
    QCOMPARE(messageSet(~QMailMessageKey::messageType(QMailMessage::Email | QMailMessage::Instant, Includes)), messageSet() << smsMessage);

    // Bitwise exclusion
    QCOMPARE(messageSet(QMailMessageKey::messageType(QMailMessage::Sms | QMailMessage::Mms, Excludes)), allEmailMessages);
    QCOMPARE(messageSet(~QMailMessageKey::messageType(QMailMessage::Sms | QMailMessage::Mms, Excludes)), messageSet() << smsMessage);
    QCOMPARE(messageSet(QMailMessageKey::messageType(QMailMessage::Sms | QMailMessage::Email, Excludes)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::messageType(QMailMessage::Sms | QMailMessage::Email, Excludes)), allMessages);
    QCOMPARE(messageSet(QMailMessageKey::messageType(QMailMessage::Email | QMailMessage::Instant, Excludes)), messageSet() << smsMessage);
    QCOMPARE(messageSet(~QMailMessageKey::messageType(QMailMessage::Email | QMailMessage::Instant, Excludes)), allEmailMessages);
}

void tst_QMailStoreKeys::messageParentFolderId()
{
    QMailFolderId localFolder(QMailFolder::LocalStorageFolderId);

    // ID equality
    QCOMPARE(messageSet(QMailMessageKey::parentFolderId(localFolder, Equal)), messageSet() << smsMessage);
    QCOMPARE(messageSet(~QMailMessageKey::parentFolderId(localFolder, Equal)), allEmailMessages);
    QCOMPARE(messageSet(QMailMessageKey::parentFolderId(inboxId1, Equal)), messageSet() << inboxMessage1);
    QCOMPARE(messageSet(~QMailMessageKey::parentFolderId(inboxId1, Equal)), messageSet() << smsMessage << archivedMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::parentFolderId(QMailFolderId(), Equal)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::parentFolderId(QMailFolderId(), Equal)), allMessages);

    // ID inequality
    QCOMPARE(messageSet(QMailMessageKey::parentFolderId(localFolder, NotEqual)), allEmailMessages);
    QCOMPARE(messageSet(~QMailMessageKey::parentFolderId(localFolder, NotEqual)), messageSet() << smsMessage);
    QCOMPARE(messageSet(QMailMessageKey::parentFolderId(inboxId1, NotEqual)), messageSet() << smsMessage << archivedMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::parentFolderId(inboxId1, NotEqual)), messageSet() << inboxMessage1);
    QCOMPARE(messageSet(QMailMessageKey::parentFolderId(QMailFolderId(), NotEqual)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::parentFolderId(QMailFolderId(), NotEqual)), noMessages);

    // List inclusion
    QCOMPARE(messageSet(QMailMessageKey::parentFolderId(QMailFolderIdList())), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::parentFolderId(QMailFolderIdList())), allMessages);
    QCOMPARE(messageSet(QMailMessageKey::parentFolderId(QMailFolderIdList() << inboxId1)), messageSet() << inboxMessage1);
    QCOMPARE(messageSet(~QMailMessageKey::parentFolderId(QMailFolderIdList() << inboxId1)), messageSet() << smsMessage << archivedMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::parentFolderId(QMailFolderIdList() << inboxId1 << archivedId1)), messageSet() << inboxMessage1 << archivedMessage1);
    QCOMPARE(messageSet(~QMailMessageKey::parentFolderId(QMailFolderIdList() << inboxId1 << archivedId1)), messageSet() << smsMessage << inboxMessage2 << savedMessage2);

    // List exclusion
    QCOMPARE(messageSet(QMailMessageKey::parentFolderId(QMailFolderIdList(), Excludes)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::parentFolderId(QMailFolderIdList(), Excludes)), noMessages);
    QCOMPARE(messageSet(QMailMessageKey::parentFolderId(QMailFolderIdList() << inboxId1, Excludes)), messageSet() << smsMessage << archivedMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::parentFolderId(QMailFolderIdList() << inboxId1, Excludes)), messageSet() << inboxMessage1);
    QCOMPARE(messageSet(QMailMessageKey::parentFolderId(QMailFolderIdList() << inboxId1 << archivedId1, Excludes)), messageSet() << smsMessage << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::parentFolderId(QMailFolderIdList() << inboxId1 << archivedId1, Excludes)), messageSet() << inboxMessage1 << archivedMessage1);

    // Key matching
    QCOMPARE(folderSet(QMailFolderKey::id(inboxId1, Equal)), folderSet() << inboxId1);
    QCOMPARE(folderSet(~QMailFolderKey::id(inboxId1, Equal)), standardFolders + folderSet() << savedId1 << archivedId1 << inboxId2 << savedId2 << archivedId2);
    QCOMPARE(messageSet(QMailMessageKey::parentFolderId(QMailFolderKey::id(inboxId1, Equal))), messageSet() << inboxMessage1);
    QCOMPARE(messageSet(QMailMessageKey::parentFolderId(~QMailFolderKey::id(inboxId1, Equal))), messageSet() << smsMessage << archivedMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::parentFolderId(QMailFolderKey::id(inboxId1, Equal))), messageSet() << smsMessage << archivedMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::parentFolderId(~QMailFolderKey::id(inboxId1, Equal))), messageSet() << inboxMessage1);

    QString path("Inbox/Saved");
    QCOMPARE(folderSet(QMailFolderKey::path(path, Equal)), folderSet() << savedId1 << savedId2);
    QCOMPARE(folderSet(~QMailFolderKey::path(path, Equal)), standardFolders + folderSet() << inboxId1 << archivedId1 << inboxId2 << archivedId2);
    QCOMPARE(messageSet(QMailMessageKey::parentFolderId(QMailFolderKey::path(path, Equal))), messageSet() << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::parentFolderId(~QMailFolderKey::path(path, Equal))), messageSet() << smsMessage << inboxMessage1 << archivedMessage1 << inboxMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::parentFolderId(QMailFolderKey::path(path, Equal))), messageSet() << smsMessage << inboxMessage1 << archivedMessage1 << inboxMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::parentFolderId(~QMailFolderKey::path(path, Equal))), messageSet() << savedMessage2);

    QCOMPARE(folderSet(QMailFolderKey::status(QMailFolder::Synchronized, Includes)), folderSet() << savedId1 << savedId2);
    QCOMPARE(folderSet(~QMailFolderKey::status(QMailFolder::Synchronized, Includes)), standardFolders + folderSet() << inboxId1 << archivedId1 << inboxId2 << archivedId2);
    QCOMPARE(messageSet(QMailMessageKey::parentFolderId(QMailFolderKey::status(QMailFolder::Synchronized, Includes))), messageSet() << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::parentFolderId(~QMailFolderKey::status(QMailFolder::Synchronized, Includes))), messageSet() << smsMessage << inboxMessage1 << archivedMessage1 << inboxMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::parentFolderId(QMailFolderKey::status(QMailFolder::Synchronized, Includes))), messageSet() << smsMessage << inboxMessage1 << archivedMessage1 << inboxMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::parentFolderId(~QMailFolderKey::status(QMailFolder::Synchronized, Includes))), messageSet() << savedMessage2);
}

void tst_QMailStoreKeys::messageSender()
{
    QString address1("account1@example.org"), address2("account2@example.org");

    // Equality
    QCOMPARE(messageSet(QMailMessageKey::sender(address1, Equal)), messageSet() << archivedMessage1 << inboxMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::sender(address1, Equal)), messageSet() << smsMessage << inboxMessage1 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::sender(address2, Equal)), messageSet() << inboxMessage1);
    QCOMPARE(messageSet(~QMailMessageKey::sender(address2, Equal)), messageSet() << smsMessage << archivedMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::sender(QString(""), Equal)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::sender(QString(""), Equal)), allMessages);
    QCOMPARE(messageSet(QMailMessageKey::sender(QString(), Equal)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::sender(QString(), Equal)), allMessages);

    // Inequality
    QCOMPARE(messageSet(QMailMessageKey::sender(address1, NotEqual)), messageSet() << smsMessage << inboxMessage1 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::sender(address1, NotEqual)), messageSet() << archivedMessage1 << inboxMessage2);
    QCOMPARE(messageSet(QMailMessageKey::sender(address2, NotEqual)), messageSet() << smsMessage << archivedMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::sender(address2, NotEqual)), messageSet() << inboxMessage1);
    QCOMPARE(messageSet(QMailMessageKey::sender(QString(""), NotEqual)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::sender(QString(""), NotEqual)), noMessages);
    QCOMPARE(messageSet(QMailMessageKey::sender(QString(), NotEqual)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::sender(QString(), NotEqual)), noMessages);

    // List inclusion
    QCOMPARE(messageSet(QMailMessageKey::sender(QStringList())), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::sender(QStringList())), allMessages);
    QCOMPARE(messageSet(QMailMessageKey::sender(QStringList() << address1)), messageSet() << archivedMessage1 << inboxMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::sender(QStringList() << address1)), messageSet() << smsMessage << inboxMessage1 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::sender(QStringList() << address1 << address2)), messageSet() << inboxMessage1 << archivedMessage1 << inboxMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::sender(QStringList() << address1 << address2)), messageSet() << smsMessage << savedMessage2);

    // List exclusion
    QCOMPARE(messageSet(QMailMessageKey::sender(QStringList(), Excludes)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::sender(QStringList(), Excludes)), noMessages);
    QCOMPARE(messageSet(QMailMessageKey::sender(QStringList() << address1, Excludes)), messageSet() << smsMessage << inboxMessage1 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::sender(QStringList() << address1, Excludes)), messageSet() << archivedMessage1 << inboxMessage2);
    QCOMPARE(messageSet(QMailMessageKey::sender(QStringList() << address1 << address2, Excludes)), messageSet() << smsMessage << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::sender(QStringList() << address1 << address2, Excludes)), messageSet() << inboxMessage1 << archivedMessage1 << inboxMessage2);

    // Inclusion
    QCOMPARE(messageSet(QMailMessageKey::sender(QString(".net"), Includes)), messageSet() << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::sender(QString(".net"), Includes)), messageSet() << smsMessage << inboxMessage1 << archivedMessage1 << inboxMessage2);
    QCOMPARE(messageSet(QMailMessageKey::sender(QString("0404"), Includes)), messageSet() << smsMessage);
    QCOMPARE(messageSet(~QMailMessageKey::sender(QString("0404"), Includes)), messageSet() << inboxMessage1 << archivedMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::sender(QString(""), Includes)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::sender(QString(""), Includes)), noMessages);
    QCOMPARE(messageSet(QMailMessageKey::sender(QString(), Includes)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::sender(QString(), Includes)), noMessages);

    // Exclusion
    QCOMPARE(messageSet(QMailMessageKey::sender(QString(".net"), Excludes)), messageSet() << smsMessage << inboxMessage1 << archivedMessage1 << inboxMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::sender(QString(".net"), Excludes)), messageSet() << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::sender(QString("0404"), Excludes)), messageSet() << inboxMessage1 << archivedMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::sender(QString("0404"), Excludes)), messageSet() << smsMessage);
    QCOMPARE(messageSet(QMailMessageKey::sender(QString(""), Excludes)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::sender(QString(""), Excludes)), allMessages);
    QCOMPARE(messageSet(QMailMessageKey::sender(QString(), Excludes)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::sender(QString(), Excludes)), allMessages);
}

void tst_QMailStoreKeys::messageRecipients()
{
    QString address1("testing@test"), address2("account2@example.org");

    // Equality
    QCOMPARE(messageSet(QMailMessageKey::recipients(address1, Equal)), messageSet() << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::recipients(address1, Equal)), messageSet() << smsMessage << inboxMessage1 << archivedMessage1 << inboxMessage2);
    QCOMPARE(messageSet(QMailMessageKey::recipients(address2, Equal)), messageSet() << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::recipients(address2, Equal)), messageSet() << smsMessage << inboxMessage1 << archivedMessage1);
    QCOMPARE(messageSet(QMailMessageKey::recipients(QString(""), Equal)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::recipients(QString(""), Equal)), allMessages);
    QCOMPARE(messageSet(QMailMessageKey::recipients(QString(), Equal)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::recipients(QString(), Equal)), allMessages);

    // Inequality
    QCOMPARE(messageSet(QMailMessageKey::recipients(address1, NotEqual)), messageSet() << smsMessage << inboxMessage1 << archivedMessage1 << inboxMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::recipients(address1, NotEqual)), messageSet() << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::recipients(address2, NotEqual)), messageSet() << smsMessage << inboxMessage1 << archivedMessage1);
    QCOMPARE(messageSet(~QMailMessageKey::recipients(address2, NotEqual)), messageSet() << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::recipients(QString(""), NotEqual)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::recipients(QString(""), NotEqual)), noMessages);
    QCOMPARE(messageSet(QMailMessageKey::recipients(QString(), NotEqual)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::recipients(QString(), NotEqual)), noMessages);

    // List inclusion - not possible due to requirement for pattern matching

    // Inclusion
    QCOMPARE(messageSet(QMailMessageKey::recipients(QString(".org"), Includes)), messageSet() << inboxMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::recipients(QString(".org"), Includes)), messageSet() << smsMessage << archivedMessage1);
    QCOMPARE(messageSet(QMailMessageKey::recipients(QString("0404"), Includes)), messageSet() << smsMessage);
    QCOMPARE(messageSet(~QMailMessageKey::recipients(QString("0404"), Includes)), messageSet() << inboxMessage1 << archivedMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::recipients(QString(""), Includes)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::recipients(QString(""), Includes)), noMessages);
    QCOMPARE(messageSet(QMailMessageKey::recipients(QString(), Includes)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::recipients(QString(), Includes)), noMessages);

    // Exclusion
    QCOMPARE(messageSet(QMailMessageKey::recipients(QString(".org"), Excludes)), messageSet() << smsMessage << archivedMessage1);
    QCOMPARE(messageSet(~QMailMessageKey::recipients(QString(".org"), Excludes)), messageSet() << inboxMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::recipients(QString("0404"), Excludes)), messageSet() << inboxMessage1 << archivedMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::recipients(QString("0404"), Excludes)), messageSet() << smsMessage);
    QCOMPARE(messageSet(QMailMessageKey::recipients(QString(""), Excludes)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::recipients(QString(""), Excludes)), allMessages);
    QCOMPARE(messageSet(QMailMessageKey::recipients(QString(), Excludes)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::recipients(QString(), Excludes)), allMessages);
}

void tst_QMailStoreKeys::messageSubject()
{
    QString subject1("inboxMessage1"), subject2("Fwd:inboxMessage2");

    // Equality
    QCOMPARE(messageSet(QMailMessageKey::subject(subject1, Equal)), messageSet() << inboxMessage1);
    QCOMPARE(messageSet(~QMailMessageKey::subject(subject1, Equal)), messageSet() << smsMessage << archivedMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::subject(subject2, Equal)), messageSet() << inboxMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::subject(subject2, Equal)), messageSet() << smsMessage << inboxMessage1 << archivedMessage1 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::subject(QString(""), Equal)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::subject(QString(""), Equal)), allMessages);
    QCOMPARE(messageSet(QMailMessageKey::subject(QString(), Equal)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::subject(QString(), Equal)), allMessages);

    // Inequality
    QCOMPARE(messageSet(QMailMessageKey::subject(subject1, NotEqual)), messageSet() << smsMessage << archivedMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::subject(subject1, NotEqual)), messageSet() << inboxMessage1);
    QCOMPARE(messageSet(QMailMessageKey::subject(subject2, NotEqual)), messageSet() << smsMessage << inboxMessage1 << archivedMessage1 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::subject(subject2, NotEqual)), messageSet() << inboxMessage2);
    QCOMPARE(messageSet(QMailMessageKey::subject(QString(""), NotEqual)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::subject(QString(""), NotEqual)), noMessages);
    QCOMPARE(messageSet(QMailMessageKey::subject(QString(), NotEqual)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::subject(QString(), NotEqual)), noMessages);

    // List inclusion
    QCOMPARE(messageSet(QMailMessageKey::subject(QStringList())), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::subject(QStringList())), allMessages);
    QCOMPARE(messageSet(QMailMessageKey::subject(QStringList() << subject1)), messageSet() << inboxMessage1);
    QCOMPARE(messageSet(~QMailMessageKey::subject(QStringList() << subject1)), messageSet() << smsMessage << archivedMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::subject(QStringList() << subject1 << subject2)), messageSet() << inboxMessage1 << inboxMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::subject(QStringList() << subject1 << subject2)), messageSet() << smsMessage << archivedMessage1 << savedMessage2);

    // List exclusion
    QCOMPARE(messageSet(QMailMessageKey::subject(QStringList(), Excludes)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::subject(QStringList(), Excludes)), noMessages);
    QCOMPARE(messageSet(QMailMessageKey::subject(QStringList() << subject1, Excludes)), messageSet() << smsMessage << archivedMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::subject(QStringList() << subject1, Excludes)), messageSet() << inboxMessage1);
    QCOMPARE(messageSet(QMailMessageKey::subject(QStringList() << subject1 << subject2, Excludes)), messageSet() << smsMessage << archivedMessage1 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::subject(QStringList() << subject1 << subject2, Excludes)), messageSet() << inboxMessage1 << inboxMessage2);

    // Inclusion
    QCOMPARE(messageSet(QMailMessageKey::subject(QString("essage"), Includes)), messageSet() << inboxMessage1 << archivedMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::subject(QString("essage"), Includes)), messageSet() << smsMessage);
    QCOMPARE(messageSet(QMailMessageKey::subject(QString("you?"), Includes)), messageSet() << smsMessage);
    QCOMPARE(messageSet(~QMailMessageKey::subject(QString("you?"), Includes)), messageSet() << inboxMessage1 << archivedMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::subject(QString(""), Includes)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::subject(QString(""), Includes)), noMessages);
    QCOMPARE(messageSet(QMailMessageKey::subject(QString(), Includes)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::subject(QString(), Includes)), noMessages);

    // Exclusion
    QCOMPARE(messageSet(QMailMessageKey::subject(QString("essage"), Excludes)), messageSet() << smsMessage);
    QCOMPARE(messageSet(~QMailMessageKey::subject(QString("essage"), Excludes)), messageSet() << inboxMessage1 << archivedMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::subject(QString("you?"), Excludes)), messageSet() << inboxMessage1 << archivedMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::subject(QString("you?"), Excludes)), messageSet() << smsMessage);
    QCOMPARE(messageSet(QMailMessageKey::subject(QString(""), Excludes)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::subject(QString(""), Excludes)), allMessages);
    QCOMPARE(messageSet(QMailMessageKey::subject(QString(), Excludes)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::subject(QString(), Excludes)), allMessages);
}

void tst_QMailStoreKeys::messageTimeStamp()
{
    QDateTime today(QDate::currentDate()), yesterday(QDate::currentDate().addDays(-1)), lastWeek(QDate::currentDate().addDays(-7));
    today = today.toUTC();
    yesterday = yesterday.toUTC();
    lastWeek = lastWeek.toUTC();
    // Comparisons
    QCOMPARE(messageSet(QMailMessageKey::timeStamp(today, Equal)), messageSet() << smsMessage << inboxMessage1 << inboxMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::timeStamp(today, Equal)), messageSet() << archivedMessage1 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::timeStamp(today, NotEqual)), messageSet() << archivedMessage1 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::timeStamp(today, NotEqual)), messageSet() << smsMessage << inboxMessage1 << inboxMessage2);
    QCOMPARE(messageSet(QMailMessageKey::timeStamp(today, LessThan)), messageSet() << archivedMessage1 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::timeStamp(today, LessThan)), messageSet() << smsMessage << inboxMessage1 << inboxMessage2);
    QCOMPARE(messageSet(QMailMessageKey::timeStamp(today, GreaterThanEqual)), messageSet() << smsMessage << inboxMessage1 << inboxMessage2); 
    QCOMPARE(messageSet(~QMailMessageKey::timeStamp(today, GreaterThanEqual)), messageSet() << archivedMessage1 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::timeStamp(today, LessThanEqual)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::timeStamp(today, LessThanEqual)), noMessages);
    QCOMPARE(messageSet(QMailMessageKey::timeStamp(today, GreaterThan)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::timeStamp(today, GreaterThan)), allMessages);

    QCOMPARE(messageSet(QMailMessageKey::timeStamp(yesterday, Equal)), messageSet() << archivedMessage1);
    QCOMPARE(messageSet(~QMailMessageKey::timeStamp(yesterday, Equal)), messageSet() << smsMessage << inboxMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::timeStamp(yesterday, NotEqual)), messageSet() << smsMessage << inboxMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::timeStamp(yesterday, NotEqual)), messageSet() << archivedMessage1);
    QCOMPARE(messageSet(QMailMessageKey::timeStamp(yesterday, LessThan)), messageSet() << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::timeStamp(yesterday, LessThan)), messageSet() << smsMessage << inboxMessage1 << archivedMessage1 << inboxMessage2);
    QCOMPARE(messageSet(QMailMessageKey::timeStamp(yesterday, GreaterThanEqual)), messageSet() << smsMessage << inboxMessage1 << archivedMessage1 << inboxMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::timeStamp(yesterday, GreaterThanEqual)), messageSet() << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::timeStamp(yesterday, LessThanEqual)), messageSet() << archivedMessage1 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::timeStamp(yesterday, LessThanEqual)), messageSet() << smsMessage << inboxMessage1 << inboxMessage2);
    QCOMPARE(messageSet(QMailMessageKey::timeStamp(yesterday, GreaterThan)), messageSet() << smsMessage << inboxMessage1 << inboxMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::timeStamp(yesterday, GreaterThan)), messageSet() << archivedMessage1 << savedMessage2);

    QCOMPARE(messageSet(QMailMessageKey::timeStamp(lastWeek, Equal)), messageSet() << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::timeStamp(lastWeek, Equal)), messageSet() << smsMessage << inboxMessage1 << archivedMessage1 << inboxMessage2);
    QCOMPARE(messageSet(QMailMessageKey::timeStamp(lastWeek, NotEqual)), messageSet() << smsMessage << inboxMessage1 << archivedMessage1 << inboxMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::timeStamp(lastWeek, NotEqual)), messageSet() << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::timeStamp(lastWeek, LessThan)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::timeStamp(lastWeek, LessThan)), allMessages);
    QCOMPARE(messageSet(QMailMessageKey::timeStamp(lastWeek, GreaterThanEqual)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::timeStamp(lastWeek, GreaterThanEqual)), noMessages);
    QCOMPARE(messageSet(QMailMessageKey::timeStamp(lastWeek, LessThanEqual)), messageSet() << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::timeStamp(lastWeek, LessThanEqual)), messageSet() << smsMessage << inboxMessage1 << archivedMessage1 << inboxMessage2);
    QCOMPARE(messageSet(QMailMessageKey::timeStamp(lastWeek, GreaterThan)), messageSet() << smsMessage << inboxMessage1 << archivedMessage1 << inboxMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::timeStamp(lastWeek, GreaterThan)), messageSet() << savedMessage2);

    QCOMPARE(messageSet(QMailMessageKey::timeStamp(QDateTime(), Equal)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::timeStamp(QDateTime(), Equal)), allMessages);
    QCOMPARE(messageSet(QMailMessageKey::timeStamp(QDateTime(), NotEqual)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::timeStamp(QDateTime(), NotEqual)), noMessages);
    QCOMPARE(messageSet(QMailMessageKey::timeStamp(QDateTime(), LessThan)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::timeStamp(QDateTime(), LessThan)), noMessages);
    QCOMPARE(messageSet(QMailMessageKey::timeStamp(QDateTime(), GreaterThanEqual)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::timeStamp(QDateTime(), GreaterThanEqual)), noMessages);
    QCOMPARE(messageSet(QMailMessageKey::timeStamp(QDateTime(), LessThanEqual)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::timeStamp(QDateTime(), LessThanEqual)), noMessages);
    QCOMPARE(messageSet(QMailMessageKey::timeStamp(QDateTime(), GreaterThan)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::timeStamp(QDateTime(), GreaterThan)), noMessages);
}

void tst_QMailStoreKeys::messageReceptionTimeStamp()
{
    QDateTime today(QDate::currentDate()), yesterday(QDate::currentDate().addDays(-1)), lastWeek(QDate::currentDate().addDays(-7));
    today = today.toUTC();
    yesterday = yesterday.toUTC();
    lastWeek = lastWeek.toUTC();

    // Comparisons
    QCOMPARE(messageSet(QMailMessageKey::receptionTimeStamp(today, Equal)), messageSet() << smsMessage << inboxMessage1 << inboxMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::receptionTimeStamp(today, Equal)), messageSet() << archivedMessage1 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::receptionTimeStamp(today, NotEqual)), messageSet() << archivedMessage1 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::receptionTimeStamp(today, NotEqual)), messageSet() << smsMessage << inboxMessage1 << inboxMessage2);
    QCOMPARE(messageSet(QMailMessageKey::receptionTimeStamp(today, LessThan)), messageSet() << archivedMessage1 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::receptionTimeStamp(today, LessThan)), messageSet() << smsMessage << inboxMessage1 << inboxMessage2);
    QCOMPARE(messageSet(QMailMessageKey::receptionTimeStamp(today, GreaterThanEqual)), messageSet() << smsMessage << inboxMessage1 << inboxMessage2); 
    QCOMPARE(messageSet(~QMailMessageKey::receptionTimeStamp(today, GreaterThanEqual)), messageSet() << archivedMessage1 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::receptionTimeStamp(today, LessThanEqual)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::receptionTimeStamp(today, LessThanEqual)), noMessages);
    QCOMPARE(messageSet(QMailMessageKey::receptionTimeStamp(today, GreaterThan)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::receptionTimeStamp(today, GreaterThan)), allMessages);

    QCOMPARE(messageSet(QMailMessageKey::receptionTimeStamp(yesterday, Equal)), messageSet() << archivedMessage1);
    QCOMPARE(messageSet(~QMailMessageKey::receptionTimeStamp(yesterday, Equal)), messageSet() << smsMessage << inboxMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::receptionTimeStamp(yesterday, NotEqual)), messageSet() << smsMessage << inboxMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::receptionTimeStamp(yesterday, NotEqual)), messageSet() << archivedMessage1);
    QCOMPARE(messageSet(QMailMessageKey::receptionTimeStamp(yesterday, LessThan)), messageSet() << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::receptionTimeStamp(yesterday, LessThan)), messageSet() << smsMessage << inboxMessage1 << archivedMessage1 << inboxMessage2);
    QCOMPARE(messageSet(QMailMessageKey::receptionTimeStamp(yesterday, GreaterThanEqual)), messageSet() << smsMessage << inboxMessage1 << archivedMessage1 << inboxMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::receptionTimeStamp(yesterday, GreaterThanEqual)), messageSet() << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::receptionTimeStamp(yesterday, LessThanEqual)), messageSet() << archivedMessage1 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::receptionTimeStamp(yesterday, LessThanEqual)), messageSet() << smsMessage << inboxMessage1 << inboxMessage2);
    QCOMPARE(messageSet(QMailMessageKey::receptionTimeStamp(yesterday, GreaterThan)), messageSet() << smsMessage << inboxMessage1 << inboxMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::receptionTimeStamp(yesterday, GreaterThan)), messageSet() << archivedMessage1 << savedMessage2);

    QCOMPARE(messageSet(QMailMessageKey::receptionTimeStamp(lastWeek, Equal)), messageSet() << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::receptionTimeStamp(lastWeek, Equal)), messageSet() << smsMessage << inboxMessage1 << archivedMessage1 << inboxMessage2);
    QCOMPARE(messageSet(QMailMessageKey::receptionTimeStamp(lastWeek, NotEqual)), messageSet() << smsMessage << inboxMessage1 << archivedMessage1 << inboxMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::receptionTimeStamp(lastWeek, NotEqual)), messageSet() << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::receptionTimeStamp(lastWeek, LessThan)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::receptionTimeStamp(lastWeek, LessThan)), allMessages);
    QCOMPARE(messageSet(QMailMessageKey::receptionTimeStamp(lastWeek, GreaterThanEqual)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::receptionTimeStamp(lastWeek, GreaterThanEqual)), noMessages);
    QCOMPARE(messageSet(QMailMessageKey::receptionTimeStamp(lastWeek, LessThanEqual)), messageSet() << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::receptionTimeStamp(lastWeek, LessThanEqual)), messageSet() << smsMessage << inboxMessage1 << archivedMessage1 << inboxMessage2);
    QCOMPARE(messageSet(QMailMessageKey::receptionTimeStamp(lastWeek, GreaterThan)), messageSet() << smsMessage << inboxMessage1 << archivedMessage1 << inboxMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::receptionTimeStamp(lastWeek, GreaterThan)), messageSet() << savedMessage2);

    QCOMPARE(messageSet(QMailMessageKey::receptionTimeStamp(QDateTime(), Equal)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::receptionTimeStamp(QDateTime(), Equal)), allMessages);
    QCOMPARE(messageSet(QMailMessageKey::receptionTimeStamp(QDateTime(), NotEqual)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::receptionTimeStamp(QDateTime(), NotEqual)), noMessages);
    QCOMPARE(messageSet(QMailMessageKey::receptionTimeStamp(QDateTime(), LessThan)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::receptionTimeStamp(QDateTime(), LessThan)), noMessages);
    QCOMPARE(messageSet(QMailMessageKey::receptionTimeStamp(QDateTime(), GreaterThanEqual)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::receptionTimeStamp(QDateTime(), GreaterThanEqual)), noMessages);
    QCOMPARE(messageSet(QMailMessageKey::receptionTimeStamp(QDateTime(), LessThanEqual)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::receptionTimeStamp(QDateTime(), LessThanEqual)), noMessages);
    QCOMPARE(messageSet(QMailMessageKey::receptionTimeStamp(QDateTime(), GreaterThan)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::receptionTimeStamp(QDateTime(), GreaterThan)), noMessages);
}

void tst_QMailStoreKeys::messageStatus()
{
    quint64 messageStatus1(QMailMessage(archivedMessage1).status());
    quint64 messageStatus2(QMailMessage(savedMessage2).status());
    quint64 allSet(QMailMessage::Incoming | QMailMessage::Outgoing | QMailMessage::Read | QMailMessage::Sent | QMailMessage::New);
    quint64 sourceSet(QMailMessage::Incoming | QMailMessage::Read);
    quint64 sinkSet(QMailMessage::Outgoing | QMailMessage::Sent);

    // Status equality
    QCOMPARE(messageSet(QMailMessageKey::status(QMailMessage::Incoming, Equal)), messageSet() << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::status(QMailMessage::Incoming, Equal)), messageSet() << smsMessage << inboxMessage1 << archivedMessage1 << inboxMessage2);
    QCOMPARE(messageSet(QMailMessageKey::status(allSet, Equal)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::status(allSet, Equal)), allMessages);
    QCOMPARE(messageSet(QMailMessageKey::status(messageStatus1, Equal)), messageSet() << archivedMessage1);
    QCOMPARE(messageSet(~QMailMessageKey::status(messageStatus1, Equal)), messageSet() << smsMessage << inboxMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::status(messageStatus2, Equal)), messageSet() << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::status(messageStatus2, Equal)), messageSet() << smsMessage << inboxMessage1 << archivedMessage1 << inboxMessage2);

    // Status inequality
    QCOMPARE(messageSet(QMailMessageKey::status(QMailMessage::Incoming, NotEqual)), messageSet() << smsMessage << inboxMessage1 << archivedMessage1 << inboxMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::status(QMailMessage::Incoming, NotEqual)), messageSet() << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::status(allSet, NotEqual)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::status(allSet, NotEqual)), noMessages);
    QCOMPARE(messageSet(QMailMessageKey::status(messageStatus1, NotEqual)), messageSet() << smsMessage << inboxMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::status(messageStatus1, NotEqual)), messageSet() << archivedMessage1);
    QCOMPARE(messageSet(QMailMessageKey::status(messageStatus2, NotEqual)), messageSet() << smsMessage << inboxMessage1 << archivedMessage1 << inboxMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::status(messageStatus2, NotEqual)), messageSet() << savedMessage2);

    // Bitwise inclusion
    QCOMPARE(messageSet(QMailMessageKey::status(allSet, Includes)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::status(allSet, Includes)), noMessages);
    QCOMPARE(messageSet(QMailMessageKey::status(sourceSet, Includes)), messageSet() << smsMessage << inboxMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::status(sourceSet, Includes)), messageSet() << archivedMessage1);
    QCOMPARE(messageSet(QMailMessageKey::status(sinkSet, Includes)), messageSet() << archivedMessage1);
    QCOMPARE(messageSet(~QMailMessageKey::status(sinkSet, Includes)), messageSet() << smsMessage << inboxMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::status(0, Includes)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::status(0, Includes)), allMessages);

    // Bitwise exclusion
    QCOMPARE(messageSet(QMailMessageKey::status(allSet, Excludes)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::status(allSet, Excludes)), allMessages);
    QCOMPARE(messageSet(QMailMessageKey::status(sourceSet, Excludes)), messageSet() << archivedMessage1);
    QCOMPARE(messageSet(~QMailMessageKey::status(sourceSet, Excludes)), messageSet() << smsMessage << inboxMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::status(sinkSet, Excludes)), messageSet() << smsMessage << inboxMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::status(sinkSet, Excludes)), messageSet() << archivedMessage1);
    QCOMPARE(messageSet(QMailMessageKey::status(0, Excludes)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::status(0, Excludes)), noMessages);

    // Test sorting by status
    QMailMessageSortKey sort;
    sort = QMailMessageSortKey::status(QMailMessage::Incoming);                  // All incoming before outgoing
    sort &= QMailMessageSortKey::status(QMailMessage::New);                      // New before non-new
    sort &= QMailMessageSortKey::status(QMailMessage::Read, Qt::AscendingOrder); // Read after non-read

    QMailMessageIdList sortedIds;
    sortedIds << inboxMessage1 << inboxMessage2 << savedMessage2 << smsMessage << archivedMessage1;
    
    QCOMPARE(QMailStore::instance()->queryMessages(QMailMessageKey(), sort), sortedIds);

    // Invert the sort
    sort = QMailMessageSortKey::status(QMailMessage::Incoming, Qt::AscendingOrder); // All incoming after outgoing
    sort &= QMailMessageSortKey::status(QMailMessage::New, Qt::AscendingOrder);     // New after non-new
    sort &= QMailMessageSortKey::status(QMailMessage::Read);                        // Read before non-read

    sortedIds.clear();
    sortedIds << archivedMessage1 << smsMessage << savedMessage2 << inboxMessage2 << inboxMessage1;
    
    QCOMPARE(QMailStore::instance()->queryMessages(QMailMessageKey(), sort), sortedIds);
}

void tst_QMailStoreKeys::messageConversation()
{
    // ID inclusion
    QCOMPARE(messageSet(QMailMessageKey::conversation(smsMessage)), messageSet() << smsMessage);
    QCOMPARE(messageSet(~QMailMessageKey::conversation(smsMessage)), messageSet() << inboxMessage1 << archivedMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::conversation(inboxMessage1)), messageSet() << inboxMessage1 << inboxMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::conversation(inboxMessage1)), messageSet() << smsMessage << archivedMessage1 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::conversation(QMailMessageId())), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::conversation(QMailMessageId())), allMessages);

    // List inclusion
    QCOMPARE(messageSet(QMailMessageKey::conversation(QMailMessageIdList())), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::conversation(QMailMessageIdList())), allMessages);
    QCOMPARE(messageSet(QMailMessageKey::conversation(QMailMessageIdList() << inboxMessage1)), messageSet() << inboxMessage1 << inboxMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::conversation(QMailMessageIdList() << inboxMessage1)), messageSet() << smsMessage << archivedMessage1 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::conversation(QMailMessageIdList() << inboxMessage1 << archivedMessage1)), messageSet() << inboxMessage1 << inboxMessage2 << archivedMessage1 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::conversation(QMailMessageIdList() << inboxMessage1 << archivedMessage1)), messageSet() << smsMessage);

    // Key matching
    QCOMPARE(messageSet(QMailMessageKey::id(inboxMessage1, Equal)), messageSet() << inboxMessage1);
    QCOMPARE(messageSet(~QMailMessageKey::id(inboxMessage1, Equal)), messageSet() << smsMessage << archivedMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::conversation(QMailMessageKey::id(inboxMessage1, Equal))), messageSet() << inboxMessage1 << inboxMessage2);
    QCOMPARE(messageSet(QMailMessageKey::conversation(~QMailMessageKey::id(inboxMessage1, Equal))), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::conversation(QMailMessageKey::id(inboxMessage1, Equal))), messageSet() << smsMessage << archivedMessage1 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::conversation(~QMailMessageKey::id(inboxMessage1, Equal))), noMessages);
}

void tst_QMailStoreKeys::messageServerUid()
{
    QString uid1("inboxMessage1"), uid2("inboxMessage2");

    // Equality
    QCOMPARE(messageSet(QMailMessageKey::serverUid(uid1, Equal)), messageSet() << inboxMessage1);
    QCOMPARE(messageSet(~QMailMessageKey::serverUid(uid1, Equal)), messageSet() << smsMessage << archivedMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::serverUid(uid2, Equal)), messageSet() << inboxMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::serverUid(uid2, Equal)), messageSet() << smsMessage << inboxMessage1 << archivedMessage1 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::serverUid(QString(""), Equal)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::serverUid(QString(""), Equal)), allMessages);
    QCOMPARE(messageSet(QMailMessageKey::serverUid(QString(), Equal)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::serverUid(QString(), Equal)), allMessages);

    // Inequality
    QCOMPARE(messageSet(QMailMessageKey::serverUid(uid1, NotEqual)), messageSet() << smsMessage << archivedMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::serverUid(uid1, NotEqual)), messageSet() << inboxMessage1);
    QCOMPARE(messageSet(QMailMessageKey::serverUid(uid2, NotEqual)), messageSet() << smsMessage << inboxMessage1 << archivedMessage1 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::serverUid(uid2, NotEqual)), messageSet() << inboxMessage2);
    QCOMPARE(messageSet(QMailMessageKey::serverUid(QString(""), NotEqual)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::serverUid(QString(""), NotEqual)), noMessages);
    QCOMPARE(messageSet(QMailMessageKey::serverUid(QString(), NotEqual)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::serverUid(QString(), NotEqual)), noMessages);

    // List inclusion
    QCOMPARE(messageSet(QMailMessageKey::serverUid(QStringList())), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::serverUid(QStringList())), allMessages);
    QCOMPARE(messageSet(QMailMessageKey::serverUid(QStringList() << uid1)), messageSet() << inboxMessage1);
    QCOMPARE(messageSet(~QMailMessageKey::serverUid(QStringList() << uid1)), messageSet() << smsMessage << archivedMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::serverUid(QStringList() << uid1 << uid2)), messageSet() << inboxMessage1 << inboxMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::serverUid(QStringList() << uid1 << uid2)), messageSet() << smsMessage << archivedMessage1 << savedMessage2);

    // List exclusion
    QCOMPARE(messageSet(QMailMessageKey::serverUid(QStringList(), Excludes)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::serverUid(QStringList(), Excludes)), noMessages);
    QCOMPARE(messageSet(QMailMessageKey::serverUid(QStringList() << uid1, Excludes)), messageSet() << smsMessage << archivedMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::serverUid(QStringList() << uid1, Excludes)), messageSet() << inboxMessage1);
    QCOMPARE(messageSet(QMailMessageKey::serverUid(QStringList() << uid1 << uid2, Excludes)), messageSet() << smsMessage << archivedMessage1 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::serverUid(QStringList() << uid1 << uid2, Excludes)), messageSet() << inboxMessage1 << inboxMessage2);

    // Inclusion
    QCOMPARE(messageSet(QMailMessageKey::serverUid(QString("essage"), Includes)), messageSet() << inboxMessage1 << archivedMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::serverUid(QString("essage"), Includes)), messageSet() << smsMessage);
    QCOMPARE(messageSet(QMailMessageKey::serverUid(QString("sim:"), Includes)), messageSet() << smsMessage);
    QCOMPARE(messageSet(~QMailMessageKey::serverUid(QString("sim:"), Includes)), messageSet() << inboxMessage1 << archivedMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::serverUid(QString(""), Includes)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::serverUid(QString(""), Includes)), noMessages);
    QCOMPARE(messageSet(QMailMessageKey::serverUid(QString(), Includes)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::serverUid(QString(), Includes)), noMessages);

    // Exclusion
    QCOMPARE(messageSet(QMailMessageKey::serverUid(QString("essage"), Excludes)), messageSet() << smsMessage);
    QCOMPARE(messageSet(~QMailMessageKey::serverUid(QString("essage"), Excludes)), messageSet() << inboxMessage1 << archivedMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::serverUid(QString("sim:"), Excludes)), messageSet() << inboxMessage1 << archivedMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::serverUid(QString("sim:"), Excludes)), messageSet() << smsMessage);
    QCOMPARE(messageSet(QMailMessageKey::serverUid(QString(""), Excludes)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::serverUid(QString(""), Excludes)), allMessages);
    QCOMPARE(messageSet(QMailMessageKey::serverUid(QString(), Excludes)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::serverUid(QString(), Excludes)), allMessages);
}

void tst_QMailStoreKeys::messageSize()
{
    // Comparisons
    QCOMPARE(messageSet(QMailMessageKey::size(0, Equal)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::size(0, Equal)), allMessages);
    QCOMPARE(messageSet(QMailMessageKey::size(0, NotEqual)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::size(0, NotEqual)), noMessages);
    QCOMPARE(messageSet(QMailMessageKey::size(0, LessThan)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::size(0, LessThan)), allMessages);
    QCOMPARE(messageSet(QMailMessageKey::size(0, GreaterThanEqual)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::size(0, GreaterThanEqual)), noMessages);
    QCOMPARE(messageSet(QMailMessageKey::size(0, LessThanEqual)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::size(0, LessThanEqual)), allMessages);
    QCOMPARE(messageSet(QMailMessageKey::size(0, GreaterThan)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::size(0, GreaterThan)), noMessages);

    QCOMPARE(messageSet(QMailMessageKey::size(1 * 1024, Equal)), messageSet() << smsMessage);
    QCOMPARE(messageSet(~QMailMessageKey::size(1 * 1024, Equal)), messageSet() << inboxMessage1 << archivedMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::size(1 * 1024, NotEqual)), messageSet() << inboxMessage1 << archivedMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::size(1 * 1024, NotEqual)), messageSet() << smsMessage);
    QCOMPARE(messageSet(QMailMessageKey::size(1 * 1024, LessThan)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::size(1 * 1024, LessThan)), allMessages);
    QCOMPARE(messageSet(QMailMessageKey::size(1 * 1024, GreaterThanEqual)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::size(1 * 1024, GreaterThanEqual)), noMessages);
    QCOMPARE(messageSet(QMailMessageKey::size(1 * 1024, LessThanEqual)), messageSet() << smsMessage);
    QCOMPARE(messageSet(~QMailMessageKey::size(1 * 1024, LessThanEqual)), messageSet() << inboxMessage1 << archivedMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::size(1 * 1024, GreaterThan)), messageSet() << inboxMessage1 << archivedMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::size(1 * 1024, GreaterThan)), messageSet() << smsMessage);

    QCOMPARE(messageSet(QMailMessageKey::size(5 * 1024, Equal)), messageSet() << inboxMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::size(5 * 1024, Equal)), messageSet() << smsMessage << archivedMessage1);
    QCOMPARE(messageSet(QMailMessageKey::size(5 * 1024, NotEqual)), messageSet() << smsMessage << archivedMessage1);
    QCOMPARE(messageSet(~QMailMessageKey::size(5 * 1024, NotEqual)), messageSet() << inboxMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::size(5 * 1024, LessThan)), messageSet() << smsMessage);
    QCOMPARE(messageSet(~QMailMessageKey::size(5 * 1024, LessThan)), messageSet() << inboxMessage1 << archivedMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::size(5 * 1024, GreaterThanEqual)), messageSet() << inboxMessage1 << archivedMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::size(5 * 1024, GreaterThanEqual)), messageSet() << smsMessage);
    QCOMPARE(messageSet(QMailMessageKey::size(5 * 1024, LessThanEqual)), messageSet() << smsMessage << inboxMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::size(5 * 1024, LessThanEqual)), messageSet() << archivedMessage1);
    QCOMPARE(messageSet(QMailMessageKey::size(5 * 1024, GreaterThan)), messageSet() << archivedMessage1);
    QCOMPARE(messageSet(~QMailMessageKey::size(5 * 1024, GreaterThan)), messageSet() << smsMessage << inboxMessage1 << inboxMessage2 << savedMessage2);

    QCOMPARE(messageSet(QMailMessageKey::size(15 * 1024, Equal)), messageSet() << archivedMessage1);
    QCOMPARE(messageSet(~QMailMessageKey::size(15 * 1024, Equal)), messageSet() << smsMessage << inboxMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::size(15 * 1024, NotEqual)), messageSet() << smsMessage << inboxMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::size(15 * 1024, NotEqual)), messageSet() << archivedMessage1);
    QCOMPARE(messageSet(QMailMessageKey::size(15 * 1024, LessThan)), messageSet() << smsMessage << inboxMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::size(15 * 1024, LessThan)), messageSet() << archivedMessage1);
    QCOMPARE(messageSet(QMailMessageKey::size(15 * 1024, GreaterThanEqual)), messageSet() << archivedMessage1);
    QCOMPARE(messageSet(~QMailMessageKey::size(15 * 1024, GreaterThanEqual)), messageSet() << smsMessage << inboxMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::size(15 * 1024, LessThanEqual)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::size(15 * 1024, LessThanEqual)), noMessages);
    QCOMPARE(messageSet(QMailMessageKey::size(15 * 1024, GreaterThan)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::size(15 * 1024, GreaterThan)), allMessages);
}

void tst_QMailStoreKeys::messageParentAccountId()
{
    // ID equality
    QCOMPARE(messageSet(QMailMessageKey::parentAccountId(accountId4, Equal)), messageSet() << smsMessage);
    QCOMPARE(messageSet(~QMailMessageKey::parentAccountId(accountId4, Equal)), allEmailMessages);
    QCOMPARE(messageSet(QMailMessageKey::parentAccountId(accountId1, Equal)), messageSet() << inboxMessage1 << archivedMessage1);
    QCOMPARE(messageSet(~QMailMessageKey::parentAccountId(accountId1, Equal)), messageSet() << smsMessage << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::parentAccountId(QMailAccountId(), Equal)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::parentAccountId(QMailAccountId(), Equal)), allMessages);

    // ID inequality
    QCOMPARE(messageSet(QMailMessageKey::parentAccountId(accountId4, NotEqual)), allEmailMessages);
    QCOMPARE(messageSet(~QMailMessageKey::parentAccountId(accountId4, NotEqual)), messageSet() << smsMessage);
    QCOMPARE(messageSet(QMailMessageKey::parentAccountId(accountId1, NotEqual)), messageSet() << smsMessage << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::parentAccountId(accountId1, NotEqual)), messageSet() << inboxMessage1 << archivedMessage1);
    QCOMPARE(messageSet(QMailMessageKey::parentAccountId(QMailAccountId(), NotEqual)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::parentAccountId(QMailAccountId(), NotEqual)), noMessages);

    // List inclusion
    QCOMPARE(messageSet(QMailMessageKey::parentAccountId(QMailAccountIdList())), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::parentAccountId(QMailAccountIdList())), allMessages);
    QCOMPARE(messageSet(QMailMessageKey::parentAccountId(QMailAccountIdList() << accountId4)), messageSet() << smsMessage);
    QCOMPARE(messageSet(~QMailMessageKey::parentAccountId(QMailAccountIdList() << accountId4)), messageSet() << inboxMessage1 << archivedMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::parentAccountId(QMailAccountIdList() << accountId4 << accountId1)), messageSet() << smsMessage << inboxMessage1 << archivedMessage1);
    QCOMPARE(messageSet(~QMailMessageKey::parentAccountId(QMailAccountIdList() << accountId4 << accountId1)), messageSet() << inboxMessage2 << savedMessage2);

    // List exclusion
    QCOMPARE(messageSet(QMailMessageKey::parentAccountId(QMailAccountIdList(), Excludes)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::parentAccountId(QMailAccountIdList(), Excludes)), noMessages);
    QCOMPARE(messageSet(QMailMessageKey::parentAccountId(QMailAccountIdList() << accountId4, Excludes)), messageSet() << inboxMessage1 << archivedMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::parentAccountId(QMailAccountIdList() << accountId4, Excludes)), messageSet() << smsMessage);
    QCOMPARE(messageSet(QMailMessageKey::parentAccountId(QMailAccountIdList() << accountId4 << accountId1, Excludes)), messageSet() << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::parentAccountId(QMailAccountIdList() << accountId4 << accountId1, Excludes)), messageSet() << smsMessage << inboxMessage1 << archivedMessage1);

    // Key matching
    QCOMPARE(accountSet(QMailAccountKey::id(accountId1, Equal)), accountSet() << accountId1);
    QCOMPARE(accountSet(~QMailAccountKey::id(accountId1, Equal)), accountSet() << accountId2 << accountId3 << accountId4);
    QCOMPARE(messageSet(QMailMessageKey::parentAccountId(QMailAccountKey::id(accountId1, Equal))), messageSet() << inboxMessage1 << archivedMessage1);
    QCOMPARE(messageSet(QMailMessageKey::parentAccountId(~QMailAccountKey::id(accountId1, Equal))), messageSet() << smsMessage << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::parentAccountId(QMailAccountKey::id(accountId1, Equal))), messageSet() << smsMessage << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::parentAccountId(~QMailAccountKey::id(accountId1, Equal))), messageSet() << inboxMessage1 << archivedMessage1); 

    QString name("Account 1");
    QCOMPARE(accountSet(QMailAccountKey::name(name, Equal)), accountSet() << accountId1);
    QCOMPARE(accountSet(~QMailAccountKey::name(name, Equal)), accountSet() << accountId2 << accountId3 << accountId4);
    QCOMPARE(messageSet(QMailMessageKey::parentAccountId(QMailAccountKey::name(name, Equal))), messageSet() << inboxMessage1 << archivedMessage1);
    QCOMPARE(messageSet(QMailMessageKey::parentAccountId(~QMailAccountKey::name(name, Equal))), messageSet() << smsMessage << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::parentAccountId(QMailAccountKey::name(name, Equal))), messageSet() << smsMessage << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::parentAccountId(~QMailAccountKey::name(name, Equal))), messageSet() << inboxMessage1 << archivedMessage1);

    QCOMPARE(accountSet(QMailAccountKey::messageType(QMailMessage::Instant, Equal)), accountSet() << accountId2);
    QCOMPARE(accountSet(~QMailAccountKey::messageType(QMailMessage::Instant, Equal)), accountSet() << accountId1 << accountId3 << accountId4);
    QCOMPARE(messageSet(QMailMessageKey::parentAccountId(QMailAccountKey::messageType(QMailMessage::Instant, Equal))), messageSet() << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::parentAccountId(~QMailAccountKey::messageType(QMailMessage::Instant, Equal))), messageSet() << smsMessage << inboxMessage1 << archivedMessage1);
    QCOMPARE(messageSet(~QMailMessageKey::parentAccountId(QMailAccountKey::messageType(QMailMessage::Instant, Equal))), messageSet() << smsMessage << inboxMessage1 << archivedMessage1);
    QCOMPARE(messageSet(~QMailMessageKey::parentAccountId(~QMailAccountKey::messageType(QMailMessage::Instant, Equal))), messageSet() << inboxMessage2 << savedMessage2);
}

void tst_QMailStoreKeys::messageAncestorFolderIds()
{
    // ID inclusion
    QCOMPARE(messageSet(QMailMessageKey::ancestorFolderIds(inboxId1, Includes)), messageSet() << archivedMessage1);
    QCOMPARE(messageSet(~QMailMessageKey::ancestorFolderIds(inboxId1, Includes)), messageSet() << smsMessage << inboxMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::ancestorFolderIds(inboxId2, Includes)), messageSet() << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::ancestorFolderIds(inboxId2, Includes)), messageSet() << smsMessage << inboxMessage1 << archivedMessage1 << inboxMessage2);
    QCOMPARE(messageSet(QMailMessageKey::ancestorFolderIds(QMailFolderId(QMailFolder::LocalStorageFolderId), Includes)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::ancestorFolderIds(QMailFolderId(QMailFolder::LocalStorageFolderId), Includes)), allMessages);

    // ID exclusion
    QCOMPARE(messageSet(QMailMessageKey::ancestorFolderIds(inboxId1, Excludes)), messageSet() << smsMessage << inboxMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::ancestorFolderIds(inboxId1, Excludes)), messageSet() << archivedMessage1);
    QCOMPARE(messageSet(QMailMessageKey::ancestorFolderIds(inboxId2, Excludes)), messageSet() << smsMessage << inboxMessage1 << archivedMessage1 << inboxMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::ancestorFolderIds(inboxId2, Excludes)), messageSet() << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::ancestorFolderIds(QMailFolderId(QMailFolder::LocalStorageFolderId), Excludes)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::ancestorFolderIds(QMailFolderId(QMailFolder::LocalStorageFolderId), Excludes)), noMessages);

    // List inclusion
    QCOMPARE(messageSet(QMailMessageKey::ancestorFolderIds(QMailFolderIdList())), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::ancestorFolderIds(QMailFolderIdList())), allMessages);
    QCOMPARE(messageSet(QMailMessageKey::ancestorFolderIds(QMailFolderIdList() << inboxId1)), messageSet() << archivedMessage1);
    QCOMPARE(messageSet(~QMailMessageKey::ancestorFolderIds(QMailFolderIdList() << inboxId1)), messageSet() << smsMessage << inboxMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::ancestorFolderIds(QMailFolderIdList() << inboxId1 << inboxId2)), messageSet() << archivedMessage1 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::ancestorFolderIds(QMailFolderIdList() << inboxId1 << inboxId2)), messageSet() << smsMessage << inboxMessage1 << inboxMessage2);

    // List exclusion
    QCOMPARE(messageSet(QMailMessageKey::ancestorFolderIds(QMailFolderIdList(), Excludes)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::ancestorFolderIds(QMailFolderIdList(), Excludes)), noMessages);
    QCOMPARE(messageSet(QMailMessageKey::ancestorFolderIds(QMailFolderIdList() << inboxId1, Excludes)), messageSet() << smsMessage << inboxMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::ancestorFolderIds(QMailFolderIdList() << inboxId1, Excludes)), messageSet() << archivedMessage1);
    QCOMPARE(messageSet(QMailMessageKey::ancestorFolderIds(QMailFolderIdList() << inboxId1 << inboxId2, Excludes)), messageSet() << smsMessage << inboxMessage1 << inboxMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::ancestorFolderIds(QMailFolderIdList() << inboxId1 << inboxId2, Excludes)), messageSet() << archivedMessage1 << savedMessage2);

    // Key matching - not currently implemented correctly
    QCOMPARE(folderSet(QMailFolderKey::id(inboxId1, Equal)), folderSet() << inboxId1);
    QCOMPARE(folderSet(~QMailFolderKey::id(inboxId1, Equal)), standardFolders + folderSet() << savedId1 << archivedId1 << inboxId2 << savedId2 << archivedId2);
    QCOMPARE(messageSet(QMailMessageKey::ancestorFolderIds(QMailFolderKey::id(inboxId1, Equal), Includes)), messageSet() << archivedMessage1);
    QCOMPARE(messageSet(QMailMessageKey::ancestorFolderIds(~QMailFolderKey::id(inboxId1, Equal), Includes)), messageSet() << archivedMessage1 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::ancestorFolderIds(QMailFolderKey::id(inboxId1, Equal), Includes)), messageSet() << smsMessage << inboxMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::ancestorFolderIds(~QMailFolderKey::id(inboxId1, Equal), Includes)), messageSet() << smsMessage << inboxMessage1 << inboxMessage2);

    QString path("Inbox/Saved");
    QCOMPARE(folderSet(QMailFolderKey::path(path, Equal)), folderSet() << savedId1 << savedId2);
    QCOMPARE(folderSet(~QMailFolderKey::path(path, Equal)), standardFolders + folderSet() << inboxId1 << archivedId1 << inboxId2 << archivedId2);
    QCOMPARE(messageSet(QMailMessageKey::ancestorFolderIds(QMailFolderKey::path(path, Equal))), messageSet() << archivedMessage1);
    QCOMPARE(messageSet(QMailMessageKey::ancestorFolderIds(~QMailFolderKey::path(path, Equal))), messageSet() << archivedMessage1 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::ancestorFolderIds(QMailFolderKey::path(path, Equal))), messageSet() << smsMessage << inboxMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::ancestorFolderIds(~QMailFolderKey::path(path, Equal))), messageSet() << smsMessage << inboxMessage1 << inboxMessage2);

    QCOMPARE(folderSet(QMailFolderKey::parentAccountId(accountId2, Equal)), folderSet() << inboxId2 << savedId2 << archivedId2);
    QCOMPARE(folderSet(~QMailFolderKey::parentAccountId(accountId2, Equal)), standardFolders + folderSet() << inboxId1 << savedId1 << archivedId1);
    QCOMPARE(messageSet(QMailMessageKey::ancestorFolderIds(QMailFolderKey::parentAccountId(accountId2, Equal))), messageSet() << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::ancestorFolderIds(~QMailFolderKey::parentAccountId(accountId2, Equal))), messageSet() << archivedMessage1);
    QCOMPARE(messageSet(~QMailMessageKey::ancestorFolderIds(QMailFolderKey::parentAccountId(accountId2, Equal))), messageSet() << smsMessage << inboxMessage1 << archivedMessage1 << inboxMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::ancestorFolderIds(~QMailFolderKey::parentAccountId(accountId2, Equal))), messageSet() << smsMessage << inboxMessage1 << inboxMessage2 << savedMessage2);
}

void tst_QMailStoreKeys::messageContentType()
{
    // ContentType equality
    QCOMPARE(messageSet(QMailMessageKey::contentType(QMailMessage::PlainTextContent, Equal)), messageSet() << smsMessage << inboxMessage1);
    QCOMPARE(messageSet(~QMailMessageKey::contentType(QMailMessage::PlainTextContent, Equal)), messageSet() << archivedMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::contentType(QMailMessage::HtmlContent, Equal)), messageSet() << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::contentType(QMailMessage::HtmlContent, Equal)), messageSet() << smsMessage << inboxMessage1 << archivedMessage1);
    QCOMPARE(messageSet(QMailMessageKey::contentType(QMailMessage::VoicemailContent, Equal)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::contentType(QMailMessage::VoicemailContent, Equal)), allMessages);

    // ContentType inequality
    QCOMPARE(messageSet(QMailMessageKey::contentType(QMailMessage::PlainTextContent, NotEqual)), messageSet() << archivedMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::contentType(QMailMessage::PlainTextContent, NotEqual)), messageSet() << smsMessage << inboxMessage1);
    QCOMPARE(messageSet(QMailMessageKey::contentType(QMailMessage::HtmlContent, NotEqual)), messageSet() << smsMessage << inboxMessage1 << archivedMessage1);
    QCOMPARE(messageSet(~QMailMessageKey::contentType(QMailMessage::HtmlContent, NotEqual)), messageSet() << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::contentType(QMailMessage::VoicemailContent, NotEqual)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::contentType(QMailMessage::VoicemailContent, NotEqual)), noMessages);

    // List inclusion
    QCOMPARE(messageSet(QMailMessageKey::contentType(QList<QMailMessage::ContentType>())), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::contentType(QList<QMailMessage::ContentType>())), allMessages);
    QCOMPARE(messageSet(QMailMessageKey::contentType(QList<QMailMessage::ContentType>() << QMailMessage::VideoContent)), messageSet() << archivedMessage1);
    QCOMPARE(messageSet(~QMailMessageKey::contentType(QList<QMailMessage::ContentType>() << QMailMessage::VideoContent)), messageSet() << smsMessage << inboxMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::contentType(QList<QMailMessage::ContentType>() << QMailMessage::VideoContent << QMailMessage::HtmlContent)), messageSet() << archivedMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::contentType(QList<QMailMessage::ContentType>() << QMailMessage::VideoContent << QMailMessage::HtmlContent)), messageSet() << smsMessage << inboxMessage1);

    // List exclusion
    QCOMPARE(messageSet(QMailMessageKey::contentType(QList<QMailMessage::ContentType>(), Excludes)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::contentType(QList<QMailMessage::ContentType>(), Excludes)), noMessages);
    QCOMPARE(messageSet(QMailMessageKey::contentType(QList<QMailMessage::ContentType>() << QMailMessage::VideoContent, Excludes)), messageSet() << smsMessage << inboxMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::contentType(QList<QMailMessage::ContentType>() << QMailMessage::VideoContent, Excludes)), messageSet() << archivedMessage1);
    QCOMPARE(messageSet(QMailMessageKey::contentType(QList<QMailMessage::ContentType>() << QMailMessage::VideoContent << QMailMessage::HtmlContent, Excludes)), messageSet() << smsMessage << inboxMessage1);
    QCOMPARE(messageSet(~QMailMessageKey::contentType(QList<QMailMessage::ContentType>() << QMailMessage::VideoContent << QMailMessage::HtmlContent, Excludes)), messageSet() << archivedMessage1 << inboxMessage2 << savedMessage2);
}

void tst_QMailStoreKeys::messagePreviousParentFolderId()
{
    QMailFolderId localFolder(QMailFolder::LocalStorageFolderId);

    // ID equality
    QCOMPARE(messageSet(QMailMessageKey::previousParentFolderId(localFolder, Equal)), noMessages);

    QCOMPARE(messageSet(~QMailMessageKey::previousParentFolderId(localFolder, Equal)), allMessages);
    QCOMPARE(messageSet(QMailMessageKey::previousParentFolderId(inboxId1, Equal)), messageSet() << archivedMessage1);
    QCOMPARE(messageSet(~QMailMessageKey::previousParentFolderId(inboxId1, Equal)), QSet<QMailMessageId>(allMessages).subtract(messageSet() << archivedMessage1));
    QCOMPARE(messageSet(QMailMessageKey::previousParentFolderId(QMailFolderId(), Equal)), messageSet() << smsMessage << inboxMessage1 << inboxMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::previousParentFolderId(QMailFolderId(), Equal)), messageSet() << archivedMessage1 << savedMessage2);

    // ID inequality
    QCOMPARE(messageSet(QMailMessageKey::previousParentFolderId(localFolder, NotEqual)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::previousParentFolderId(localFolder, NotEqual)), messageSet());
    QCOMPARE(messageSet(QMailMessageKey::previousParentFolderId(inboxId1, NotEqual)), QSet<QMailMessageId>(allMessages).subtract(messageSet() << archivedMessage1));
    QCOMPARE(messageSet(~QMailMessageKey::previousParentFolderId(inboxId1, NotEqual)), messageSet() << archivedMessage1);
    QCOMPARE(messageSet(QMailMessageKey::previousParentFolderId(QMailFolderId(), NotEqual)), messageSet() << archivedMessage1 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::previousParentFolderId(QMailFolderId(), NotEqual)), messageSet() << smsMessage << inboxMessage1 << inboxMessage2);

    // List inclusion
    QCOMPARE(messageSet(QMailMessageKey::previousParentFolderId(QMailFolderIdList())), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::previousParentFolderId(QMailFolderIdList())), allMessages);
    QCOMPARE(messageSet(QMailMessageKey::previousParentFolderId(QMailFolderIdList() << inboxId1)), messageSet() << archivedMessage1);
    QCOMPARE(messageSet(~QMailMessageKey::previousParentFolderId(QMailFolderIdList() << inboxId1)), QSet<QMailMessageId>(allMessages).subtract(messageSet() << archivedMessage1));
    QCOMPARE(messageSet(QMailMessageKey::previousParentFolderId(QMailFolderIdList() << inboxId1 << inboxId2)), messageSet() << archivedMessage1 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::previousParentFolderId(QMailFolderIdList() << inboxId1 << inboxId2)), QSet<QMailMessageId>(allMessages).subtract(messageSet() << archivedMessage1 << savedMessage2));

    // List exclusion
    QCOMPARE(messageSet(QMailMessageKey::previousParentFolderId(QMailFolderIdList(), Excludes)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::previousParentFolderId(QMailFolderIdList(), Excludes)), noMessages);
    QCOMPARE(messageSet(QMailMessageKey::previousParentFolderId(QMailFolderIdList() << inboxId1, Excludes)), QSet<QMailMessageId>(allMessages).subtract(messageSet() << archivedMessage1));
    QCOMPARE(messageSet(~QMailMessageKey::previousParentFolderId(QMailFolderIdList() << inboxId1, Excludes)), messageSet() << archivedMessage1);
    QCOMPARE(messageSet(QMailMessageKey::previousParentFolderId(QMailFolderIdList() << inboxId1 << inboxId2, Excludes)), messageSet() << smsMessage << inboxMessage1 << inboxMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::previousParentFolderId(QMailFolderIdList() << inboxId1 << inboxId2, Excludes)), messageSet() << archivedMessage1 << savedMessage2);

    // Key matching
    QCOMPARE(folderSet(QMailFolderKey::id(inboxId1, Equal)), folderSet() << inboxId1);
    QCOMPARE(folderSet(~QMailFolderKey::id(inboxId1, Equal)), standardFolders + folderSet() << savedId1 << archivedId1 << inboxId2 << savedId2 << archivedId2);
    QCOMPARE(messageSet(QMailMessageKey::previousParentFolderId(QMailFolderKey::id(inboxId1, Equal))), messageSet() << archivedMessage1);
    QCOMPARE(messageSet(QMailMessageKey::previousParentFolderId(~QMailFolderKey::id(inboxId1, Equal))), messageSet() << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::previousParentFolderId(QMailFolderKey::id(inboxId1, Equal))), QSet<QMailMessageId>(allMessages).subtract(messageSet() << archivedMessage1));
    QCOMPARE(messageSet(~QMailMessageKey::previousParentFolderId(~QMailFolderKey::id(inboxId1, Equal))), QSet<QMailMessageId>(allMessages).subtract(messageSet() << savedMessage2));

    QString path("Inbox/Saved");
    QCOMPARE(folderSet(QMailFolderKey::path(path, Equal)), folderSet() << savedId1 << savedId2);
    QCOMPARE(folderSet(~QMailFolderKey::path(path, Equal)), standardFolders + folderSet() << inboxId1 << archivedId1 << inboxId2 << archivedId2);
    QCOMPARE(messageSet(QMailMessageKey::previousParentFolderId(QMailFolderKey::path(path, Equal))), noMessages);
    QCOMPARE(messageSet(QMailMessageKey::previousParentFolderId(~QMailFolderKey::path(path, Equal))), messageSet() << archivedMessage1 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::previousParentFolderId(QMailFolderKey::path(path, Equal))), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::previousParentFolderId(~QMailFolderKey::path(path, Equal))), messageSet() << smsMessage << inboxMessage1 << inboxMessage2);

    QCOMPARE(folderSet(QMailFolderKey::status(QMailFolder::SynchronizationEnabled, Includes)), folderSet() << inboxId1 << savedId1);
    QCOMPARE(folderSet(~QMailFolderKey::status(QMailFolder::SynchronizationEnabled, Includes)), standardFolders + folderSet() << archivedId1 << inboxId2 << savedId2 << archivedId2);
    QCOMPARE(messageSet(QMailMessageKey::previousParentFolderId(QMailFolderKey::status(QMailFolder::SynchronizationEnabled, Includes))), messageSet() << archivedMessage1);
    QCOMPARE(messageSet(QMailMessageKey::previousParentFolderId(~QMailFolderKey::status(QMailFolder::SynchronizationEnabled, Includes))), messageSet() << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::previousParentFolderId(QMailFolderKey::status(QMailFolder::SynchronizationEnabled, Includes))),QSet<QMailMessageId>(allMessages).subtract(messageSet() << archivedMessage1));
    QCOMPARE(messageSet(~QMailMessageKey::previousParentFolderId(~QMailFolderKey::status(QMailFolder::SynchronizationEnabled, Includes))), QSet<QMailMessageId>(allMessages).subtract(messageSet() << savedMessage2));
}

void tst_QMailStoreKeys::messageInResponseTo()
{
    // ID equality
    QCOMPARE(messageSet(QMailMessageKey::inResponseTo(smsMessage, Equal)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::inResponseTo(smsMessage, Equal)), allMessages);
    QCOMPARE(messageSet(QMailMessageKey::inResponseTo(inboxMessage1, Equal)), messageSet() << inboxMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::inResponseTo(inboxMessage1, Equal)), messageSet() << smsMessage << inboxMessage1 << archivedMessage1 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::inResponseTo(QMailMessageId(), Equal)), messageSet() << smsMessage << inboxMessage1 << archivedMessage1);
    QCOMPARE(messageSet(~QMailMessageKey::inResponseTo(QMailMessageId(), Equal)), messageSet() << inboxMessage2 << savedMessage2);

    // ID inequality
    QCOMPARE(messageSet(QMailMessageKey::inResponseTo(smsMessage, NotEqual)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::inResponseTo(smsMessage, NotEqual)), noMessages);
    QCOMPARE(messageSet(QMailMessageKey::inResponseTo(inboxMessage1, NotEqual)), messageSet() << smsMessage << inboxMessage1 << archivedMessage1 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::inResponseTo(inboxMessage1, NotEqual)), messageSet() << inboxMessage2);
    QCOMPARE(messageSet(QMailMessageKey::inResponseTo(QMailMessageId(), NotEqual)), messageSet() << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::inResponseTo(QMailMessageId(), NotEqual)), messageSet() << smsMessage << inboxMessage1 << archivedMessage1);

    // List inclusion
    QCOMPARE(messageSet(QMailMessageKey::inResponseTo(QMailMessageIdList())), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::inResponseTo(QMailMessageIdList())), allMessages);
    QCOMPARE(messageSet(QMailMessageKey::inResponseTo(QMailMessageIdList() << inboxMessage1)), messageSet() << inboxMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::inResponseTo(QMailMessageIdList() << inboxMessage1)), messageSet() << smsMessage << inboxMessage1 << archivedMessage1 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::inResponseTo(QMailMessageIdList() << inboxMessage1 << archivedMessage1)), messageSet() << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::inResponseTo(QMailMessageIdList() << inboxMessage1 << archivedMessage1)), messageSet() << smsMessage << inboxMessage1 << archivedMessage1);

    // List exclusion
    QCOMPARE(messageSet(QMailMessageKey::inResponseTo(QMailMessageIdList(), Excludes)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::inResponseTo(QMailMessageIdList(), Excludes)), noMessages);
    QCOMPARE(messageSet(QMailMessageKey::inResponseTo(QMailMessageIdList() << inboxMessage1, Excludes)), messageSet() << smsMessage << inboxMessage1 << archivedMessage1 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::inResponseTo(QMailMessageIdList() << inboxMessage1, Excludes)), messageSet() << inboxMessage2);
    QCOMPARE(messageSet(QMailMessageKey::inResponseTo(QMailMessageIdList() << inboxMessage1 << archivedMessage1, Excludes)), messageSet() << smsMessage << inboxMessage1 << archivedMessage1);
    QCOMPARE(messageSet(~QMailMessageKey::inResponseTo(QMailMessageIdList() << inboxMessage1 << archivedMessage1, Excludes)), messageSet() << inboxMessage2 << savedMessage2);

    // Key matching
    QCOMPARE(messageSet(QMailMessageKey::id(inboxMessage1, Equal)), messageSet() << inboxMessage1);
    QCOMPARE(messageSet(~QMailMessageKey::id(inboxMessage1, Equal)), messageSet() << smsMessage << archivedMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::inResponseTo(QMailMessageKey::id(inboxMessage1, Equal))), messageSet() << inboxMessage2);
    QCOMPARE(messageSet(QMailMessageKey::inResponseTo(~QMailMessageKey::id(inboxMessage1, Equal))), messageSet() << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::inResponseTo(QMailMessageKey::id(inboxMessage1, Equal))), messageSet() << smsMessage << inboxMessage1 << archivedMessage1 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::inResponseTo(~QMailMessageKey::id(inboxMessage1, Equal))), messageSet() << smsMessage << inboxMessage1 << archivedMessage1 << inboxMessage2);

    QString subject("archivedMessage1");
    QCOMPARE(messageSet(QMailMessageKey::subject(subject, Equal)), messageSet() << archivedMessage1);
    QCOMPARE(messageSet(~QMailMessageKey::subject(subject, Equal)), messageSet() << smsMessage << inboxMessage1 << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::inResponseTo(QMailMessageKey::subject(subject, Equal))), messageSet() << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::inResponseTo(~QMailMessageKey::subject(subject, Equal))), messageSet() << inboxMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::inResponseTo(QMailMessageKey::subject(subject, Equal))), messageSet() << smsMessage << inboxMessage1 << archivedMessage1 << inboxMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::inResponseTo(~QMailMessageKey::subject(subject, Equal))), messageSet() << smsMessage << inboxMessage1 << archivedMessage1 << savedMessage2);

    QCOMPARE(messageSet(QMailMessageKey::status(QMailMessage::New, Includes)), messageSet() << inboxMessage1 << inboxMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::status(QMailMessage::New, Includes)), messageSet() << smsMessage << archivedMessage1 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::inResponseTo(QMailMessageKey::status(QMailMessage::New, Includes))), messageSet() << inboxMessage2);
    QCOMPARE(messageSet(QMailMessageKey::inResponseTo(~QMailMessageKey::status(QMailMessage::New, Includes))), messageSet() << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::inResponseTo(QMailMessageKey::status(QMailMessage::New, Includes))), messageSet() << smsMessage << inboxMessage1 << archivedMessage1 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::inResponseTo(~QMailMessageKey::status(QMailMessage::New, Includes))), messageSet() << smsMessage << inboxMessage1 << archivedMessage1 << inboxMessage2);
}

void tst_QMailStoreKeys::messageResponseType()
{
    // MessageType equality
    QCOMPARE(messageSet(QMailMessageKey::responseType(QMailMessage::NoResponse, Equal)), messageSet() << smsMessage << inboxMessage1 << archivedMessage1);
    QCOMPARE(messageSet(~QMailMessageKey::responseType(QMailMessage::NoResponse, Equal)), messageSet() << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::responseType(QMailMessage::Reply, Equal)), messageSet() << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::responseType(QMailMessage::Reply, Equal)), messageSet() << smsMessage << inboxMessage1 << archivedMessage1 << inboxMessage2);
    QCOMPARE(messageSet(QMailMessageKey::responseType(QMailMessage::Forward, Equal)), messageSet() << inboxMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::responseType(QMailMessage::Forward, Equal)), messageSet() << smsMessage << inboxMessage1 << archivedMessage1 << savedMessage2);

    // ResponseType inequality
    QCOMPARE(messageSet(QMailMessageKey::responseType(QMailMessage::NoResponse, NotEqual)), messageSet() << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::responseType(QMailMessage::NoResponse, NotEqual)), messageSet() << smsMessage << inboxMessage1 << archivedMessage1);
    QCOMPARE(messageSet(QMailMessageKey::responseType(QMailMessage::Reply, NotEqual)), messageSet() << smsMessage << inboxMessage1 << archivedMessage1 << inboxMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::responseType(QMailMessage::Reply, NotEqual)), messageSet() << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::responseType(QMailMessage::Forward, NotEqual)), messageSet() << smsMessage << inboxMessage1 << archivedMessage1 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::responseType(QMailMessage::Forward, NotEqual)), messageSet() << inboxMessage2);

    // List inclusion
    QCOMPARE(messageSet(QMailMessageKey::responseType(QList<QMailMessage::ResponseType>())), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::responseType(QList<QMailMessage::ResponseType>())), allMessages);
    QCOMPARE(messageSet(QMailMessageKey::responseType(QList<QMailMessage::ResponseType>() << QMailMessage::Reply)), messageSet() << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::responseType(QList<QMailMessage::ResponseType>() << QMailMessage::Reply)), messageSet() << smsMessage << inboxMessage1 << archivedMessage1 << inboxMessage2);
    QCOMPARE(messageSet(QMailMessageKey::responseType(QList<QMailMessage::ResponseType>() << QMailMessage::Reply << QMailMessage::Forward)), messageSet() << inboxMessage2 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::responseType(QList<QMailMessage::ResponseType>() << QMailMessage::Reply << QMailMessage::Forward)), messageSet() << smsMessage << inboxMessage1 << archivedMessage1);

    // List exclusion
    QCOMPARE(messageSet(QMailMessageKey::responseType(QList<QMailMessage::ResponseType>(), Excludes)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::responseType(QList<QMailMessage::ResponseType>(), Excludes)), noMessages);
    QCOMPARE(messageSet(QMailMessageKey::responseType(QList<QMailMessage::ResponseType>() << QMailMessage::Reply, Excludes)), messageSet() << smsMessage << inboxMessage1 << archivedMessage1 << inboxMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::responseType(QList<QMailMessage::ResponseType>() << QMailMessage::Reply, Excludes)), messageSet() << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::responseType(QList<QMailMessage::ResponseType>() << QMailMessage::Reply << QMailMessage::Forward, Excludes)), messageSet() << smsMessage << inboxMessage1 << archivedMessage1);
    QCOMPARE(messageSet(~QMailMessageKey::responseType(QList<QMailMessage::ResponseType>() << QMailMessage::Reply << QMailMessage::Forward, Excludes)), messageSet() << inboxMessage2 << savedMessage2);
}

void tst_QMailStoreKeys::messageCustom()
{
    // Test for existence
    QCOMPARE(messageSet(QMailMessageKey::customField("present")), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::customField("present")), noMessages);
    QCOMPARE(messageSet(QMailMessageKey::customField("todo")), messageSet() << smsMessage << inboxMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::customField("todo")), messageSet() << inboxMessage1 << archivedMessage1 << savedMessage2);
    QCOMPARE(messageSet(QMailMessageKey::customField("spam")), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::customField("spam")), allMessages);
    QCOMPARE(messageSet(QMailMessageKey::customField(QString(""))), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::customField(QString(""))), allMessages);
    QCOMPARE(messageSet(QMailMessageKey::customField(QString())), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::customField(QString())), allMessages);

    // Test for non-existence
    QCOMPARE(messageSet(QMailMessageKey::customField("present", Absent)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::customField("present", Absent)), allMessages);
    QCOMPARE(messageSet(QMailMessageKey::customField("todo", Absent)), messageSet() << inboxMessage1 << archivedMessage1 << savedMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::customField("todo", Absent)), messageSet() << smsMessage << inboxMessage2);
    QCOMPARE(messageSet(QMailMessageKey::customField("spam", Absent)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::customField("spam", Absent)), noMessages);
    QCOMPARE(messageSet(QMailMessageKey::customField(QString(""), Absent)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::customField(QString(""), Absent)), noMessages);
    QCOMPARE(messageSet(QMailMessageKey::customField(QString(), Absent)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::customField(QString(), Absent)), noMessages);

    // Test for content equality
    QCOMPARE(messageSet(QMailMessageKey::customField("present", "true")), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::customField("present", "true")), noMessages);
    QCOMPARE(messageSet(QMailMessageKey::customField("present", "false")), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::customField("present", "false")), allMessages);
    QCOMPARE(messageSet(QMailMessageKey::customField("todo", "true")), messageSet() << smsMessage);
    QCOMPARE(messageSet(~QMailMessageKey::customField("todo", "true")), messageSet() << inboxMessage2);
    QCOMPARE(messageSet(QMailMessageKey::customField("todo", "false")), messageSet() << inboxMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::customField("todo", "false")), messageSet() << smsMessage);
    QCOMPARE(messageSet(QMailMessageKey::customField("todo", QString(""))), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::customField("todo", QString(""))), messageSet() << smsMessage << inboxMessage2);
    QCOMPARE(messageSet(QMailMessageKey::customField("todo", QString())), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::customField("todo", QString())), messageSet() << smsMessage << inboxMessage2);

    // Test for content inequality
    QCOMPARE(messageSet(QMailMessageKey::customField("present", "true", NotEqual)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::customField("present", "true", NotEqual)), allMessages);
    QCOMPARE(messageSet(QMailMessageKey::customField("present", "false", NotEqual)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::customField("present", "false", NotEqual)), noMessages);
    QCOMPARE(messageSet(QMailMessageKey::customField("todo", "true", NotEqual)), messageSet() << inboxMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::customField("todo", "true", NotEqual)), messageSet() << smsMessage);
    QCOMPARE(messageSet(QMailMessageKey::customField("todo", "false", NotEqual)), messageSet() << smsMessage);
    QCOMPARE(messageSet(~QMailMessageKey::customField("todo", "false", NotEqual)), messageSet() << inboxMessage2);
    QCOMPARE(messageSet(QMailMessageKey::customField("todo", QString(""), NotEqual)), messageSet() << smsMessage << inboxMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::customField("todo", QString(""), NotEqual)), noMessages);
    QCOMPARE(messageSet(QMailMessageKey::customField("todo", QString(), NotEqual)), messageSet() << smsMessage << inboxMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::customField("todo", QString(), NotEqual)), noMessages);

    // Test for partial matches
    QCOMPARE(messageSet(QMailMessageKey::customField("present", "bicycle", Includes)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::customField("present", "bicycle", Includes)), allMessages);
    QCOMPARE(messageSet(QMailMessageKey::customField("present", "rue", Includes)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::customField("present", "rue", Includes)), noMessages);
    QCOMPARE(messageSet(QMailMessageKey::customField("todo", "fal", Includes)), messageSet() << inboxMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::customField("todo", "fal", Includes)), messageSet() << smsMessage);
    QCOMPARE(messageSet(QMailMessageKey::customField("todo", "r", Includes)), messageSet() << smsMessage);
    QCOMPARE(messageSet(~QMailMessageKey::customField("todo", "r", Includes)), messageSet() << inboxMessage2);
    QCOMPARE(messageSet(QMailMessageKey::customField("todo", QString(""), Includes)), messageSet() << smsMessage << inboxMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::customField("todo", QString(""), Includes)), noMessages);
    QCOMPARE(messageSet(QMailMessageKey::customField("todo", QString(), Includes)), messageSet() << smsMessage << inboxMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::customField("todo", QString(), Includes)), noMessages);

    // Test for partial match exclusion
    QCOMPARE(messageSet(QMailMessageKey::customField("present", "bicycle", Excludes)), allMessages);
    QCOMPARE(messageSet(~QMailMessageKey::customField("present", "bicycle", Excludes)), noMessages);
    QCOMPARE(messageSet(QMailMessageKey::customField("present", "rue", Excludes)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::customField("present", "rue", Excludes)), allMessages);
    QCOMPARE(messageSet(QMailMessageKey::customField("todo", "fal", Excludes)), messageSet() << smsMessage);
    QCOMPARE(messageSet(~QMailMessageKey::customField("todo", "fal", Excludes)), messageSet() << inboxMessage2);
    QCOMPARE(messageSet(QMailMessageKey::customField("todo", "r", Excludes)), messageSet() << inboxMessage2);
    QCOMPARE(messageSet(~QMailMessageKey::customField("todo", "r", Excludes)), messageSet() << smsMessage);
    QCOMPARE(messageSet(QMailMessageKey::customField("todo", QString(""), Excludes)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::customField("todo", QString(""), Excludes)), messageSet() << smsMessage << inboxMessage2);
    QCOMPARE(messageSet(QMailMessageKey::customField("todo", QString(), Excludes)), noMessages);
    QCOMPARE(messageSet(~QMailMessageKey::customField("todo", QString(), Excludes)), messageSet() << smsMessage << inboxMessage2);
}

void tst_QMailStoreKeys::listModel()
{
    QMailMessageListModel model(this);

    // Default sort
    model.setKey(QMailMessageKey());

    QCOMPARE(model.indexFromId(smsMessage).row(), 0);
    QCOMPARE(model.indexFromId(inboxMessage1).row(), 1);
    QCOMPARE(model.indexFromId(archivedMessage1).row(), 2);
    QCOMPARE(model.indexFromId(inboxMessage2).row(), 3);
    QCOMPARE(model.indexFromId(savedMessage2).row(), 4);

    QCOMPARE(model.rowCount(), 5);

    QCOMPARE(model.idFromIndex(model.index(0, 0)), smsMessage);
    QCOMPARE(model.idFromIndex(model.index(1, 0)), inboxMessage1);
    QCOMPARE(model.idFromIndex(model.index(2, 0)), archivedMessage1);
    QCOMPARE(model.idFromIndex(model.index(3, 0)), inboxMessage2);
    QCOMPARE(model.idFromIndex(model.index(4, 0)), savedMessage2);

    // Sort by descending subject - note majuscules sort before miniscules
    model.setKey(QMailMessageKey());
    model.setSortKey(QMailMessageSortKey::subject(Qt::DescendingOrder));

    QCOMPARE(model.indexFromId(inboxMessage1).row(), 0);
    QCOMPARE(model.indexFromId(archivedMessage1).row(), 1);
    QCOMPARE(model.indexFromId(smsMessage).row(), 2);
    QCOMPARE(model.indexFromId(savedMessage2).row(), 3);
    QCOMPARE(model.indexFromId(inboxMessage2).row(), 4);

    QCOMPARE(model.rowCount(), 5);

    QCOMPARE(model.idFromIndex(model.index(0, 0)), inboxMessage1);
    QCOMPARE(model.idFromIndex(model.index(1, 0)), archivedMessage1);
    QCOMPARE(model.idFromIndex(model.index(2, 0)), smsMessage);
    QCOMPARE(model.idFromIndex(model.index(3, 0)), savedMessage2);
    QCOMPARE(model.idFromIndex(model.index(4, 0)), inboxMessage2);

    // Only display messages from inbox1, or with a response type
    model.setKey(QMailMessageKey::parentFolderId(inboxId1) | QMailMessageKey::responseType(QMailMessage::NoResponse, QMailDataComparator::NotEqual));

    QCOMPARE(model.indexFromId(inboxMessage1).row(), 0);
    QCOMPARE(model.indexFromId(savedMessage2).row(), 1);
    QCOMPARE(model.indexFromId(inboxMessage2).row(), 2);

    QCOMPARE(model.rowCount(), 3);

    QCOMPARE(model.idFromIndex(model.index(0, 0)), inboxMessage1);
    QCOMPARE(model.idFromIndex(model.index(1, 0)), savedMessage2);
    QCOMPARE(model.idFromIndex(model.index(2, 0)), inboxMessage2);
}

void tst_QMailStoreKeys::threadedModel()
{
    QMailMessageThreadedModel model(this);

    // Default sort
    model.setKey(QMailMessageKey());

    QCOMPARE(model.indexFromId(smsMessage).row(), 0);
    QCOMPARE(model.indexFromId(inboxMessage1).row(), 1);
    QCOMPARE(model.indexFromId(archivedMessage1).row(), 2);
    QCOMPARE(model.indexFromId(inboxMessage2).row(), 0);
    QCOMPARE(model.indexFromId(inboxMessage2).parent().row(), 1);
    QCOMPARE(model.indexFromId(savedMessage2).row(), 0);
    QCOMPARE(model.indexFromId(savedMessage2).parent().row(), 2);

    QCOMPARE(model.rowCount(), 3);
    QCOMPARE(model.rowCount(model.indexFromId(inboxMessage1)), 1);
    QCOMPARE(model.rowCount(model.indexFromId(archivedMessage1)), 1);

    QCOMPARE(model.idFromIndex(model.index(0, 0)), smsMessage);
    QCOMPARE(model.idFromIndex(model.index(1, 0)), inboxMessage1);
    QCOMPARE(model.idFromIndex(model.index(2, 0)), archivedMessage1);
    QCOMPARE(model.idFromIndex(model.index(0, 0, model.indexFromId(inboxMessage1))), inboxMessage2);
    QCOMPARE(model.idFromIndex(model.index(0, 0, model.indexFromId(archivedMessage1))), savedMessage2);

    // Sort by descending subject - note majuscules sort before miniscules
    model.setKey(QMailMessageKey());
    model.setSortKey(QMailMessageSortKey::subject(Qt::DescendingOrder));

    QCOMPARE(model.indexFromId(inboxMessage1).row(), 0);
    QCOMPARE(model.indexFromId(archivedMessage1).row(), 1);
    QCOMPARE(model.indexFromId(smsMessage).row(), 2);
    QCOMPARE(model.indexFromId(inboxMessage2).row(), 0);
    QCOMPARE(model.indexFromId(inboxMessage2).parent().row(), 0);
    QCOMPARE(model.indexFromId(savedMessage2).row(), 0);
    QCOMPARE(model.indexFromId(savedMessage2).parent().row(), 1);

    QCOMPARE(model.rowCount(), 3);
    QCOMPARE(model.rowCount(model.indexFromId(inboxMessage1)), 1);
    QCOMPARE(model.rowCount(model.indexFromId(archivedMessage1)), 1);

    QCOMPARE(model.idFromIndex(model.index(0, 0)), inboxMessage1);
    QCOMPARE(model.idFromIndex(model.index(1, 0)), archivedMessage1);
    QCOMPARE(model.idFromIndex(model.index(2, 0)), smsMessage);
    QCOMPARE(model.idFromIndex(model.index(0, 0, model.indexFromId(inboxMessage1))), inboxMessage2);
    QCOMPARE(model.idFromIndex(model.index(0, 0, model.indexFromId(archivedMessage1))), savedMessage2);

    // Only display messages from inbox1, or with a response type
    model.setKey(QMailMessageKey::parentFolderId(inboxId1) | QMailMessageKey::responseType(QMailMessage::NoResponse, QMailDataComparator::NotEqual));

    QCOMPARE(model.indexFromId(inboxMessage1).row(), 0);
    QCOMPARE(model.indexFromId(savedMessage2).row(), 1);
    QCOMPARE(model.indexFromId(inboxMessage2).row(), 0);
    QCOMPARE(model.indexFromId(inboxMessage2).parent().row(), 0);

    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(model.rowCount(model.indexFromId(inboxMessage1)), 1);

    QCOMPARE(model.idFromIndex(model.index(0, 0)), inboxMessage1);
    QCOMPARE(model.idFromIndex(model.index(1, 0)), savedMessage2);
    QCOMPARE(model.idFromIndex(model.index(0, 0, model.indexFromId(inboxMessage1))), inboxMessage2);
}

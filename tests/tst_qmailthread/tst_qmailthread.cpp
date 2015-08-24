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

#include <QObject>
#include <QTest>
#include <qmailaddress.h>
#include <qmailaccount.h>
#include <qmailthread.h>
#include <qmailstore.h>
#include <qmailthreadkey.h>
#include <qmailthreadsortkey.h>
#include <ctype.h>


class tst_qmailthread : public QObject
{
    Q_OBJECT

public:
    tst_qmailthread();
    virtual ~tst_qmailthread();

private slots:
    virtual void initTestCase();
    virtual void cleanupTestCase();
    virtual void init();
    virtual void cleanup();

    void test_messageCount();
    void test_UnreadCount();
    void test_parentId();
    void test_cloneId();
    void test_threadKeys();
private:
    void initAccount();
    QMailAccount account;
    QMailThread mailthread;

    QMailAccountId accountId1, accountId2, accountId3, accountId4;
    QMailFolderId inboxId1, inboxId2, savedId1, savedId2, archivedId1, archivedId2;
    QMailMessageId smsMessage, inboxMessage1, archivedMessage1, inboxMessage2, savedMessage2;

    QSet<QMailAccountId> noAccounts, verifiedAccounts, unverifiedAccounts, allAccounts;
    QSet<QMailFolderId> noFolders, allFolders, standardFolders;
    QSet<QMailMessageId> noMessages, allMessages, allEmailMessages;
};

QTEST_MAIN(tst_qmailthread)
#include "tst_qmailthread.moc"

#define CRLF "\015\012"

tst_qmailthread::tst_qmailthread()
{

}

tst_qmailthread::~tst_qmailthread()
{

}

void tst_qmailthread::initTestCase()
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

        mailthread.setParentAccountId(account.id());
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

void tst_qmailthread::cleanupTestCase()
{
    QMailStore::instance()->clearContent();
}

void tst_qmailthread::init()
{    

}

void tst_qmailthread::cleanup()
{

}

void tst_qmailthread::test_UnreadCount()
{
    QCOMPARE((int) mailthread.unreadCount(), 0);
    mailthread.setUnreadCount(1);
    QCOMPARE((int) mailthread.unreadCount(), 1);
}

void tst_qmailthread::test_messageCount()
{
    QCOMPARE((int) mailthread.messageCount(), 0);
    mailthread.setMessageCount(1);
    QCOMPARE((int) mailthread.messageCount(), 1);
}

void tst_qmailthread::test_parentId()
{
    QCOMPARE(mailthread.parentAccountId(), accountId1);
    mailthread.setMessageCount(10);
    mailthread.setUnreadCount(20);

    mailthread.setServerUid("40");
    QMailThread thread1 = mailthread;
    QCOMPARE(thread1.id(), mailthread.id());
    QCOMPARE(thread1.serverUid(), QString("40") );
    QCOMPARE((int) thread1.messageCount(), 10);
    QCOMPARE((int) thread1.unreadCount(), 20);
}

void tst_qmailthread::test_cloneId()
{
    QMailThread emptyThread;

    QMailThread thread1(emptyThread.id());
    QCOMPARE(thread1.id(), emptyThread.id());
    QCOMPARE(thread1.serverUid(), emptyThread.serverUid());
    QCOMPARE(thread1.messageCount(), emptyThread.messageCount());
}

void tst_qmailthread::test_threadKeys()
{
    QMailThreadKey threadkey;
    QCOMPARE(threadkey.isEmpty(), true);





    QMailAccount account;
    account.setName("Account");

    QVERIFY(QMailStore::instance()->addAccount(&account, 0));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(account.id().isValid());

    QMailFolder folder1;
    folder1.setPath("Folder 1");

    QVERIFY(!folder1.id().isValid());
    QVERIFY(QMailStore::instance()->addFolder(&folder1));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(folder1.id().isValid());

    QMailFolder folder2;
    folder2.setPath("Folder 2");

    QVERIFY(!folder2.id().isValid());
    QVERIFY(QMailStore::instance()->addFolder(&folder2));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(folder2.id().isValid());

    QMailMessage message1;
    message1.setParentAccountId(account.id());
    message1.setParentFolderId(folder1.id());
    message1.setMessageType(QMailMessage::Sms);
    message1.setServerUid("M0");
    message1.setSubject("Message 1");
    message1.setTo(QMailAddress("alice@example.org"));
    message1.setFrom(QMailAddress("bob@example.com"));
    message1.setBody(QMailMessageBody::fromData(QString("Hi"), QMailMessageContentType("text/plain"), QMailMessageBody::SevenBit));
    message1.setStatus(QMailMessage::Incoming, true);
    message1.setStatus(QMailMessage::Read, true);
    message1.setCustomField("question", "What is your dog's name?");
    message1.setCustomField("answer", "Fido");
    message1.setCustomField("temporary", "true");
    message1.setCustomField("tag", "Work");

    // Verify that addition is successful
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(!message1.id().isValid());
    QVERIFY(QMailStore::instance()->addMessage(&message1));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(message1.id().isValid());

    // Modify the content
    message1.setMessageType(QMailMessage::Sms);
    message1.setServerUid("M1");
    message1.setSubject("Not Message 1");
    message1.setFrom(QMailAddress("cornelius@example.com"));
    message1.setStatus(QMailMessage::Read, true);
    message1.setCustomField("answer", "Fido");
    message1.setCustomField("permanent", "true");
    message1.removeCustomField("temporary");

    // Verify that update is successful
    QCOMPARE(message1.dataModified(), true);
    QCOMPARE(message1.contentModified(), true);
    QVERIFY(QMailStore::instance()->updateMessage(&message1));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(message1.id().isValid());
    QCOMPARE(message1.dataModified(), false);
    QCOMPARE(message1.contentModified(), false);

    // Verify that retrieval yields matching result
    QMailMessage message2(message1.id());
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QCOMPARE(message2.id(), message1.id());
    QCOMPARE(message2.parentFolderId(), message1.parentFolderId());
    QCOMPARE(message2.messageType(), message1.messageType());
    QCOMPARE(message2.serverUid(), message1.serverUid());
    QCOMPARE(message2.subject(), message1.subject());
    QCOMPARE(message2.to(), message1.to());
    QCOMPARE(message2.from(), message1.from());
    QCOMPARE(message2.body().data(), message1.body().data());
    QCOMPARE((message2.status() | QMailMessage::UnloadedData), (message1.status() | QMailMessage::UnloadedData));
    QCOMPARE(message2.customFields(), message1.customFields());
    QCOMPARE(message2.customField("answer"), QString("Fido"));
    QCOMPARE(message2.customField("tag"), QString("Work"));
    QCOMPARE(message2.dataModified(), false);
    QCOMPARE(message2.contentModified(), false);

    // Add an additional message
    QMailMessage message3;
    message3.setParentAccountId(account.id());
    message3.setParentFolderId(folder1.id());
    message3.setMessageType(QMailMessage::Sms);
    message3.setServerUid("M2");
    message3.setSubject("Message 2");
    message3.setTo(QMailAddress("bob@example.com"));
    message3.setFrom(QMailAddress("alice@example.org"));
    message3.setBody(QMailMessageBody::fromData(QString("Hello"), QMailMessageContentType("text/plain"), QMailMessageBody::SevenBit));
    message3.setStatus(QMailMessage::Outgoing, true);
    message3.setStatus(QMailMessage::Sent, true);
    message3.setInResponseTo(message1.id());
    message3.setResponseType(QMailMessage::Reply);
    message3.setCustomField("question", "What is your dog's name?");
    message3.setCustomField("answer", "Fido");
    message3.setCustomField("tag", "Play");

    // Verify that addition is successful
    QVERIFY(!message3.id().isValid());
    QVERIFY(QMailStore::instance()->addMessage(&message3));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(message3.id().isValid());

    // Verify that retrieval yields matching result
    QMailMessage message4(message3.id());
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QCOMPARE(message4.id(), message3.id());
    QCOMPARE(message4.parentFolderId(), message3.parentFolderId());
    QCOMPARE(message4.messageType(), message3.messageType());
    QCOMPARE(message4.serverUid(), message3.serverUid());
    QCOMPARE(message4.subject(), message3.subject());
    QCOMPARE(message4.to(), message3.to());
    QCOMPARE(message4.from(), message3.from());
    QCOMPARE(message4.body().data(), message3.body().data());
    QCOMPARE((message4.status() | QMailMessage::UnloadedData), (message3.status() | QMailMessage::UnloadedData));
    QCOMPARE(message4.inResponseTo(), message3.inResponseTo());
    QCOMPARE(message4.responseType(), message3.responseType());
    QCOMPARE(message4.customFields(), message3.customFields());

    // Test that we can extract the custom field values
    QMailMessageKey tagKey(QMailMessageKey::customField("tag"));
    QCOMPARE(QMailStore::instance()->countMessages(tagKey), 2);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);

    QStringList tags;
    foreach (const QMailMessageMetaData &metaData, QMailStore::instance()->messagesMetaData(tagKey, QMailMessageKey::Custom, QMailStore::ReturnDistinct)) {
        QString value(metaData.customField("tag"));
        if (!value.isNull())
            tags.append(value);
    }
    // Note: results may not be in any order
    QCOMPARE(tags.count(), 2);
    foreach (const QString &value, QStringList() << "Play" << "Work")
        QVERIFY(tags.contains(value));


    QMailThreadKey key1;
    QMailMessageKey mailkey1(QMailMessageKey::subject("(Deleted)"));

    QCOMPARE(QMailStore::instance()->countThreads(), 4);
//    QCOMPARE(QMailThreadKey::includes(mailkey1), 0);
    QMailThreadKey key2 = QMailThreadKey::includes(mailkey1);
    QCOMPARE(QMailThreadKey::includes(mailkey1), key2);
    QCOMPARE(key1.isEmpty(), true);
    QCOMPARE(key1.isNegated(), false);
    QCOMPARE(key1.isNonMatching(), false);
    QMailThreadKey key3 = QMailThreadKey::id(mailthread.id());
    QMailThreadKey key4 = QMailThreadKey::id(QMailThreadIdList() << mailthread.id());
    QCOMPARE(key3, key4);

    QCOMPARE(key3, key3|key4);
    QCOMPARE(key3, key3&key4);

    QMailThreadKey key5 = QMailThreadKey::serverUid(QString("M1"));
    QMailThreadKey key6 = QMailThreadKey::serverUid(QStringList() << "M1");
    QMailThreadKey key7 = QMailThreadKey::serverUid(QStringList() << "M2");
    QMailThreadKey key8 = QMailThreadKey::serverUid(QStringList() << "M1" << "M2");

    QMailThreadKey key9 = QMailThreadKey::includes(QMailMessageIdList() << message1.id() << message2.id());
    QMailThreadKey key10 = QMailThreadKey::parentAccountId(accountId1);
    QMailThreadKey key11 = QMailThreadKey::parentAccountId(QMailAccountIdList() << accountId1 << accountId2);

    QCOMPARE(key5, key6);
    QVERIFY(key6 != key7);
    QVERIFY(key7 != key8);
    QVERIFY(key9 != key10);
    QVERIFY(key11 != key10);
    key10 |= key11;
    QVERIFY(key10 != key11);
    key11 &= key9;
    QVERIFY(key9 != key10);

    QMailAccountKey acckey;
    QMailThreadKey key12 = QMailThreadKey::parentAccountId(acckey);
    QCOMPARE(key12.isEmpty(), false);

    QMailThreadKey key13 = ~key12;
    QVERIFY(key13!= key12);

    QMailThreadKey key14 = key13.nonMatchingKey();
    QVERIFY(key14 != key13);

    QByteArray array;
    QDataStream stream(&array, QIODevice::ReadWrite);

//    key1.serialize<QDataStream>(stream);
//    key1.deserialize<QDataStream>(stream);

    QMailThreadSortKey skey1;
    QMailThreadSortKey skey2 = QMailThreadSortKey::serverUid();
    QVERIFY(skey1 != skey2);
    skey1 = skey2;
    QVERIFY(skey1 == skey2);
    QMailThreadSortKey skey3 = QMailThreadSortKey::id();
    QVERIFY(skey1 != skey3);
    skey1 &= skey3;
    QVERIFY(skey1 != skey3);
    QVERIFY((skey1 & skey2) != skey3);


    QDataStream stream1(&array, QIODevice::ReadWrite);
//    skey1.serialize<QDataStream>(stream);
//    skey1.deserialize<QDataStream>(stream1);

    QMailThreadSortKey skey4(skey1);
    QCOMPARE(skey1, skey4);
}


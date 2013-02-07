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
#include <QSqlQuery>
#include <QSignalSpy>
#include <qmailstore.h>
#include <QSettings>
#include <qmailnamespace.h>
#include "qmailstoreimplementation_p.h"

//TESTED_CLASS=QMailStore
//TESTED_FILES=src/libraries/qtopiamail/qmailstore.cpp

/*
    Unit test for QMailStore class.
    This class tests that QMailStore correctly handles addition, updates, removal, counting and
    querying of QMailMessages and QMailFolders.
*/
class tst_QMailStore : public QObject
{
    Q_OBJECT

public:
    tst_QMailStore();
    virtual ~tst_QMailStore();

private slots:
    void initTestCase();
    void cleanup();
    void cleanupTestCase();

    void addAccount();
    void addFolder();
    void addMessage();
    void addMessages();
    void addMessages2();
    void locking();
    void updateAccount();
    void updateFolder();
    void updateMessage();
    void updateMessages();
    void removeAccount();
    void removeFolder();
    void removeMessage();
    void remove1000Messages();
    void removeMessageWithInResponse();
    void message();
    void implementationbase();
};

QTEST_MAIN(tst_QMailStore)

#include "tst_qmailstore.moc"

#define CRLF "\015\012"

tst_QMailStore::tst_QMailStore()
{
}

tst_QMailStore::~tst_QMailStore()
{
}

void tst_QMailStore::initTestCase()
{
    QMailStore::instance()->clearContent();
}

void tst_QMailStore::cleanup()
{
    QMailStore::instance()->clearContent();
}

void tst_QMailStore::cleanupTestCase()
{
}

void tst_QMailStore::addAccount()
{
    QMailAccount account1;
    account1.setName("Account 1");
    account1.setFromAddress(QMailAddress("Account 1", "account1@example.org"));
    account1.setStatus(QMailAccount::SynchronizationEnabled, true);
    account1.setStatus(QMailAccount::Synchronized, false);
    account1.setStandardFolder(QMailFolder::SentFolder, QMailFolderId(333));
    account1.setStandardFolder(QMailFolder::TrashFolder, QMailFolderId(666));
    account1.setCustomField("question", "What is your dog's name?");
    account1.setCustomField("answer", "Fido");

    QMailAccountConfiguration config1;
    config1.addServiceConfiguration("imap4");
    if (QMailAccountConfiguration::ServiceConfiguration *svcCfg = &config1.serviceConfiguration("imap4")) {
        svcCfg->setValue("server", "mail.example.org");
        svcCfg->setValue("username", "account1");
    }
    config1.addServiceConfiguration("smtp");
    if (QMailAccountConfiguration::ServiceConfiguration *svcCfg = &config1.serviceConfiguration("smtp")) {
        svcCfg->setValue("server", "mail.example.org");
        svcCfg->setValue("username", "account1");
    }

    // Verify that invalid retrieval fails
    QMailAccount accountX(account1.id());
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::InvalidId);
    QVERIFY(!accountX.id().isValid());
    
    // Verify that addition is successful
    QCOMPARE(QMailStore::instance()->countAccounts(), 0);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(!account1.id().isValid());
    QVERIFY(QMailStore::instance()->addAccount(&account1, &config1));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(account1.id().isValid());
    QCOMPARE(QMailStore::instance()->countAccounts(), 1);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);

    // Verify that retrieval yields matching result
    QMailAccount account2(account1.id());
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QCOMPARE(account2.id(), account1.id());
    QCOMPARE(account2.name(), account1.name());
    QCOMPARE(account2.fromAddress(), account1.fromAddress());
    QCOMPARE(account2.status(), account1.status());
    QCOMPARE(account2.standardFolder(QMailFolder::InboxFolder), QMailFolderId());
    QCOMPARE(account2.standardFolder(QMailFolder::SentFolder), QMailFolderId(333));
    QCOMPARE(account2.standardFolder(QMailFolder::TrashFolder), QMailFolderId(666));
    QCOMPARE(account2.customFields(), account1.customFields());
    QCOMPARE(account2.customField("answer"), QString("Fido"));

    QMailAccountConfiguration config2(account1.id());
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QCOMPARE(config2.services(), config1.services());
    foreach (const QString &service, config2.services()) {
        if (QMailAccountConfiguration::ServiceConfiguration *svcCfg = &config2.serviceConfiguration(service)) {
            QCOMPARE(svcCfg->values(), config1.serviceConfiguration(service).values());
        } else QFAIL(qPrintable(QString("no config for %1!").arg(service)));
    }

    QCOMPARE(QMailStore::instance()->countAccounts(QMailAccountKey::id(account1.id())), 1);
    QCOMPARE(QMailStore::instance()->countAccounts(QMailAccountKey::id(account1.id(), QMailDataComparator::NotEqual)), 0);

    QCOMPARE(QMailStore::instance()->countAccounts(QMailAccountKey::name(account1.name())), 1);
    QCOMPARE(QMailStore::instance()->countAccounts(QMailAccountKey::name(account1.name().mid(3))), 0);
    QCOMPARE(QMailStore::instance()->countAccounts(QMailAccountKey::name(account1.name().mid(3), QMailDataComparator::Includes)), 1);

    // From Address field gets special handling
    QCOMPARE(QMailStore::instance()->countAccounts(QMailAccountKey::fromAddress(account1.fromAddress().address())), 1);
    QCOMPARE(QMailStore::instance()->countAccounts(~QMailAccountKey::fromAddress(account1.fromAddress().address())), 0);
    QCOMPARE(QMailStore::instance()->countAccounts(QMailAccountKey::fromAddress(account1.fromAddress().address(), QMailDataComparator::NotEqual)), 0);
    QCOMPARE(QMailStore::instance()->countAccounts(~QMailAccountKey::fromAddress(account1.fromAddress().address(), QMailDataComparator::NotEqual)), 1);
    QCOMPARE(QMailStore::instance()->countAccounts(QMailAccountKey::fromAddress(account1.fromAddress().address(), QMailDataComparator::Includes)), 1);
    QCOMPARE(QMailStore::instance()->countAccounts(~QMailAccountKey::fromAddress(account1.fromAddress().address(), QMailDataComparator::Includes)), 0);
    QCOMPARE(QMailStore::instance()->countAccounts(QMailAccountKey::fromAddress(account1.fromAddress().address(), QMailDataComparator::Excludes)), 0);
    QCOMPARE(QMailStore::instance()->countAccounts(~QMailAccountKey::fromAddress(account1.fromAddress().address(), QMailDataComparator::Excludes)), 1);

    QCOMPARE(QMailStore::instance()->countAccounts(QMailAccountKey::fromAddress(account1.fromAddress().address().mid(4), QMailDataComparator::Includes)), 1);
    QCOMPARE(QMailStore::instance()->countAccounts(~QMailAccountKey::fromAddress(account1.fromAddress().address().mid(4), QMailDataComparator::Includes)), 0);
    QCOMPARE(QMailStore::instance()->countAccounts(QMailAccountKey::fromAddress(account1.fromAddress().address().mid(4), QMailDataComparator::Excludes)), 0);
    QCOMPARE(QMailStore::instance()->countAccounts(~QMailAccountKey::fromAddress(account1.fromAddress().address().mid(4), QMailDataComparator::Excludes)), 1);

    QCOMPARE(QMailStore::instance()->countAccounts(QMailAccountKey::fromAddress(QString(), QMailDataComparator::Equal)), 0);
    QCOMPARE(QMailStore::instance()->countAccounts(~QMailAccountKey::fromAddress(QString(), QMailDataComparator::Equal)), 1);
    QCOMPARE(QMailStore::instance()->countAccounts(QMailAccountKey::fromAddress(QString(), QMailDataComparator::NotEqual)), 1);
    QCOMPARE(QMailStore::instance()->countAccounts(~QMailAccountKey::fromAddress(QString(), QMailDataComparator::NotEqual)), 0);

    QCOMPARE(QMailStore::instance()->countAccounts(QMailAccountKey::fromAddress(QString(), QMailDataComparator::Includes)), 1);
    QCOMPARE(QMailStore::instance()->countAccounts(~QMailAccountKey::fromAddress(QString(), QMailDataComparator::Includes)), 0);
    QCOMPARE(QMailStore::instance()->countAccounts(QMailAccountKey::fromAddress(QString(), QMailDataComparator::Excludes)), 0);
    QCOMPARE(QMailStore::instance()->countAccounts(~QMailAccountKey::fromAddress(QString(), QMailDataComparator::Excludes)), 1);

    // Test basic limit/offset
    QMailAccountKey key;
    QMailAccountSortKey sort;
    QMailAccountIdList accountIds(QMailStore::instance()->queryAccounts(key, sort));
    QCOMPARE(QMailStore::instance()->queryAccounts(key, sort, 1, 0), accountIds);
}

void tst_QMailStore::addFolder()
{
    //root folder

    QMailFolder newFolder("new folder 1");
    QVERIFY(QMailStore::instance()->addFolder(&newFolder));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(newFolder.id().isValid());

    QMailFolder retrieved1(newFolder.id());
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(retrieved1.id().isValid());
    QCOMPARE(retrieved1.path(), QString("new folder 1"));

    //root folder with no path

    QMailFolder newFolder2("");
    QVERIFY(QMailStore::instance()->addFolder(&newFolder2));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(newFolder2.id().isValid());

    QMailFolder retrieved2(newFolder2.id());
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(retrieved2.id().isValid());
    QCOMPARE(retrieved2.path(), QString(""));

    //root folder with valid parent

    QMailFolder newFolder3("new folder 3",newFolder2.id());
    QVERIFY(QMailStore::instance()->addFolder(&newFolder3));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(newFolder3.id().isValid());

    QMailFolder retrieved3(newFolder3.id());
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(retrieved3.id().isValid());
    QCOMPARE(retrieved3.path(), QString("new folder 3"));
    QCOMPARE(retrieved3.parentFolderId(), newFolder2.id());

    //delete root folder 

    QVERIFY(QMailStore::instance()->removeFolder(newFolder3.id()));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);

    QMailFolder retrieved4(newFolder3.id());
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::InvalidId);
    QVERIFY(!retrieved4.id().isValid());

    //root folder with invalid parent - fail

    QMailFolder newFolder4("new folder 4",newFolder3.id());
    QVERIFY(!QMailStore::instance()->addFolder(&newFolder4));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::ConstraintFailure);
    QVERIFY(!newFolder4.id().isValid());

    QMailFolder retrieved5(newFolder4.id());
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::InvalidId);
    QVERIFY(!retrieved5.id().isValid());

    //root folder with no path and invalid parent - fail

    QMailFolder newFolder5("new folder 5",newFolder3.id());
    QVERIFY(!QMailStore::instance()->addFolder(&newFolder5));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::ConstraintFailure);
    QVERIFY(!newFolder5.id().isValid());

    QMailFolder retrieved6(newFolder5.id());
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::InvalidId);
    QVERIFY(!retrieved6.id().isValid());

    // Test addition of status and custom fields
    QMailFolder folder6;
    folder6.setPath("Folder 6");
    folder6.setParentFolderId(newFolder.id());
    folder6.setStatus(QMailFolder::SynchronizationEnabled, true);
    folder6.setStatus(QMailFolder::Synchronized, false);
    folder6.setCustomField("question", "What is your dog's name?");
    folder6.setCustomField("answer", "Fido");

    // Verify that addition is successful
    QCOMPARE(QMailStore::instance()->countFolders(), 2);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(!folder6.id().isValid());
    QVERIFY(QMailStore::instance()->addFolder(&folder6));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(folder6.id().isValid());
    QCOMPARE(QMailStore::instance()->countFolders(), 3);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);

    // Verify that retrieval yields matching result
    QMailFolder folder7(folder6.id());
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QCOMPARE(folder7.id(), folder6.id());
    QCOMPARE(folder7.path(), folder6.path());
    QCOMPARE(folder7.status(), folder6.status());
    QCOMPARE(folder7.customFields(), folder6.customFields());
    QCOMPARE(folder7.customField("answer"), QString("Fido"));

    // Various tests that exposed a bug in QMailFolderKey::isNonMatching()...
    QCOMPARE(QMailStore::instance()->countFolders(), 3);
    QCOMPARE(QMailStore::instance()->countFolders(QMailFolderKey::id(folder6.id())), 1);
    QCOMPARE(QMailStore::instance()->countFolders(QMailFolderKey::path(folder6.path())), 1);
    QCOMPARE(QMailStore::instance()->countFolders(QMailFolderKey::parentFolderId(newFolder.id())), 1);
    QCOMPARE(QMailStore::instance()->countFolders(QMailFolderKey::parentFolderId(QMailFolderId())), 2);
    QCOMPARE(QMailStore::instance()->countFolders(QMailFolderKey::parentFolderId(QMailFolderId(), QMailDataComparator::NotEqual)), 1);
    QCOMPARE(QMailStore::instance()->countFolders(QMailFolderKey::parentFolderId(QMailFolderId(), QMailDataComparator::NotEqual) & QMailFolderKey::path(folder6.path())), 1);

    // Test basic limit/offset
    QMailFolderKey key;
    QMailFolderSortKey sort;
    QMailFolderIdList folderIds(QMailStore::instance()->queryFolders(key, sort));
    QCOMPARE(QMailStore::instance()->queryFolders(key, sort, 1, 0), folderIds.mid(0, 1));
    QCOMPARE(QMailStore::instance()->queryFolders(key, sort, 1, 1), folderIds.mid(1, 1));
    QCOMPARE(QMailStore::instance()->queryFolders(key, sort, 1, 2), folderIds.mid(2, 1));
    QCOMPARE(QMailStore::instance()->queryFolders(key, sort, 2, 0), folderIds.mid(0, 2));
    QCOMPARE(QMailStore::instance()->queryFolders(key, sort, 2, 1), folderIds.mid(1, 2));
    QCOMPARE(QMailStore::instance()->queryFolders(key, sort, 3, 0), folderIds);
}

void tst_QMailStore::addMessage()
{
    QSignalSpy spyMessagesAdded(QMailStore::instance(), SIGNAL(messagesAdded(QMailMessageIdList)));
    QSignalSpy spyMessageDataAdded(QMailStore::instance(), SIGNAL(messageDataAdded(QMailMessageMetaDataList)));

    QMailAccount account;
    account.setName("Account");

    QCOMPARE(QMailStore::instance()->countAccounts(), 0);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(!account.id().isValid());
    QVERIFY(QMailStore::instance()->addAccount(&account, 0));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(account.id().isValid());
    QCOMPARE(QMailStore::instance()->countAccounts(), 1);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);

    QMailFolder folder;
    folder.setPath("Folder");
    folder.setParentAccountId(account.id());

    QCOMPARE(QMailStore::instance()->countFolders(), 0);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(!folder.id().isValid());
    QVERIFY(QMailStore::instance()->addFolder(&folder));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(folder.id().isValid());
    QCOMPARE(QMailStore::instance()->countFolders(), 1);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);

    QMailMessage message1;
    message1.setServerUid("Just some message");
    message1.setParentAccountId(account.id());
    message1.setParentFolderId(folder.id());
    message1.setMessageType(QMailMessage::Sms);
    message1.setSubject("Message 1");
    message1.setTo(QMailAddress("alice@example.org"));
    message1.setFrom(QMailAddress("bob@example.com"));
    message1.setBody(QMailMessageBody::fromData(QString("Hi"), QMailMessageContentType("text/plain"), QMailMessageBody::SevenBit));
    message1.setStatus(QMailMessage::Incoming, true);
    message1.setStatus(QMailMessage::Read, true);
    message1.setCustomField("question", "What is your dog's name?");
    message1.setCustomField("answer", "Fido");

    // Verify that addition is successful
    QCOMPARE(message1.dataModified(), true);
    QCOMPARE(message1.contentModified(), true);
    QCOMPARE(QMailStore::instance()->countMessages(), 0);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(!message1.id().isValid());
    QVERIFY(QMailStore::instance()->addMessage(&message1));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(message1.id().isValid());
    QCOMPARE(message1.dataModified(), false);
    QCOMPARE(message1.contentModified(), false);
    QCOMPARE(QMailStore::instance()->countMessages(), 1);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);

    // Verify that retrieval yields matching result
    QMailMessage message2(message1.id());
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QCOMPARE(message2.id(), message1.id());
    QCOMPARE(message2.serverUid(), message1.serverUid());
    QCOMPARE(message2.parentAccountId(), message1.parentAccountId());
    QCOMPARE(message2.parentFolderId(), message1.parentFolderId());
    QCOMPARE(message2.messageType(), message1.messageType());
    QCOMPARE(message2.subject(), message1.subject());
    QCOMPARE(message2.to(), message1.to());
    QCOMPARE(message2.from(), message1.from());
    QCOMPARE(message2.body().data(), message1.body().data());
    QCOMPARE((message2.status() | QMailMessage::UnloadedData), (message1.status() | QMailMessage::UnloadedData));
    QCOMPARE(message2.customFields(), message1.customFields());
    QCOMPARE(message2.customField("answer"), QString("Fido"));
    QCOMPARE(message2.dataModified(), false);
    QCOMPARE(message2.contentModified(), false);

    // Verify that UID/Account retrieval yields matching result
    QMailMessage message3(message1.serverUid(), message1.parentAccountId());
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QCOMPARE(message3.id(), message1.id());
    QCOMPARE(message3.serverUid(), message1.serverUid());
    QCOMPARE(message3.parentAccountId(), message1.parentAccountId());
    QCOMPARE(message3.parentFolderId(), message1.parentFolderId());
    QCOMPARE(message3.messageType(), message1.messageType());
    QCOMPARE(message3.subject(), message1.subject());
    QCOMPARE(message3.to(), message1.to());
    QCOMPARE(message3.from(), message1.from());
    QCOMPARE(message3.body().data(), message1.body().data());
    QCOMPARE((message3.status() | QMailMessage::UnloadedData), (message1.status() | QMailMessage::UnloadedData));
    QCOMPARE(message3.customFields(), message1.customFields());
    QCOMPARE(message3.customField("answer"), QString("Fido"));
    QCOMPARE(message3.dataModified(), false);
    QCOMPARE(message3.contentModified(), false);

    //Verify that the signals are only emitted once
    QCOMPARE(spyMessagesAdded.count(), 1);
    QCOMPARE(spyMessageDataAdded.count(), 1);
}

void tst_QMailStore::addMessages()
{
    QSignalSpy spyMessagesAdded(QMailStore::instance(), SIGNAL(messagesAdded(QMailMessageIdList)));
    QSignalSpy spyMessageDataAdded(QMailStore::instance(), SIGNAL(messageDataAdded(QMailMessageMetaDataList)));

    QMailAccount account;
    account.setName("Account");

    QCOMPARE(QMailStore::instance()->countAccounts(), 0);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(!account.id().isValid());
    QVERIFY(QMailStore::instance()->addAccount(&account, 0));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(account.id().isValid());
    QCOMPARE(QMailStore::instance()->countAccounts(), 1);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);

    QMailFolder folder;
    folder.setPath("Folder");
    folder.setParentAccountId(account.id());

    QCOMPARE(QMailStore::instance()->countFolders(), 0);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(!folder.id().isValid());
    QVERIFY(QMailStore::instance()->addFolder(&folder));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(folder.id().isValid());
    QCOMPARE(QMailStore::instance()->countFolders(), 1);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);

    QList<QMailMessage> messages;
    QList<QMailMessage*> messageAddresses;
    for (int i = 1; i <= 10; ++i) {
        QMailMessage message;
        message.setParentAccountId(account.id());
        message.setParentFolderId(folder.id());
        message.setMessageType(QMailMessage::Sms);
        message.setSubject(QString("Message %1").arg(i));
        message.setBody(QMailMessageBody::fromData(QString("Hi #%1").arg(i), QMailMessageContentType("text/plain"), QMailMessageBody::SevenBit));

        messages.append(message);
        messageAddresses.append(&messages.last());
    }

    // Verify that addition is successful
    QCOMPARE(QMailStore::instance()->countMessages(), 0);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(!messages.first().id().isValid());
    QVERIFY(!messages.last().id().isValid());
    QVERIFY(QMailStore::instance()->addMessages(messageAddresses));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(messages.first().id().isValid());
    QVERIFY(messages.last().id().isValid());
    QCOMPARE(QMailStore::instance()->countMessages(), 10);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);

    // Verify that retrieval yields matching result
    for (int i = 1; i <= 10; ++i) {
        QMailMessage message(messages.at(i - 1).id());
        QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
        QCOMPARE(message.subject(), QString("Message %1").arg(i));
        QCOMPARE(message.body().data(), QString("Hi #%1").arg(i));
    }

    // Test basic limit/offset
    QMailMessageKey key;
    QMailMessageSortKey sort;
    QMailMessageIdList messageIds(QMailStore::instance()->queryMessages(key, sort));
    QCOMPARE(QMailStore::instance()->queryMessages(key, sort, 4, 0), messageIds.mid(0, 4));
    QCOMPARE(QMailStore::instance()->queryMessages(key, sort, 4, 3), messageIds.mid(3, 4));
    QCOMPARE(QMailStore::instance()->queryMessages(key, sort, 4, 6), messageIds.mid(6, 4));
    QCOMPARE(QMailStore::instance()->queryMessages(key, sort, 9, 0), messageIds.mid(0, 9));
    QCOMPARE(QMailStore::instance()->queryMessages(key, sort, 9, 1), messageIds.mid(1, 9));
    QCOMPARE(QMailStore::instance()->queryMessages(key, sort, 10, 0), messageIds);

    //Verify that the signals are only emitted once
    QCOMPARE(spyMessagesAdded.count(), 1);
    QCOMPARE(spyMessageDataAdded.count(), 1);
}

void tst_QMailStore::addMessages2()
{
    QMailAccount account;
    account.setName("Account");

    QCOMPARE(QMailStore::instance()->countAccounts(), 0);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(!account.id().isValid());
    QVERIFY(QMailStore::instance()->addAccount(&account, 0));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(account.id().isValid());
    QCOMPARE(QMailStore::instance()->countAccounts(), 1);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);

    QMailFolder folder;
    folder.setPath("Folder");
    folder.setParentAccountId(account.id());

    QCOMPARE(QMailStore::instance()->countFolders(), 0);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(!folder.id().isValid());
    QVERIFY(QMailStore::instance()->addFolder(&folder));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(folder.id().isValid());
    QCOMPARE(QMailStore::instance()->countFolders(), 1);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);

    QList<QMailMessage> messages;
    QList<QMailMessage*> messageAddresses;
    for (int i = 1; i <= 10; ++i) {
        QMailMessage message;
        message.setParentAccountId(account.id());
        message.setParentFolderId(folder.id());
        message.setMessageType(QMailMessage::Sms);
        message.setSubject(QString("Message %1").arg(i));
        message.setBody(QMailMessageBody::fromData(QString("Hi #%1").arg(i), QMailMessageContentType("text/plain"), QMailMessageBody::SevenBit));

        messages.append(message);
        messageAddresses.append(&messages.last());
    }

    // Verify that addition is successful
    QCOMPARE(QMailStore::instance()->countMessages(), 0);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(!messages.first().id().isValid());
    QVERIFY(!messages.last().id().isValid());
    QVERIFY(QMailStore::instance()->addMessages(messageAddresses));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(messages.first().id().isValid());
    QVERIFY(messages.last().id().isValid());
    QCOMPARE(QMailStore::instance()->countMessages(), 10);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);

    // Verify that retrieval yields matching result
    for (int i = 1; i <= 10; ++i) {
        QMailMessage message(messages.at(i - 1).id());
        QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
        QCOMPARE(message.subject(), QString("Message %1").arg(i));
        QCOMPARE(message.body().data(), QString("Hi #%1").arg(i));
    }

    // Test basic limit/offset
    QMailMessageKey key;
    QMailMessageSortKey sort;
    QMailMessageIdList messageIds(QMailStore::instance()->queryMessages(key, sort));
    QCOMPARE(QMailStore::instance()->queryMessages(key, sort, 4, 0), messageIds.mid(0, 4));
    QCOMPARE(QMailStore::instance()->queryMessages(key, sort, 4, 3), messageIds.mid(3, 4));
    QCOMPARE(QMailStore::instance()->queryMessages(key, sort, 4, 6), messageIds.mid(6, 4));
    QCOMPARE(QMailStore::instance()->queryMessages(key, sort, 9, 0), messageIds.mid(0, 9));
    QCOMPARE(QMailStore::instance()->queryMessages(key, sort, 9, 1), messageIds.mid(1, 9));
    QCOMPARE(QMailStore::instance()->queryMessages(key, sort, 10, 0), messageIds);
}

void tst_QMailStore::locking()
{
    QMailAccount accnt;
    accnt.setName("locking account");
    accnt.setFromAddress(QMailAddress("Lochlan", "locking@locker.org"));

    QMailAccountConfiguration config1;
    config1.addServiceConfiguration("imap4");
    if (QMailAccountConfiguration::ServiceConfiguration *svcCfg = &config1.serviceConfiguration("imap4")) {
        svcCfg->setValue("server", "mail.example.org");
        svcCfg->setValue("username", "account1");
    }

    // Test that locking stop modifying
    QMailStore::instance()->lock();

    // Make sure we can actually modify after unlocking
    QMailStore::instance()->unlock();
    int added = QMailStore::instance()->addAccount(&accnt, &config1);
    QVERIFY(added == true);

    // Make sure we can read when locked
    QMailStore::instance()->lock();
    QCOMPARE(QMailStore::instance()->account(accnt.id()).id(), accnt.id());

    // Test recursive locking
    QMailStore::instance()->lock();
    QCOMPARE(QMailStore::instance()->account(accnt.id()).id(), accnt.id()); //still can read?

    QMailStore::instance()->unlock();
    //..should not be able to write here..
    QMailStore::instance()->unlock();
    // Now we should be able to modify.
    int removed = QMailStore::instance()->removeAccount(accnt.id());
    QVERIFY(removed == true);
}

void tst_QMailStore::updateAccount()
{
    QMailFolder folder("new folder 1");
    QVERIFY(QMailStore::instance()->addFolder(&folder));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);

    QMailAccount account1;
    account1.setName("Account 1");
    account1.setFromAddress(QMailAddress("Account 1", "account1@example.org"));
    account1.setStatus(QMailAccount::SynchronizationEnabled, true);
    account1.setStatus(QMailAccount::Synchronized, false);
    account1.setStandardFolder(QMailFolder::SentFolder, folder.id());
    account1.setStandardFolder(QMailFolder::TrashFolder, folder.id());
    account1.setCustomField("question", "What is your dog's name?");
    account1.setCustomField("answer", "Fido");
    account1.setCustomField("temporary", "true");

    QMailAccountConfiguration config1;
    config1.addServiceConfiguration("imap4");
    if (QMailAccountConfiguration::ServiceConfiguration *svcCfg = &config1.serviceConfiguration("imap4")) {
        svcCfg->setValue("server", "mail.example.org");
        svcCfg->setValue("username", "account1");
    }
    config1.addServiceConfiguration("smtp");
    if (QMailAccountConfiguration::ServiceConfiguration *svcCfg = &config1.serviceConfiguration("smtp")) {
        svcCfg->setValue("server", "mail.example.org");
        svcCfg->setValue("username", "account1");
    }

    // Verify that addition is successful
    QCOMPARE(QMailStore::instance()->countAccounts(), 0);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(!account1.id().isValid());
    QVERIFY(QMailStore::instance()->addAccount(&account1, &config1));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(account1.id().isValid());
    QCOMPARE(QMailStore::instance()->countAccounts(), 1);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);

    QCOMPARE(account1.standardFolders().count(), 2);
    QCOMPARE(account1.standardFolder(QMailFolder::InboxFolder), QMailFolderId());
    QCOMPARE(account1.standardFolder(QMailFolder::SentFolder), folder.id());
    QCOMPARE(account1.standardFolder(QMailFolder::TrashFolder), folder.id());

    // Modify the content
    account1.setName("Not Account 1");
    account1.setFromAddress(QMailAddress("Not Account 1", "account2@somewhere.test"));
    account1.setStatus(QMailAccount::SynchronizationEnabled, false);
    account1.setStatus(QMailAccount::Synchronized, true);
    account1.setStandardFolder(QMailFolder::SentFolder, QMailFolderId());
    account1.setCustomField("answer", "Fido");
    account1.setCustomField("permanent", "true");
    account1.removeCustomField("temporary");

    config1.removeServiceConfiguration("imap4");
    if (QMailAccountConfiguration::ServiceConfiguration *svcCfg = &config1.serviceConfiguration("smtp")) {
        svcCfg->setValue("server", "smtp.example.org");
        svcCfg->setValue("login", "account1");
        svcCfg->removeValue("username");
    }
    config1.addServiceConfiguration("pop3");
    if (QMailAccountConfiguration::ServiceConfiguration *svcCfg = &config1.serviceConfiguration("pop3")) {
        svcCfg->setValue("server", "pop.example.org");
        svcCfg->setValue("username", "Account -1");
    }

    // Verify that update is successful
    QVERIFY(QMailStore::instance()->updateAccount(&account1, &config1));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(account1.id().isValid());
    QCOMPARE(QMailStore::instance()->countAccounts(), 1);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);

    // Verify that retrieval yields matching result
    QMailAccount account2(account1.id());
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QCOMPARE(account2.id(), account1.id());
    QCOMPARE(account2.name(), account1.name());
    QCOMPARE(account2.fromAddress(), account1.fromAddress());
    QCOMPARE(account2.status(), account1.status());
    QCOMPARE(account2.standardFolders(), account1.standardFolders());
    QCOMPARE(account2.standardFolders().count(), 1);
    QCOMPARE(account1.standardFolder(QMailFolder::InboxFolder), QMailFolderId());
    QCOMPARE(account2.standardFolder(QMailFolder::SentFolder), QMailFolderId());
    QCOMPARE(account2.standardFolder(QMailFolder::TrashFolder), folder.id());
    QCOMPARE(account2.customFields(), account1.customFields());
    QCOMPARE(account2.customField("answer"), QString("Fido"));
    QVERIFY(account2.customField("temporary").isNull());

    QMailAccountConfiguration config2(account1.id());
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QCOMPARE(config2.services(), config1.services());
    foreach (const QString &service, config2.services()) {
        if (QMailAccountConfiguration::ServiceConfiguration *svcCfg = &config2.serviceConfiguration(service)) {
            QCOMPARE(svcCfg->values(), config1.serviceConfiguration(service).values());
        } else QFAIL(qPrintable(QString("no config for %1!").arg(service)));
    }
}

void tst_QMailStore::updateFolder()
{
    //update an existing folder with a new path

    QMailFolder newFolder("new folder 1");
    QVERIFY(QMailStore::instance()->addFolder(&newFolder));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);

    newFolder.setPath("newer folder!!");

    QVERIFY(QMailStore::instance()->updateFolder(&newFolder));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);

    QMailFolder retrieved1(newFolder.id());
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(retrieved1.id().isValid());
    QCOMPARE(retrieved1.path(), QString("newer folder!!"));

    //update existing folder with empty path

    newFolder.setPath("");

    QVERIFY(QMailStore::instance()->updateFolder(&newFolder));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);

    QMailFolder retrieved2(newFolder.id());
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(retrieved2.id().isValid());
    QCOMPARE(retrieved2.path(), QString(""));

    //update a folder that does not exist in the db - fail

    QMailFolder bogusFolder("does not exist");
    QVERIFY(!QMailStore::instance()->updateFolder(&bogusFolder)); 
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::ConstraintFailure);
    QVERIFY(!bogusFolder.id().isValid());

    QMailFolder retrieved3(bogusFolder.id());
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::InvalidId);

    //update a folder with an invalid parent - fail

    QMailFolder newFolder2("new folder 2");
    QVERIFY(QMailStore::instance()->addFolder(&newFolder2));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QMailFolder newFolder3("new folder 3",newFolder2.id());
    QVERIFY(QMailStore::instance()->addFolder(&newFolder3));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QMailFolder newFolder4("new folder 4");
    QVERIFY(QMailStore::instance()->addFolder(&newFolder4));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);

    QVERIFY(QMailStore::instance()->removeFolder(newFolder3.id()));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    
    newFolder4.setParentFolderId(newFolder3.id());
    QVERIFY(!QMailStore::instance()->updateFolder(&newFolder4));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::ConstraintFailure);

    QMailFolder retrieved4(newFolder4.id());
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(retrieved4.id().isValid());
    QCOMPARE(retrieved4.path(), QString("new folder 4"));
    QCOMPARE(retrieved4.parentFolderId(), QMailFolderId());

    //update a folder to valid parent

    newFolder4.setParentFolderId(newFolder2.id());
    QVERIFY(QMailStore::instance()->updateFolder(&newFolder4));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);

    QMailFolder retrieved5(newFolder4.id());
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(retrieved5.id().isValid());
    QCOMPARE(retrieved5.path(), QString("new folder 4"));
    QCOMPARE(retrieved5.parentFolderId(), newFolder2.id());

    //update a folder to be a root folder

    newFolder4.setParentFolderId(QMailFolderId());
    QVERIFY(QMailStore::instance()->updateFolder(&newFolder4));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);

    QMailFolder retrieved6(newFolder4.id());
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(retrieved6.id().isValid());
    QCOMPARE(retrieved6.path(), QString("new folder 4"));
    QCOMPARE(retrieved6.parentFolderId(), QMailFolderId());

    //update a folder with a reference to itself - fail

    newFolder2.setParentFolderId(newFolder2.id());
    QVERIFY(!QMailStore::instance()->updateFolder(&newFolder2));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::ConstraintFailure);

    // Test update of status and custom fields
    QMailFolder folder5;
    folder5.setPath("Folder 5");
    folder5.setStatus(QMailFolder::SynchronizationEnabled, true);
    folder5.setStatus(QMailFolder::Synchronized, false);
    folder5.setCustomField("question", "What is your dog's name?");
    folder5.setCustomField("answer", "Fido");
    folder5.setCustomField("temporary", "true");

    // Verify that addition is successful
    QCOMPARE(QMailStore::instance()->countFolders(), 3);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(!folder5.id().isValid());
    QVERIFY(QMailStore::instance()->addFolder(&folder5));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(folder5.id().isValid());
    QCOMPARE(QMailStore::instance()->countFolders(), 4);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);

    folder5.setPath("Not Folder 5");
    folder5.setStatus(QMailFolder::SynchronizationEnabled, false);
    folder5.setStatus(QMailFolder::Synchronized, true);
    folder5.setCustomField("answer", "Fido");
    folder5.setCustomField("permanent", "true");
    folder5.removeCustomField("temporary");

    // Verify that update is successful
    QVERIFY(QMailStore::instance()->updateFolder(&folder5));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(folder5.id().isValid());
    QCOMPARE(QMailStore::instance()->countFolders(), 4);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);

    // Verify that retrieval yields matching result
    QMailFolder folder6(folder5.id());
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QCOMPARE(folder6.id(), folder5.id());
    QCOMPARE(folder6.path(), folder5.path());
    QCOMPARE(folder6.status(), folder5.status());
    QCOMPARE(folder6.customFields(), folder5.customFields());
    QCOMPARE(folder6.customField("answer"), QString("Fido"));
    QVERIFY(folder6.customField("temporary").isNull());
}

void tst_QMailStore::updateMessage()
{
    QSignalSpy spyMessagesUpdated(QMailStore::instance(), SIGNAL(messagesUpdated(QMailMessageIdList)));
    QSignalSpy spyMessagesDataUpdated(QMailStore::instance(), SIGNAL(messageDataUpdated(QMailMessageMetaDataList)));

    QMailAccount account;
    account.setName("Account");

    QCOMPARE(QMailStore::instance()->countAccounts(), 0);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(!account.id().isValid());
    QVERIFY(QMailStore::instance()->addAccount(&account, 0));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(account.id().isValid());
    QCOMPARE(QMailStore::instance()->countAccounts(), 1);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);

    QMailFolder folder1;
    folder1.setPath("Folder 1");

    QCOMPARE(QMailStore::instance()->countFolders(), 0);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(!folder1.id().isValid());
    QVERIFY(QMailStore::instance()->addFolder(&folder1));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(folder1.id().isValid());
    QCOMPARE(QMailStore::instance()->countFolders(), 1);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);

    QMailFolder folder2;
    folder2.setPath("Folder 2");

    QCOMPARE(QMailStore::instance()->countFolders(), 1);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(!folder2.id().isValid());
    QVERIFY(QMailStore::instance()->addFolder(&folder2));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(folder2.id().isValid());
    QCOMPARE(QMailStore::instance()->countFolders(), 2);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);

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
    QCOMPARE(QMailStore::instance()->countMessages(), 0);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(!message1.id().isValid());
    QVERIFY(QMailStore::instance()->addMessage(&message1));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(message1.id().isValid());
    QCOMPARE(QMailStore::instance()->countMessages(), 1);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);

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
    QCOMPARE(QMailStore::instance()->countMessages(), 1);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);

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

    //Verify that the signals are only emitted once
    QCOMPARE(spyMessagesUpdated.count(), 1);
    QCOMPARE(spyMessagesDataUpdated.count(), 1);

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
    QCOMPARE(QMailStore::instance()->countMessages(), 1);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(!message3.id().isValid());
    QVERIFY(QMailStore::instance()->addMessage(&message3));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(message3.id().isValid());
    QCOMPARE(QMailStore::instance()->countMessages(), 2);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    
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

    // Verify mass update
    QMailMessageKey key1(QMailMessageKey::subject("(Deleted)"));
    QMailMessageKey key2(QMailMessageKey::status(QMailMessage::Read, QMailDataComparator::Includes));
    QMailMessageKey key3(QMailMessageKey::status(QMailMessage::ContentAvailable, QMailDataComparator::Includes));
    QMailMessageKey key4(QMailMessageKey::customField("question"));
    QMailMessageKey key5(QMailMessageKey::customField("answer", "Fido"));
    QMailMessageKey key6(QMailMessageKey::customField("bicycle"));
    QMailMessageKey key7(QMailMessageKey::parentFolderId(folder1.id()));
    QMailMessageKey key8(QMailMessageKey::inResponseTo(message1.id()));
    QMailMessageKey key9(QMailMessageKey::inResponseTo(QMailMessageId()));
    QMailMessageKey key10(QMailMessageKey::responseType(QMailMessage::Reply));
    QMailMessageKey key11(QMailMessageKey::responseType(QMailMessage::Forward));
    QMailMessageKey key12(QMailMessageKey::serverUid(QString()));
    QMailMessageKey key13(QMailMessageKey::serverUid(QString("M1")));
    QMailMessageKey key14(QMailMessageKey::serverUid(QStringList() << "M2"));
    QMailMessageKey key15(QMailMessageKey::serverUid(QStringList() << "M1" << "M2"));

    QCOMPARE(QMailStore::instance()->queryMessages(key1), QMailMessageIdList());
    QCOMPARE(QMailStore::instance()->queryMessages(key2), QMailMessageIdList() << message1.id());
    QCOMPARE(QMailStore::instance()->queryMessages(key3), QMailMessageIdList());
    QCOMPARE(QMailStore::instance()->queryMessages(key4), QMailMessageIdList() << message1.id() << message3.id());
    QCOMPARE(QMailStore::instance()->queryMessages(key5), QMailMessageIdList() << message1.id() << message3.id());
    QCOMPARE(QMailStore::instance()->queryMessages(key6), QMailMessageIdList());
    QCOMPARE(QMailStore::instance()->queryMessages(key7), QMailMessageIdList() << message1.id() << message3.id());
    QCOMPARE(QMailStore::instance()->queryMessages(key8), QMailMessageIdList() << message3.id());
    QCOMPARE(QMailStore::instance()->queryMessages(key9), QMailMessageIdList() << message1.id());
    QCOMPARE(QMailStore::instance()->queryMessages(key10), QMailMessageIdList() << message3.id());
    QCOMPARE(QMailStore::instance()->queryMessages(key11), QMailMessageIdList());
    QCOMPARE(QMailStore::instance()->queryMessages(key12), QMailMessageIdList());
    QCOMPARE(QMailStore::instance()->queryMessages(key13), QMailMessageIdList() << message1.id());
    QCOMPARE(QMailStore::instance()->queryMessages(key14), QMailMessageIdList() << message3.id());
    QCOMPARE(QMailStore::instance()->queryMessages(key15), QMailMessageIdList() << message1.id() << message3.id());

    QMailMessageMetaData data;
    data.setSubject("(Deleted)");

    QVERIFY(QMailStore::instance()->updateMessagesMetaData(QMailMessageKey(), QMailMessageKey::Subject, data));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QCOMPARE(QMailStore::instance()->countMessages(key1), 2);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);

    QVERIFY(QMailStore::instance()->updateMessagesMetaData(QMailMessageKey(), QMailMessage::Read, false));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QCOMPARE(QMailStore::instance()->countMessages(key2), 0);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);

    QVERIFY(QMailStore::instance()->updateMessagesMetaData(QMailMessageKey(), QMailMessage::ContentAvailable, true));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QCOMPARE(QMailStore::instance()->countMessages(key3), 2);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);

    data.setCustomField("answer", "Fido");
    data.setCustomField("bicycle", "fish");

    QVERIFY(QMailStore::instance()->updateMessagesMetaData(QMailMessageKey(), QMailMessageKey::Custom, data));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QCOMPARE(QMailStore::instance()->countMessages(key4), 0);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QCOMPARE(QMailStore::instance()->countMessages(key5), 2);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QCOMPARE(QMailStore::instance()->countMessages(key6), 2);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    
    data.setParentFolderId(folder2.id());

    QVERIFY(QMailStore::instance()->updateMessagesMetaData(QMailMessageKey(), QMailMessageKey::ParentFolderId, data));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QCOMPARE(QMailStore::instance()->countMessages(key7), 0);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);

    // Temporary location for these tests:
    QCOMPARE(QMailStore::instance()->countMessages(QMailMessageKey::contentScheme("qmfstoragemanager")), 2);
    QCOMPARE(QMailStore::instance()->countMessages(~QMailMessageKey::contentScheme("qmfstoragemanager")), 0);

    QCOMPARE(QMailStore::instance()->countMessages(QMailMessageKey::contentScheme("qmfstoragemanager", QMailDataComparator::NotEqual)), 0);
    QCOMPARE(QMailStore::instance()->countMessages(~QMailMessageKey::contentScheme("qmfstoragemanager", QMailDataComparator::NotEqual)), 2);

    QCOMPARE(QMailStore::instance()->countMessages(QMailMessageKey::contentScheme("qmfstoragemanager", QMailDataComparator::Includes)), 2);
    QCOMPARE(QMailStore::instance()->countMessages(~QMailMessageKey::contentScheme("qmfstoragemanager", QMailDataComparator::Includes)), 0);

    QCOMPARE(QMailStore::instance()->countMessages(QMailMessageKey::contentScheme("qmf", QMailDataComparator::Includes)), 2);
    QCOMPARE(QMailStore::instance()->countMessages(~QMailMessageKey::contentScheme("qmf", QMailDataComparator::Includes)), 0);

    QCOMPARE(QMailStore::instance()->countMessages(QMailMessageKey::contentScheme("qmf", QMailDataComparator::Excludes)), 0);
    QCOMPARE(QMailStore::instance()->countMessages(~QMailMessageKey::contentScheme("qmf", QMailDataComparator::Excludes)), 2);

    QCOMPARE(QMailStore::instance()->countMessages(QMailMessageKey::contentScheme("qtexended", QMailDataComparator::Includes)), 0);
    QCOMPARE(QMailStore::instance()->countMessages(~QMailMessageKey::contentScheme("qtexended", QMailDataComparator::Includes)), 2);

    QCOMPARE(QMailStore::instance()->countMessages(QMailMessageKey::contentScheme("qtexended", QMailDataComparator::Excludes)), 2);
    QCOMPARE(QMailStore::instance()->countMessages(~QMailMessageKey::contentScheme("qtexended", QMailDataComparator::Excludes)), 0);

    QCOMPARE(QMailStore::instance()->queryMessages(QMailMessageKey::contentIdentifier(message1.contentIdentifier(), QMailDataComparator::Equal)), QMailMessageIdList() << message1.id());
    QCOMPARE(QMailStore::instance()->queryMessages(QMailMessageKey::contentIdentifier(message1.contentIdentifier(), QMailDataComparator::NotEqual)), QMailMessageIdList() << message3.id());
    QCOMPARE(QMailStore::instance()->queryMessages(QMailMessageKey::contentIdentifier(message1.contentIdentifier().remove(0, 1), QMailDataComparator::Includes)), QMailMessageIdList() << message1.id());
    QCOMPARE(QMailStore::instance()->queryMessages(QMailMessageKey::contentIdentifier(message1.contentIdentifier().remove(0, 1), QMailDataComparator::Excludes)), QMailMessageIdList() << message3.id());

    QCOMPARE(QMailStore::instance()->queryMessages(~QMailMessageKey::contentIdentifier(message1.contentIdentifier(), QMailDataComparator::Equal)), QMailMessageIdList() << message3.id());
    QCOMPARE(QMailStore::instance()->queryMessages(~QMailMessageKey::contentIdentifier(message1.contentIdentifier(), QMailDataComparator::NotEqual)), QMailMessageIdList() << message1.id());
    QCOMPARE(QMailStore::instance()->queryMessages(~QMailMessageKey::contentIdentifier(message1.contentIdentifier().remove(0, 1), QMailDataComparator::Includes)), QMailMessageIdList() << message3.id());
    QCOMPARE(QMailStore::instance()->queryMessages(~QMailMessageKey::contentIdentifier(message1.contentIdentifier().remove(0, 1), QMailDataComparator::Excludes)), QMailMessageIdList() << message1.id());

    QCOMPARE(QMailStore::instance()->queryMessages(QMailMessageKey::contentScheme("qmfstoragemanager") & QMailMessageKey::contentIdentifier(message1.contentIdentifier())), QMailMessageIdList() << message1.id());
    QCOMPARE(QMailStore::instance()->queryMessages(QMailMessageKey::contentScheme("qmfstoragemanager") & ~QMailMessageKey::contentIdentifier(message1.contentIdentifier())), QMailMessageIdList() << message3.id());
}

void tst_QMailStore::updateMessages()
{
    QSignalSpy spyMessagesUpdated(QMailStore::instance(), SIGNAL(messagesUpdated(QMailMessageIdList)));
    QSignalSpy spyMessagesDataUpdated(QMailStore::instance(), SIGNAL(messageDataUpdated(QMailMessageMetaDataList)));

    QMailAccount account;
    account.setName("Account");

    QCOMPARE(QMailStore::instance()->countAccounts(), 0);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(!account.id().isValid());
    QVERIFY(QMailStore::instance()->addAccount(&account, 0));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(account.id().isValid());
    QCOMPARE(QMailStore::instance()->countAccounts(), 1);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);

    QMailFolder folder;
    folder.setPath("Folder");
    folder.setParentAccountId(account.id());

    QCOMPARE(QMailStore::instance()->countFolders(), 0);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(!folder.id().isValid());
    QVERIFY(QMailStore::instance()->addFolder(&folder));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(folder.id().isValid());
    QCOMPARE(QMailStore::instance()->countFolders(), 1);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);

    QList<QMailMessage> messages;
    QList<QMailMessage*> messageAddresses;
    for (int i = 1; i <= 10; ++i) {
        QMailMessage message;
        message.setParentAccountId(account.id());
        message.setParentFolderId(folder.id());
        message.setMessageType(QMailMessage::Sms);
        message.setSubject(QString("Message %1").arg(i));
        message.setBody(QMailMessageBody::fromData(QString("Hi #%1").arg(i), QMailMessageContentType("text/plain"), QMailMessageBody::SevenBit));

        messages.append(message);
        messageAddresses.append(&messages.last());
    }

    // Verify that addition is successful
    QCOMPARE(QMailStore::instance()->countMessages(), 0);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(!messages.first().id().isValid());
    QVERIFY(!messages.last().id().isValid());
    QVERIFY(QMailStore::instance()->addMessages(messageAddresses));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(messages.first().id().isValid());
    QVERIFY(messages.last().id().isValid());
    QCOMPARE(QMailStore::instance()->countMessages(), 10);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);

    // Change the properties of each message
    for (int i = 1; i <= 10; ++i) {
        QMailMessage *message(messageAddresses.at(i - 1));
        message->setSubject(QString("Message %1").arg(i + 100));
        message->setBody(QMailMessageBody::fromData(QString("Hi #%1").arg(i + 100), QMailMessageContentType("text/plain"), QMailMessageBody::SevenBit));
    }

    // Verify that update is successful
    QCOMPARE(QMailStore::instance()->countMessages(), 10);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(QMailStore::instance()->updateMessages(messageAddresses));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QCOMPARE(QMailStore::instance()->countMessages(), 10);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);

    // Verify that retrieval yields matching result
    for (int i = 1; i <= 10; ++i) {
        QMailMessage message(messages.at(i - 1).id());
        QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
        QCOMPARE(message.subject(), QString("Message %1").arg(i + 100));
        QCOMPARE(message.body().data(), QString("Hi #%1").arg(i + 100));
    }

    //Verify that the signals are only emitted once
    QCOMPARE(spyMessagesUpdated.count(), 1);
    QCOMPARE(spyMessagesDataUpdated.count(), 1);
}

void tst_QMailStore::removeAccount()
{
    QMailAccount account1;
    account1.setName("Account 1");
    account1.setFromAddress(QMailAddress("Account 1", "account1@example.org"));
    account1.setStatus(QMailAccount::SynchronizationEnabled, true);
    account1.setStatus(QMailAccount::Synchronized, false);
    account1.setCustomField("question", "What is your dog's name?");
    account1.setCustomField("answer", "Fido");

    QMailAccountConfiguration config1;
    config1.addServiceConfiguration("imap4");
    if (QMailAccountConfiguration::ServiceConfiguration *svcCfg = &config1.serviceConfiguration("imap4")) {
        svcCfg->setValue("server", "mail.example.org");
        svcCfg->setValue("username", "account1");
    }
    config1.addServiceConfiguration("smtp");
    if (QMailAccountConfiguration::ServiceConfiguration *svcCfg = &config1.serviceConfiguration("smtp")) {
        svcCfg->setValue("server", "mail.example.org");
        svcCfg->setValue("username", "account1");
    }

    // Verify that addition is successful
    QCOMPARE(QMailStore::instance()->countAccounts(), 0);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(!account1.id().isValid());
    QVERIFY(QMailStore::instance()->addAccount(&account1, &config1));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(account1.id().isValid());
    QCOMPARE(QMailStore::instance()->countAccounts(), 1);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);

    // Verify that retrieval yields matching result
    QMailAccount account2(account1.id());
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QCOMPARE(account2.id(), account1.id());
    QCOMPARE(account2.name(), account1.name());
    QCOMPARE(account2.fromAddress(), account1.fromAddress());
    QCOMPARE(account2.status(), account1.status());
    QCOMPARE(account2.customFields(), account1.customFields());
    QCOMPARE(account2.customField("answer"), QString("Fido"));

    QMailAccountConfiguration config2(account2.id());
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QCOMPARE(config2.services(), config1.services());
    foreach (const QString &service, config2.services()) {
        if (QMailAccountConfiguration::ServiceConfiguration *svcCfg = &config2.serviceConfiguration(service)) {
            QCOMPARE(svcCfg->values(), config1.serviceConfiguration(service).values());
        } else QFAIL(qPrintable(QString("no config for %1!").arg(service)));
    }

    // Verify that removal is successful 
    QVERIFY(QMailStore::instance()->removeAccount(account1.id()));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QCOMPARE(QMailStore::instance()->countAccounts(), 0);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);

    // Verify that retrieval yields invalid result
    QMailAccount account3(account1.id());
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::InvalidId);
    QVERIFY(!account3.id().isValid());

    QMailAccountConfiguration config3(account1.id());
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::InvalidId);
    QVERIFY(!config3.id().isValid());
}

void tst_QMailStore::removeFolder()
{
    //remove a folder that does not exist

    QVERIFY(QMailStore::instance()->removeFolder(QMailFolderId()));
    // TODO: Is this acceptable?
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);

    //remove a root folder with some mails in it
    //remove a child folder with mails in it
    //remove a folder that has mails and child folders with mails

    QMailFolder folder1;
    folder1.setPath("Folder 1");
    folder1.setStatus(QMailFolder::SynchronizationEnabled, true);
    folder1.setStatus(QMailFolder::Synchronized, false);
    folder1.setCustomField("question", "What is your dog's name?");
    folder1.setCustomField("answer", "Fido");

    // Verify that addition is successful
    QCOMPARE(QMailStore::instance()->countFolders(), 0);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(!folder1.id().isValid());
    QVERIFY(QMailStore::instance()->addFolder(&folder1));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(folder1.id().isValid());
    QCOMPARE(QMailStore::instance()->countFolders(), 1);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);

    // Verify that retrieval yields matching result
    QMailFolder folder2(folder1.id());
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QCOMPARE(folder2.id(), folder1.id());
    QCOMPARE(folder2.path(), folder1.path());
    QCOMPARE(folder2.status(), folder1.status());
    QCOMPARE(folder2.customFields(), folder1.customFields());
    QCOMPARE(folder2.customField("answer"), QString("Fido"));

    // Verify that removal is successful 
    QVERIFY(QMailStore::instance()->removeFolder(folder1.id()));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QCOMPARE(QMailStore::instance()->countFolders(), 0);

    // Verify that retrieval yields invalid result
    QMailFolder folder3(folder1.id());
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::InvalidId);
    QVERIFY(!folder3.id().isValid());
}

void tst_QMailStore::removeMessage()
{
    QMailAccount account;
    account.setName("Account");

    QCOMPARE(QMailStore::instance()->countAccounts(), 0);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(!account.id().isValid());
    QVERIFY(QMailStore::instance()->addAccount(&account, 0));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(account.id().isValid());
    QCOMPARE(QMailStore::instance()->countAccounts(), 1);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);

    QMailFolder folder;
    folder.setPath("Folder");

    QCOMPARE(QMailStore::instance()->countFolders(), 0);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(!folder.id().isValid());
    QVERIFY(QMailStore::instance()->addFolder(&folder));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(folder.id().isValid());
    QCOMPARE(QMailStore::instance()->countFolders(), 1);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);

    QMailMessage message1;
    message1.setParentAccountId(account.id());
    message1.setParentFolderId(folder.id());
    message1.setMessageType(QMailMessage::Sms);
    message1.setSubject("Message 1");
    message1.setTo(QMailAddress("alice@example.org"));
    message1.setFrom(QMailAddress("bob@example.com"));
    message1.setBody(QMailMessageBody::fromData(QString("Hi"), QMailMessageContentType("text/plain"), QMailMessageBody::SevenBit));
    message1.setStatus(QMailMessage::Incoming, true);
    message1.setStatus(QMailMessage::Read, true);
    message1.setCustomField("question", "What is your dog's name?");
    message1.setCustomField("answer", "Fido");

    // Verify that addition is successful
    QCOMPARE(QMailStore::instance()->countMessages(), 0);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(!message1.id().isValid());
    QVERIFY(QMailStore::instance()->addMessage(&message1));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(message1.id().isValid());
    QCOMPARE(QMailStore::instance()->countMessages(), 1);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);

	// On win32, the message cannot be removed while someone has the body object open!
	{
		// Verify that retrieval yields matching result
		QMailMessage message2(message1.id());
		QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
		QCOMPARE(message2.id(), message1.id());
		QCOMPARE(message2.parentFolderId(), message1.parentFolderId());
		QCOMPARE(message2.messageType(), message1.messageType());
		QCOMPARE(message2.subject(), message1.subject());
		QCOMPARE(message2.to(), message1.to());
		QCOMPARE(message2.from(), message1.from());
		QCOMPARE(message2.body().data(), message1.body().data());
		QCOMPARE((message2.status() | QMailMessage::UnloadedData), (message1.status() | QMailMessage::UnloadedData));
		QCOMPARE(message2.customFields(), message1.customFields());
		QCOMPARE(message2.customField("answer"), QString("Fido"));
	}

    // Verify that removal is successful 
    QVERIFY(QMailStore::instance()->removeMessage(message1.id()));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QCOMPARE(QMailStore::instance()->countMessages(), 0);

    // Verify that retrieval yields invalid result
    QMailMessage message3(message1.id());
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::InvalidId);
    QVERIFY(!message3.id().isValid());
}

void tst_QMailStore::remove1000Messages()
{
    QMailAccount account;
    account.setName("Account");

    QCOMPARE(QMailStore::instance()->countAccounts(), 0);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(!account.id().isValid());
    QVERIFY(QMailStore::instance()->addAccount(&account, 0));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(account.id().isValid());
    QCOMPARE(QMailStore::instance()->countAccounts(), 1);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);

    QMailFolder folder;
    folder.setPath("Folder");

    QCOMPARE(QMailStore::instance()->countFolders(), 0);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(!folder.id().isValid());
    QVERIFY(QMailStore::instance()->addFolder(&folder));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(folder.id().isValid());
    QCOMPARE(QMailStore::instance()->countFolders(), 1);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);

    //without message removal record

    static const int largeMessageCount = 10;

    for(int i = 0; i < largeMessageCount; ++i)
    {
        QMailMessage message1;
        message1.setServerUid(QString("%1|Just some message").arg(i));
        message1.setParentAccountId(account.id());
        message1.setParentFolderId(folder.id());
        message1.setMessageType(QMailMessage::Sms);
        message1.setSubject(QString("Message %1").arg(i));
        message1.setTo(QMailAddress("alice@example.org"));
        message1.setFrom(QMailAddress("bob@example.com"));
        message1.setBody(QMailMessageBody::fromData(QString("Hi"), QMailMessageContentType("text/plain"), QMailMessageBody::SevenBit));
        message1.setStatus(QMailMessage::Incoming, true);
        message1.setStatus(QMailMessage::Read, true);
        message1.setCustomField("question", "What is your dog's name?");
        message1.setCustomField("answer", "Fido");
        QVERIFY(QMailStore::instance()->addMessage(&message1));
    }

    QVERIFY(QMailStore::instance()->removeMessages(QMailMessageKey()));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QCOMPARE(QMailStore::instance()->countMessages(),0);


    //with message removal record

    for(int i = 0; i < largeMessageCount; ++i)
    {
        QMailMessage message1;
        message1.setServerUid(QString("Just some message$%1").arg(i));
        message1.setParentAccountId(account.id());
        message1.setParentFolderId(folder.id());
        message1.setMessageType(QMailMessage::Sms);
        message1.setSubject(QString("Message %1").arg(i));
        message1.setTo(QMailAddress("alice@example.org"));
        message1.setFrom(QMailAddress("bob@example.com"));
        message1.setBody(QMailMessageBody::fromData(QString("Hi"), QMailMessageContentType("text/plain"), QMailMessageBody::SevenBit));
        message1.setStatus(QMailMessage::Incoming, true);
        message1.setStatus(QMailMessage::Read, true);
        message1.setCustomField("question", "What is your dog's name?");
        message1.setCustomField("answer", "Fido");
        QVERIFY(QMailStore::instance()->addMessage(&message1));
    }

    QVERIFY(QMailStore::instance()->removeMessages(QMailMessageKey(),QMailStore::CreateRemovalRecord));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QCOMPARE(QMailStore::instance()->countMessages(),0);
    QCOMPARE(QMailStore::instance()->messageRemovalRecords(account.id(),folder.id()).count(),largeMessageCount);
    QVERIFY(QMailStore::instance()->purgeMessageRemovalRecords(account.id()));
    QCOMPARE(QMailStore::instance()->messageRemovalRecords(account.id(),folder.id()).count(),0);
}

void tst_QMailStore::removeMessageWithInResponse()
{
    QMailAccount account;
    account.setName("Account");

    QCOMPARE(QMailStore::instance()->countAccounts(), 0);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(!account.id().isValid());
    QVERIFY(QMailStore::instance()->addAccount(&account, 0));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(account.id().isValid());
    QCOMPARE(QMailStore::instance()->countAccounts(), 1);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);

    QMailFolder folder;
    folder.setPath("Folder");

    QCOMPARE(QMailStore::instance()->countFolders(), 0);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(!folder.id().isValid());
    QVERIFY(QMailStore::instance()->addFolder(&folder));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(folder.id().isValid());
    QCOMPARE(QMailStore::instance()->countFolders(), 1);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);

    QMailMessage message1;
    message1.setParentAccountId(account.id());
    message1.setParentFolderId(folder.id());
    message1.setMessageType(QMailMessage::Email);
    message1.setSubject("Message 1");
    message1.setTo(QMailAddress("alice@example.org"));
    message1.setFrom(QMailAddress("bob@example.com"));
    message1.setBody(QMailMessageBody::fromData(QString("Hi"), QMailMessageContentType("text/plain"), QMailMessageBody::SevenBit));
    message1.setStatus(QMailMessage::Incoming, true);
    message1.setStatus(QMailMessage::Read, true);

    // Verify that addition is successful
    QCOMPARE(QMailStore::instance()->countMessages(), 0);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(!message1.id().isValid());
    QVERIFY(QMailStore::instance()->addMessage(&message1));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(message1.id().isValid());
    QCOMPARE(QMailStore::instance()->countMessages(), 1);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    
    QMailMessage messageResponse;
    messageResponse.setParentAccountId(account.id());
    messageResponse.setParentFolderId(folder.id());
    messageResponse.setInResponseTo(message1.id());
    messageResponse.setMessageType(QMailMessage::Email);
    messageResponse.setSubject("Re: Message 1");
    messageResponse.setTo(QMailAddress("bob@example.com"));
    messageResponse.setFrom(QMailAddress("alice@example.org"));
    messageResponse.setBody(QMailMessageBody::fromData(QString("Hello"), QMailMessageContentType("text/plain"), QMailMessageBody::SevenBit));
    messageResponse.setStatus(QMailMessage::Incoming, true);
    messageResponse.setStatus(QMailMessage::Read, true);    

    // Verify that addition is successful
    QVERIFY(!messageResponse.id().isValid());
    QVERIFY(QMailStore::instance()->addMessage(&messageResponse));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QVERIFY(messageResponse.id().isValid());
    QCOMPARE(QMailStore::instance()->countMessages(), 2);
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);

	// On win32, the message cannot be removed while someone has the body object open!
	{
		// Verify that retrieval yields matching result
		QMailMessage message2(message1.id());
		QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
		QCOMPARE(message2.id(), message1.id());
		QCOMPARE(message2.parentFolderId(), message1.parentFolderId());
		QCOMPARE(message2.messageType(), message1.messageType());
		QCOMPARE(message2.subject(), message1.subject());
		QCOMPARE(message2.to(), message1.to());
		QCOMPARE(message2.from(), message1.from());
		QCOMPARE(message2.body().data(), message1.body().data());
		QCOMPARE((message2.status() | QMailMessage::UnloadedData), (message1.status() | QMailMessage::UnloadedData));

		// Verify that retrieval yields matching result
		QMailMessage messageResponse2(messageResponse.id());
		QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
		QCOMPARE(messageResponse2.id(), messageResponse.id());
		QCOMPARE(messageResponse2.inResponseTo(), messageResponse.inResponseTo());
		QCOMPARE(messageResponse2.inResponseTo(), message1.id());
		QCOMPARE(messageResponse2.parentFolderId(), messageResponse.parentFolderId());
        }

    // Verify that removal is successful 
    QVERIFY(QMailStore::instance()->removeMessage(message1.id()));
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QCOMPARE(QMailStore::instance()->countMessages(), 1);

    // Verify that retrieval yields invalid result
    QMailMessage message3(message1.id());
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::InvalidId);
    QVERIFY(!message3.id().isValid());
    
    // Verify that the child message is orphaned correctly
    QMailMessage messageResponse3(messageResponse.id());
    QCOMPARE(QMailStore::instance()->lastError(), QMailStore::NoError);
    QCOMPARE(messageResponse3.inResponseTo(), QMailMessageId()); // No parent now
}

void tst_QMailStore::message()
{
    // initializations
    QByteArray mime1("From: aperson@domain.example" CRLF
                     "To: bperson@domain.example" CRLF
                     "Content-Type: multipart/digest; boundary=XYZ" CRLF
                     "" CRLF
                     "--XYZ" CRLF
                     "Content-Type: text/plain" CRLF
                     "" CRLF
                     "This is a text plain part that is counter to recommended practice in" CRLF
                     "RFC 2046, $5.1.5, but is not illegal" CRLF
                     "" CRLF
                     "--XYZ" CRLF
                     "From: cperson@domain.example" CRLF
                     "To: dperson@domain.example" CRLF
                     "" CRLF
                     "A submessage" CRLF
                     "" CRLF
                     "--XYZ--" CRLF);

    QMailAccount account2;
    account2.setName("Account 2");
    account2.setMessageType(QMailMessage::Email);
    account2.setFromAddress(QMailAddress("Account 2", "account2@example.org"));
    account2.setStatus(QMailAccount::SynchronizationEnabled, true);
    account2.setStatus(QMailAccount::Synchronized, true);
    account2.setStatus(QMailAccount::MessageSource, true);
    account2.setStatus(QMailAccount::CanRetrieve, true);
    account2.setStatus(QMailAccount::MessageSink, true);
    account2.setStatus(QMailAccount::CanTransmit, true);
    account2.setCustomField("verified", "true");
    account2.setCustomField("question", "What is your dog's name?");
    account2.setCustomField("answer", "Milo");

    QMailAccountConfiguration config;
    config.addServiceConfiguration("imap4");
    if (QMailAccountConfiguration::ServiceConfiguration *svcCfg = &config.serviceConfiguration("imap4")) {
        svcCfg->setValue("server", "mail.example.org");
        svcCfg->setValue("username", "Account 2");
    }
    config.addServiceConfiguration("smtp");
    if (QMailAccountConfiguration::ServiceConfiguration *svcCfg = &config.serviceConfiguration("smtp")) {
        svcCfg->setValue("server", "mail.example.org");
        svcCfg->setValue("username", "Account 2");
    }

    QVERIFY(QMailStore::instance()->addAccount(&account2, &config));

    QMailMessage msg2 = QMailMessage::fromRfc2822(mime1);
    msg2.setMessageType(QMailMessage::Email);
    msg2.setParentAccountId(account2.id());
    msg2.setParentFolderId(QMailFolder::LocalStorageFolderId);
    msg2.setSubject("email message test");
    msg2.setDate(QMailTimeStamp(QDateTime(QDate::currentDate())));
    msg2.setReceivedDate(QMailTimeStamp(QDateTime(QDate::currentDate())));
    msg2.setStatus(QMailMessage::Incoming, true);
    msg2.setStatus(QMailMessage::New, true);
    msg2.setStatus(QMailMessage::Read, false);
    msg2.setServerUid("inboxmsg21");
    msg2.setSize(5 * 1024);
    msg2.setCustomField("present", "true");
    QVERIFY(QMailStore::instance()->addMessage(&msg2));

    QCOMPARE(msg2.partCount(), 2u);

    QCOMPARE(QMailStore::instance()->message(msg2.id()).partCount(), 2u);
}

void tst_QMailStore::implementationbase()
{
    QMailAccount account1;
    account1.setName("Account 10");
    account1.setFromAddress(QMailAddress("Account 10", "account10@example.org"));
    account1.setStatus(QMailAccount::SynchronizationEnabled, true);
    account1.setStatus(QMailAccount::Synchronized, false);
    account1.setStandardFolder(QMailFolder::SentFolder, QMailFolderId(333));
    account1.setStandardFolder(QMailFolder::TrashFolder, QMailFolderId(666));
    account1.setCustomField("question", "What is your dog's name?");
    account1.setCustomField("answer", "Fido");

    QMailAccountConfiguration config1;
    config1.addServiceConfiguration("imap4");
    if (QMailAccountConfiguration::ServiceConfiguration *svcCfg = &config1.serviceConfiguration("imap4")) {
        svcCfg->setValue("server", "mail.example.org");
        svcCfg->setValue("username", "account10");
    }
    config1.addServiceConfiguration("smtp");
    if (QMailAccountConfiguration::ServiceConfiguration *svcCfg = &config1.serviceConfiguration("smtp")) {
        svcCfg->setValue("server", "mail.example.org");
        svcCfg->setValue("username", "account10");
    }
    QVERIFY(QMailStore::instance()->addAccount(&account1, &config1));



    QMailStoreNullImplementation impl(QMailStore::instance());
    QVERIFY(!impl.asynchronousEmission());
    impl.flushIpcNotifications();
    impl.processIpcMessageQueue();

    impl.setRetrievalInProgress(QMailAccountIdList()<<account1.id());
    impl.notifyRetrievalInProgress(QMailAccountIdList()<<account1.id());

}

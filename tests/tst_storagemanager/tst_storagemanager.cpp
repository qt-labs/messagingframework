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
#include <ctype.h>
#include "qmailstore.h"
#include "qmailcontentmanager.h"

class tst_StorageManager : public QObject
{
    Q_OBJECT

public:
    tst_StorageManager() {}
    virtual ~tst_StorageManager() {}

private slots:
    void initTestCase();
    void cleanupTestCase();

    void test_init();
    void test_add();
    void test_remove();
    void test_update();

private:
    QMailAccountConfiguration makeConfig(const QString &accountName);
    QMailAccount account1;
    QMailAccount account2;

    QMailMessage msg1;
    QMailMessage msg2;

};

QTEST_MAIN(tst_StorageManager)
#include "tst_storagemanager.moc"

#define CRLF "\015\012"

QMailAccountConfiguration tst_StorageManager::makeConfig(const QString &accountName)
{
    QMailAccountConfiguration config;
    config.addServiceConfiguration("imap4");
    if (QMailAccountConfiguration::ServiceConfiguration *svcCfg = &config.serviceConfiguration("imap4")) {
        svcCfg->setValue("server", "mail.example.org");
        svcCfg->setValue("username", accountName);
    }
    config.addServiceConfiguration("smtp");
    if (QMailAccountConfiguration::ServiceConfiguration *svcCfg = &config.serviceConfiguration("smtp")) {
        svcCfg->setValue("server", "mail.example.org");
        svcCfg->setValue("username", accountName);
    }
    return config;
}

void tst_StorageManager::initTestCase()
{
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


    QMailStore::instance();

    // QMailAccount initializations

    QMailAccount &account = account1;
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
    QMailAccountConfiguration config1 = makeConfig("account1");
    QVERIFY(QMailStore::instance()->addAccount(&account1, &config1));

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
    QMailAccountConfiguration config2 = makeConfig("account2");
    QVERIFY(QMailStore::instance()->addAccount(&account2, &config2));

    // QMailMessage initializations
    QByteArray m1("From: newguy@example.org" CRLF
                  "To: oldguy@example.org" CRLF
                  "Cc: anotherguy@example.org" CRLF
                  "Date: Tue, 22 Feb 2011 16:00 +0200" CRLF
                  "Subject: Email message 1" CRLF
                  "Content-Type: text/plain; charset=UTF-8" CRLF
                  CRLF
                  "Plain text message for testing.");

    msg1.setMessageType(QMailMessage::Sms);
    msg1.setParentAccountId(account1.id());
    msg1.setParentFolderId(QMailFolder::LocalStorageFolderId);
    msg1.setFrom(QMailAddress("0404404040"));
    msg1.setTo(QMailAddress("0404040404"));
    msg1.setSubject("Where are you?");
    msg1.setDate(QMailTimeStamp(QDateTime(QDate::currentDate())));
    msg1.setReceivedDate(QMailTimeStamp(QDateTime(QDate::currentDate())));
    msg1.setStatus(QMailMessage::Incoming, true);
    msg1.setStatus(QMailMessage::New, false);
    msg1.setStatus(QMailMessage::Read, true);
    msg1.setServerUid("sim:12345");
    msg1.setSize(1 * 1024);
    msg1.setContent(QMailMessage::PlainTextContent);
    msg1.setCustomField("present", "true");
    msg1.setCustomField("todo", "true");
    QVERIFY(QMailStore::instance()->addMessage(&msg1));

    msg2.setMessageType(QMailMessage::Email);
    msg2.setParentAccountId(account1.id());
    msg2.setParentFolderId(QMailFolder::LocalStorageFolderId);
    msg2.setFrom(QMailAddress("newguy@example.org"));
    msg2.setTo(QMailAddress("old@example.org"));
    msg2.setCc(QMailAddressList() << QMailAddress("anotherguy@example.org"));
    msg2.setSubject("email message test");
    msg2.setDate(QMailTimeStamp(QDateTime(QDate::currentDate())));
    msg2.setReceivedDate(QMailTimeStamp(QDateTime(QDate::currentDate())));
    msg2.setStatus(QMailMessage::Incoming, true);
    msg2.setStatus(QMailMessage::New, true);
    msg2.setStatus(QMailMessage::Read, false);
    msg2.setServerUid("inboxmsg21");
    msg2.setSize(5 * 1024);
    msg2.setContent(QMailMessage::PlainTextContent);
    msg2.setCustomField("present", "true");
    msg2.setBody(QMailMessageBody::fromData(mime1,QMailMessageContentType("multipart/digest"),QMailMessageBody::QuotedPrintable));
    QVERIFY(QMailStore::instance()->addMessage(&msg2));

}

void tst_StorageManager::cleanupTestCase()
{
    QMailStore::instance()->removeAccounts(QMailAccountKey::customField("verified"));
    QMailStore::instance()->removeMessages(QMailMessageKey::customField("present"));
}

void tst_StorageManager::test_init()
{
    QMailContentManager *mgr = QMailContentManagerFactory::create(QMailContentManagerFactory::defaultScheme());

    QVERIFY(mgr->init());
}

void tst_StorageManager::test_add()
{
    // also tested in initTestCase()

    // with "qmf-detached-filename"
    QMailMessage msg3;
    msg3.setMessageType(QMailMessage::Email);
    msg3.setParentAccountId(account1.id());
    msg3.setParentFolderId(QMailFolder::LocalStorageFolderId);
    msg3.setFrom(QMailAddress("newguy@example.org"));
    msg3.setTo(QMailAddress("old@example.org"));
    msg3.setCc(QMailAddressList() << QMailAddress("anotherguy@example.org"));
    msg3.setSubject("email message test");
    msg3.setDate(QMailTimeStamp(QDateTime(QDate::currentDate())));
    msg3.setReceivedDate(QMailTimeStamp(QDateTime(QDate::currentDate())));
    msg3.setStatus(QMailMessage::Incoming, true);
    msg3.setStatus(QMailMessage::New, true);
    msg3.setStatus(QMailMessage::Read, false);
    msg3.setServerUid("inboxmsg22");
    msg3.setSize(5 * 1024);
    msg3.setContent(QMailMessage::PlainTextContent);
    msg3.setCustomField("present", "true");
    msg3.setBody(QMailMessageBody::fromData("plaintext",QMailMessageContentType("text/plain"),QMailMessageBody::QuotedPrintable));
    msg3.setCustomField("qmf-detached-filename", "file1x2y-detached");
    QVERIFY(QMailStore::instance()->addMessage(&msg3));
}

void tst_StorageManager::test_remove()
{
    QMailMessage msg4;
    msg4.setMessageType(QMailMessage::Email);
    msg4.setParentAccountId(account1.id());
    msg4.setParentFolderId(QMailFolder::LocalStorageFolderId);
    msg4.setFrom(QMailAddress("newguy@example.org"));
    msg4.setTo(QMailAddress("old@example.org"));
    msg4.setCc(QMailAddressList() << QMailAddress("anotherguy@example.org"));
    msg4.setSubject("email message test");
    msg4.setDate(QMailTimeStamp(QDateTime(QDate::currentDate())));
    msg4.setReceivedDate(QMailTimeStamp(QDateTime(QDate::currentDate())));
    msg4.setStatus(QMailMessage::Incoming, true);
    msg4.setStatus(QMailMessage::New, true);
    msg4.setStatus(QMailMessage::Read, false);
    msg4.setServerUid("unique-inboxmsg22");
    msg4.setSize(5 * 1024);
    msg4.setContent(QMailMessage::PlainTextContent);
    msg4.setCustomField("present", "true");
    msg4.setBody(QMailMessageBody::fromData("plaintext",QMailMessageContentType("text/plain"),QMailMessageBody::QuotedPrintable));
    msg4.setCustomField("qmf-detached-filename", "non-existent");
    msg4.setAttachments(QStringList() << "/fake/path");
    msg4.setAttachments(QStringList() << "qmf-detached-filename");
    QVERIFY(QMailStore::instance()->addMessage(&msg4));

}

void tst_StorageManager::test_update()
{

}


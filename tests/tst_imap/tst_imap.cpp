/****************************************************************************
**
** Copyright (C) 2023 The Qt Company Ltd.
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
#include <QSignalSpy>

#include <imapclient.h>
#include <imapstrategy.h>
#include <imapconfiguration.h>

class tst_ImapClient : public QObject
{
    Q_OBJECT

public:
    tst_ImapClient() {}
    virtual ~tst_ImapClient() {}

private slots:
    void initTestCase();
    void cleanupTestCase();

    // Runs before test_connection(), which makes a real (failing) network
    // connection attempt; this test needs the client's protocol/strategy
    // state to be untouched by that attempt.
    void test_specialUseFlagsWithoutCapability();

    void test_connection();

private:
    ImapClient *mClient = nullptr;
    QMailAccountId mAccountId;
};

void tst_ImapClient::initTestCase()
{
    QMailAccount account;
    account.setName("Test account for IMAP");
    account.setMessageType(QMailMessage::Email);
    account.setFromAddress(QMailAddress("Account IMAP", "account.imap@example.org"));
    account.setCustomField("test_account", "true");

    QMailAccountConfiguration config;
    config.addServiceConfiguration("imap4");
    ImapConfigurationEditor imap(&config);
    // Provide real server / credentials here
    imap.setMailServer("imap.free.fr");
    imap.setMailUserName("test account");
    imap.setMailPassword("test password");
    //imap.setValue("CredentialsId", "13");
    imap.setMailEncryption(QMailTransport::Encrypt_SSL);
    imap.setMailPort(993);

    QVERIFY(QMailStore::instance()->addAccount(&account, &config));

    mAccountId = account.id();
    mClient = new ImapClient(mAccountId, this);
}

void tst_ImapClient::cleanupTestCase()
{
    QMailStore::instance()->removeAccounts(QMailAccountKey::customField("test_account"));
    delete mClient;
    mClient = nullptr;
}

class ConnectionStrategy: public ImapStrategy
{
public:
    void transition(ImapStrategyContextBase *context,
                    const ImapCommand cmd,
                    const OperationStatus op) override
    {
        if (op != OpOk)
            qWarning() << "Test response to cmd:" << cmd << " is not ok: " << op;

        switch (cmd) {
        case IMAP_Login:
            context->operationCompleted();
            break;
        default:
            qWarning() << "Unhandled test response:" << cmd;
        }
    }
};

void tst_ImapClient::test_specialUseFlagsWithoutCapability()
{
    // Simulate a server telling us about the archive folder
    mClient->mailboxListed("\\Archive", "Archive");

    QMailFolderId archiveId = mClient->mailboxId("Archive");
    QVERIFY(archiveId.isValid());

    // Account should have that folder set as its archive folder
    QMailAccount account(mAccountId);
    QCOMPARE(account.standardFolder(QMailFolder::ArchiveFolder), archiveId);

    // The folder should have its archive flag set
    QMailFolder archiveFolder(archiveId);
    QVERIFY(archiveFolder.status() & QMailFolder::Archive);

    // Simulate server telling us about the same folder, but it doesn't send
    // the Archive flag.
    mClient->mailboxListed(QString(), "Archive");

    // Account should still have that folder set as its archive folder
    account = QMailAccount(mAccountId);
    QCOMPARE(account.standardFolder(QMailFolder::ArchiveFolder), archiveId);

    // Folder should still have its archive flag set
    archiveFolder = QMailFolder(archiveId);
    QVERIFY(archiveFolder.status() & QMailFolder::Archive);

    // Simulate server telling us a different folder is now the archive folder
    mClient->mailboxListed("\\Archive", "Archive2");

    QMailFolderId archive2Id = mClient->mailboxId("Archive2");
    QVERIFY(archive2Id.isValid());

    // Account should now have Archive2 set as its archive folder
    account = QMailAccount(mAccountId);
    QCOMPARE(account.standardFolder(QMailFolder::ArchiveFolder), archive2Id);

    // The old archive folder should not have its archive flag set
    archiveFolder = QMailFolder(archiveId);
    QVERIFY(!(archiveFolder.status() & QMailFolder::Archive));
}

void tst_ImapClient::test_connection()
{
    ConnectionStrategy strategy;
    QSignalSpy completed(mClient, &ImapClient::retrievalCompleted);
    QSignalSpy updateStatus(mClient, &ImapClient::statusChanged);

    mClient->setStrategy(&strategy);
    mClient->newConnection();
    QVERIFY(!completed.wait(2500)); // Fails with wrong credentials

    QCOMPARE(updateStatus.count(), 4);
    QCOMPARE(updateStatus.takeFirst().first().toString(), QString::fromLatin1("DNS lookup"));
    QCOMPARE(updateStatus.takeFirst().first().toString(), QString::fromLatin1("Connected"));
    QCOMPARE(updateStatus.takeFirst().first().toString(), QString::fromLatin1("Checking capabilities"));
    QCOMPARE(updateStatus.takeFirst().first().toString(), QString::fromLatin1("Logging in"));
}

QTEST_MAIN(tst_ImapClient)

#include "tst_imap.moc"

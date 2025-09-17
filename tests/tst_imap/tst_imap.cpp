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

    void test_connection();

private:
    ImapClient *mClient = nullptr;
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

    mClient = new ImapClient(account.id(), this);
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

void tst_ImapClient::test_connection()
{
    ConnectionStrategy strategy;
    QSignalSpy completed(mClient, &ImapClient::retrievalCompleted);
    QSignalSpy updateStatus(mClient, &ImapClient::updateStatus);

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

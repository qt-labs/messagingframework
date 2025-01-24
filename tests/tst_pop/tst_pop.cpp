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

#include <popclient.h>
#include <popconfiguration.h>

class tst_PopClient : public QObject
{
    Q_OBJECT

public:
    tst_PopClient() {}
    virtual ~tst_PopClient() {}

private slots:
    void initTestCase();
    void cleanupTestCase();

    void test_connection();

private:
    PopClient *mClient = nullptr;
};

QTEST_MAIN(tst_PopClient)

#include "tst_pop.moc"

void tst_PopClient::initTestCase()
{
    QMailAccount account;
    account.setName("Test account for POP");
    account.setMessageType(QMailMessage::Email);
    account.setFromAddress(QMailAddress("Account POP", "account.pop@example.org"));
    account.setCustomField("test_account", "true");

    QMailAccountConfiguration config;
    config.addServiceConfiguration("pop3");
    PopConfigurationEditor pop(&config);
    // Provide real server / credentials here
    pop.setMailServer("pop.free.fr");
    pop.setMailUserName("test account");
    pop.setMailPassword("test password");
    // pop.setValue("CredentialsId", "13");
    pop.setMailEncryption(QMailTransport::Encrypt_SSL);
    pop.setMailPort(995);

    QVERIFY(QMailStore::instance()->addAccount(&account, &config));

    mClient = new PopClient(account.id(), this);
}

void tst_PopClient::cleanupTestCase()
{
    QMailStore::instance()->removeAccounts(QMailAccountKey::customField("test_account"));
    delete mClient;
    mClient = nullptr;
}

void tst_PopClient::test_connection()
{
    QSignalSpy completed(mClient, &PopClient::retrievalCompleted);
    QSignalSpy updateStatus(mClient, &PopClient::updateStatus);

    mClient->testConnection();
    QVERIFY(!completed.wait(250)); // Fails with wrong credentials

    QCOMPARE(updateStatus.count(), 4);
    QCOMPARE(updateStatus.takeFirst().first().toString(), QString::fromLatin1("DNS lookup"));
    QCOMPARE(updateStatus.takeFirst().first().toString(), QString::fromLatin1("Connected"));
    QCOMPARE(updateStatus.takeFirst().first().toString(), QString::fromLatin1("Connected"));
    QCOMPARE(updateStatus.takeFirst().first().toString(), QString::fromLatin1("Logging in"));
}

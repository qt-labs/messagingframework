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

#include <smtpclient.h>
#include <smtpconfiguration.h>

class tst_SmtpClient : public QObject
{
    Q_OBJECT

public:
    tst_SmtpClient() {}
    virtual ~tst_SmtpClient() {}

private slots:
    void initTestCase();
    void cleanupTestCase();

    void test_connection();
    void test_auth();

private:
    SmtpClient *mClient = nullptr;
};

QTEST_MAIN(tst_SmtpClient)

#include "tst_smtp.moc"

void tst_SmtpClient::initTestCase()
{
    QMailAccount account;
    account.setName("Test account for SMTP");
    account.setMessageType(QMailMessage::Email);
    account.setFromAddress(QMailAddress("Account SMTP", "account.smtp@example.org"));
    account.setCustomField("test_account", "true");

    QMailAccountConfiguration config;
    config.addServiceConfiguration("smtp");
    SmtpConfigurationEditor smtp(&config);
    // Provide real server / credentials here
    smtp.setSmtpServer("smtp.free.fr");
    smtp.setSmtpUsername("test account");
    smtp.setSmtpPassword("test password");

    QVERIFY(QMailStore::instance()->addAccount(&account, &config));

    mClient = new SmtpClient(this);
    mClient->setAccount(account.id());
}

void tst_SmtpClient::cleanupTestCase()
{
    QMailStore::instance()->removeAccounts(QMailAccountKey::customField("test_account"));
    delete mClient;
    mClient = nullptr;
}

void tst_SmtpClient::test_connection()
{
    QSignalSpy completed(mClient, &SmtpClient::sendCompleted);
    QSignalSpy updateStatus(mClient, &SmtpClient::updateStatus);

    mClient->newConnection();
    QVERIFY(completed.wait());

    QCOMPARE(updateStatus.count(), 3);
    QCOMPARE(updateStatus.takeFirst().first(), QString::fromLatin1("DNS lookup"));
    QCOMPARE(updateStatus.takeFirst().first(), QString::fromLatin1("Connected"));
    QCOMPARE(updateStatus.takeFirst().first(), QString::fromLatin1("Connected"));
}

void tst_SmtpClient::test_auth()
{
    QSignalSpy completed(mClient, &SmtpClient::sendCompleted);
    QSignalSpy updateStatus(mClient, &SmtpClient::updateStatus);

    QMailAccountConfiguration config(mClient->account());
    SmtpConfigurationEditor smtp(&config);
    smtp.setSmtpAuthFromCapabilities(true); // Auto, will choose CRAM-MD5
    smtp.setSmtpEncryption(QMailTransport::Encrypt_SSL);
    smtp.setSmtpPort(465);
    QVERIFY(QMailStore::instance()->updateAccountConfiguration(&config));

    mClient->newConnection();
    QVERIFY(!completed.wait(250)); // Fails with wrong credentials

    QCOMPARE(updateStatus.count(), 3);
    QCOMPARE(updateStatus.takeFirst().first(), QString::fromLatin1("DNS lookup"));
    QCOMPARE(updateStatus.takeFirst().first(), QString::fromLatin1("Connected"));
    QCOMPARE(updateStatus.takeFirst().first(), QString::fromLatin1("Connected"));

    smtp.setSmtpAuthentication(QMail::PlainMechanism);
    QVERIFY(QMailStore::instance()->updateAccountConfiguration(&config));

    mClient->newConnection();
    QVERIFY(!completed.wait(250)); // Fails with wrong credentials

    QCOMPARE(updateStatus.count(), 3);
    QCOMPARE(updateStatus.takeFirst().first(), QString::fromLatin1("DNS lookup"));
    QCOMPARE(updateStatus.takeFirst().first(), QString::fromLatin1("Connected"));
    QCOMPARE(updateStatus.takeFirst().first(), QString::fromLatin1("Connected"));
}

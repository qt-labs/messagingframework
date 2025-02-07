/* -*- c-basic-offset: 4 -*- */
/****************************************************************************
**
** Copyright (C) 2025 Caliste Damien.
** Contact: Damien Caliste <dcaliste@free.fr>
**
** Copyright (C) 2025 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
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

#include <QtTest>
#include <QObject>

#include <qmailstore.h>
#include <qmailaccount.h>
#include <qmailaccountconfiguration.h>

class tst_QMailAccountConfiguration : public QObject
{
    Q_OBJECT

public:
    tst_QMailAccountConfiguration() {}
    virtual ~tst_QMailAccountConfiguration() {}

public slots:
    void initTestCase();
    void cleanupTestCase();

private slots:
    void test_service();

private:
    QMailAccountId m_id;
};

void tst_QMailAccountConfiguration::initTestCase()
{
    QMailAccount account;
    account.setName(QStringLiteral("Test account for QMF"));
    account.setCustomField(QStringLiteral("test_account"), QStringLiteral("true"));

    QVERIFY(QMailStore::instance()->addAccount(&account, nullptr));
    QVERIFY(account.id().isValid());

    m_id = account.id();
}

void tst_QMailAccountConfiguration::cleanupTestCase()
{
    QMailStore::instance()->removeAccounts(QMailAccountKey::customField("test_account"));
    m_id = QMailAccountId();
}

void tst_QMailAccountConfiguration::test_service()
{
    QMailAccountConfiguration config(m_id);

    QCOMPARE(config.id(), m_id);

    QVERIFY(config.addServiceConfiguration(QStringLiteral("test")));
    QVERIFY(config.services().contains(QStringLiteral("test")));

    QMailAccountConfiguration::ServiceConfiguration service
        = config.serviceConfiguration(QStringLiteral("test"));
    QCOMPARE(service.id(), m_id);
    QCOMPARE(service.service(), QStringLiteral("test"));
    QVERIFY(service.values().isEmpty());

    service.setValue(QStringLiteral("string"), QStringLiteral("test string"));
    QCOMPARE(service.value(QStringLiteral("string")), QStringLiteral("test string"));

    service.setValue(QStringLiteral("empty list"), QStringList());
    QCOMPARE(service.listValue(QStringLiteral("empty list")), QStringList());

    const QStringList list1 = QStringList() << QStringLiteral("ele 1");
    service.setValue(QStringLiteral("one element list"), list1);
    QCOMPARE(service.listValue(QStringLiteral("one element list")), list1);

    const QStringList list2 = QStringList() << QStringLiteral("ele 1")
                                            << QStringLiteral("foo")
                                            << QStringLiteral("foo bar");
    service.setValue(QStringLiteral("some element list"), list2);
    QCOMPARE(service.listValue(QStringLiteral("some element list")), list2);

    QVERIFY(QMailStore::instance()->updateAccountConfiguration(&config));

    config = QMailAccountConfiguration(m_id);
    QVERIFY(config.services().contains(QStringLiteral("test")));
    service = config.serviceConfiguration(QStringLiteral("test"));

    QCOMPARE(service.value(QStringLiteral("string")), QStringLiteral("test string"));
    QCOMPARE(service.listValue(QStringLiteral("empty list")), QStringList());
    QCOMPARE(service.listValue(QStringLiteral("one element list")), list1);
    QCOMPARE(service.listValue(QStringLiteral("some element list")), list2);
}


#include "tst_qmailaccountconfiguration.moc"
QTEST_MAIN(tst_QMailAccountConfiguration)

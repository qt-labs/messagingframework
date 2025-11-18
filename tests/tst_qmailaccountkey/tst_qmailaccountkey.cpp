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

#include "qmailaccountkey.h"

class tst_qmailaccountkey : public QObject
{
    Q_OBJECT

public:
    tst_qmailaccountkey();
    virtual ~tst_qmailaccountkey();

private slots:
    void testQMailAccountKey();
};

tst_qmailaccountkey::tst_qmailaccountkey()
{
}

tst_qmailaccountkey::~tst_qmailaccountkey()
{
}

void tst_qmailaccountkey::testQMailAccountKey()
{
    QMailAccountKey firstKey(QMailAccountKey::id(QMailAccountId(0)));
    QMailAccountKey secondKey(QMailAccountKey::id(QMailAccountId(1)));
    QMailAccountKey testKey;
    testKey = firstKey;
    QVERIFY (firstKey != secondKey);
    testKey &= firstKey;
    QCOMPARE(firstKey, testKey);
    testKey |= secondKey;
    QVERIFY((testKey | firstKey) == secondKey);
}

QTEST_MAIN(tst_qmailaccountkey)

#include "tst_qmailaccountkey.moc"

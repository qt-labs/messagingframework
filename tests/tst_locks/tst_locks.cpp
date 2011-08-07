/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QObject>
#include <QTest>
#include <qmailaddress.h>
#include "locks_p.h"
#include "qmailaccountkey.h"
#include <ctype.h>


class tst_locks : public QObject
{
    Q_OBJECT

public:
    tst_locks();
    virtual ~tst_locks();

private slots:
    virtual void initTestCase();
    virtual void cleanupTestCase();
    virtual void init();
    virtual void cleanup();

    void testLock();
    void testQMailAccountKey();

};

QTEST_MAIN(tst_locks)
#include "tst_locks.moc"


tst_locks::tst_locks()
{
}

tst_locks::~tst_locks()
{
}

void tst_locks::initTestCase()
{
}

void tst_locks::cleanupTestCase()
{
}

void tst_locks::init()
{    

}

void tst_locks::cleanup()
{
}

void tst_locks::testLock()
{
    ProcessReadLock *prl = new ProcessReadLock("");
    QVERIFY(prl != NULL);
    prl->lock();
    prl->unlock();


    
    delete prl;
}

void tst_locks::testQMailAccountKey()
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

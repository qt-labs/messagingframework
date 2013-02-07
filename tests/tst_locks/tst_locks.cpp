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

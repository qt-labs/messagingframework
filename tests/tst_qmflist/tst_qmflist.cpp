/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include <qmflist.h>

#include <QObject>
#include <QTest>

//TESTED_CLASS=QmfList
//TESTED_FILES=src/libraries/qmfclient/qmflist.h

class tst_QmfList : public QObject
{
    Q_OBJECT

public:
    tst_QmfList();
    virtual ~tst_QmfList();

private slots:
    virtual void initTestCase();
    virtual void cleanupTestCase();
    virtual void init();
    virtual void cleanup();

    void count();
    void isEmpty();
    void append();
    void append_list();
    void last();
    void first();
    void takeFirst();
    void removeAt();
    void removeOne();
    void removeAll();
    void at();
    void subscript();
    void subscript_const();
    void contains();
    void indexOf();

    void toQList();
    void fromQList();
};

tst_QmfList::tst_QmfList()
{
}

tst_QmfList::~tst_QmfList()
{
}

void tst_QmfList::initTestCase()
{
}

void tst_QmfList::cleanupTestCase()
{
}

void tst_QmfList::init()
{
}

void tst_QmfList::cleanup()
{
}

void tst_QmfList::count()
{
    QmfList<int> list { 5, 6, 7, 8 };
    QCOMPARE(list.count(), 4);
    QCOMPARE(list.count(), static_cast<qsizetype>(list.size()));
    list.pop_back();
    QCOMPARE(list.count(), 3);
    QCOMPARE(list.count(), static_cast<qsizetype>(list.size()));
}

void tst_QmfList::isEmpty()
{
    QmfList<int> list { 5 };
    QCOMPARE(list.isEmpty(), false);
    list.pop_front();
    QCOMPARE(list.isEmpty(), true);
    list.push_back(6);
    QCOMPARE(list.isEmpty(), false);
}

void tst_QmfList::append()
{
    QmfList<int> list { 5 };
    list.append(6);
    QCOMPARE(list.count(), 2);
    QCOMPARE(list.at(0), 5);
    QCOMPARE(list.at(1), 6);
}

void tst_QmfList::append_list()
{
    QmfList<int> list { 5 };
    const QmfList<int> append_list { 6, 7 };
    list.append(append_list);
    QCOMPARE(list.count(), 3);
    QCOMPARE(list.at(0), 5);
    QCOMPARE(list.at(1), 6);
    QCOMPARE(list.at(2), 7);
}

void tst_QmfList::last()
{
    QmfList<int> list { 5, 6, 7 };
    QCOMPARE(list.last(), 7);
    list.pop_back();
    QCOMPARE(list.last(), 6);
}

void tst_QmfList::first()
{
    QmfList<int> list { 5, 6, 7 };
    QCOMPARE(list.first(), 5);
    list.pop_front();
    QCOMPARE(list.first(), 6);
}

void tst_QmfList::takeFirst()
{
    QmfList<int> list { 5, 6, 7 };
    QCOMPARE(list.count(), 3);
    QCOMPARE(list.at(0), 5);
    QCOMPARE(list.at(1), 6);
    QCOMPARE(list.at(2), 7);
    int first = list.takeFirst();
    QCOMPARE(first, 5);
    QCOMPARE(list.count(), 2);
    QCOMPARE(list.at(0), 6);
    QCOMPARE(list.at(1), 7);
}

void tst_QmfList::removeAt()
{
    QmfList<int> list { 5, 6, 7, 5 };
    QCOMPARE(list.front(), 5);
    QCOMPARE(list.back(), 5);
    QCOMPARE(list.count(), 4);
    list.removeAt(0);
    QCOMPARE(list.front(), 6);
    QCOMPARE(list.back(), 5);
    QCOMPARE(list.count(), 3);
    list.removeAt(2);
    QCOMPARE(list.front(), 6);
    QCOMPARE(list.back(), 7);
    QCOMPARE(list.count(), 2);
}

void tst_QmfList::removeOne()
{
    QmfList<int> list { 5, 6, 7, 5 };
    QCOMPARE(list.front(), 5);
    QCOMPARE(list.back(), 5);
    QCOMPARE(list.count(), 4);
    list.removeOne(5);
    QCOMPARE(list.front(), 6);
    QCOMPARE(list.back(), 5);
    QCOMPARE(list.count(), 3);
    list.removeOne(5);
    QCOMPARE(list.front(), 6);
    QCOMPARE(list.back(), 7);
    QCOMPARE(list.count(), 2);
    list.removeOne(5);
    QCOMPARE(list.front(), 6);
    QCOMPARE(list.back(), 7);
    QCOMPARE(list.count(), 2);
}

void tst_QmfList::removeAll()
{
    QmfList<int> list { 5, 6, 7, 5 };
    QCOMPARE(list.front(), 5);
    QCOMPARE(list.back(), 5);
    QCOMPARE(list.count(), 4);
    list.removeAll(5);
    QCOMPARE(list.front(), 6);
    QCOMPARE(list.back(), 7);
    QCOMPARE(list.count(), 2);
    list.removeAll(5);
    QCOMPARE(list.front(), 6);
    QCOMPARE(list.back(), 7);
    QCOMPARE(list.count(), 2);
}

void tst_QmfList::at()
{
    const QmfList<int> list { 5, 6, 7, 8 };
    QCOMPARE(list.at(0), 5);
    QCOMPARE(list.at(2), 7);
}

void tst_QmfList::subscript()
{
    QmfList<int> list { 5, 6, 7, 8 };
    list[0] = 55;
    list[2] = 77;
    QCOMPARE(list.at(0), 55);
    QCOMPARE(list.at(2), 77);

    list.push_back(9);
    QCOMPARE(list[4], 9);
    list[4] = 99;
    QCOMPARE(list[4], 99);
}

void tst_QmfList::subscript_const()
{
    const QmfList<int> list { 5, 6, 7, 8 };
    QCOMPARE(list.at(0), list[0]);
    QCOMPARE(list.at(2), list[2]);
}

void tst_QmfList::contains()
{
    QmfList<int> list { 5, 6, 7, 8 };
    QCOMPARE(list.contains(5), true);
    QCOMPARE(list.contains(9), false);
    list.append(9);
    QCOMPARE(list.contains(9), true);
}

void tst_QmfList::indexOf()
{
    QmfList<int> list { 5, 6, 7, 8 };
    QCOMPARE(list.indexOf(6), 1);
    QCOMPARE(list.indexOf(3), -1);
    QCOMPARE(list.indexOf(5), 0);
    list.append(2);
    QCOMPARE(list.indexOf(2), 4);
    list.pop_front();
    QCOMPARE(list.indexOf(5), -1);
}

void tst_QmfList::toQList()
{
    QmfList<int> qmflist { 5, 6, 7, 8 };
    QList<int> qlist { 5, 6, 7, 8 };

    QCOMPARE(qmflist.toQList(), qlist);
    qmflist.takeFirst();
    QVERIFY(qmflist.toQList() != qlist);
    qlist.takeFirst();
    QCOMPARE(qmflist.toQList(), qlist);
}

void tst_QmfList::fromQList()
{
    QmfList<int> qmflist { 5, 6, 7, 8 };
    QList<int> qlist { 5, 6, 7, 8 };

    QCOMPARE(QmfList<int>::fromQList(qlist), qmflist);
    qlist.takeFirst();
    QVERIFY(QmfList<int>::fromQList(qlist) != qmflist);
    qmflist.takeFirst();
    QCOMPARE(QmfList<int>::fromQList(qlist), qmflist);
}

QTEST_MAIN(tst_QmfList)
#include "tst_qmflist.moc"

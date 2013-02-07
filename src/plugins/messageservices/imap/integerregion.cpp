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

#include "integerregion.h"
#include <qmaillog.h>

/*
  \class IntegerRegion
  \brief The IntegerRegion class provides a one dimensional integer region 
  i.e. a sequence of integer ranges.

  Instantiations can be used to store IMAP sequences sets as described
  in RFC3501.

  Currently the internal representation consists of a list of inclusive 
  ranges stored in ascending order. A range is stored as a qpair of ints, 
  the first int in the pair being the beginning of the range (low value) 
  and the second being the end of the range (high value). A single integer 
  is stored as a range with the start and end being equal. Ranges are 
  non-overlapping, and non-adjacent i.e. there is a gap of at least 
  length one between ranges.
  
  If useful this class could be extended to contain new methods such as
  left (first n integers contained), mid, right, union, intersection, 
  at (nth integer contained). cardinality could be optimized.
*/

/*!
    Constructs an empty IntegerRegion object.
*/
IntegerRegion::IntegerRegion()
{
}

/*
  Constructs an IntegerRegion object, from \a uids, which may be unordered.
*/
IntegerRegion::IntegerRegion(const QStringList &uids)
{
    // Performance note currently O(n^2), n = uids.count()
    // TODO: sort uids if they are not already sorted
    foreach(const QString &uid, uids) {
        bool ok(false);
        uint number = uid.toUInt(&ok);
        if (ok)
            add(number);
    }
}

/*
  Constructs an IntegerRegion object, from \a uidString an IMAP sequence-set
  string as described in RFC3501. \a uidString may contain an unordered 
  list of ids, wildcards are not currently supported.
*/
IntegerRegion::IntegerRegion(const QString &uidString)
{
    // Performance note currently O(n^2), n = uids.count()
    // TODO: sort uids in uidString if they are not already sorted
    QStringList rangeList = uidString.split(",", QString::SkipEmptyParts);
    foreach(const QString &s, rangeList) {
        bool ok = false;
        int index = s.indexOf(":");
        if (index == -1) {
            int a = s.toInt(&ok);
            if (!ok)
                continue;
            add(a);
        } else if (index > 0) {
            int a = s.left(index).toInt(&ok);
            if (!ok)
                continue;
            int b = s.mid(index+1).toInt(&ok);
            if (!ok)
                continue;
            for(int i = a; i <= b; ++i) {
                // could be optimized if union is implemented
                add(i);
            }
        }
    }
}

IntegerRegion::IntegerRegion(int begin, int end)
{
    clear();
    if (begin > end)
        return;
    mRangeList.append(IntegerRange(begin, end));
}

/*
  Removes all ranges from the region.
*/
void IntegerRegion::clear()
{
    return mRangeList.clear();
}

/*
  Returns true if the region contains no integers; otherwise returns false.
*/
bool IntegerRegion::isEmpty() const
{
    return mRangeList.isEmpty();
}

/*
  Number of integers contained in the region. Could be optimized by keeping 
  a running count in a member variable.
*/
//  Maybe count would be a better name.
uint IntegerRegion::cardinality() const
{
    uint result(0);

    foreach( const IntegerRange &range, mRangeList)
        result += range.second - range.first + 1;

    return result;
}

/*
  The maximum integer contained in the region. The region must be not
  be empty. If the region can be empty call isEmpty() before calling this function.
*/
int IntegerRegion::maximum() const
{
    return mRangeList.last().second;
}

/*
  The minimum integer contained in the region. The region must be not
  be empty. If the region can be empty call isEmpty() before calling this function.
*/
int IntegerRegion::minimum() const
{
    return mRangeList.first().first;
}

/*
  Returns a string list of integers contained in the region.
*/
QStringList IntegerRegion::toStringList() const
{
    QStringList result;
    foreach(const IntegerRange &range, mRangeList) {
        result += QString::number(range.first);
        for (int i = range.first + 1; i <= range.second; ++i)
            result += QString::number(i);
    }
    return result;
}

/*
  Returns a sequence-set string as described in RFC3501.
*/
QString IntegerRegion::toString() const
{
    QString result;
    bool first(true);
    foreach(const IntegerRange &range, mRangeList) {
        if (!first)
            result += ",";
        result += QString::number(range.first);
        if (range.second > range.first)
            result += QString(":%1").arg(range.second);
        first = false;
    }
    return result;
}

/*
  Inserts \a number into the integer region if it is not already contained; 
  otherwise does nothing.
  
  Currently optimized for appending integers greater than any contained by 
  the region.
*/
void IntegerRegion::add(int number)
{
    // Start from the end of the list of ranges since it's expected that 
    // normally numbers above the current range contained will be added.
    QList< IntegerRange >::iterator previous = mRangeList.end();
    QList< IntegerRange >::iterator next;

    while (previous != mRangeList.begin()) {
        next = previous;
        --previous;
        const int first = (*previous).first;
        const int second = (*previous).second;
        if (number < first - 1) {
            continue;
        } else if (number > second + 1) {
            // insert new range between previous and next item
            mRangeList.insert(next, IntegerRange(number, number));
            return;
        } else if (number == second + 1) {
            // increment upper bound
            (*previous).second = number;
            return;
        } else if ((number >= first) && (number <= second)) {
            // already contained nothing todo
            return;
        } else if (number == first - 1) {
            if (previous == mRangeList.begin()) {
                // decrement lower bound first item
                (*previous).first = number;
                return;
            }

            next = previous;
            --previous;
            if ((*previous).second == first - 2) {
                // coalesce current and previous item
                (*previous).second = (*next).second;
                mRangeList.erase(next);
            } else {
                // decrement lower bound
                (*next).first = number;
            }
            return;
        }
    }
    // insert new item at start of list
    mRangeList.insert(previous, IntegerRange(number, number));
}

/*
  Returns a region containing all integers in this region that are not also 
  in the \a other region.
*/
IntegerRegion IntegerRegion::subtract(IntegerRegion other) const
{
    // Performance note O(n), n = max(a.cardinality(), b.cardinality())
    IntegerRegion result(*this);
    QList< IntegerRange >::iterator a = result.mRangeList.begin();
    QList< IntegerRange >::iterator b = other.mRangeList.begin();
    
    while (a != result.mRangeList.end()
           && b != other.mRangeList.end()) {
        if ((*b).second < (*a).first) {
            // b < a, strictly
            ++b;
            continue;
        } else if ((*b).first > (*a).second) {
            // b > a, strictly
            ++a;
            continue;
        } else if (((*b).first <= (*a).first) && ((*b).second >= (*a).second)) {
            // b contains a
            a = result.mRangeList.erase(a);
            continue;
        } else if (((*b).first > (*a).first) && ((*b).second < (*a).second)) {
            // a strictly contains b
            IntegerRange lowerSlice((*a).first, (*b).first - 1);
            a = result.mRangeList.insert(a, lowerSlice);
            ++a;
            (*a).first = (*b).second + 1;
            ++b;
            continue;
        } else if (((*b).first <= (*a).first) && ((*b).second < (*a).second)) {
            // b < a, but overlap
            (*a).first = (*b).second + 1;
            ++b;
        } else if (((*b).first <= (*a).second) && ((*b).second >= (*a).second)) {
            // b > a, but overlap
            (*a).second = (*b).first - 1;
            ++a;
        } else {
            qWarning() << "Unhandled IntegerRegion case a " << *a << " b " << *b;
            return result;
        }
    }
    return result;
}

/*
  Returns the union of this region and the \a other region.
*/
IntegerRegion IntegerRegion::add(IntegerRegion other) const
{
    if (!cardinality())
        return other;
    
    if (!other.cardinality())
        return *this;
    
    int min = qMin(minimum(), other.minimum());
    int max = qMax(maximum(), other.maximum());
    IntegerRegion c(min, max);
    // a + b = c - (c - a - b)
    return c.subtract(c.subtract(*this).subtract(other));
}

/*
  Returns the intersection of this region and the \a other region.
*/
IntegerRegion IntegerRegion::intersect(IntegerRegion other) const
{
    IntegerRegion A(*this);
    IntegerRegion B(other);
    // A n B = (A U B) - ((A - B) U (B - A)) 
    IntegerRegion result(A.add(B).subtract(A.subtract(B).add(B.subtract(A))));
    return result;
}

/*
  Returns true if \a uids contains a list of integers; otherwise returns false.
*/
bool IntegerRegion::isIntegerRegion(QStringList uids)
{
    foreach(const QString &uid, uids) {
        bool ok(false);
        uid.toUInt(&ok);
        if (!ok)
            return false;
    }
    return true;
}

/* 
    Returns the list of integers contained by the description \a region.
*/
QList<int> IntegerRegion::toList(const QString &region)
{
    QList<int> result;

    QRegExp range("(\\d+)(?::(\\d+))?(?:,)?");

    int index = 0;
    while ((index = range.indexIn(region, index)) != -1) {
        index += range.cap(0).length();

        int first = range.cap(1).toInt();
        int second = first;
        if (!range.cap(2).isEmpty()) {
            second = range.cap(2).toInt();
            if (second < first) {
                second = first;
            }
        }

        for ( ; first <= second; ++first) {
            result.append(first);
        }
    }

    return result;
}


#if 0
//TODO Convert these tests to standard qunit style

/*
  Test function.
  
  Returns an integer region containing all integers marked in the binary 
  string \s, counting from zero.
*/
IntegerRegion IntegerRegion::fromBinaryString(const QString &s)
{
    IntegerRegion result;
    for (int i = 0; i < s.length(); ++i) {
        if (!s[i].isSpace())
            result.add(i);
    }
    return result;
}

/*
  Test function.
  
  Returns a binary string of all integers contained by \ir, counting from zero.
*/
QString IntegerRegion::toBinaryString(const IntegerRegion &ir)
{
    QString result;
    int last(0);
    foreach(const QString &s, ir.toStringList()) {
        bool ok;
        int value = s.toInt(&ok);
        for (int i = last; i < value; ++i)
            result += ' ';
        result += '-';
        last = value + 1;
    }
    return result;
}

/*
  Test function.
  
  Run through some tests.
*/
int IntegerRegion::tests()
{
    QList<int> values;
    values << 12 << 13 << 16 << 20 << 22 << 23 << 24 << 27 << 28 << 34;

    QStringList list;
    foreach (const int &v, values) {
        list << QString::number(v);
    }

    qMailLog(Messaging) << "Constructing from QStringList " << list;
    
    IntegerRegion ir(list);
    qMailLog(Messaging) << "IntegerRegion = " << ir.toString();
    qMailLog(Messaging) << "IntegerRegion::IntegerRegion(QStringList) test1"
                        << ((ir.toString() == "12:13,16,20,22:24,27:28,34") ? "passed" : "failed");
    qMailLog(Messaging) << "IntegerRegion::toList(QString) test1"
                        << ((IntegerRegion::toList(ir.toString()) == values) ? "passed" : "failed");
    qMailLog(Messaging) << "Adding 31. Insert new region between existing regions.";
    ir.add(31);
    qMailLog(Messaging) << "IntegerRegion = " << ir.toString();
    qMailLog(Messaging) << "Adding 14. Increment first region.";
    ir.add(14);
    qMailLog(Messaging) << "IntegerRegion = " << ir.toString();
    qMailLog(Messaging) << "Adding 13. Ignore.";
    ir.add(13);
    qMailLog(Messaging) << "IntegerRegion = " << ir.toString();
    qMailLog(Messaging) << "Adding 11. Decrement first region.";
    ir.add(11);
    qMailLog(Messaging) << "IntegerRegion = " << ir.toString();
    qMailLog(Messaging) << "Adding 15. Decrement second region and coalesce.";
    ir.add(15);
    qMailLog(Messaging) << "IntegerRegion = " << ir.toString();
    qMailLog(Messaging) << "Adding 26. Decrement lower bound.";
    ir.add(26);
    qMailLog(Messaging) << "IntegerRegion = " << ir.toString();
    qMailLog(Messaging) << "Adding 7. Insert new region at start of list";
    ir.add(7);
    qMailLog(Messaging) << "IntegerRegion = " << ir.toString();
    qMailLog(Messaging) << "Adding 42. Insert new region at end";
    ir.add(42);
    qMailLog(Messaging) << "IntegerRegion " << ir.toString();
    qMailLog(Messaging) << "Adding 43. Increment last region.";
    ir.add(43);
    qMailLog(Messaging) << "IntegerRegion = " << ir.toString();
    qMailLog(Messaging) << "Adding 25. Another coalesce.";
    ir.add(25);
    qMailLog(Messaging) << "IntegerRegion = " << ir.toString();
    qMailLog(Messaging) << "IntegerRegion::toString test2"
                        << ((ir.toString() == "7,11:16,20,22:28,31,34,42:43") ? "passed" : "failed");
    qMailLog(Messaging) << "IntegerRegion::cardinality test3"
                        << ((ir.cardinality() == 19) ? "passed" : "failed");

    qMailLog(Messaging) << "IntegerRegion cardinality() = " << ir.cardinality();
    qMailLog(Messaging) << "IntegerRegion toStringList() = " << ir.toStringList();
    qMailLog(Messaging) << "IntegerRegion::isIntegerRegion test4: " 
                    << (IntegerRegion::isIntegerRegion(ir.toStringList()) ? "passed" : "failed");
    QStringList hippo;
    hippo << "1" << "3" << "hippo" << "7";
    qMailLog(Messaging) << "IntegerRegion::isIntegerRegion test5: " 
                    << (IntegerRegion::isIntegerRegion(hippo) ? "failed" : "passed");

    QList<int> newValues;
    foreach (const QString &number, ir.toStringList()) {
        newValues << number.toInt();
    }
    qMailLog(Messaging) << "IntegerRegion::toList(QString) test2"
                        << ((IntegerRegion::toList(ir.toString()) == newValues) ? "passed" : "failed");

    QString a("    --  -  --  --- -- -- -   --   -- -  -");
    QString b("  - -- -- ----  -   - -   -   -- --  -   ");
    QString c("               - - -   - -   -     -    -");
    IntegerRegion ar = IntegerRegion::fromBinaryString(a);
    IntegerRegion br = IntegerRegion::fromBinaryString(b);
    IntegerRegion cr = ar.subtract(br);
    qMailLog(Messaging) << "a " << ar.toString();
    qMailLog(Messaging) << "b " << br.toString();
    qMailLog(Messaging) << "a   " << IntegerRegion::toBinaryString(ar) << " - ";
    qMailLog(Messaging) << "b   " << IntegerRegion::toBinaryString(br) << " = ";
    qMailLog(Messaging) << "a-b " << IntegerRegion::toBinaryString(cr);
    qMailLog(Messaging) << "c   " << c;
    qMailLog(Messaging) << "IntegerRegion::subtract test6: " 
                        << ((IntegerRegion::toBinaryString(cr) == c) ? "passed" : "failed");
    QString a2("     ---  --  ----  ----- ---- ---- ---     ---     --- --  -");
    QString b2("  -  --- --- ------  ---    -- --      ---    --- ---   --   ");
    QString c2("                    -   - --     -- ---     --       --     -");
    ar = IntegerRegion::fromBinaryString(a2);
    br = IntegerRegion::fromBinaryString(b2);
    cr = ar.subtract(br);
    qMailLog(Messaging) << "a2 " << ar.toString();
    qMailLog(Messaging) << "b2 " << br.toString();
    qMailLog(Messaging) << "a   " << IntegerRegion::toBinaryString(ar) << " - ";
    qMailLog(Messaging) << "b   " << IntegerRegion::toBinaryString(br) << " = ";
    qMailLog(Messaging) << "a-b " << IntegerRegion::toBinaryString(cr);
    qMailLog(Messaging) << "c   " << c2;
    qMailLog(Messaging) << "IntegerRegion::subtract test7: " 
                        << ((IntegerRegion::toBinaryString(cr) == c2) ? "passed" : "failed");

    QString a3("1:7,9:15");
    QString b3("2:5,10:13,7,1,6,9,12:15");
    ar = IntegerRegion(b3);
    qMailLog(Messaging) << "b3 " << b3;
    qMailLog(Messaging) << "c3 " << ar.toString();
    qMailLog(Messaging) << "IntegerRegion::fromString test8: " 
                        << ((ar.toString() == a3) ? "passed" : "failed");


    values.clear();
    list.clear();
    values << 15555 << 15556 << 15557 << 15558 << 15559 << 15561 << 15562 << 15563 << 15565 
           << 15566 << 15567 << 15569 << 15573 << 15578 << 15579 << 15580 << 15581 << 15582 
           << 15584 << 15586 << 15587 << 15590 << 15593 << 15595 << 15596 << 15599 << 15600 
           << 15602 << 15605 << 15606 << 15607 << 15609;

    foreach (const int &v, values) {
        list << QString::number(v);
    }
    
    ir = IntegerRegion(list);
    IntegerRegion jr(ir.minimum(), ir.maximum());
    ar = jr.subtract(ir);
    QString a4("15560,15564,15568,15570:15572,15574:15577,15583,15585,15588:15589,15591:15592,15594,15597:15598,15601,15603:15604,15608");
    
    qMailLog(Messaging) << "IntegerRegion::subtractTest test9: " 
                        << ((ar.toString() == a4) ? "passed" : "failed");
    
    ir = IntegerRegion("1:20");
    jr = IntegerRegion("10:30");
    ar = jr.intersect(ir);
    QString a5("10:20");
    qMailLog(Messaging) << "IntegerRegion::intersectionTest test10: " 
                        << ((ar.toString() == a5) ? "passed" : "failed");

    ar = ir.intersect(jr);
    qMailLog(Messaging) << "IntegerRegion::intersectionTest test11: " 
                        << ((ar.toString() == a5) ? "passed" : "failed");
    
    ir = IntegerRegion("1:10");
    jr = IntegerRegion("20:30");
    ar = jr.intersect(ir);
    QString a6("");
    qMailLog(Messaging) << "IntegerRegion::intersectionTest test12: " 
                        << ((ar.toString() == a6) ? "passed" : "failed");

    ir = IntegerRegion("1:10");
    jr = IntegerRegion("20:30");
    ar = ir.intersect(jr);
    QString a7("");
    qMailLog(Messaging) << "IntegerRegion::intersectionTest test13: " 
                        << ((ar.toString() == a7) ? "passed" : "failed");

    ar = ir.intersect(ir);
    QString a8("1:10");
    qMailLog(Messaging) << "IntegerRegion::intersectionTest test14: " 
                        << ((ar.toString() == a8) ? "passed" : "failed");
    
    ir = IntegerRegion("1:4,6:8,9,10,30,1000,20000,30000");
    jr = IntegerRegion("1:30000");
    ar = ir.intersect(jr);
    qMailLog(Messaging) << "IntegerRegion::intersectionTest test15: " 
                        << ((ar.toString() == ir.toString()) ? "passed" : "failed");
    ar = jr.intersect(ir);
    qMailLog(Messaging) << "IntegerRegion::intersectionTest test16: " 
                        << ((ar.toString() == ir.toString()) ? "passed" : "failed");
    
    jr = IntegerRegion("2,5,7,100:10000,25000,26000,27000,28000");
    ar = jr.intersect(ir);
    QString a9("2,7,1000");
    qMailLog(Messaging) << "IntegerRegion::intersectionTest test17: " 
                        << ((ar.toString() == a9) ? "passed" : "failed");
    
    ar = ir.intersect(jr);
    qMailLog(Messaging) << "IntegerRegion::intersectionTest test18: " 
                        << ((ar.toString() == a9) ? "passed" : "failed");

    return 1;
}

static const int testsRun = IntegerRegion::tests();
#endif

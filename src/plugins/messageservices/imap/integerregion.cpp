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

    foreach( const IntegerRange range, mRangeList)
        result += range.second - range.first + 1;

    return result;
}

/*
  Returns a string list of integers contained in the region.
*/
QStringList IntegerRegion::toStringList() const
{
    QStringList result;
    foreach(const IntegerRange range, mRangeList) {
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
    foreach(IntegerRange range, mRangeList) {
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
            result.mRangeList.insert(a, lowerSlice);
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
            result += " ";
        result += "-";
        last = value + 1;
    }
    return result;
}

/*
  Test function.
  
  Run through some tests.
*/
void IntegerRegion::tests()
{
    QStringList list;
    list << "12" << "13" << "16" << "20" << "22" << "23" << "24" << "27" << "28" << "34";
    qMailLog(Messaging) << "Constructing from QStringList " << list;
    
    IntegerRegion ir(list);
    qMailLog(Messaging) << "IntegerRegion = " << ir.toString();
    qMailLog(Messaging) << "IntegerRegion::IntegerRegion(QStringList) test1"
                        << ((ir.toString() == "12:13,16,20,22:24,27:28,34") ? "passed" : "failed");
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
}
#endif

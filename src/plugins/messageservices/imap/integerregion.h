/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef INTEGERREGION_H
#define INTEGERREGION_H

#include <QString>
#include <QStringList>
#include <QPair>

typedef QPair<int, int> IntegerRange;

class IntegerRegion
{
public:
    IntegerRegion();
    IntegerRegion(const QStringList &uids);
    IntegerRegion(const QString &uidString);

    void clear();
    bool isEmpty() const;
    uint cardinality() const;
    QStringList toStringList() const;
    QString toString() const;

    void add(int number);
    IntegerRegion subtract(IntegerRegion) const;

    static bool isIntegerRegion(QStringList uids);

#if 0
// TODO Convert these tests to standard qunit style
    static IntegerRegion fromBinaryString(const QString &);
    static QString toBinaryString(const IntegerRegion &);
    static void tests();
#endif

private:
    // TODO: Consider using shared pointer
    QList< QPair<int, int> > mRangeList;
};

#endif

/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef QMAILDATACOMPARATOR_H
#define QMAILDATACOMPARATOR_H

namespace QMailDataComparator {

enum EqualityComparator
{
    Equal,
    NotEqual
};

enum InclusionComparator 
{
    Includes,
    Excludes
};

enum RelationComparator
{
    LessThan,
    LessThanEqual,
    GreaterThan,
    GreaterThanEqual
};

enum PresenceComparator
{
    Present,
    Absent
};

}

#endif

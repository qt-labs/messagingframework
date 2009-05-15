/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

/*!
    \namespace QMailDataComparator
    \ingroup messaginglibrary

    \brief The QMailDataComparator namespace contains types used in specifying the comparison
    of QMailStore objects with user-defined values.
*/

/*!
    \enum QMailDataComparator::EqualityComparator

    Defines the comparison operations that can be used to compare data elements 
    of QMailStore objects for equality and inequality.

    \value Equal Represents the '==' operator.
    \value NotEqual Represents the '!=' operator.
*/

/*!
    \enum QMailDataComparator::InclusionComparator

    Defines the comparison operations that can be used to compare data elements 
    of QMailStore objects for inclusion or exclusion.

    \value Includes Represents an operation in which an associated property is tested to 
                    determine whether it is equal to any of a supplied set of values.
                    Alternatively, it may be used to determine whether a single supplied
                    value is included within the associated QMailStore property.
    \value Excludes Represents an operation in which an associated property is tested to 
                    determine whether it is equal to none of a supplied set of values.
                    Alternatively, it may be used to determine whether a single supplied
                    value is not included within the associated QMailStore property.
*/

/*!
    \enum QMailDataComparator::RelationComparator

    Defines the comparison operations that can be used to compare data elements 
    of QMailStore objects, according to a specific relation.

    \value LessThan Represents the '<' operator.
    \value LessThanEqual Represents the '<=' operator.
    \value GreaterThan Represents the '>' operator.
    \value GreaterThanEqual Represents the '>= operator'.
*/

/*!
    \enum QMailDataComparator::PresenceComparator

    Defines the comparison operations that can be used to compare data elements 
    of QMailStore objects, according to presence or absence.

    \value Present Tests whether the specified property is present in the QMailStore object.
    \value Absent Tests whether the specified property is absent in the QMailStore object.
*/


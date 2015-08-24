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

/*!
    \namespace QMailDataComparator
    \ingroup messaginglibrary

    \brief The QMailDataComparator namespace contains types used in specifying the comparison
    of QMailStore objects with user-defined values.
*/

/*!
    \enum QMailDataComparator::EqualityComparator

    Defines the comparison operations that can be used to compare data elements 
    of QMailStore objects for equality and inequality. This is case sensitive.

    \value Equal Represents the '==' operator.
    \value NotEqual Represents the '!=' operator.
*/

/*!
    \enum QMailDataComparator::InclusionComparator

    Defines the comparison operations that can be used to compare data elements 
    of QMailStore objects for inclusion or exclusion. This is case insensitive.

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


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

#ifndef QMAILKEYARGUMENT_P_H
#define QMAILKEYARGUMENT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt Extended API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qmaildatacomparator.h"
#include <QDataStream>
#include <QVariantList>

namespace QMailKey {

enum Comparator
{
    LessThan,
    LessThanEqual,
    GreaterThan,
    GreaterThanEqual,
    Equal,
    NotEqual,
    Includes,
    Excludes,
    Present,
    Absent
};

enum Combiner
{
    None,
    And,
    Or
};

inline Comparator comparator(QMailDataComparator::EqualityComparator cmp)
{
    if (cmp == QMailDataComparator::Equal) {
        return Equal;
    } else { // if (cmp == QMailDataComparator::NotEqual) {
        return NotEqual;
    }
}

inline Comparator comparator(QMailDataComparator::InclusionComparator cmp)
{
    if (cmp == QMailDataComparator::Includes) {
        return Includes;
    } else { // if (cmp == QMailDataComparator::Excludes) {
        return Excludes;
    }
}

inline Comparator comparator(QMailDataComparator::RelationComparator cmp)
{
    if (cmp == QMailDataComparator::LessThan) {
        return LessThan;
    } else if (cmp == QMailDataComparator::LessThanEqual) {
        return LessThanEqual;
    } else if (cmp == QMailDataComparator::GreaterThan) {
        return GreaterThan;
    } else { // if (cmp == QMailDataComparator::GreaterThanEqual) {
        return GreaterThanEqual;
    }
}

inline Comparator comparator(QMailDataComparator::PresenceComparator cmp)
{
    if (cmp == QMailDataComparator::Present) {
        return Present;
    } else { // if (cmp == QMailDataComparator::Absent) {
        return Absent;
    }
}

inline QString stringValue(const QString &value)
{
    if (value.isNull()) {
        return QLatin1String("");
    } else {
        return value;
    }
}

}

template<typename PropertyType, typename ComparatorType = QMailKey::Comparator>
class QMailKeyArgument
{
public:
    class ValueList : public QVariantList
    {
    public:
        bool operator==(const ValueList& other) const
        {
            if (count() != other.count())
                return false;

            if (isEmpty())
                return true;

            // We can't compare QVariantList directly, since QVariant can't compare metatypes correctly
            QByteArray serialization, otherSerialization;
            {
                QDataStream serializer(&serialization, QIODevice::WriteOnly);
                serialize(serializer);

                QDataStream otherSerializer(&otherSerialization, QIODevice::WriteOnly);
                other.serialize(otherSerializer);
            }
            return (serialization == otherSerialization);
        }

        template <typename Stream> void serialize(Stream &stream) const
        {
            stream << count();
            Q_FOREACH (const QVariant& var, *this)
                stream << var;
        }

        template <typename Stream> void deserialize(Stream &stream)
        {
            clear();

            int v = 0;
            stream >> v;
            for (int i = 0; i < v; ++i) {
                QVariant var;
                stream >> var;
                append(var);
            }
        }
    };

    typedef PropertyType Property;
    typedef ComparatorType Comparator;

    Property property;
    Comparator op;
    ValueList valueList;

    QMailKeyArgument()
    {
    }

    QMailKeyArgument(Property p, Comparator c, const QVariant& v)
        : property(p),
          op(c)
    {
          valueList.append(v);
    }
    
    template<typename ListType>
    QMailKeyArgument(const ListType& l, Property p, Comparator c)
        : property(p),
          op(c)
    {
        Q_FOREACH (typename ListType::const_reference v, l)
            valueList.append(v);
    }
    
    bool operator==(const QMailKeyArgument<PropertyType, ComparatorType>& other) const
    {
        return property == other.property &&
               op == other.op &&
               valueList == other.valueList;
    }

    template <typename Stream> void serialize(Stream &stream) const
    {
        stream << property;
        stream << op;
        stream << valueList;
    }

    template <typename Stream> void deserialize(Stream &stream)
    {
        int v = 0;

        stream >> v;
        property = static_cast<Property>(v);
        stream >> v;
        op = static_cast<Comparator>(v);

        stream >> valueList;
    }
};

#endif


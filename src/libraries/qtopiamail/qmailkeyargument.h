/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
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
        return QString("");
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
            foreach (const QVariant& value, *this)
                stream << value;
        }

        template <typename Stream> void deserialize(Stream &stream)
        {
            clear();

            int v = 0;
            stream >> v;
            for (int i = 0; i < v; ++i) {
                QVariant value;
                stream >> value;
                append(value);
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
        foreach (typename ListType::const_reference v, l)
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


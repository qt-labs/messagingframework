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

#ifndef MAILKEYIMPL_P_H
#define MAILKEYIMPL_P_H

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

#include <QList>
#include <QStringList>


template<typename Key> 
class MailKeyImpl : public QSharedData
{
public:
    typedef Key KeyType;
    typedef typename Key::IdType IdType;
    typedef typename Key::Property Property;
    typedef typename Key::ArgumentType Argument;

    MailKeyImpl();
    MailKeyImpl(Property p, const QVariant &value, QMailKey::Comparator c);

    template<typename ListType>
    MailKeyImpl(const ListType &list, Property p, QMailKey::Comparator c);

    static Key negate(const Key &self);

    static Key andCombine(const Key &self, const Key &other);
    static Key orCombine(const Key &self, const Key &other);

    static const Key& andAssign(Key &self, const Key &other);
    static const Key& orAssign(Key &self, const Key &other);

    bool operator==(const MailKeyImpl &other) const;

    bool isEmpty() const;
    bool isNonMatching() const;

    static Key nonMatchingKey();

    template <typename Stream> void serialize(Stream &stream) const;
    template <typename Stream> void deserialize(Stream &stream);

    QMailKey::Combiner combiner;
    bool negated;
    QList<Argument> arguments;
    QList<Key> subKeys;
};


template<typename Key> 
MailKeyImpl<Key>::MailKeyImpl()
    : QSharedData(),
      combiner(QMailKey::None),
      negated(false)
{
}

template<typename Key> 
MailKeyImpl<Key>::MailKeyImpl(Property p, const QVariant &value, QMailKey::Comparator c)
    : QSharedData(),
      combiner(QMailKey::None),
      negated(false)
{
    arguments.append(Argument(p, c, value));
}
      
template<typename Key> 
template<typename ListType> 
MailKeyImpl<Key>::MailKeyImpl(const ListType &list, Property p, QMailKey::Comparator c)
    : QSharedData(),
      combiner(QMailKey::None),
      negated(false)
{
    if (list.isEmpty()) {
        if (c == QMailKey::Includes) {
            // Return a non-matching key
            arguments.append(Argument(Key::Id, QMailKey::Equal, QVariant(IdType())));
        } else {
            // Return an empty key
        }
    } else if (list.count() == 1) {
        // Suppress Includes/Excludes pattern matching
        if (c == QMailKey::Includes) {
            c = QMailKey::Equal;
        } else if (c == QMailKey::Excludes) {
            c = QMailKey::NotEqual;
        }
        arguments.append(Argument(p, c, list.first()));
    } else {
        arguments.append(Argument(list, p, c));
    }
}
      
template<typename Key> 
Key MailKeyImpl<Key>::negate(const Key &self)
{
    if (self.isEmpty()) {
        return nonMatchingKey();
    } else if (self.isNonMatching()) {
        return Key();
    }

    Key result(self);

    if (!self.d->arguments.isEmpty() && (self.d->arguments.first().property == Key::Custom)) {
        // Cannot allow negated custom keys, due to SQL expansion variation
        Argument &arg(result.d->arguments.first());
        if (arg.op == QMailKey::Equal) {
            arg.op = QMailKey::NotEqual;
        } else if (arg.op == QMailKey::NotEqual) {
            arg.op = QMailKey::Equal;
        } else if (arg.op == QMailKey::Excludes) {
            arg.op = QMailKey::Includes;
        } else if (arg.op == QMailKey::Includes) {
            arg.op = QMailKey::Excludes;
        } else if (arg.op == QMailKey::Present) {
            arg.op = QMailKey::Absent;
        } else if (arg.op == QMailKey::Absent) {
            arg.op = QMailKey::Present;
        }
    } else {
        result.d->negated = !self.d->negated;
    }

    return result;
}

template<typename Key> 
Key MailKeyImpl<Key>::andCombine(const Key &self, const Key &other)
{
    if (self.isNonMatching()) {
        return self;
    } else if (self.isEmpty()) {
        return other;
    } else if (other.isNonMatching()) {
        return other;
    } else if (other.isEmpty()) {
        return self;
    }

    Key result;
    result.d->combiner = QMailKey::And;

    if (self.d->combiner != QMailKey::Or && !self.d->negated && other.d->combiner != QMailKey::Or && !other.d->negated) {
        result.d->subKeys = self.d->subKeys + other.d->subKeys;
        result.d->arguments = self.d->arguments + other.d->arguments;
    } else {
        result.d->subKeys.append(self);
        result.d->subKeys.append(other); 
    }

    return result;            
}

template<typename Key> 
Key MailKeyImpl<Key>::orCombine(const Key &self, const Key &other)
{
    if (self.isNonMatching()) {
        return other;
    } else if (self.isEmpty()) {
        return (other.isNonMatching() ? self : other);
    } else if (other.isEmpty() || other.isNonMatching()) {
        return self;
    }

    Key result;
    result.d->combiner = QMailKey::Or;

    if (self.d->combiner != QMailKey::And && !self.d->negated && other.d->combiner != QMailKey::And && !other.d->negated) {
        result.d->subKeys = self.d->subKeys + other.d->subKeys;
        result.d->arguments = self.d->arguments + other.d->arguments;
    } else {
        result.d->subKeys.append(self);    
        result.d->subKeys.append(other);
    }

    return result;
}

template<typename Key> 
const Key& MailKeyImpl<Key>::andAssign(Key &self, const Key &other)
{
    self = (self & other);
    return self;
}

template<typename Key> 
const Key& MailKeyImpl<Key>::orAssign(Key &self, const Key &other)
{
    self = (self | other);
    return self;
}

template<typename Key> 
bool MailKeyImpl<Key>::operator==(const MailKeyImpl &other) const
{
    return ((combiner == other.combiner) && (negated == other.negated) && (subKeys == other.subKeys) && (arguments == other.arguments));
}

template<typename Key> 
bool MailKeyImpl<Key>::isEmpty() const
{
    return ((combiner == QMailKey::None) && (negated == false) && subKeys.isEmpty() && arguments.isEmpty());
}

template<typename Key> 
bool MailKeyImpl<Key>::isNonMatching() const
{
    if ((arguments.count() == 1) &&
    (arguments.first().property == Key::Id) &&
    (arguments.first().op == QMailKey::Equal) &&
    (arguments.first().valueList.count() == 1)) {
        QVariant v = arguments.first().valueList.first();
        return (v.canConvert<IdType>() && !v.value<IdType>().isValid());
    }
    return false;
}

template<typename Key> 
Key MailKeyImpl<Key>::nonMatchingKey()
{
    return Key(Key::Id, IdType(), QMailKey::Equal);
}

template<typename Key> 
template <typename Stream> 
void MailKeyImpl<Key>::serialize(Stream &stream) const
{
    stream << combiner;          
    stream << negated;

    stream << arguments.count();
    foreach (const Argument& a, arguments)
        a.serialize(stream);

    stream << subKeys.count();
    foreach (const Key& k, subKeys)
        k.serialize(stream);
}

template<typename Key> 
template <typename Stream> 
void MailKeyImpl<Key>::deserialize(Stream &stream)
{
    int i = 0;
    stream >> i;
    combiner = static_cast<QMailKey::Combiner>(i);
    stream >> negated;

    stream >> i;
    for (int j = 0; j < i; ++j) {
        Argument a;
        a.deserialize(stream);
        arguments.append(a);
    }

    stream >> i;
    for (int j = 0; j < i; ++j) {
        Key subKey;
        subKey.deserialize(stream);
        subKeys.append(subKey);
    }
}

#endif


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

#ifndef MAILSORTKEYIMPL_H
#define MAILSORTKEYIMPL_H

#include "qmailglobal.h"
#include <QSharedData>
#include <QtGlobal>
#include <QPair>


template<typename Key>
class MailSortKeyImpl : public QSharedData
{
public:
    typedef Key KeyType;
    typedef typename Key::Property Property;
    typedef typename Key::ArgumentType Argument;

    MailSortKeyImpl();
    MailSortKeyImpl(Property p, Qt::SortOrder order, quint64 mask);
    MailSortKeyImpl(const QList<Argument> &args);

    bool operator==(const MailSortKeyImpl& other) const;
    bool operator!=(const MailSortKeyImpl& other) const;

    MailSortKeyImpl& operator=(const MailSortKeyImpl& other);

    bool isEmpty() const;

    const QList<Argument> &arguments() const;

    template <typename Stream> void serialize(Stream &stream) const;
    template <typename Stream> void deserialize(Stream &stream);

    QList<Argument> _arguments;
};


template<typename Key>
MailSortKeyImpl<Key>::MailSortKeyImpl()
    : QSharedData()
{
}

template<typename Key>
MailSortKeyImpl<Key>::MailSortKeyImpl(Property p, Qt::SortOrder order, quint64 mask)
    : QSharedData()
{
    _arguments.append(Argument(p, order, mask));
}

template<typename Key>
MailSortKeyImpl<Key>::MailSortKeyImpl(const QList<Argument> &args)
    : QSharedData()
{
    _arguments = args;
}

template<typename Key>
bool MailSortKeyImpl<Key>::operator==(const MailSortKeyImpl& other) const
{
    return _arguments == other._arguments;
}

template<typename Key>
bool MailSortKeyImpl<Key>::operator!=(const MailSortKeyImpl& other) const
{
   return !(*this == other); 
}

template<typename Key>
bool MailSortKeyImpl<Key>::isEmpty() const
{
    return _arguments.isEmpty();
}

template<typename Key>
const QList<typename MailSortKeyImpl<Key>::Argument> &MailSortKeyImpl<Key>::arguments() const
{
    return _arguments;
}

template<typename Key>
template <typename Stream> 
void MailSortKeyImpl<Key>::serialize(Stream &stream) const
{
    stream << _arguments.count();
    foreach (const Argument& a, _arguments) {
        a.serialize(stream);
    }
}

template<typename Key>
template <typename Stream> 
void MailSortKeyImpl<Key>::deserialize(Stream &stream)
{
    int i = 0;
    stream >> i;
    for (int j = 0; j < i; ++j) {
        Argument a;
        a.deserialize(stream);
        _arguments.append(a);
    }
}

#endif


/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qmailthreadsortkey.h"
#include "qmailthreadsortkey_p.h"

/*!
    \class QMailThreadSortKey

    \preliminary
    \brief The QMailThreadSortKey class defines the parameters used for sorting a subset of
    threads from the mail store.
    \ingroup messaginglibrary

    \sa QMailStore, QMailThreadKey
*/

/*!
    \enum QMailThreadSortKey::Property

    This enum type describes the sortable data properties of a QMailThread.

    \value Id The ID of the thread.
    \value ServerUid The ServerUid of the thread.
*/

/*!
    \typedef QMailThreadSortKey::ArgumentType

    Defines the type used to represent a single sort criterion of a message sort key.
*/

/*!
    Create a QMailThreadSortyKey.

    A default-constructed key (one for which isEmpty() returns true) sorts no messages.

    The result of combining an empty key with a non-empty key is the same as the original
    non-empty key.

    The result of combining two empty keys is an empty key.
*/
QMailThreadSortKey::QMailThreadSortKey()
    : d(new QMailThreadSortKeyPrivate())
{
}

/*! \internal */
QMailThreadSortKey::QMailThreadSortKey(Property p, Qt::SortOrder order, quint64 mask)
    : d(new QMailThreadSortKeyPrivate(p, order, mask))
{
}

/*! \internal */
QMailThreadSortKey::QMailThreadSortKey(const QList<QMailThreadSortKey::ArgumentType> &args)
    : d(new QMailThreadSortKeyPrivate(args))
{
}

/*!
    Create a copy of the QMailThreadSortKey \a other.
*/
QMailThreadSortKey::QMailThreadSortKey(const QMailThreadSortKey& other)
    : d(new QMailThreadSortKeyPrivate())
{
    this->operator=(other);
}

/*!
    Destroys this QMailThreadSortKey.
*/
QMailThreadSortKey::~QMailThreadSortKey()
{
}

/*!
    Returns a key that is the logical AND of this key and the value of key \a other.
*/
QMailThreadSortKey QMailThreadSortKey::operator&(const QMailThreadSortKey& other) const
{
    return QMailThreadSortKey(d->arguments() + other.d->arguments());
}

/*!
    Performs a logical AND with this key and the key \a other and assigns the result
    to this key.
*/
QMailThreadSortKey& QMailThreadSortKey::operator&=(const QMailThreadSortKey& other)
{
    *this = *this & other;
    return *this;
}

/*!
    Returns \c true if the value of this key is the same as the key \a other. Returns
    \c false otherwise.
*/
bool QMailThreadSortKey::operator==(const QMailThreadSortKey& other) const
{
    return (*d == *other.d);
}

/*!
    Returns \c true if the value of this key is not the same as the key \a other. Returns
    \c false otherwise.
*/
bool QMailThreadSortKey::operator!=(const QMailThreadSortKey& other) const
{
   return !(*this == other);
}

/*!
    Assign the value of the QMailThreadSortKey \a other to this.
*/
QMailThreadSortKey& QMailThreadSortKey::operator=(const QMailThreadSortKey& other)
{
    d = other.d;
    return *this;
}

/*!
    Returns true if the key remains empty after default construction; otherwise returns false.
*/
bool QMailThreadSortKey::isEmpty() const
{
    return d->isEmpty();
}

/*!
    Returns the list of arguments to this QMailThreadSortKey.
*/
const QList<QMailThreadSortKey::ArgumentType> &QMailThreadSortKey::arguments() const
{
    return d->arguments();
}

/*!
    \fn QMailThreadSortKey::serialize(Stream &stream) const

    Writes the contents of a QMailThreadSortKey to a \a stream.
*/
template <typename Stream> void QMailThreadSortKey::serialize(Stream &stream) const
{
    d->serialize(stream);
}

/*!
    \fn QMailThreadSortKey::deserialize(Stream &stream)

    Reads the contents of a QMailThreadSortKey from \a stream.
*/
template <typename Stream> void QMailThreadSortKey::deserialize(Stream &stream)
{
    d->deserialize(stream);
}

/*!
    Returns a key that sorts threads by their identifiers, according to \a order.

    \sa QMailThread::id()
*/
QMailThreadSortKey QMailThreadSortKey::id(Qt::SortOrder order)
{
    return QMailThreadSortKey(Id, order);
}


/*!
    Returns a key that sorts threads by their server uid string according to \a order.

    \sa QMailThread::serverUid()
*/
QMailThreadSortKey QMailThreadSortKey::serverUid(Qt::SortOrder order)
{
    return QMailThreadSortKey(ServerUid, order);
}



Q_IMPLEMENT_USER_METATYPE(QMailThreadSortKey)


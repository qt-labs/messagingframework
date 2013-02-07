/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
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
    \value ParentAccountId The ID of the account thread is in.
    \value ServerUid The ServerUid of the thread.
    \value UnreadCount The UnreadCount of the thread.
    \value MessageCount The MessageCount of the thread.
    \value Subject The Subject of the thread.
    \value Senders The Senders of the thread.
    \value Preview The Preview of the thread.
    \value StartedDate The StartedDate of the thread.
    \value LastDate The LastDate of the thread.
    \value Status The Status of the thread.
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
    Returns a key that sorts threads by their account identifiers, according to \a order.

    \sa QMailThread::parentAccountId()
*/
QMailThreadSortKey QMailThreadSortKey::parentAccountId(Qt::SortOrder order)
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

/*!
    Returns a key that sorts threads by their unread count according to \a order.

    \sa QMailThread::unreadCount()
*/
QMailThreadSortKey QMailThreadSortKey::unreadCount(Qt::SortOrder order)
{
    return QMailThreadSortKey(UnreadCount, order);
}

/*!
    Returns a key that sorts threads by their message count according to \a order.

    \sa QMailThread::messageCount()
*/
QMailThreadSortKey QMailThreadSortKey::messageCount(Qt::SortOrder order)
{
    return QMailThreadSortKey(MessageCount, order);
}

/*!
    Returns a key that sorts threads by their subjects, according to \a order.

    \sa QMailThread::subject()
*/
QMailThreadSortKey QMailThreadSortKey::subject(Qt::SortOrder order)
{
    return QMailThreadSortKey(Subject, order);
}

/*!
    Returns a key that sorts threads by their senders, according to \a order.

    \sa QMailThread::senders()
*/
QMailThreadSortKey QMailThreadSortKey::senders(Qt::SortOrder order)
{
    return QMailThreadSortKey(Senders, order);
}

/*!
    Returns a key that sorts threads by their previews, according to \a order.

    \sa QMailThread::preview()
*/
QMailThreadSortKey QMailThreadSortKey::preview(Qt::SortOrder order)
{
    return QMailThreadSortKey(Preview, order);
}

/*!
    Returns a key that sorts threads by their last dates, according to \a order.

    \sa QMailThread::lastDate()
*/
QMailThreadSortKey QMailThreadSortKey::lastDate(Qt::SortOrder order)
{
    return QMailThreadSortKey(LastDate, order);
}

/*!
    Returns a key that sorts threads by their start dates, according to \a order.

    \sa QMailThread::startedDate()
*/
QMailThreadSortKey QMailThreadSortKey::startedDate(Qt::SortOrder order)
{
    return QMailThreadSortKey(StartedDate, order);
}

/*!
    Returns a key that sorts threads by comparing their status value bitwise ANDed with \a mask, according to \a order.

    \sa QMailThread::status()
*/
QMailThreadSortKey QMailThreadSortKey::status(quint64 mask, Qt::SortOrder order)
{
    return QMailThreadSortKey(Status, order, mask);
}



Q_IMPLEMENT_USER_METATYPE(QMailThreadSortKey)


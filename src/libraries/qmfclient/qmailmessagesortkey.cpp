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

#include "qmailmessagesortkey.h"
#include "qmailmessagesortkey_p.h"

/*!
    \class QMailMessageSortKey

    \preliminary
    \brief The QMailMessageSortKey class defines the parameters used for sorting a subset of 
    queried messages from the mail store.
    \ingroup messaginglibrary

    A QMailMessageSortKey is composed of a message property to sort and a sort order. 
    The QMailMessageSortKey class is used in conjunction with the QMailStore::queryMessages() 
    function to sort message results according to the criteria defined by the sort key.

    For example:
    To create a query for all messages sorted by their timestamp in decending order:
    \code
    QMailMessageSortKey sortKey(QMailMessageSortKey::timeStamp(Qt::DescendingOrder));
    QMailIdList results = QMailStore::instance()->queryMessages(QMailMessageKey(), sortKey);
    \endcode
    
    \sa QMailStore, QMailMessageKey
*/

/*!
    \enum QMailMessageSortKey::Property

    This enum type describes the sortable data properties of a QMailFolder.

    \value Id The ID of the message.
    \value Type The type of the message.
    \value ParentFolderId The parent folder ID this message is contained in.
    \value Sender The message sender address string.
    \value Recipients The message recipient address string.
    \value Subject The message subject string.
    \value TimeStamp The message origination timestamp.
    \value ReceptionTimeStamp The message reception timestamp.
    \value Status The message status flags.
    \value ServerUid The IMAP server UID of the message.
    \value ParentAccountId The ID of the account the mesasge was downloaded from.
    \value Size The size of the message.
    \value ContentType The type of data contained within the message.
    \value PreviousParentFolderId The parent folder ID this message was contained in, prior to moving to the current parent folder.
    \value CopyServerUid The server uid of the message this is a copy of
    \value ListId The name of the list
    \value RestoreFolderId The folderId of where the message should be restored to
    \value RfcId The message rfcId, that is the message-id header field value.
    \value ParentThreadId The QMailThreadId of the thread (conversation) of the message.
*/

/*!
    \typedef QMailMessageSortKey::ArgumentType
    
    Defines the type used to represent a single sort criterion of a message sort key.
*/

/*!
    Create a QMailMessageSortKey with specifying matching parameters.

    A default-constructed key (one for which isEmpty() returns true) sorts no messages. 

    The result of combining an empty key with a non-empty key is the same as the original 
    non-empty key.

    The result of combining two empty keys is an empty key.
*/
QMailMessageSortKey::QMailMessageSortKey()
    : d(new QMailMessageSortKeyPrivate())
{
}

/*! \internal */
QMailMessageSortKey::QMailMessageSortKey(Property p, Qt::SortOrder o, quint64 mask)
    : d(new QMailMessageSortKeyPrivate(p, o, mask))
{
}

/*! \internal */
QMailMessageSortKey::QMailMessageSortKey(const QList<QMailMessageSortKey::ArgumentType> &args)
    : d(new QMailMessageSortKeyPrivate(args))
{
}

/*!
    Create a copy of the QMailMessageSortKey \a other.
*/
QMailMessageSortKey::QMailMessageSortKey(const QMailMessageSortKey& other)
    : d(new QMailMessageSortKeyPrivate())
{
    this->operator=(other);
}

/*!
    Destroys this QMailMessageSortKey.
*/
QMailMessageSortKey::~QMailMessageSortKey()
{
}

/*!
    Returns a key that is the logical AND of this key and the value of key \a other.
*/
QMailMessageSortKey QMailMessageSortKey::operator&(const QMailMessageSortKey& other) const
{
    return QMailMessageSortKey(d->arguments() + other.d->arguments());
}

/*!
    Performs a logical AND with this key and the key \a other and assigns the result
    to this key.
*/
QMailMessageSortKey& QMailMessageSortKey::operator&=(const QMailMessageSortKey& other)
{
    *this = *this & other;
    return *this;
}

/*!
    Returns \c true if the value of this key is the same as the key \a other. Returns 
    \c false otherwise.
*/
bool QMailMessageSortKey::operator==(const QMailMessageSortKey& other) const
{
    return (*d == *other.d);
}

/*!
    Returns \c true if the value of this key is not the same as the key \a other. Returns
    \c false otherwise.
*/
bool QMailMessageSortKey::operator!=(const QMailMessageSortKey& other) const
{
   return !(*this == other); 
}

/*!
    Assign the value of the QMailMessageSortKey \a other to this.
*/
QMailMessageSortKey& QMailMessageSortKey::operator=(const QMailMessageSortKey& other)
{
    d = other.d;
    return *this;
}

/*!
    Returns true if the key remains empty after default construction; otherwise returns false.
*/
bool QMailMessageSortKey::isEmpty() const
{
    return d->isEmpty();
}

/*!
    Returns the list of arguments to this QMailMessageSortKey.
*/
const QList<QMailMessageSortKey::ArgumentType> &QMailMessageSortKey::arguments() const
{
    return d->arguments();
}

/*!
    \fn QMailMessageSortKey::serialize(Stream &stream) const

    Writes the contents of a QMailMessageSortKey to a \a stream.
*/
template <typename Stream> void QMailMessageSortKey::serialize(Stream &stream) const
{
    d->serialize(stream);
}

/*!
    \fn QMailMessageSortKey::deserialize(Stream &stream)

    Reads the contents of a QMailMessageSortKey from \a stream.
*/
template <typename Stream> void QMailMessageSortKey::deserialize(Stream &stream)
{
    d->deserialize(stream);
}

/*!
    Returns a key that sorts messages by their identifiers, according to \a order.

    \sa QMailMessage::id()
*/
QMailMessageSortKey QMailMessageSortKey::id(Qt::SortOrder order)
{
    return QMailMessageSortKey(Id, order);
}

/*!
    Returns a key that sorts messages by their message type, according to \a order.

    \sa QMailMessage::messageType()
*/
QMailMessageSortKey QMailMessageSortKey::messageType(Qt::SortOrder order)
{
    return QMailMessageSortKey(Type, order);
}

/*!
    Returns a key that sorts messages by their parent folders' identifiers, according to \a order.

    \sa QMailMessage::parentFolderId()
*/
QMailMessageSortKey QMailMessageSortKey::parentFolderId(Qt::SortOrder order)
{
    return QMailMessageSortKey(ParentFolderId, order);
}

/*!
    Returns a key that sorts messages by the address from which they were sent, according to \a order.

    \sa QMailMessage::from()
*/
QMailMessageSortKey QMailMessageSortKey::sender(Qt::SortOrder order)
{
    return QMailMessageSortKey(Sender, order);
}

/*!
    Returns a key that sorts messages by the addresses to which they were sent, according to \a order.

    \sa QMailMessage::to()
*/
QMailMessageSortKey QMailMessageSortKey::recipients(Qt::SortOrder order)
{
    return QMailMessageSortKey(Recipients, order);
}

/*!
    Returns a key that sorts messages by their subject, according to \a order.

    \sa QMailMessage::subject()
*/
QMailMessageSortKey QMailMessageSortKey::subject(Qt::SortOrder order)
{
    return QMailMessageSortKey(Subject, order);
}

/*!
    Returns a key that sorts messages by their origination timestamp, according to \a order.

    \sa QMailMessage::date()
*/
QMailMessageSortKey QMailMessageSortKey::timeStamp(Qt::SortOrder order)
{
    return QMailMessageSortKey(TimeStamp, order);
}

/*!
    Returns a key that sorts messages by their reception timestamp, according to \a order.

    \sa QMailMessage::receivedDate()
*/
QMailMessageSortKey QMailMessageSortKey::receptionTimeStamp(Qt::SortOrder order)
{
    return QMailMessageSortKey(ReceptionTimeStamp, order);
}

/*!
    Returns a key that sorts messages by their server identifiers, according to \a order.

    \sa QMailMessage::serverUid()
*/
QMailMessageSortKey QMailMessageSortKey::serverUid(Qt::SortOrder order)
{
    return QMailMessageSortKey(ServerUid, order);
}

/*!
    Returns a key that sorts messages by their size, according to \a order.

    \sa QMailMessage::size()
*/
QMailMessageSortKey QMailMessageSortKey::size(Qt::SortOrder order)
{
    return QMailMessageSortKey(Size, order);
}

/*!
    Returns a key that sorts messages by their parent accounts' identifiers, according to \a order.

    \sa QMailMessage::parentAccountId()
*/
QMailMessageSortKey QMailMessageSortKey::parentAccountId(Qt::SortOrder order)
{
    return QMailMessageSortKey(ParentAccountId, order);
}

/*!
    Returns a key that sorts messages by their content types, according to \a order.

    \sa QMailMessage::content()
*/
QMailMessageSortKey QMailMessageSortKey::contentType(Qt::SortOrder order)
{
    return QMailMessageSortKey(ContentType, order);
}

/*!
    Returns a key that sorts messages by their previous parent folders' identifiers, according to \a order.

    \sa QMailMessage::previousParentFolderId()
*/
QMailMessageSortKey QMailMessageSortKey::previousParentFolderId(Qt::SortOrder order)
{
    return QMailMessageSortKey(PreviousParentFolderId, order);
}

/*!
    Returns a key that sorts messages by their copy server identifiers, according to \a order.

    \sa QMailMessage::copyServerUid()
*/
QMailMessageSortKey QMailMessageSortKey::copyServerUid(Qt::SortOrder order)
{
    return QMailMessageSortKey(CopyServerUid, order);
}

/*!
    Returns a key that sorts messages by their restore folder identifiers, according to \a order.

    \sa QMailMessage::restoreFolderId()
*/
QMailMessageSortKey QMailMessageSortKey::restoreFolderId(Qt::SortOrder order)
{
    return QMailMessageSortKey(RestoreFolderId, order);
}

/*!
    Returns a key that sorts messages by list identifier according to \a order.

    \sa QMailMessage::listId()
*/
QMailMessageSortKey QMailMessageSortKey::listId(Qt::SortOrder order)
{
    return QMailMessageSortKey(ListId, order);
}

/*!
    Returns a key that sorts messages by message-id headerfield according to \a order.

    \sa QMailMessage::listId()
*/
QMailMessageSortKey QMailMessageSortKey::rfcId(Qt::SortOrder order)
{
    return QMailMessageSortKey(RfcId, order);
}

/*!
    Returns a key that sorts messages by comparing their status value bitwise ANDed with \a mask, according to \a order.

    \sa QMailMessage::status()
*/
QMailMessageSortKey QMailMessageSortKey::status(quint64 mask, Qt::SortOrder order)
{
    return QMailMessageSortKey(Status, order, mask);
}

        
Q_IMPLEMENT_USER_METATYPE(QMailMessageSortKey)


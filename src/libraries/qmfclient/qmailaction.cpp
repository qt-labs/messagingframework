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

#include "qmailaction.h"
#include "qmailserviceaction.h"

class QMailActionDataPrivate : public QSharedData
{
public:
    QMailActionDataPrivate();
    QMailActionDataPrivate(QMailActionId id, QMailServerRequestType requestType, uint current, uint total, const QMailServiceAction::Status &status);
    QMailActionDataPrivate(const QMailActionDataPrivate& other);
    ~QMailActionDataPrivate();

    bool operator==(const QMailActionDataPrivate& other) const;

    template <typename Stream>
    void serialize(Stream &stream) const;

    template <typename Stream>
    void deserialize(Stream &stream);

    QMailActionId _id;
    QMailServerRequestType _requestType;
    uint _current;
    uint _total;
    QMailServiceAction::Status _status;    
};

QMailActionDataPrivate::QMailActionDataPrivate()
    :_id(0),
     _requestType(AcknowledgeNewMessagesRequestType),
     _current(0),
     _total(0),
     _status(QMailServiceAction::Status(QMailServiceAction::Status::ErrNoError, QString(), QMailAccountId(), QMailFolderId(), QMailMessageId()))
{
}

QMailActionDataPrivate::QMailActionDataPrivate(QMailActionId id, QMailServerRequestType rt, uint c, uint t, const QMailServiceAction::Status &s)
    :_id(id),
     _requestType(rt),
     _current(c),
     _total(t),
     _status(s)
{
}                                 

QMailActionDataPrivate::QMailActionDataPrivate(const QMailActionDataPrivate& other) 
    : QSharedData(other)
{
    _id = other._id;
    _requestType = other._requestType;
    _current = other._current;
    _total = other._total;
    _status = other._status;
}

QMailActionDataPrivate::~QMailActionDataPrivate()
{
}

bool QMailActionDataPrivate::operator==(const QMailActionDataPrivate& other) const
{
    return (_id == other._id &&
            _requestType == other._requestType &&
            _current == other._current &&
            _total == other._total &&
            _status.errorCode == other._status.errorCode &&
            _status.text == other._status.text &&
            _status.accountId == other._status.accountId &&
            _status.folderId == other._status.folderId &&
            _status.messageId == other._status.messageId);
}

template <typename Stream> 
void QMailActionDataPrivate::serialize(Stream &stream) const
{
    stream << _id << _requestType << _current << _total << _status;
}

template <typename Stream> 
void QMailActionDataPrivate::deserialize(Stream &stream)
{
    stream >> _id >> _requestType >> _current >> _total >> _status;
}

/*!
    \class QMailActionData

    \brief The QMailActionData class provides an interface for accessing service action
    data
    \ingroup messaginglibrary

    \sa QMailActionInfo
*/

/*!
    Constructs an empty QMailActionData object.
*/
QMailActionData::QMailActionData()
{
    d = new QMailActionDataPrivate();
}

/*!
    Constructs a QMailActionData object with the given \a id, \a requestType, 
    current progress \a progressCurrent, total progress \a progressTotal,
    \a errorCode, \a text, \a accountId, \a folderId and \a messageId.
*/
QMailActionData::QMailActionData(QMailActionId id,
                                 QMailServerRequestType requestType,
                                 uint progressCurrent,
                                 uint progressTotal,
                                 int errorCode,
                                 const QString &text,
                                 const QMailAccountId &accountId,
                                 const QMailFolderId &folderId,
                                 const QMailMessageId &messageId)
{
    const QMailServiceAction::Status status(QMailServiceAction::Status::ErrorCode(errorCode), text, accountId, folderId, messageId);
    d = new QMailActionDataPrivate(id, requestType, progressCurrent, progressTotal, status);
}

/*! \internal */
QMailActionData::QMailActionData(const QMailActionData& other)
{
    this->operator=(other);
}

/*!
    Returns the action id of the QMailActionData object.
*/
QMailActionId QMailActionData::id() const
{
    return d->_id;
}

/*!
    Returns the requestType of the QMailActionData object.
*/
QMailServerRequestType QMailActionData::requestType() const
{
    return d->_requestType;
}

/*!
    Returns the current progress of the QMailActionData object.
*/
uint QMailActionData::progressCurrent() const
{
    return d->_current;
}

/*!
    Returns the total (maximum) progress of the QMailActionData object.
*/
uint QMailActionData::progressTotal() const
{
    return d->_total;
}

/*!
    Returns the status error code of the QMailActionData object.
*/
int QMailActionData::errorCode() const
{
    return d->_status.errorCode;
}

/*!
    Returns the status text of the QMailActionData object.
*/
QString QMailActionData::text() const
{
    return d->_status.text;
}

/*!
    Returns the account id of the QMailActionData object.
*/
QMailAccountId QMailActionData::accountId() const
{
    return d->_status.accountId;
}

/*!
    Returns the folder id of the QMailActionData object.
*/
QMailFolderId QMailActionData::folderId() const
{
    return d->_status.folderId;
}

/*!
    Returns the message id of the QMailActionData object.
*/
QMailMessageId QMailActionData::messageId() const
{
    return d->_status.messageId;
}

/*!
    Destroys a QMailActionData object.
*/
QMailActionData::~QMailActionData()
{
}

/*! \internal */
const QMailActionData& QMailActionData::operator= (const QMailActionData& other)
{
    d = other.d;
    return *this;
}

/*! \internal */
bool QMailActionData::operator== (const QMailActionData& other) const
{
    return d->operator==(*other.d);
}

/*! \internal */
bool QMailActionData::operator!= (const QMailActionData& other) const
{
    return !(d->operator==(*other.d));
}

/*! 
    \fn QMailActionData::serialize(Stream&) const
    \internal 
*/
template <typename Stream> 
void QMailActionData::serialize(Stream &stream) const
{
    d->serialize(stream);
}

/*! 
    \fn QMailActionData::deserialize(Stream&)
    \internal 
*/
template <typename Stream> 
void QMailActionData::deserialize(Stream &stream)
{
    d->deserialize(stream);
}

Q_IMPLEMENT_USER_METATYPE_ENUM(QMailServerRequestType)
Q_IMPLEMENT_USER_METATYPE(QMailActionData)
Q_IMPLEMENT_USER_METATYPE_TYPEDEF(QMailActionDataList, QMailActionDataList)


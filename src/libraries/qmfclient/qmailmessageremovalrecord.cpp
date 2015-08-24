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

#include "qmailmessageremovalrecord.h"

class QMailMessageRemovalRecordPrivate : public QSharedData
{
public:
    QMailMessageRemovalRecordPrivate():QSharedData(){};
    QMailMessageRemovalRecordPrivate(const QMailAccountId& parentAccountId, 
                                     const QString& serverUid,
                                     const QMailFolderId& parentFolderId)
        : QSharedData(), 
          parentAccountId(parentAccountId), 
          serverUid(serverUid), 
          parentFolderId(parentFolderId)
    {
    };

    ~QMailMessageRemovalRecordPrivate(){};

    QMailAccountId parentAccountId;
    QString serverUid;
    QMailFolderId parentFolderId;
};


/*!
    \class QMailMessageRemovalRecord

    \preliminary
    \ingroup messaginglibrary

    \brief The QMailMessageRemovalRecord class represents the record of a message which has been 
    removed from the mail store.

    QMailMessageRemovalRecord represents messages that have been previously removed, so that the 
    message server can finalize its interest in the message with an external service, after
    the message has been removed locally.  The removal record contains only the information 
    needed to identify the message to the external service.

    Message removal records should be purged from the mail store once the message server has 
    finalized its interest in the message with the external service.

    \sa QMailStore::messageRemovalRecords(), QMailStore::purgeMessageRemovalRecords()
*/

/*! \internal */
QMailMessageRemovalRecord::QMailMessageRemovalRecord()
:
    d(new QMailMessageRemovalRecordPrivate())
{
}

/*!
    Constructs a removal record which is a copy of \a other.
*/
QMailMessageRemovalRecord::QMailMessageRemovalRecord(const QMailMessageRemovalRecord& other)
{
    d = other.d;
}

/*!
    Constructs a removal record for the message identified by the UID \a serverUid in the
    folder \a parentFolderId, in the account identified by \a parentAccountId.
*/
QMailMessageRemovalRecord::QMailMessageRemovalRecord(const QMailAccountId& parentAccountId,
                                                     const QString& serverUid,
                                                     const QMailFolderId& parentFolderId)
    : d(new QMailMessageRemovalRecordPrivate(parentAccountId, serverUid, parentFolderId))
{
}

/*! \internal */
QMailMessageRemovalRecord::~QMailMessageRemovalRecord()
{
}

/*! \internal */
QMailMessageRemovalRecord& QMailMessageRemovalRecord::operator=(const QMailMessageRemovalRecord& other)
{
    if(&other != this)
        d = other.d;
    return *this;
}

/*!
    Returns the identifier of the account owning this removal record.
*/
QMailAccountId QMailMessageRemovalRecord::parentAccountId() const
{
    return d->parentAccountId;
}

/*!
    Sets the identifier of the message removal record's owner account to \a id.
*/
void QMailMessageRemovalRecord::setParentAccountId(const QMailAccountId& id)
{
    d->parentAccountId = id;
}

/*!
    Returns the UID at the external service of the removed message.
*/
QString QMailMessageRemovalRecord::serverUid() const
{
    return d->serverUid;
}

/*!
    Sets the UID at the external service of the removed message to \a serverUid.
*/
void QMailMessageRemovalRecord::setServerUid(const QString& serverUid)
{
    d->serverUid = serverUid;
}

/*!
    Returns the identifier of the folder in which the removed message was located.
*/
QMailFolderId QMailMessageRemovalRecord::parentFolderId() const
{
    return d->parentFolderId;
}

/*!
    Sets the identifier of the folder in which the removed message was located to \a parentFolderId.
*/
void QMailMessageRemovalRecord::setParentFolderId(const QMailFolderId& parentFolderId)
{
    d->parentFolderId = parentFolderId;
}


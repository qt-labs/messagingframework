/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
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


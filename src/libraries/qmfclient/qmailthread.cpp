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

#include "qmailthread.h"
#include "qmailstore.h"
#include "qmaillog.h"


class QMailThreadPrivate
{
public:
    QMailThreadPrivate()
        : messageCount(0),
          unreadCount(0)
    {
    }

    QMailThreadId id;
    uint messageCount;
    uint unreadCount;
    QString serverUid;

    QMap<QString, QString> customFields;
    bool customFieldsModified;

    QString customField(const QString &name) const
    {
        QMap<QString, QString>::const_iterator it = customFields.find(name);
        if (it != customFields.end()) {
            return *it;
        }

        return QString();
    }

    void setCustomField(const QString &name, const QString &value)
    {
        QMap<QString, QString>::iterator it = customFields.find(name);
        if (it != customFields.end()) {
            if (*it != value) {
                *it = value;
                customFieldsModified = true;
            }
        } else {
            customFields.insert(name, value);
            customFieldsModified = true;
        }
    }

    void setCustomFields(const QMap<QString, QString> &fields)
    {
        QMap<QString, QString>::const_iterator it = fields.begin(), end = fields.end();
        for ( ; it != end; ++it)
            setCustomField(it.key(), it.value());
    }

    void removeCustomField(const QString &name)
    {
        QMap<QString, QString>::iterator it = customFields.find(name);
        if (it != customFields.end()) {
            customFields.erase(it);
            customFieldsModified = true;
        }
    }
};

/*!
    \class QMailThread

    \preliminary
    \brief The QMailThread class represents a thread of mail messages in the mail store.
    \ingroup messaginglibrary

    QMailThread represents a thread of mail messages, either defined internally for
    application use, or to represent a folder object held by an external message
    service, such as an IMAP account.

    A QMailThread object has an optional parent of the same type, allowing folders
    to be arranged in tree structures.  Messages may be associated with folders,
    allowing for simple classification and access by their
    \l{QMailMessage::parentFolderId()}{parentFolderId} property.

    \sa QMailMessage, QMailStore::thread()
*/

/*!
  Constructor that creates an empty and invalid \c QMailThread.
  An empty thread is one which has no id or messages account.
  An invalid folder does not exist in the database and has an invalid id.
*/

QMailThread::QMailThread()
    : d(new QMailThreadPrivate)
{
}

/*!
  Constructor that creates a QMailThread by loading the data from the message store as
  specified by the QMailThreadId \a id. If the thread does not exist in the message store,
  then this constructor will create an empty and invalid QMailThread.
*/

QMailThread::QMailThread(const QMailThreadId& id)
{
    QMailThread thread(QMailStore::instance()->thread(id));
    d = thread.d;
    thread.d = 0;
}

/*!
  Creates a copy of the \c QMailThread object \a other.
*/

QMailThread::QMailThread(const QMailThread& other)
    : d(new QMailThreadPrivate(*other.d))
{
}


/*!
  Destroys the \c QMailThread object.
*/

QMailThread::~QMailThread()
{
    delete d;
}

/*!
  Assigns the value of the \c QMailThread object \a other to this.
*/

QMailThread& QMailThread::operator=(const QMailThread& other)
{
    d = other.d;
    return *this;
}

/*!
  Returns the \c ID of the \c QMailThread object. A \c QMailThread with an invalid ID
  is one which does not yet exist on the message store.
*/

QMailThreadId QMailThread::id() const
{
    return d->id;
}

/*!
  Sets the ID of this folder to \a id
*/

void QMailThread::setId(const QMailThreadId& id)
{
    d->id = id;
}

/*!
  Returns the path of the folder.
*/
QString QMailThread::serverUid() const
{
    return d->serverUid;
}

/*!
  Sets the path of this folder within the parent account to \a path.
*/

void QMailThread::setServerUid(const QString& serverUid)
{
    d->serverUid = serverUid;
}

uint QMailThread::unreadCount() const
{
    return d->unreadCount;
}

uint QMailThread::messageCount() const
{
    return d->messageCount;
}

void QMailThread::setMessageCount(uint value)
{
    d->messageCount = value;
}

void QMailThread::setUnreadCount(uint value)
{
    d->unreadCount = value;
}

/*!
    Returns the value recorded in the custom field named \a name.

    \sa setCustomField(), customFields()
*/
QString QMailThread::customField(const QString &name) const
{
    return d->customField(name);
}

/*!
    Sets the value of the custom field named \a name to \a value.

    \sa customField(), customFields()
*/
void QMailThread::setCustomField(const QString &name, const QString &value)
{
    d->setCustomField(name, value);
}

/*!
    Sets the folder to contain the custom fields in \a fields.

    \sa setCustomField(), customFields()
*/
void QMailThread::setCustomFields(const QMap<QString, QString> &fields)
{
    d->setCustomFields(fields);
}

/*!
    Removes the custom field named \a name.

    \sa customField(), customFields()
*/
void QMailThread::removeCustomField(const QString &name)
{
    d->removeCustomField(name);
}

/*!
    Returns the map of custom fields stored in the folder.

    \sa customField(), setCustomField()
*/
const QMap<QString, QString> &QMailThread::customFields() const
{
    return d->customFields;
}

/*! \internal */
bool QMailThread::customFieldsModified() const
{
    return d->customFieldsModified;
}

/*! \internal */
void QMailThread::setCustomFieldsModified(bool set)
{
    d->customFieldsModified = set;
}


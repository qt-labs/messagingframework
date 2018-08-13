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

#include "qmailaccount.h"
#include "qmailcodec.h"
#include "qmailid.h"
#include "qmaillog.h"
#include "qmailmessage.h"
#include "qmailmessageremovalrecord.h"
#include "qmailstore.h"
#include "qmailtimestamp.h"

#include <QDir>
#include <qtimer.h>
#include <qsettings.h>

static quint64 synchronizationEnabledFlag = 0;
static quint64 synchronizedFlag = 0;
static quint64 appendSignatureFlag = 0;
static quint64 userEditableFlag = 0;
static quint64 userRemovableFlag = 0;
static quint64 preferredSenderFlag = 0;
static quint64 messageSourceFlag = 0;
static quint64 canRetrieveFlag = 0;
static quint64 messageSinkFlag = 0;
static quint64 canTransmitFlag = 0;
static quint64 enabledFlag = 0;
static quint64 canReferenceExternalDataFlag = 0;
static quint64 canTransmitViaReferenceFlag = 0;
static quint64 canCreateFoldersFlag = 0;
static quint64 useSmartReplyFlag = 0;
static quint64 canSearchOnServerFlag = 0;
static quint64 hasPersistentConnectionFlag = 0;

class QMailAccountPrivate : public QSharedData
{
public:
    QMailAccountPrivate() : QSharedData(),
                            _messageType(QMailMessage::None),
                            _status(0),
                            _customFieldsModified(false)
    {}

    ~QMailAccountPrivate()
    {
    }

    QMailAccountId _id;
    QString _name;
    QMailMessage::MessageType _messageType;
    quint64 _status;
    QString _signature;
    QMailTimeStamp _lastSynchronized;
    QMailAddress _address;
    QStringList _sources;
    QStringList _sinks;
    QMap<QMailFolder::StandardFolder, QMailFolderId> _standardFolders;
    QString _iconPath;

    QMap<QString, QString> _customFields;
    bool _customFieldsModified;

    QString customField(const QString &name) const
    {
        QMap<QString, QString>::const_iterator it = _customFields.find(name);
        if (it != _customFields.end()) {
            return *it;
        }

        return QString();
    }

    void setCustomField(const QString &name, const QString &value)
    {
        QMap<QString, QString>::iterator it = _customFields.find(name);
        if (it != _customFields.end()) {
            if (*it != value) {
                *it = value;
                _customFieldsModified = true;
            }
        } else {
            _customFields.insert(name, value);
            _customFieldsModified = true;
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
        QMap<QString, QString>::iterator it = _customFields.find(name);
        if (it != _customFields.end()) {
            _customFields.erase(it);
            _customFieldsModified = true;
        }
    }
};

/*!
    \class QMailAccount

    \preliminary
    \ingroup messaginglibrary

    \brief The QMailAccount class represents a messaging account in the mail store.

    A QMailAccount is a logical entity that groups messages according to the
    method by which they are sent and received.  An account can be configured
    to support one more message sources, from which messages are imported into
    the mail store, and one or more message sinks by which messages are transmitted
    to external messaging services.  Although an account can support multiple 
    sources or sinks, this facility is for grouping those that are logically equivalent;
    for example, using one of multiple connectivity options to retrieve messages from 
    the same external server.

    The QMailAccount class is used for accessing properties of the account related
    to dealing with the account's folders and messages, rather than for modifying 
    the account itself.  The QMailAccountConfiguration class allows for the configuration 
    details of the account itself to be modified.  A newly created account must also
    have a QMailAccountConfiguration defined, in order to be used for the transfer of
    messages.

    QMailAccount allows the communications properties of the account to be tested.
    The \l{QMailAccount::MessageSource}{MessageSource} status flag indicates that the
    account acts a source of incoming messages, and the 
    \l{QMailAccount::MessageSink}{MessageSink} status flag indicates that the account
    provides a mechanism for transmitting outgoing messages.  The messageSources() and 
    messageSinks() functions return the protocol tags for each message source or message 
    sink implementation configured for the account.  These tags can be used to identify 
    the implementation details of the account if necessary:

    \code 
    void someFunction(const QMailMessage &message) 
    {
        QMailAccount msgAccount(message.parentAccountId());
        if (msgAccount.messageSources().contains("imap4", Qt::CaseInsensitive)) {
            // This account uses IMAP
            ...
        }
    }
    \endcode

    The QMailAccount class also provides functions which help clients to access 
    the resources of the account.  The mailboxes() function returns a list of 
    each folder associated with the account, while the mailbox() function
    allows a mailbox to be located by name.  The deletedMessages() and serverUids() 
    functions are primarily used in synchronizing the account's contents with 
    those present on an external server.

    \sa QMailAccountConfiguration, QMailStore::account()
*/

/*!
    \variable QMailAccount::SynchronizationEnabled

    The status mask needed for testing the value of the registered status flag named 
    \c "SynchronizationEnabled" against the result of QMailAccount::status().

    This flag indicates that an account should be synchronized against an external message source.
*/

/*!
    \variable QMailAccount::Synchronized

    The status mask needed for testing the value of the registered status flag named 
    \c "Synchronized" against the result of QMailAccount::status().

    This flag indicates that an account has been synchronized by a synchronization operation.
*/

/*!
    \variable QMailAccount::AppendSignature

    The status mask needed for testing the value of the registered status flag named 
    \c "AppendSignature" against the result of QMailAccount::status().

    This flag indicates that an account has been configured to append a signature block to outgoing messages.
*/

/*!
    \variable QMailAccount::UserEditable

    The status mask needed for testing the value of the registered status flag named 
    \c "UserEditable" against the result of QMailAccount::status().

    This flag indicates that the account's configuration may be modified by the user.
*/

/*!
    \variable QMailAccount::UserRemovable

    The status mask needed for testing the value of the registered status flag named 
    \c "UserRemovable" against the result of QMailAccount::status().

    This flag indicates that the account may be removed by the user.
*/

/*!
    \variable QMailAccount::PreferredSender

    The status mask needed for testing the value of the registered status flag named 
    \c "PreferredSender" against the result of QMailAccount::status().

    This flag indicates that the account is the user's preferred account for sending the 
    type of message that the account creates.

    \sa QMailAccount::messageType()
*/

/*!
    \variable QMailAccount::MessageSource

    The status mask needed for testing the value of the registered status flag named 
    \c "MessageSink" against the result of QMailAccount::status().

    This flag indicates that the account has been configured to act as a source of incoming messages.
*/

/*!
    \variable QMailAccount::CanRetrieve

    The status mask needed for testing the value of the registered status flag named 
    \c "CanRetrieve" against the result of QMailAccount::status().

    This flag indicates that the account has been sufficiently configured that an attempt to
    retrieve messages may be performed.
*/

/*!
    \variable QMailAccount::MessageSink

    The status mask needed for testing the value of the registered status flag named 
    \c "MessageSink" against the result of QMailAccount::status().

    This flag indicates that the account has been configured to act as a transmitter of outgoing messages.
*/

/*!
    \variable QMailAccount::CanTransmit

    The status mask needed for testing the value of the registered status flag named 
    \c "CanTransmit" against the result of QMailAccount::status().

    This flag indicates that the account has been sufficiently configured that an attempt to
    transmit messages may be performed.
*/

/*!
    \variable QMailAccount::Enabled

    The status mask needed for testing the value of the registered status flag named 
    \c "Enabled" against the result of QMailAccount::status().

    This flag indicates that the account has been marked as suitable for use by the messaging server.
*/

/*!
    \variable QMailAccount::CanReferenceExternalData

    The status mask needed for testing the value of the registered status flag named 
    \c "CanReferenceExternalData" against the result of QMailAccount::status().

    This flag indicates that the account can contain messages that reference data in other messages.

    \sa QMailMessagePart::setReference()
*/

/*!
    \variable QMailAccount::CanTransmitViaReference

    The status mask needed for testing the value of the registered status flag named 
    \c "CanTransmitViaReference" against the result of QMailAccount::status().

    This flag indicates that the account can be used to transmit messages that contain references.

    \sa QMailMessagePart::setReference()
*/

/*!
    \variable QMailAccount::CanCreateFolders

    The status mask needed for testing the value of the registered status flag named 
    \c "CanCreateFolders" against the result of QMailAccount::status().

    This flag indicates that top level folders can be created for the account.

    \sa QMailStorageAction::onlineCreateFolder(), QMailFolder::ChildCreationPermitted
*/

/*!
    \variable QMailAccount::UseSmartReply

    The status mask needed for testing the value of the registered status flag named
    \c "UseSmartReply" against the result of QMailAccount::status().

    This flag indicates that the account uses 'Smart Reply', a technique in which the server will
    automatically append the text of the replied-to message, and as such does not need to be done
    manually.
*/

/*!
    \variable QMailAccount::CanSearchOnServer

    The status mask needed for testing the value of the registered status flag named
    \c "CanSearchOnServer" against the result of QMailAccount::status().

    This flag indicates that the remote server associated with the account supports
    server based searching.
    
    \sa QMailSearchAction::searchMessages()
*/

/*!
    \variable QMailAccount::HasPersistentConnection

    The status mask needed for testing the value of the registered status flag named
    \c "HasPersistentConnection" against the result of QMailAccount::status().

    This flag indicates that an account has a persistent connection to the server(always online).
*/

const quint64 &QMailAccount::SynchronizationEnabled = synchronizationEnabledFlag;
const quint64 &QMailAccount::Synchronized = synchronizedFlag;
const quint64 &QMailAccount::AppendSignature = appendSignatureFlag;
const quint64 &QMailAccount::UserEditable = userEditableFlag;
const quint64 &QMailAccount::UserRemovable = userRemovableFlag;
const quint64 &QMailAccount::PreferredSender = preferredSenderFlag;
const quint64 &QMailAccount::MessageSource = messageSourceFlag;
const quint64 &QMailAccount::CanRetrieve = canRetrieveFlag;
const quint64 &QMailAccount::MessageSink = messageSinkFlag;
const quint64 &QMailAccount::CanTransmit = canTransmitFlag;
const quint64 &QMailAccount::Enabled = enabledFlag;
const quint64 &QMailAccount::CanReferenceExternalData = canReferenceExternalDataFlag;
const quint64 &QMailAccount::CanTransmitViaReference = canTransmitViaReferenceFlag;
const quint64 &QMailAccount::CanCreateFolders = canCreateFoldersFlag;
const quint64 &QMailAccount::UseSmartReply = useSmartReplyFlag;
const quint64 &QMailAccount::CanSearchOnServer = canSearchOnServerFlag;
const quint64 &QMailAccount::HasPersistentConnection = hasPersistentConnectionFlag;

/*!
    Creates an uninitialised account object.
*/
QMailAccount::QMailAccount()
    : d(new QMailAccountPrivate)
{
}

/*!
  Convenience constructor that creates a \c QMailAccount by loading the data from the store as
  specified by the QMailAccountId \a id. If the account does not exist in the store, then this constructor
  will create an empty and invalid QMailAccount.
*/

QMailAccount::QMailAccount(const QMailAccountId& id)
    : d(new QMailAccountPrivate)
{
    *this = QMailStore::instance()->account(id);
}

/*!
    Creates a copy of the QMailAccount \a other.
*/

QMailAccount::QMailAccount(const QMailAccount& other)
{
    d = other.d;
}

/*!
   Assigns the value of this account to the account \a other
*/

QMailAccount& QMailAccount::operator=(const QMailAccount& other)
{
    if(&other != this)
        d = other.d;
    return *this;
}

/*!
  Destroys the account object.
*/
QMailAccount::~QMailAccount()
{
}

/*!
  Returns the name of the account for display purposes.

  \sa setName()
*/
QString QMailAccount::name() const
{
    return d->_name;
}

/*!
  Sets the name of the account for display purposes to \a str.

  \sa name()
*/
void QMailAccount::setName(const QString &str)
{
    d->_name = str;
}

/*!
    Returns the address from which the account's outgoing messages should be reported as originating.

    \sa setFromAddress()
*/
QMailAddress QMailAccount::fromAddress() const
{
    return d->_address;
}

/*!
    Sets the address from which the account's outgoing messages should be reported as originating to \a address.

    \sa fromAddress()
*/
void QMailAccount::setFromAddress(const QMailAddress &address)
{
    d->_address = address;
}

/*!
    Returns the signature text configured for the account.

    \sa setSignature()
*/
QString QMailAccount::signature() const
{
    return d->_signature;
}

/*!
    Sets the signature text configured for the account to \a str.

    \sa signature()
*/
void QMailAccount::setSignature(const QString &str)
{
    d->_signature = str;
}

/*!
    Returns the time the account was last succesfully synchronized.
    
    Should be updated by QMailRetrievalAction::retrieveMessageList() when an account is synchronized.

    \sa setLastSynchronized(), QMailRetrievalAction::retrieveMessageList()
*/
QMailTimeStamp QMailAccount::lastSynchronized() const
{
    return d->_lastSynchronized;
}

/*!
    Sets the last successful synchronized time to \a synced
*/
void QMailAccount::setLastSynchronized(const QMailTimeStamp &synced)
{
    d->_lastSynchronized = synced;
}

/*!
  Returns the storage id for this account.
 */
QMailAccountId QMailAccount::id() const
{
    return d->_id;
}

/*!
  Sets the storage id for this account to \a id.
 */

void QMailAccount::setId(const QMailAccountId& id)
{
    d->_id = id;
}

/*!
    Returns the types of messages this account deals with.
*/
QMailMessageMetaDataFwd::MessageType QMailAccount::messageType() const
{
    return d->_messageType;
}

/*!
    Sets the types of messages this account deals with to \a type.
*/
void QMailAccount::setMessageType(QMailMessageMetaDataFwd::MessageType type)
{
    d->_messageType = type;
}

/*!
    Returns the list of protocol tags identifying the message source implementations
    that provide the messages for this account.
*/
QStringList QMailAccount::messageSources() const
{
    return d->_sources;
}

/*!
    Returns the list of protocol tags identifying the message sink implementations
    that can transmit messages for this account.
*/
QStringList QMailAccount::messageSinks() const
{
    return d->_sinks;
}

/*! 
    Returns the folder configured for the standard folder role \a folder for this account, if there is one.

    \sa setStandardFolder()
*/
QMailFolderId QMailAccount::standardFolder(QMailFolder::StandardFolder folder) const
{
    const QMap<QMailFolder::StandardFolder, QMailFolderId>::const_iterator it = d->_standardFolders.find(folder);
    if (it != d->_standardFolders.end())
        return it.value();

    return QMailFolderId();
}

/*! 
    Sets the folder configured for the standard folder role \a folder for this account to \a folderId.

    \sa standardFolder()
*/
void QMailAccount::setStandardFolder(QMailFolder::StandardFolder folder, const QMailFolderId &folderId)
{
    if (folder == QMailFolder::OutboxFolder) {
        qWarning() << "Cannot configure Outbox for account!";
    } else {
        if (folderId == QMailFolderId()) {
            // Resetting to default
            d->_standardFolders.remove(folder);
        } else {
            d->_standardFolders.insert(folder, folderId);
        }
    }
}

/*!
  Returns the icon path for this account, if there is one.

  \sa setIconPath()
*/
QString QMailAccount::iconPath() const
{
    return d->_iconPath;
}

/*!
    Sets the icon path \a iconPath  for this account.

    \sa iconPath()
*/

void QMailAccount::setIconPath(const QString &iconPath)
{
    d->_iconPath = iconPath;
}

/*! 
    Returns the map of standard folders configured for this account.

    \sa standardFolder(), setStandardFolder()
*/
const QMap<QMailFolder::StandardFolder, QMailFolderId> &QMailAccount::standardFolders() const
{
    return d->_standardFolders;
}

/*! 
    Returns the status value for the account.

    \sa setStatus(), statusMask()
*/
quint64 QMailAccount::status() const
{
    return d->_status;
}

/*! 
    Sets the status value for the account to \a newStatus.

    \sa status(), statusMask()
*/
void QMailAccount::setStatus(quint64 newStatus)
{
    d->_status = newStatus;
}

/*! 
    Sets the status flags indicated in \a mask to \a set.

    \sa status(), statusMask()
*/
void QMailAccount::setStatus(quint64 mask, bool set)
{
    if (set)
        d->_status |= mask;
    else
        d->_status &= ~mask;
}

/*! 
    Returns the value recorded in the custom field named \a name.

    \sa setCustomField(), customFields()
*/
QString QMailAccount::customField(const QString &name) const
{
    return d->customField(name);
}

/*! 
    Sets the value of the custom field named \a name to \a value.

    \sa customField(), customFields()
*/
void QMailAccount::setCustomField(const QString &name, const QString &value)
{
    d->setCustomField(name, value);
}

/*! 
    Sets the account to contain the custom fields in \a fields.

    \sa setCustomField(), customFields()
*/
void QMailAccount::setCustomFields(const QMap<QString, QString> &fields)
{
    d->setCustomFields(fields);
}

/*! 
    Removes the custom field named \a name.

    \sa customField(), customFields()
*/
void QMailAccount::removeCustomField(const QString &name)
{
    d->removeCustomField(name);
}

/*! 
    Returns the map of custom fields stored in the account.

    \sa customField(), setCustomField()
*/
const QMap<QString, QString> &QMailAccount::customFields() const
{
    return d->_customFields;
}

/*! \internal */
bool QMailAccount::customFieldsModified() const
{
    return d->_customFieldsModified;
}

/*! \internal */
void QMailAccount::setCustomFieldsModified(bool set)
{
    d->_customFieldsModified = set;
}

/*!
    Returns the status bitmask needed to test the result of QMailAccount::status() 
    against the QMailAccount status flag registered with the identifier \a flagName.

    \sa status(), QMailStore::accountStatusMask()
*/
quint64 QMailAccount::statusMask(const QString &flagName)
{
    return QMailStore::instance()->accountStatusMask(flagName);
}

/*! \internal */
void QMailAccount::addMessageSource(const QString &source)
{
    d->_sources.append(source);
}

/*! \internal */
void QMailAccount::addMessageSink(const QString &sink)
{
    d->_sinks.append(sink);
}


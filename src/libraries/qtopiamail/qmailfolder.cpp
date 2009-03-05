/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#include "qmailfolder.h"
#include "qmailstore.h"
#include "qmaillog.h"

static quint64 synchronizationEnabledFlag = 0;
static quint64 synchronizedFlag = 0;
static quint64 partialContentFlag = 0;

class QMailFolderPrivate : public QSharedData
{
public:
    QMailFolderPrivate()
        : QSharedData(),
          status(0),
          serverCount(0),
          serverUnreadCount(0),
          customFieldsModified(false)
    {
    }

    QMailFolderId id;
    QString path;
    QString displayName;
    QMailFolderId parentFolderId;
    QMailAccountId parentAccountId;
    quint64 status;
    uint serverCount;
    uint serverUnreadCount;

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

    static void initializeFlags()
    {
        static bool flagsInitialized = false;
        if (!flagsInitialized) {
            flagsInitialized = true;

            synchronizationEnabledFlag = registerFlag("SynchronizationEnabled");
            synchronizedFlag = registerFlag("Synchronized");
            partialContentFlag = registerFlag("PartialContent");
        }
    }

private:
    static quint64 registerFlag(const QString &name)
    {
        if (!QMailStore::instance()->registerFolderStatusFlag(name)) {
            qMailLog(Messaging) << "Unable to register folder status flag:" << name << "!";
        }

        return QMailFolder::statusMask(name);
    }
};

/*!
    \class QMailFolder
    \inpublicgroup QtMessagingModule
    \inpublicgroup QtPimModule

    \preliminary
    \brief The QMailFolder class represents a folder for mail messages in the mail store.
    \ingroup messaginglibrary

    QMailFolder represents a folder of mail messages, either defined internally for
    application use, or to represent a folder object held by an external message
    service, such as an IMAP account.

    A QMailFolder object has an optional parent of the same type, allowing folders
    to be arranged in tree structures.  Messages may be associated with folders, 
    allowing for simple classification and access by their 
    \l{QMailMessage::parentFolderId()}{parentFolderId} property.

    \sa QMailMessage, QMailStore::folder()
*/

/*!
    \variable QMailFolder::SynchronizationEnabled

    The status mask needed for testing the value of the registered status flag named 
    \c "SynchronizationEnabled" against the result of QMailFolder::status().

    This flag indicates that a folder should be included during account synchronization.
*/

/*!
    \variable QMailFolder::Synchronized

    The status mask needed for testing the value of the registered status flag named 
    \c "Synchronized" against the result of QMailFolder::status().

    This flag indicates that a folder has been synchronized during account synchronization.
*/

/*!
    \variable QMailFolder::PartialContent

    The status mask needed for testing the value of the registered status flag named 
    \c "PartialContent" against the result of QMailFolder::status().

    This flag indicates that a folder contains a metadata record for only some of the messages known to exist at the external server.
*/

/*!
    \enum QMailFolder::StandardFolder

    This enum type describes the standard standard storage folders.
    These folders cannot be removed or updated.

    \value InboxFolder Represents the standard inbox folder.
    \value OutboxFolder Represents the standard outbox folder.
    \value DraftsFolder Represents the standard drafts folder.
    \value SentFolder Represents the standard sent folder.
    \value TrashFolder Represents the standard trash folder.
*/


const quint64 &QMailFolder::SynchronizationEnabled = synchronizationEnabledFlag;
const quint64 &QMailFolder::Synchronized = synchronizedFlag;
const quint64 &QMailFolder::PartialContent = partialContentFlag;

/*!
  Constructor that creates an empty and invalid \c QMailFolder.
  An empty folder is one which has no path, no parent folder and no parent account.
  An invalid folder does not exist in the database and has an invalid id.
*/

QMailFolder::QMailFolder()
{
    d = new QMailFolderPrivate();
}

/*!
  Constructor that loads a standard QMailFolder specified by \a sf from the message store.
*/

QMailFolder::QMailFolder(const StandardFolder& sf)
{
    *this = QMailStore::instance()->folder(QMailFolderId(static_cast<quint64>(sf)));
}

/*!
  Constructor that creates a QMailFolder by loading the data from the message store as
  specified by the QMailFolderId \a id. If the folder does not exist in the message store, 
  then this constructor will create an empty and invalid QMailFolder.
*/

QMailFolder::QMailFolder(const QMailFolderId& id)
{
    *this = QMailStore::instance()->folder(id);
}


/*!
  Creates a QMailFolder object with path \a path and  parent folder ID \a parentFolderId,
  that is linked to a parent account \a parentAccountId.
*/

QMailFolder::QMailFolder(const QString& path, const QMailFolderId& parentFolderId, const QMailAccountId& parentAccountId)
    : d(new QMailFolderPrivate())
{
    d->path = path;
    d->parentFolderId = parentFolderId;
    d->parentAccountId = parentAccountId;
}

/*!
  Creates a copy of the \c QMailFolder object \a other.
*/

QMailFolder::QMailFolder(const QMailFolder& other)
{
    d = other.d;
}


/*!
  Destroys the \c QMailFolder object.
*/

QMailFolder::~QMailFolder()
{
}

/*!
  Assigns the value of the \c QMailFolder object \a other to this.
*/

QMailFolder& QMailFolder::operator=(const QMailFolder& other)
{
    d = other.d;
    return *this;
}

/*!
  Returns the \c ID of the \c QMailFolder object. A \c QMailFolder with an invalid ID
  is one which does not yet exist on the message store.
*/

QMailFolderId QMailFolder::id() const
{
    return d->id;
}

/*!
  Sets the ID of this folder to \a id
*/

void QMailFolder::setId(const QMailFolderId& id)
{
    d->id = id;
}

/*!
  Returns the path of the folder.
*/
QString QMailFolder::path() const
{
    return d->path;
}

/*!
  Sets the path of this folder within the parent account to \a path.
*/

void QMailFolder::setPath(const QString& path)
{
    d->path = path;
}

/*!
  Returns the display name of the folder.
*/
QString QMailFolder::displayName() const
{
    if (!d->displayName.isNull())
        return d->displayName;

    return d->path;
}

/*!
  Sets the display name of this folder to \a displayName.
*/

void QMailFolder::setDisplayName(const QString& displayName)
{
    d->displayName = displayName;
}

/*!
  Returns the ID of the parent folder. This folder is a root folder if the parent
  ID is invalid.
*/

QMailFolderId QMailFolder::parentFolderId() const
{
    return d->parentFolderId;
}

/*!
 Sets the parent folder ID to \a id. \bold{Warning}: it is the responsibility of the
 application to make sure that no circular folder refernces are created.
*/

void QMailFolder::setParentFolderId(const QMailFolderId& id)
{
    d->parentFolderId = id;
}

/*!
  Returns the id of the account which owns the folder. If the folder
  is not linked to an account an invalid id is returned.
*/

QMailAccountId QMailFolder::parentAccountId() const
{
    return d->parentAccountId;
}

/*!
  Sets the id of the account which owns the folder to \a id.
*/

void QMailFolder::setParentAccountId(const QMailAccountId& id)
{
    d->parentAccountId = id;
}

/*! 
    Returns the status value for the folder.

    \sa setStatus(), statusMask()
*/
quint64 QMailFolder::status() const
{
    return d->status;
}

/*! 
    Sets the status value for the folder to \a newStatus.

    \sa status(), statusMask()
*/
void QMailFolder::setStatus(quint64 newStatus)
{
    d->status = newStatus;
}

/*! 
    Sets the status flags indicated in \a mask to \a set.

    \sa status(), statusMask()
*/
void QMailFolder::setStatus(quint64 mask, bool set)
{
    if (set)
        d->status |= mask;
    else
        d->status &= ~mask;
}

/*!
    Returns the status bitmask needed to test the result of QMailFolder::status() 
    against the QMailFolder status flag registered with the identifier \a flagName.

    \sa status(), QMailStore::folderStatusMask()
*/
quint64 QMailFolder::statusMask(const QString &flagName)
{
    return QMailStore::instance()->folderStatusMask(flagName);
}


/*! 
    Returns the count of messages on the server for the folder.
    
    Will be upated when an operation to the folder is made on the server. For example
    when \link QMailRetrievalAction::retrieveMessageList() 
    QMailRetrievalAction::retrieveMessageList() \endlink is called on the folder.

    \sa setServerCount(), serverUnreadCount()
*/
uint QMailFolder::serverCount() const
{
    return d->serverCount;
}

/*! 
    Sets the count of messages on the server for the folder to \a serverCount.
    
    \sa serverCount(), serverUnreadCount()
*/
void QMailFolder::setServerCount(uint serverCount)
{
    d->serverCount = serverCount;
}

/*! 
    Returns the count of unread messages on the server for the folder.

    Will be upated when an operation to the folder is made on the server. For example
    when \link QMailRetrievalAction::retrieveMessageList() 
    QMailRetrievalAction::retrieveMessageList() \endlink is called on the folder.

    \sa setServerUnreadCount(), serverCount()
*/
uint QMailFolder::serverUnreadCount() const
{
    return d->serverUnreadCount;
}

/*! 
    Sets the count of unread messages on the server for the folder to \a serverUnreadCount.
    
    \sa serverUnreadCount(), serverCount()
*/
void QMailFolder::setServerUnreadCount(uint serverUnreadCount)
{
    d->serverUnreadCount = serverUnreadCount;
}

/*! 
    Returns the value recorded in the custom field named \a name.

    \sa setCustomField(), customFields()
*/
QString QMailFolder::customField(const QString &name) const
{
    return d->customField(name);
}

/*! 
    Sets the value of the custom field named \a name to \a value.

    \sa customField(), customFields()
*/
void QMailFolder::setCustomField(const QString &name, const QString &value)
{
    d->setCustomField(name, value);
}

/*! 
    Sets the folder to contain the custom fields in \a fields.

    \sa setCustomField(), customFields()
*/
void QMailFolder::setCustomFields(const QMap<QString, QString> &fields)
{
    d->setCustomFields(fields);
}

/*! 
    Removes the custom field named \a name.

    \sa customField(), customFields()
*/
void QMailFolder::removeCustomField(const QString &name)
{
    d->removeCustomField(name);
}

/*! 
    Returns the map of custom fields stored in the folder.

    \sa customField(), setCustomField()
*/
const QMap<QString, QString> &QMailFolder::customFields() const
{
    return d->customFields;
}

/*! \internal */
bool QMailFolder::customFieldsModified() const
{
    return d->customFieldsModified;
}

/*! \internal */
void QMailFolder::setCustomFieldsModified(bool set)
{
    d->customFieldsModified = set;
}

/*! \internal */
void QMailFolder::initStore()
{
    QMailFolderPrivate::initializeFlags();
}


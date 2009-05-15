/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#include "qmailmessageservice.h"
#include <longstream_p.h>
#include <QAbstractSocket>
#include <QCoreApplication>
#include <QList>
#include <qmailstore.h>
#include <qmailserviceaction.h>
#include <QPair>
#include <qmailpluginmanager.h>
#include <QTimer>
#include <qmaillog.h>

#define PLUGIN_KEY "messageservices"


namespace {

typedef QMap<QString, QMailMessageServicePlugin*> PluginMap;

PluginMap initMap(QMailPluginManager &manager)
{
    PluginMap map;

    foreach (const QString &item, manager.list()) {
        QObject *instance(manager.instance(item));
        if (QMailMessageServicePlugin *iface = qobject_cast<QMailMessageServicePlugin*>(instance))
            map.insert(iface->key(), iface);
    }

    return map;
}

PluginMap &pluginMap()
{
    static QMailPluginManager manager(PLUGIN_KEY);
    static PluginMap map(initMap(manager));
    return map;
}

QMailMessageServicePlugin *mapping(const QString &key)
{
    PluginMap::const_iterator it = pluginMap().find(key);
    if (it != pluginMap().end())
        return it.value();

    qMailLog(Messaging) << "Unable to map service for key:" << key;
    return 0;
}

}


/*!
    \class QMailMessageServiceFactory
    \ingroup libmessageserver

    \brief The QMailMessageServiceFactory class creates objects implementing the QMailMessageService interface.

    The QMailMessageServiceFactory class creates objects that provide messaging services to the
    messageserver daemon.  The factory allows implementations to be loaded from plugin libraries, 
    and to be retrieved and instantiated by name.

    To create a new service that can be created via the QMailMessageServiceFactory, derive from the
    QMailMessageService base class, and optionally implement the QMailMessageSource and QMailMessageSink
    interfaces.  Export your service via a subclass of the QMailMessageServicePlugin class.

    \sa QMailMessageService, QMailMessageServicePlugin
*/

/*!
    \enum QMailMessageServiceFactory::ServiceType
    
    This enum type is used to differentiate between the types of services that QMailMessageServiceFactory can manage.

    \value Any      Any type of service.
    \value Source   A message provision service.
    \value Sink     A message transmission service.
    \value Storage  A message content storage service.
*/

/*!
    Returns a list of all message services of type \a type that can be instantiated by the factory.
*/
QStringList QMailMessageServiceFactory::keys(QMailMessageServiceFactory::ServiceType type)
{
    if (type == QMailMessageServiceFactory::Any)
        return pluginMap().keys();

    QStringList result;
    foreach (QMailMessageServicePlugin *plugin, pluginMap().values())
        if (plugin->supports(type))
            result.append(plugin->key());

    return result;
}

/*!
    Returns true if the service identified by \a key supports the service type \a type.
*/
bool QMailMessageServiceFactory::supports(const QString &key, QMailMessageServiceFactory::ServiceType type)
{
    if (QMailMessageServicePlugin* plugin = mapping(key))
        return plugin->supports(type);

    return false;
}

/*!
    Returns true if the service identified by \a key supports the message type \a messageType.
*/
bool QMailMessageServiceFactory::supports(const QString &key, QMailMessage::MessageType messageType)
{
    if (QMailMessageServicePlugin* plugin = mapping(key))
        return plugin->supports(messageType);

    return false;
}

/*!
    Returns a new instance of the service identified by \a key, associating it with the 
    account identified by \a accountId.
*/
QMailMessageService *QMailMessageServiceFactory::createService(const QString &key, const QMailAccountId &accountId)
{
    if (QMailMessageServicePlugin* plugin = mapping(key))
        return plugin->createService(accountId);

    return 0;
}

/*!
    Returns a new instance of the configurator class for the service identified by \a key.
*/
QMailMessageServiceConfigurator *QMailMessageServiceFactory::createServiceConfigurator(const QString &key)
{
    if (QMailMessageServicePlugin* plugin = mapping(key))
        return plugin->createServiceConfigurator();

    return 0;
}


/*!
    \class QMailMessageServicePluginInterface
    \ingroup libmessageserver

    \brief The QMailMessageServicePluginInterface class defines the interface to plugins that provide messaging services.

    The QMailMessageServicePluginInterface class defines the interface to message service plugins.  Plugins will 
    typically inherit from QMailMessageServicePlugin rather than this class.

    \sa QMailMessageServicePlugin, QMailMessageService, QMailMessageServiceFactory
*/

/*!
    \fn QString QMailMessageServicePluginInterface::key() const

    Returns a string identifying the messaging service implemented by the plugin.
*/

/*!
    \fn bool QMailMessageServicePluginInterface::supports(QMailMessageServiceFactory::ServiceType type) const

    Returns true if the service provided by the plugin supports the service type \a type.
*/

/*!
    \fn bool QMailMessageServicePluginInterface::supports(QMailMessage::MessageType messageType) const

    Returns true if the service provided by the plugin supports the message type \a messageType.
*/

/*!
    \fn QMailMessageService* QMailMessageServicePluginInterface::createService(const QMailAccountId &accountId)

    Creates an instance of the QMailMessageService class provided by the plugin, associated with the account \a accountId.
*/

/*!
    Creates an instance of the configurator for the QMailMessageService class provided by the plugin.
*/
QMailMessageServiceConfigurator *QMailMessageServicePluginInterface::createServiceConfigurator()
{
    return 0;
}


/*!
    \class QMailMessageServicePlugin
    \ingroup libmessageserver

    \brief The QMailMessageServicePlugin class defines a base class for implementing messaging service plugins.

    The QMailMessageServicePlugin class provides a base class for plugin classes that provide messaging service
    functionality.  Classes that inherit QMailMessageServicePlugin need to provide overrides of the
    \l {QMailMessageServicePlugin::key()}{key}, \l {QMailMessageServicePlugin::supports()}{supports} and 
    \l {QMailMessageServicePlugin::createService()}{createService} member functions.

    \sa QMailMessageServicePluginInterface, QMailMessageService, QMailMessageServiceFactory
*/

/*!
    Creates a messaging service plugin instance.
*/
QMailMessageServicePlugin::QMailMessageServicePlugin()
{
}

/*! \internal */
QMailMessageServicePlugin::~QMailMessageServicePlugin()
{
}

/*!
    Returns the list of interfaces implemented by this plugin.
*/
QStringList QMailMessageServicePlugin::keys() const
{
    return QStringList() << "QMailMessageServicePluginInterface";
}


#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

namespace {

struct ErrorEntry { int code; const char* text; };
typedef QPair<const ErrorEntry*, size_t> ErrorMap;
typedef QList<ErrorMap> ErrorSet;

static ErrorMap socketErrorInit()
{
    static const ErrorEntry map[] = 
    {
        { QAbstractSocket::ConnectionRefusedError, QT_TRANSLATE_NOOP( "QMailServiceAction",  "Connection refused" ) },
        { QAbstractSocket::RemoteHostClosedError, QT_TRANSLATE_NOOP( "QMailServiceAction",  "Remote host closed the connection" ) },
        { QAbstractSocket::HostNotFoundError, QT_TRANSLATE_NOOP( "QMailServiceAction",  "Host not found" ) },
        { QAbstractSocket::SocketAccessError, QT_TRANSLATE_NOOP( "QMailServiceAction",  "Permission denied" ) },
        { QAbstractSocket::SocketResourceError, QT_TRANSLATE_NOOP( "QMailServiceAction",  "Insufficient resources" ) },
        { QAbstractSocket::SocketTimeoutError, QT_TRANSLATE_NOOP( "QMailServiceAction",  "Operation timed out" ) },
        { QAbstractSocket::DatagramTooLargeError, QT_TRANSLATE_NOOP( "QMailServiceAction",  "Datagram too large" ) },
        { QAbstractSocket::NetworkError, QT_TRANSLATE_NOOP( "QMailServiceAction",  "Network error" ) },
        { QAbstractSocket::AddressInUseError, QT_TRANSLATE_NOOP( "QMailServiceAction",  "Address in use" ) },
        { QAbstractSocket::SocketAddressNotAvailableError, QT_TRANSLATE_NOOP( "QMailServiceAction",  "Address not available" ) },
        { QAbstractSocket::UnsupportedSocketOperationError, QT_TRANSLATE_NOOP( "QMailServiceAction",  "Unsupported operation" ) },
        { QAbstractSocket::UnknownSocketError, QT_TRANSLATE_NOOP( "QMailServiceAction",  "Unknown error" ) },
    };

    return qMakePair( static_cast<const ErrorEntry*>(map), ARRAY_SIZE(map) );
}

static ErrorMap mailErrorInit()
{
    static const ErrorEntry map[] = 
    {
        { QMailServiceAction::Status::ErrNotImplemented, QT_TRANSLATE_NOOP( "QMailServiceAction", "This function is not currently supported.") },
        { QMailServiceAction::Status::ErrFrameworkFault, QT_TRANSLATE_NOOP( "QMailServiceAction", "Framework error occurred.") },
        { QMailServiceAction::Status::ErrSystemError, "" },
        { QMailServiceAction::Status::ErrUnknownResponse, "" },
        { QMailServiceAction::Status::ErrLoginFailed, QT_TRANSLATE_NOOP( "QMailServiceAction", "Login failed - check user name and password.") },
        { QMailServiceAction::Status::ErrCancel, QT_TRANSLATE_NOOP( "QMailServiceAction", "Operation cancelled.") },
        { QMailServiceAction::Status::ErrFileSystemFull, QT_TRANSLATE_NOOP( "QMailServiceAction", "Mail check failed.") },
        { QMailServiceAction::Status::ErrNonexistentMessage, QT_TRANSLATE_NOOP( "QMailServiceAction", "Message deleted from server.") },
        { QMailServiceAction::Status::ErrEnqueueFailed, QT_TRANSLATE_NOOP( "QMailServiceAction", "Unable to queue message for transmission.") },
        { QMailServiceAction::Status::ErrNoConnection, QT_TRANSLATE_NOOP( "QMailServiceAction", "Cannot determine the connection to transmit message on.") },
        { QMailServiceAction::Status::ErrConnectionInUse, QT_TRANSLATE_NOOP( "QMailServiceAction", "Outgoing connection already in use by another operation.") },
        { QMailServiceAction::Status::ErrConnectionNotReady, QT_TRANSLATE_NOOP( "QMailServiceAction", "Outgoing connection is not ready to transmit message.") },
        { QMailServiceAction::Status::ErrConfiguration, QT_TRANSLATE_NOOP( "QMailServiceAction", "Unable to use account due to invalid configuration.") },
        { QMailServiceAction::Status::ErrInvalidAddress, QT_TRANSLATE_NOOP( "QMailServiceAction", "Message recipient addresses are not correctly formatted.") },
        { QMailServiceAction::Status::ErrInvalidData, QT_TRANSLATE_NOOP( "QMailServiceAction", "Configured service unable to handle supplied data.") },
        { QMailServiceAction::Status::ErrTimeout, QT_TRANSLATE_NOOP( "QMailServiceAction", "Configured service failed to perform action within a reasonable period of time.") },
    };

    return qMakePair( static_cast<const ErrorEntry*>(map), ARRAY_SIZE(map) );
}

bool appendErrorText(QString* message, int code, const ErrorMap& map)
{
    const ErrorEntry *it = map.first, *end = map.first + map.second; // ptr arithmetic!

    for ( ; it != end; ++it)
        if (it->code == code) {
            QString extra(qApp->translate("QMailServiceAction", it->text));
            if (!extra.isEmpty()) {
                if (message->isEmpty()) {
                    *message = extra;
                } else {
                    message->append("\n[").append(extra).append(']');
                }
            }
            return true;
        }

    return false;
}

bool appendErrorText(QString* message, int code, const ErrorSet& mapList)
{
    foreach (const ErrorMap& map, mapList)
        if (appendErrorText(message, code, map))
            return true;

    return false;
}

void decorate(QString* message, int code, const ErrorSet& errorSet)
{
    bool handledByErrorSet = appendErrorText(message, code, errorSet);

    bool handledByHandler = true;
    if (code == QMailServiceAction::Status::ErrFileSystemFull) {
        message->append(' ').append(LongStream::errorMessage());
    } else if (code == QMailServiceAction::Status::ErrEnqueueFailed) {
        message->append("\n" + qApp->translate("QMailServiceAction", "Unable to send; message moved to Drafts folder"));
    } else if (code == QMailServiceAction::Status::ErrUnknownResponse) {
        message->prepend(qApp->translate("QMailServiceAction", "Unexpected response from server: "));
    } else {
        handledByHandler = false;
    }

    if (!handledByErrorSet && !handledByHandler) {
        if (!message->isEmpty())
            message->append('\n');
        message->append('<' + QString(qApp->translate("QMailServiceAction", "Error %1", "%1 contains numeric error code")).arg(code) + '>');
    }
}

}


class QMailMessageSourcePrivate
{
public:
    QMailMessageSourcePrivate(QMailMessageService *service);

    QMailMessageService *_service;
    QMailMessageIdList _ids;
    QMailFolderId _destinationId;
};

QMailMessageSourcePrivate::QMailMessageSourcePrivate(QMailMessageService *service)
    : _service(service)
{
}


/*!
    \class QMailMessageSource
    \ingroup libmessageserver

    \brief The QMailMessageSource class defines the interface to objects that provide access to externally sourced 
    messages to the messageserver.

    The Qt Extended messageserver uses the QMailMessageSource interface to cooperate with components loaded
    from plugin libraries, that act as sources of messaging data for the messaging framework.  Instances of
    QMailMessageSource are not created directly by the messageserver, but are exported by QMailMessageService
    objects via their \l{QMailMessageService::source()}{source} function.
    
    \sa QMailMessageService, QMailStore 
*/

/*!
    Creates a message source object associated with the service \a service.
*/
QMailMessageSource::QMailMessageSource(QMailMessageService *service)
    : d(new QMailMessageSourcePrivate(service))
{
}

/*! \internal */
QMailMessageSource::~QMailMessageSource()
{
    delete d;
}

/*!
    Returns the removal option used when deleting messages via this message source.

    \sa QMailStore::removeMessages()
*/
QMailStore::MessageRemovalOption QMailMessageSource::messageRemovalOption() const
{
    // By default, allow the messages to disappear
    return QMailStore::NoRemovalRecord;
}

/*!
    Invoked by the message server to initiate a folder listing operation.

    Retrieve the set of folders available for the account \a accountId.  
    If \a folderId is valid, only the identified folder is searched for child folders; 
    otherwise the search begins at the root of the account.  If \a descending is true, 
    the search should also recursively search for child folders within folders 
    discovered during the search.

    The QMailFolder::serverCount(), QMailFolder::serverUnreadCount() and 
    QMailFolder::serverUndiscoveredCount() properties should be updated for each 
    folder that is searched for child folders; these properties need not be updated 
    for folders that are merely discovered by searching.

    Return true if an operation is initiated.

    \sa retrieveAll()
*/
bool QMailMessageSource::retrieveFolderList(const QMailAccountId &accountId, const QMailFolderId &folderId, bool descending)
{
    notImplemented();
    return false;

    Q_UNUSED(accountId)
    Q_UNUSED(folderId)
    Q_UNUSED(descending)
}

/*!
    Invoked by the message server to initiate a message listing operation.

    Retrieve the list of messages available for the account \a accountId.
    If \a folderId is valid, then only messages within that folder should be retrieved; otherwise 
    messages within all folders in the account should be retrieved.  If \a minimum is non-zero,
    then that value will be used to restrict the number of messages to be retrieved from
    each folder; otherwise, all messages will be retrieved.
    
    If \a sort is not empty, reported the discovered messages in the ordering indicated by the 
    sort criterion, if possible.  Message sources are not required to support this facility.

    If a folder messages are being retrieved from contains at least \a minimum messages then the 
    messageserver should ensure that at least \a minimum messages are available from the mail 
    store for that folder; otherwise if the folder contains less than \a minimum messages the 
    messageserver should ensure all the messages for that folder are available from the mail store.
    If a folder has messages locally available, then all previously undiscovered messages should be
    retrieved for that folder, even if that number exceeds \a minimum.
    
    The QMailFolder::serverCount(), QMailFolder::serverUnreadCount() and 
    QMailFolder::serverUndiscoveredCount() properties should be updated for each folder 
    from which messages are retrieved.
    
    New messages should be added to the mail store in meta data form as they are discovered, 
    and marked with the \l QMailMessage::New status flag.  Messages that are present
    in the mail store but found to be no longer available should be marked with the 
    \l QMailMessage::Removed status flag.  

    Return true if an operation is initiated.

    \sa retrieveAll()
*/
bool QMailMessageSource::retrieveMessageList(const QMailAccountId &accountId, const QMailFolderId &folderId, uint minimum, const QMailMessageSortKey &sort)
{
    notImplemented();
    return false;

    Q_UNUSED(accountId)
    Q_UNUSED(folderId)
    Q_UNUSED(minimum)
    Q_UNUSED(sort)
}

/*!
    Invoked by the message server to initiate a message retrieval operation.

    Retrieve data regarding each of the messages listed in \a ids.

    If \a spec is \l QMailRetrievalAction::Flags, then the message server should detect if 
    the messages identified by \a ids have been marked as read or have been removed.
    Messages that have been read will be marked with the \l QMailMessage::ReadElsewhere flag, and
    messages that have been removed will be marked with the \l QMailMessage::Removed status flag.

    If \a spec is \l QMailRetrievalAction::MetaData, then the message server should 
    retrieve the meta data of the each message listed in \a ids.
    
    If \a spec is \l QMailRetrievalAction::Content, then the message server should 
    retrieve the entirety of each message listed in \a ids.
    
    The QMailFolder::serverCount(), QMailFolder::serverUnreadCount() and 
    QMailFolder::serverUndiscoveredCount() properties should be updated for each folder 
    from which messages are retrieved.

    Return true if an operation is initiated.
*/
bool QMailMessageSource::retrieveMessages(const QMailMessageIdList &ids, QMailRetrievalAction::RetrievalSpecification spec)
{
    notImplemented();
    return false;

    Q_UNUSED(ids)
    Q_UNUSED(spec)
}

/*!
    Invoked by the message server to initiate a message part retrieval operation.

    Retrieve the content of the message part indicated by the location \a partLocation.
    
    The QMailFolder::serverCount(), QMailFolder::serverUnreadCount() and 
    QMailFolder::serverUndiscoveredCount() properties should be updated for the folder 
    from which the part is retrieved.

    Return true if an operation is initiated.
*/
bool QMailMessageSource::retrieveMessagePart(const QMailMessagePart::Location &partLocation)
{
    notImplemented();
    return false;

    Q_UNUSED(partLocation)
}

/*!
    Invoked by the message server to initiate a message range retrieval operation.

    Retrieve a portion of the content of the message identified by \a messageId, ensuring
    that at least \a minimum bytes are available in the mail store.

    The QMailFolder::serverCount(), QMailFolder::serverUnreadCount() and 
    QMailFolder::serverUndiscoveredCount() properties should be updated for the folder 
    from which the message is retrieved.

    Return true if an operation is initiated.
*/
bool QMailMessageSource::retrieveMessageRange(const QMailMessageId &messageId, uint minimum)
{
    notImplemented();
    return false;

    Q_UNUSED(messageId)
    Q_UNUSED(minimum)
}

/*!
    Invoked by the message server to initiate a message part range retrieval operation.

    Retrieve a portion of the content of the message part indicated by the location 
    \a partLocation, ensuring that at least \a minimum bytes are available in the mail store.

    The QMailFolder::serverCount(), QMailFolder::serverUnreadCount() and 
    QMailFolder::serverUndiscoveredCount() properties should be updated for the folder 
    from which the part is retrieved.

    Return true if an operation is initiated.
*/
bool QMailMessageSource::retrieveMessagePartRange(const QMailMessagePart::Location &partLocation, uint minimum)
{
    notImplemented();
    return false;

    Q_UNUSED(partLocation)
    Q_UNUSED(minimum)
}

/*!
    Invoked by the message server to initiate a retrieval operation.

    Retrieve all folders and meta data for all messages available for the account \a accountId. 

    All folders within the account should be discovered and searched for child folders.
    The QMailFolder::serverCount(), QMailFolder::serverUnreadCount() and 
    QMailFolder::serverUndiscoveredCount() properties should be updated for each folder 
    in the account.

    New messages should be added to the mail store in meta data form as they are discovered, 
    and marked with the \l QMailMessage::New status flag.  Messages that are present
    in the mail store but found to be no longer available should be marked with the 
    \l QMailMessage::Removed status flag.  

    Return true if an operation is initiated.
    
    \sa retrieveFolderList(), retrieveMessageList(), synchronize()
*/
bool QMailMessageSource::retrieveAll(const QMailAccountId &accountId)
{
    notImplemented();
    return false;

    Q_UNUSED(accountId)
}

/*!
    Invoked by the message server to initiate an export operation.

    Update the external server with any changes to message status that have been 
    effected on the local device for account \a accountId.

    Return true if an operation is initiated.

    \sa synchronize()
*/
bool QMailMessageSource::exportUpdates(const QMailAccountId &accountId)
{
    notImplemented();
    return false;

    Q_UNUSED(accountId)
}

/*!
    Invoked by the message server to initiate a synchronization operation.

    Synchronize the set of known folder and message identifiers with those currently 
    available for the account identified by \a accountId.
    Newly discovered messages should have their meta data retrieved,
    and local changes to message status should be exported to the external server.

    New messages should be added to the mail store in meta data form as they are discovered, 
    and marked with the \l QMailMessage::New status flag.  Messages that are present
    in the mail store but found to be no longer available should be marked with the 
    \l QMailMessage::Removed status flag.  

    The folder structure of the account should be synchronized with that available from 
    the external service.  The QMailFolder::serverCount(), QMailFolder::serverUnreadCount() and 
    QMailFolder::serverUndiscoveredCount() properties should be updated for each folder.

    Return true if an operation is initiated.

    \sa retrieveAll(), exportUpdates()
*/
bool QMailMessageSource::synchronize(const QMailAccountId &accountId)
{
    notImplemented();
    return false;

    Q_UNUSED(accountId)
}

/*!
    Invoked by the message server to initiate a message deletion operation.

    Delete all messages listed in \a ids from the local mail store and the external server.

    Return true if an operation is initiated.

    \sa messagesDeleted()
*/
bool QMailMessageSource::deleteMessages(const QMailMessageIdList &ids)
{
    d->_ids = ids;
    QTimer::singleShot(0, this, SLOT(deleteMessages()));
    return true;
}

/*!
    Invoked by the message server to initiate a message copy operation.

    For each message listed in \a ids, create a new copy in the folder identified by \a destinationId.

    Successfully copied messages should be progressively reported via messagesCopied().

    Return true if an operation is initiated.

    \sa messagesCopied()
*/
bool QMailMessageSource::copyMessages(const QMailMessageIdList &ids, const QMailFolderId &destinationId)
{
    d->_ids = ids;
    d->_destinationId = destinationId;
    QTimer::singleShot(0, this, SLOT(copyMessages()));
    return true;
}

/*!
    Invoked by the message server to initiate a message move operation.

    Move each message listed in \a ids into the folder identified by \a destinationId.

    Successfully moved messages should be progressively reported via messagesMoved().

    Return true if an operation is initiated.

    \sa messagesMoved()
*/
bool QMailMessageSource::moveMessages(const QMailMessageIdList &ids, const QMailFolderId &destinationId)
{
    d->_ids = ids;
    d->_destinationId = destinationId;
    QTimer::singleShot(0, this, SLOT(moveMessages()));
    return true;
}

/*!
    Invoked by the message server to initiate a remote message search operation.

    Search the remote server for messages that match the search criteria encoded by 
    \a searchCriteria.  If \a bodyText is non-empty, matching messages must also 
    contain the specified string.  Messages whose content is already present on
    the local device should be excluded from the remote search.

    If \a sort is not empty, matched messages should be discovered by testing for
    matches in the ordering indicated by the sort criterion, if possible.

    Messages matching the search criteria should be added to the mail store in
    meta data form marked with the \l QMailMessage::New status flag, and 
    progressively reported via matchingMessageIds().

    Return true if a search operation is initiated.
    
    \sa matchingMessageIds(), retrieveMessages()
*/
bool QMailMessageSource::searchMessages(const QMailMessageKey &searchCriteria, const QString &bodyText, const QMailMessageSortKey &sort)
{
    notImplemented();
    return false;

    Q_UNUSED(searchCriteria)
    Q_UNUSED(bodyText)
    Q_UNUSED(sort)
}

/*!
    Invoked by the message server to initiate a message preparation operation.

    Prepare each message listed in \a ids for transmission by resolving any external 
    references into URLs.

    Messages successfully prepared for transmission should be progressively reported via messagesPrepared().

    Return true if an operation is initiated.
    
    \sa messagesPrepared()
*/
bool QMailMessageSource::prepareMessages(const QMailMessageIdList &ids)
{
    notImplemented();
    return false;

    Q_UNUSED(ids)
}

/*!
    Invoked by the message server to initiate a protocol-specific operation.

    If \a request corresponds to a protocol-specific action implemented by the source, initiate
    the requested operation for \a accountId, using any relevant information extracted from \a data.

    Any responses resulting from the action should be progressively reported via protocolResponse().

    Return true if an operation is initiated.
    
    \sa protocolResponse()
*/
bool QMailMessageSource::protocolRequest(const QMailAccountId &accountId, const QString &request, const QVariant &data)
{
    notImplemented();
    return false;

    Q_UNUSED(accountId)
    Q_UNUSED(request)
    Q_UNUSED(data)
}

/*!
    \fn void QMailMessageSource::newMessagesAvailable();

    Signal emitted by the source to report the availability of new messages.
*/

/*!
    \fn void QMailMessageSource::messagesDeleted(const QMailMessageIdList &ids);

    Signal emitted by the source to report the deletion of the messages listed in \a ids.
*/

/*!
    \fn void QMailMessageSource::messagesCopied(const QMailMessageIdList &ids);

    Signal emitted by the source to report the copying of the messages listed in \a ids.
*/

/*!
    \fn void QMailMessageSource::messagesMoved(const QMailMessageIdList &ids);

    Signal emitted by the source to report the moving of the messages listed in \a ids.
*/

/*!
    \fn void QMailMessageSource::matchingMessageIds(const QMailMessageIdList &ids);

    Signal emitted by the source to report the messages listed in \a ids as matching the current search.
*/

/*!
    \fn void QMailMessageSource::messagesPrepared(const QMailMessageIdList &ids);

    Signal emitted by the source to report the successful preparation for transmission of the messages listed in \a ids.
*/

/*!
    \fn void QMailMessageSource::protocolResponse(const QString &response, const QVariant &data);

    Signal emitted by the source to report the response \a response resulting from a 
    protocol-specific request, with any associated \a data.
*/

/*! \internal */
void QMailMessageSource::notImplemented()
{
    d->_service->updateStatus(QMailServiceAction::Status::ErrNotImplemented, QString());
    emit d->_service->actionCompleted(false);
}

/*! \internal */
void QMailMessageSource::deleteMessages()
{
    uint total = d->_ids.count();
    emit d->_service->progressChanged(0, total);

    // Just remove these locally and store a deletion record for later synchronization
    QMailMessageKey idsKey(QMailMessageKey::id(d->_ids));
    if (!QMailStore::instance()->removeMessages(idsKey, messageRemovalOption())) {
        qMailLog(Messaging) << "Unable to remove messages!";
    } else {
        emit d->_service->progressChanged(total, total);
        emit messagesDeleted(d->_ids);
        emit d->_service->activityChanged(QMailServiceAction::Successful);
        emit d->_service->actionCompleted(true);
        return;
    }

    emit d->_service->statusChanged(QMailServiceAction::Status(QMailServiceAction::Status::ErrFrameworkFault, tr("Unable to delete messages"), QMailAccountId(), QMailFolderId(), QMailMessageId()));
    emit d->_service->activityChanged(QMailServiceAction::Failed);
    emit d->_service->actionCompleted(false);
}

/*! \internal */
void QMailMessageSource::copyMessages()
{
    bool successful(true);

    unsigned int size = QMailStore::instance()->sizeOfMessages(QMailMessageKey::id(d->_ids));
    if (!LongStream::freeSpace("", size + 1024*10)) {
        qMailLog(Messaging) << "Insufficient space to copy messages to folder:" << d->_destinationId << "bytes required:" << size;
        emit d->_service->statusChanged(QMailServiceAction::Status(QMailServiceAction::Status::ErrFileSystemFull, tr("Insufficient space to copy messages to folder"), QMailAccountId(), d->_destinationId, QMailMessageId()));
        successful = false;
    }

    if (successful) {
        uint progress = 0;
        uint total = d->_ids.count();
        emit d->_service->progressChanged(progress, total);

        // Create a copy of each message
        foreach (const QMailMessageId id, d->_ids) {
            QMailMessage message(id);

            message.setId(QMailMessageId());
            message.setContentIdentifier(QString());

            message.setParentFolderId(d->_destinationId);

            if (!QMailStore::instance()->addMessage(&message)) {
                qMailLog(Messaging) << "Unable to copy messages to folder:" << d->_destinationId << "for account:" << message.parentAccountId();

                emit d->_service->statusChanged(QMailServiceAction::Status(QMailServiceAction::Status::ErrFrameworkFault, tr("Unable to copy messages for account"), message.parentAccountId(), d->_destinationId, QMailMessageId()));
                successful = false;
                break;
            } else {
                emit d->_service->progressChanged(++progress, total);
            }
        }

        if (progress > 0)
            emit messagesCopied(d->_ids.mid(0, progress));
    }

    emit d->_service->activityChanged(successful ? QMailServiceAction::Successful : QMailServiceAction::Failed);
    emit d->_service->actionCompleted(successful);
}

/*! \internal */
void QMailMessageSource::moveMessages()
{
    uint total = d->_ids.count();
    emit d->_service->progressChanged(0, total);

    QMailMessageMetaData metaData;
    metaData.setParentFolderId(d->_destinationId);

    QMailMessageKey idsKey(QMailMessageKey::id(d->_ids));
    if (!QMailStore::instance()->updateMessagesMetaData(idsKey, QMailMessageKey::ParentFolderId, metaData)) {
        qMailLog(Messaging) << "Unable to move messages to folder:" << d->_destinationId;
    } else {
        if (d->_destinationId != QMailFolderId(QMailFolder::TrashFolder)) {
            // If they were in Trash folder before, they are trash no longer
            if (!QMailStore::instance()->updateMessagesMetaData(idsKey, QMailMessage::Trash, false)) {
                qMailLog(Messaging) << "Unable to mark messages as not Trash!";
            }
        }

        emit d->_service->progressChanged(total, total);
        emit messagesMoved(d->_ids);
        emit d->_service->activityChanged(QMailServiceAction::Successful);
        emit d->_service->actionCompleted(true);
        return;
    }

    emit d->_service->statusChanged(QMailServiceAction::Status(QMailServiceAction::Status::ErrFrameworkFault, tr("Unable to move messages to folder"), QMailAccountId(), QMailFolderId(), QMailMessageId()));
    emit d->_service->activityChanged(QMailServiceAction::Failed);
    emit d->_service->actionCompleted(false);
}


class QMailMessageSinkPrivate
{
public:
    QMailMessageSinkPrivate(QMailMessageService *service);

    QMailMessageService *_service;
};

QMailMessageSinkPrivate::QMailMessageSinkPrivate(QMailMessageService *service)
    : _service(service)
{
}


/*!
    \class QMailMessageSink
    \ingroup libmessageserver

    \brief The QMailMessageSink class defines the interface to objects that provide external message transmission
    services to the messageserver.

    The Qt Extended messageserver uses the QMailMessageSink interface to cooperate with components loaded
    from plugin libraries, that act as external transmitters of messaging data for the messaging framework.  
    Instances of QMailMessageSink are not created directly by the messageserver, but are exported by 
    QMailMessageService objects via their \l{QMailMessageService::sink()}{sink} function.
    
    \sa QMailMessageService
*/

/*!
    Creates a message sink object associated with the service \a service.
*/
QMailMessageSink::QMailMessageSink(QMailMessageService *service)
    : d(new QMailMessageSinkPrivate(service))
{
}

/*! \internal */
QMailMessageSink::~QMailMessageSink()
{
    delete d;
}

/*!
    Invoked by the message server to initiate a transmission operation.

    Attempt to transmit each message listed in \a ids to the external server.

    Successfully transmitted messages should be progressively reported via messagesTransmitted().

    Return true if an operation is initiated.
*/
bool QMailMessageSink::transmitMessages(const QMailMessageIdList &ids)
{
    notImplemented();
    return false;

    Q_UNUSED(ids)
}

/*! \internal */
void QMailMessageSink::notImplemented()
{
    d->_service->updateStatus(QMailServiceAction::Status::ErrNotImplemented, QString());
    emit d->_service->actionCompleted(false);
}

/*!
    \fn void QMailMessageSink::messagesTransmitted(const QMailMessageIdList &ids);

    Signal emitted by the sink to report the successful transmission of the messages listed in \a ids.
*/


/*!
    \class QMailMessageService
    \ingroup libmessageserver

    \preliminary
    \brief The QMailMessageService class provides the interface between the message server and components loaded
    from plugin libraries.

    QMailMessageService provides the interface through which the message server daemon communicates with
    components that provide message access and transmission services.  The components are loaded from plugin 
    libraries; the message server process remains ignorant of the messages types they deal with, and the 
    protocols they use to perform their tasks.

    The QMailMessageService class provides the signals and functions that message server uses to receive
    information about the actions of messaging service components.  It also provides the 
    \l{QMailMessageService::source()}{source} and \l{QMailMessageService::sink()}{sink} functions that 
    the message server uses to acquire access to the functionality that the service may implement.

    Subclasses of QMailMessageService are instantiated by the message server process, one for each 
    enabled account that is configured to use that service.  The QMailMessageService interface does
    not cater for concurrent actions; each instance may only service a single request at any given
    time.  The message server process provides request queueing so that QMailMessageService objects
    see only a sequential series of requests.
*/

/*!
    Constructs a messaging service object.
*/
QMailMessageService::QMailMessageService()
{
}

/*! \internal */
QMailMessageService::~QMailMessageService()
{
}

/*!
    \fn QString QMailMessageService::service() const;

    Returns the identifier of this service.
*/

/*!
    \fn QMailAccountId QMailMessageService::accountId() const;

    Returns the identifier of the account for which this service is configured.
*/

/*!
    Returns true if this service exports a QMailMessageSource interface.
*/
bool QMailMessageService::hasSource() const
{
    return false;
}

/*!
    Returns the QMailMessageSource interface exported by the service, if there is one.

    \sa hasSource()
*/
QMailMessageSource &QMailMessageService::source() const
{
    Q_ASSERT(0);
    return *(reinterpret_cast<QMailMessageSource*>(0));
}

/*!
    Returns true if this service exports a QMailMessageSink interface.
*/
bool QMailMessageService::hasSink() const
{
    return false;
}

/*!
    Returns the QMailMessageSink interface exported by the service, if there is one.

    \sa hasSink()
*/
QMailMessageSink &QMailMessageService::sink() const
{
    Q_ASSERT(0);
    return *(reinterpret_cast<QMailMessageSink*>(0));
}

/*!
    \fn bool QMailMessageService::available() const;

    Returns true if the service is currently available to process client requests.
*/

/*!
    \fn bool QMailMessageService::cancelOperation();

    Invoked by the message server to attempt cancellation of any request currently in progress.
    Return true to indicate cancellation of the request attempt.
*/
    
/*!
    \fn void QMailMessageService::availabilityChanged(bool available);

    Signal emitted by the service to report a change in the availability of the service to \a available.

    \sa available()
*/
    
/*!
    \fn void QMailMessageService::connectivityChanged(QMailServiceAction::Connectivity connectivity);

    Signal emitted by the service to report a change in the connectivity of the service.
    The new connectivity status is described by \a connectivity.

    Emitting this signal will reset the expiry timer for a service operation in progress.
*/
    
/*!
    \fn void QMailMessageService::activityChanged(QMailServiceAction::Activity activity);

    Signal emitted by the service to report a change in the activity of the service's current operation.
    The new activity status is described by \a activity.

    Emitting this signal will reset the expiry timer for a service operation in progress.
*/
    
/*!
    \fn void QMailMessageService::statusChanged(const QMailServiceAction::Status status);

    Signal emitted by the service to report a change in the status of the service's current operation.
    The new status is described by \a status.

    Emitting this signal will reset the expiry timer for a service operation in progress.
*/
    
/*!
    \fn void QMailMessageService::progressChanged(uint progress, uint total);

    Signal emitted by the service to report a change in the progress of the service's current operation;
    \a total indicates the extent of the operation to be performed, \a progress indicates the current degree of completion.

    Emitting this signal will reset the expiry timer for a service operation in progress.
*/
    
/*!
    \fn void QMailMessageService::actionCompleted(bool success);

    Signal emitted by the service to report the completion of an operation, with result \a success.
*/

/*!
    Emits the statusChanged() signal with the Status object constructed from \a code, \a text, \a accountId, \a folderId and \a messageId.

    If possible, a standardized error message is determined from \a code, and prepended to the error message.
*/
void QMailMessageService::updateStatus(QMailServiceAction::Status::ErrorCode code, const QString &text, const QMailAccountId &accountId, const QMailFolderId &folderId, const QMailMessageId &messageId)
{
    if (code == QMailServiceAction::Status::ErrNoError) {
        emit statusChanged(QMailServiceAction::Status(QMailServiceAction::Status::ErrNoError, text, accountId, folderId, messageId));
    } else {
        static ErrorMap mailErrorMap(mailErrorInit());

        // See if we can convert the error code into a readable message
        QString message(text);
        decorate(&message, code, (ErrorSet() << mailErrorMap));

        emit statusChanged(QMailServiceAction::Status(code, message, accountId, folderId, messageId));
    }
}

/*!
    Emits the statusChanged() signal with the Status object constructed from \a code, \a text, \a accountId, \a folderId and \a messageId.

    If possible, a standardized error message is determined from \a code, and prepended to the error message.
*/
void QMailMessageService::updateStatus(int code, const QString &text, const QMailAccountId &accountId, const QMailFolderId &folderId, const QMailMessageId &messageId)
{
    if (code == QMailServiceAction::Status::ErrNoError) {
        emit statusChanged(QMailServiceAction::Status(QMailServiceAction::Status::ErrNoError, text, accountId, folderId, messageId));
    } else {
        static ErrorMap socketErrorMap(socketErrorInit());

        // Code has been offset by +2 on transmit to normalise range
        code -= 2;

        // See if we can convert the error code into a system error message
        QString message(text);
        decorate(&message, code, (ErrorSet() << socketErrorMap));

        emit statusChanged(QMailServiceAction::Status(QMailServiceAction::Status::ErrSystemError, message, accountId, folderId, messageId));
    }
}


/*!
    \class QMailMessageServiceEditor
    \ingroup libmessageserver

    \preliminary
    \brief The QMailMessageServiceEditor class provides an interface that allows a service to be edited graphically.

    QMailMessageServiceEditor provides the base class for a GUI component that can edit the configuration for a messaging service.
*/

/*! \internal */
QMailMessageServiceEditor::QMailMessageServiceEditor()
{
}

/*! \internal */
QMailMessageServiceEditor::~QMailMessageServiceEditor()
{
}

/*!
    \fn void QMailMessageServiceEditor::displayConfiguration(const QMailAccount &account, const QMailAccountConfiguration &config);

    Invoked to set the editor with the details of the account \a account, described by \a config.
*/

/*!
    \fn bool QMailMessageServiceEditor::updateAccount(QMailAccount *account, QMailAccountConfiguration *config);

    Invoked to update the account \a account and configuration \a config with the details currently displayed by the editor.
    Return true if the account and configuration are appropriately updated, and any necessary data storage external to the mail store has been performed.
*/

/*!
    \class QMailMessageServiceConfigurator
    \ingroup libmessageserver

    \preliminary
    \brief The QMailMessageServiceConfigurator class provides an interface that allows a service to be configured.

    QMailMessageServiceConfigurator provides the interface that a messaging service must provide to allow
    its configuration to be editted by a generic GUI editor framework.
*/

/*! \internal */
QMailMessageServiceConfigurator::QMailMessageServiceConfigurator()
{
}

/*! \internal */
QMailMessageServiceConfigurator::~QMailMessageServiceConfigurator()
{
}

/*!
    \fn QString QMailMessageServiceConfigurator::service() const;

    Returns the identifier of the service configured by this class.
*/

/*!
    \fn QString QMailMessageServiceConfigurator::displayName() const;

    Returns the name of the service configured by this class, in a form suitable for display.
*/

/*!
    Returns a list of services of the type \a type that are compatible with this service.
    If the service does not constrain the possible list of compatible services, an empty list should be returned.
*/
QStringList QMailMessageServiceConfigurator::serviceConstraints(QMailMessageServiceFactory::ServiceType) const
{
    return QStringList();
}

/*!
    \fn QMailMessageServiceEditor *QMailMessageServiceConfigurator::createEditor(QMailMessageServiceFactory::ServiceType type);

    Creates an instance of the editor class for the service of type \a type.
*/


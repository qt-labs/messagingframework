/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#include "qmailcontentmanager.h"
#include "qmailmessage.h"
#include "qmaillog.h"
#include "qmailpluginmanager.h"

#define PLUGIN_KEY "contentmanagers"

namespace {

typedef QMap<QString, QMailContentManager*> PluginMap;

PluginMap initMap(QMailPluginManager& manager)
{
    PluginMap map;

    foreach (const QString &item, manager.list()) {
        QObject *instance(manager.instance(item));
        if (QMailContentManagerPlugin *iface = qobject_cast<QMailContentManagerPlugin*>(instance))
            map.insert(iface->key(), iface->create());
    }

    return map;
}

PluginMap &pluginMap()
{
    static QMailPluginManager manager(PLUGIN_KEY);
    static PluginMap map(initMap(manager));
    return map;
}

QMailContentManager *mapping(const QString &scheme)
{
    PluginMap::const_iterator it = pluginMap().find(scheme);
    if (it != pluginMap().end())
        return it.value();

    qMailLog(Messaging) << "Unable to map content manager for scheme:" << scheme;
    return 0;
}

}

/*!
    \class QMailContentManagerFactory
    \inpublicgroup QtMessagingModule
    \inpublicgroup QtPimModule

    \brief The QMailContentManagerFactory class creates objects implementing the QMailContentManager interface.
    \ingroup messaginglibrary

    The QMailContentManagerFactory class creates objects that manage the storage and retrieval
    of message content via the QMailContentManger interface.  The factory allows implementations to
    be loaded from plugin libraries, and to be retrieved by the name of the content management
    scheme they implement.

    To create a new class that can be created via the QMailContentManagerFactory,
    implement a plugin that derives from QMailContentManagerPlugin.

    \sa QMailContentManager, QMailContentManagerPlugin
*/

/*!
    Returns a list of all content management schemes for which content manager objects can be instantiated by the factory.
*/
QStringList QMailContentManagerFactory::schemes()
{
    return pluginMap().keys();
}

/*!
    Returns the default content management scheme supported by the factory.
*/
QString QMailContentManagerFactory::defaultScheme()
{
    const QStringList &list(schemes());
    if (list.isEmpty())
        return QString();

    if (list.contains("qtopiamailfile"))
        return "qtopiamailfile";
    else 
        return list.first();
}

/*!
    Creates a content manager object for the scheme identified by \a scheme.
*/
QMailContentManager *QMailContentManagerFactory::create(const QString &scheme)
{
    return mapping(scheme);
}

/*!
    Clears the content managed by all content managers known to the factory.
*/
void QMailContentManagerFactory::clearContent()
{
    foreach (QMailContentManager *manager, pluginMap().values())
        manager->clearContent();
}


/*!
    \class QMailContentManagerPluginInterface
    \inpublicgroup QtMessagingModule
    \inpublicgroup QtPimModule

    \brief The QMailContentManagerPluginInterface class defines the interface to plugins that provide message content management facilities.
    \ingroup messaginglibrary

    The QMailContentManagerPluginInterface class defines the interface to message content manager plugins.  Plugins will 
    typically inherit from QMailContentManagerPlugin rather than this class.

    \sa QMailContentManagerPlugin, QMailContentManager, QMailContentManagerFactory
*/

/*!
    \fn QString QMailContentManagerPluginInterface::key() const

    Returns a string identifying the content management scheme implemented by the plugin.
*/

/*!
    \fn QMailContentManager* QMailContentManagerPluginInterface::create()

    Creates an instance of the QMailContentManager class provided by the plugin.
*/


/*!
    \class QMailContentManagerPlugin
    \inpublicgroup QtMessagingModule
    \inpublicgroup QtPimModule

    \brief The QMailContentManagerPlugin class defines a base class for implementing message content manager plugins.
    \ingroup messaginglibrary

    The QMailContentManagerPlugin class provides a base class for plugin classes that provide message content management
    functionality.  Classes that inherit QMailContentManagerPlugin need to provide overrides of the
    \l {QMailContentManagerPlugin::key()}{key} and \l {QMailContentManagerPlugin::create()}{create} member functions.

    \sa QMailContentManagerPluginInterface, QMailContentManager, QMailContentManagerFactory
*/

/*!
    Creates a message content manager plugin instance.
*/
QMailContentManagerPlugin::QMailContentManagerPlugin()
{
}

/*!
    Destroys the QMailContentManagerPlugin object.
*/
QMailContentManagerPlugin::~QMailContentManagerPlugin()
{
}

/*!
    Returns the list of interfaces implemented by this plugin.
*/
QStringList QMailContentManagerPlugin::keys() const
{
    return QStringList() << "QMailContentManagerPluginInterface";
}


/*!
    \class QMailContentManager
    \inpublicgroup QtMessagingModule
    \inpublicgroup QtPimModule

    \brief The QMailContentManager class defines the interface to objects that provide a storage facility for message content.
    \ingroup messaginglibrary

    Qt Extended uses the QMailContentManager interface to delegate the storage and retrieval of message content from the
    QMailStore class to classes loaded from plugin libraries.  A library may provide this service by exporting a 
    class implementing the QMailContentManager interface, and an associated instance of QMailContentManagerPlugin.
    
    The content manager used to store the content of a message is determined by the 
    \l{QMailMessageMetaData::contentScheme()}{contentScheme} function of a QMailMessage object.  The identifier of
    the message content is provided by the corresponding \l{QMailMessageMetaData::contentIdentifier()}{contentIdentifier} 
    function; this property is provided for the use of the content manager code, and is opaque to the remainder of the
    system.  A QMailMessage object may be associated with a particular content manager by calling
    \l{QMailMessageMetaData::setContentScheme()}{setContentScheme} to set the relevant scheme before adding the 
    message to the mail store.

    \sa QMailStore, QMailMessage
*/

/*!
    \fn QMailStore::ErrorCode QMailContentManager::add(QMailMessage *message)

    Requests that the content manager add the content of \a message to its storage.  The 
    message should be updated such that its \l{QMailMessageMetaData::contentIdentifier()}{contentIdentifier} 
    property contains the location at which the content is stored.  
    Returns \l{QMailStore::NoError}{NoError} to indicate successful addition of the message content to permanent storage.

    If \l{QMailMessageMetaData::contentIdentifier()}{contentIdentifier} is already populated at invocation, 
    the content manager should determine whether the supplied identifier can be used.  If not, it should 
    use an alternate location and update \a message with the new identifier.
*/

/*!
    \fn QMailStore::ErrorCode QMailContentManager::update(QMailMessage *message)

    Requests that the content manager update the message content stored at the location indicated 
    by \l{QMailMessageMetaData::contentIdentifier()}{contentIdentifier}, to contain the current 
    content of \a message.  
    Returns \l{QMailStore::NoError}{NoError} to indicate successful update of the message content.

    If the updated content is not stored to the existing location, the content manager should 
    use an alternate location and update \a message with the new identifier.
*/

/*!
    \fn QMailStore::ErrorCode QMailContentManager::remove(const QString &identifier)

    Requests that the content manager remove the message content stored at the location indicated
    by \a identifier.  Returns \l{QMailStore::NoError}{NoError} to indicate that the message content has been successfully
    removed.

    If the identified content does not already exist, the content manager should return \l{QMailStore::InvalidId}{InvalidId}.
*/

/*!
    \fn QMailStore::ErrorCode QMailContentManager::load(const QString &identifier, QMailMessage *message)

    Requests that the content manager load the message content stored at the location indicated
    by \a identifier into the message record \a message.  Returns \l{QMailStore::NoError}{NoError} to indicate that the message 
    content has been successfully loaded.

    If the identified content does not already exist, the content manager should return \l{QMailStore::InvalidId}{InvalidId}.
*/

/*! \internal */
QMailContentManager::QMailContentManager()
{
}

/*! \internal */
QMailContentManager::~QMailContentManager()
{
}

/*!
    Directs the content manager to clear any message content that it is responsible for.

    This function is called by the mail store to remove all existing data, typically in test conditions.
*/
void QMailContentManager::clearContent()
{
}


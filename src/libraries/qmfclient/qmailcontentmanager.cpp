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

#include "qmailcontentmanager.h"
#include "qmailmessage.h"
#include "qmaillog.h"
#include "qmailpluginmanager.h"

#define PLUGIN_KEY "contentmanagers"

namespace {

typedef QMap<QString, QMailContentManager*> ContentPluginMap;

ContentPluginMap init(QMailPluginManager& manager)
{
    ContentPluginMap map;

    foreach (const QString &item, manager.list()) {
        QObject *instance(manager.instance(item));
        if (QMailContentManagerPlugin *iface = qobject_cast<QMailContentManagerPlugin*>(instance))
            map.insert(iface->key(), iface->create());
    }

    if(map.isEmpty()) {
        qMailLog(Messaging) << "No content manager plugins found. Message content will not be available.";
    }

    return map;
}

QString & defaultFilter()
{
    static QString filter;
    return filter;
}

QString & defaultIndexer()
{
    static QString index;
    return index;
}

ContentPluginMap &contentPlugins()
{
    static QMailPluginManager manager(PLUGIN_KEY);
    static ContentPluginMap map(init(manager));
    return map;
}

QMailContentManager *mapping(const QString &scheme)
{
    ContentPluginMap::const_iterator it = contentPlugins().find(scheme);
    if (it != contentPlugins().end())
        return it.value();

    qMailLog(Messaging) << "Unable to map content manager for scheme:" << scheme;
    return 0;
}

}

/*!
    \class QMailContentManagerFactory

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
    return contentPlugins().keys();
}


/*!
    Returns the default filter content manager scheme for filtering supported by the factory. Returns an empty string
    if none are found.
*/
QString QMailContentManagerFactory::defaultFilterScheme()
{
    return defaultFilter();
}

/*!
    Returns the default content indexing scheme supported by the factory. Returns an empty string
    if none are found.
*/

QString QMailContentManagerFactory::defaultIndexerScheme()
{
    return defaultIndexer();
}

/*!
    Returns the default content management scheme supported by the factory.
*/
QString QMailContentManagerFactory::defaultScheme()
{
    const QStringList &list(schemes());
    if (list.isEmpty())
        return QString();

    if (list.contains("qmfstoragemanager"))
        return "qmfstoragemanager";
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
    Performs any initialization tasks for content managers known to the factory.
    Returns false if any content managers are unable to perform initialiation tasks.
*/
bool QMailContentManagerFactory::init()
{
    ContentPluginMap::iterator end(contentPlugins().end());
    for (ContentPluginMap::iterator it(contentPlugins().begin()); it != end; ++it) {
        if (!(*it)->init())
            return false;
        else if (defaultFilter().isEmpty() && (*it)->role() == QMailContentManager::FilterRole)
            defaultFilter() = it.key();
        else if (defaultIndexer().isEmpty() && (*it)->role() == QMailContentManager::IndexRole)
            defaultIndexer() = it.key();

    }
    return true;
}

/*!
    Clears the content managed by all content managers known to the factory.
*/
void QMailContentManagerFactory::clearContent()
{
    foreach (QMailContentManager *manager, contentPlugins().values())
        manager->clearContent();
}


/*!
    \class QMailContentManagerPluginInterface

    \brief The QMailContentManagerPluginInterface class defines the interface to plugins that provide message content management facilities.
    \ingroup messaginglibrary

    The QMailContentManagerPluginInterface class defines the interface to message content manager plugins.  Plugins will 
    typically inherit from QMailContentManagerPlugin rather than this class.

    \sa QMailContentManagerPlugin, QMailContentManager, QMailContentManagerFactory
*/

/*!
    \fn QString QMailContentManagerPluginInterface::key() const

    Returns a unique string identifying the content management scheme implemented by the plugin.
*/

/*!
    \fn QMailContentManager* QMailContentManagerPluginInterface::create()

    Creates an instance of the QMailContentManager class provided by the plugin.
*/


/*!
    \class QMailContentManagerPlugin

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

    \brief The QMailContentManager class defines the interface to objects that provide a storage facility for message content.
    \ingroup messaginglibrary

    Qt Extended uses the QMailContentManager interface to delegate the filtering, storage, retrieval and indexing of message content
    from the QMailStore class to classes loaded from plugin libraries.  A library may provide this service by exporting a
    class implementing the QMailContentManager interface, and an associated instance of QMailContentManagerPlugin.

    There are three roles a content manager can take, as defined by \l{QMailContentManager::ManagerRole}{ManagerRole}. This changes
    the order in which the plugins are called.

    FilterRole: A content manager may be defined as a filtering content manager by returning FilterRole from
    \l{QMailContentManager::role()}{Role}. Filtering content managers are called before message operations take place, and before
    the message is stored on the file system. Filtering content managers may change any details, and messages will be added to the database with
    the changes. As messages are not in the database, they will not have an id -- and setting one is invalid. Filtering content managers
    are not notified when a message is deleted, so they should be careful not to leak file system space. Filtering content managers
    should only return errors in fatal errors, not to indicate they filtered a message or not.

    IndexRole: A content manager may be defined as an indexing content manager by returning IndexRole from
    \l{QMailContentManager::role()}{Role}. Indexing content managers are called after successful operations have taken place,
    and the fields updated. An indexing content manager should not modify existing messages, but only be used to do something
    external to QMF (e.g. Add/Remove the message from an external index). Indexing content manager should only return errors
    in very serious circumstances.

    StorageRole: A content manager may be defined as a storage content manager by returning StorageRole from
    \l{QMailContentManager::role()}{Role} (the default). Storage content managers are used for saving and
    loading messages from the file system. The particular storage content managed used for a message is determined by
    the \l{QMailMessageMetaData::contentScheme()}{contentScheme} function of a QMailMessage object.  The identifier of
    the message content is provided by the corresponding \l{QMailMessageMetaData::contentIdentifier()}{contentIdentifier} 
    function; this property is provided for the use of the storage content manager code, and is opaque to the remainder of the
    system.

    If a storage content manager provides data to clients by creating references to file-backed memory
    mappings, then the content manager must ensure that those files remain valid.  The existing content
    within the file must not be modified, and the file must not be truncated.  If the content manager
    updates the content of a message which is already exported using memory mappings, then the updated 
    content should be stored to a new content location, and the message object updated with the new 
    \l{QMailMessageMetaData::contentIdentifier()}{contentIdentifier} information.

    If a stored message contains parts whose content is only partially available (as defined by
    QMailMessagePartContainer::partialContentAvailable() and QMailMessagePartContainer::contentAvailable()), 
    the content manager must ensure that the partial data is returned to clients in the same transfer 
    encoding that it was stored with.

    \sa QMailStore, QMailMessage
*/

/*!
    \enum QMailContentManager::DurabilityRequirement
    
    This enum type is used to define the dequirement for durability in a content management request.

    \value EnsureDurability     The content manager should make the requested changes durable before reporting success.
    \value DeferDurability      The content manager may defer ensuring that changes are durable until the next invocation of ensureDurability().
    \value NoDurability         The content manager will not make changes durable.
*/

/*!
    \enum QMailContentManager::ManagerRole

    This enum type is used to define the purpose of the plugin

    \value FilterRole The content manager is a filtering content manager. It is used to modify messages
           that match its own criteria. Filtering content managers are called before any others.
    \value StorageRole The content manager is a storage content manager. It is used to storage messages
           persistently. Storage content managers are called after filter content managers, but
           before index content managers.
    \value IndexRole The content manager is for indexing purposes. It is called after the message has been
           stored.

*/

/*!
    \fn QMailStore::ErrorCode QMailContentManager::add(QMailMessage *message, QMailContentManager::DurabilityRequirement durability)

    Tells the content manager of a new \a message. If this is a storage content manager it should add to storage and then update the
    message such that its \l{QMailMessageMetaData::contentIdentifier()}{contentIdentifier}
    property contains the location at which the content is stored.

    Returns \l{QMailStore::NoError}{NoError} to indicate success.

    If \l{QMailMessageMetaData::contentIdentifier()}{contentIdentifier} is already populated at invocation, 
    the content manager should determine whether the supplied identifier can be used.  If not, it should 
    use an alternate location and update \a message with the new identifier.

    If \a durability is \l{QMailContentManager::EnsureDurability}{EnsureDurability} then the content
    manager should ensure that the message addition has been recorded in a durable fashion before reporting
    success to the caller.
*/

/*!
    \fn QMailStore::ErrorCode QMailContentManager::update(QMailMessage *message, QMailContentManager::DurabilityRequirement durability)

    Notifies the content manager that \a message has been updated. If this is a storage content manager
    it should update the message content stored at the location indicated
    by \l{QMailMessageMetaData::contentIdentifier()}{contentIdentifier}, to contain the current 
    content of \a message.  
    Returns \l{QMailStore::NoError}{NoError} to indicate successful update of the message content.

    If the updated content is not stored to the existing location, the content manager should 
    use an alternate location and update \a message with the new 
    \l{QMailMessageMetaData::contentIdentifier()}{contentIdentifier}.

    The existing content should be removed if the update causes a new content identifier to 
    be allocated.  If the previous content cannot be removed, but the update was otherwise 
    successful, the content manager should return \l{QMailStore::ContentNotRemoved}{ContentNotRemoved} 
    to indicate that removal of the content should be retried at a later time.

    If \a durability is \l{QMailContentManager::EnsureDurability}{EnsureDurability} then the content
    manager should ensure that the message update has been recorded in a durable fashion before reporting
    success to the caller.
*/

/*!
    \fn QMailStore::ErrorCode QMailContentManager::ensureDurability()

    Requests that the content manager ensure that any previous actions that were performed with the
    \l{QMailContentManager::DeferDurability}{DeferDurability} option be made durable.
*/

/*!
    \fn QMailStore::ErrorCode QMailContentManager::ensureDurability(const QList<QString> &identifiers)

    Requests that the content manager ensure that message data will be made durable for messages
    identified by the given content \a identifiers.

    To be used with the \l{QMailContentManager::DeferDurability}{NoDurability} option.
*/

/*!
    \fn QMailStore::ErrorCode QMailContentManager::remove(const QString &identifier)

    Notifies the content manager that the message identified by \a identifier has been removed.

    The content manager should only remove data it has created with association to identifier. For instance
    a content manager that does indexing, should only remove the data it has created reaction to the identifier,
    but not the file itself.

    Returns \l{QMailStore::NoError}{NoError} to indicate that the message content
    has been successfully removed.

    If the content cannot be removed, the content manager should return 
    \l{QMailStore::ContentNotRemoved}{ContentNotRemoved} to indicate that removal of the content
    should be retried at a later time.

    If the identified content does not already exist, the content manager should return \l{QMailStore::InvalidId}{InvalidId}.
*/

/*!
    \fn QMailStore::ErrorCode QMailContentManager::load(const QString &identifier, QMailMessage *message)

    Requests that the storage content manager load the message content stored at the location indicated
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
    Notifies the content manager that the messages identified by \a identifiers have been removed.

    The content manager should only remove data it has created with association to identifier. For instance
    a content manager that does indexing, should only remove the data it has created reaction to the identifier,
    but not the file itself.

    Returns \l{QMailStore::NoError}{NoError} to indicate that the message content
    has been successfully removed.

    If the content cannot be removed, the content manager should return
    \l{QMailStore::ContentNotRemoved}{ContentNotRemoved} to indicate that removal of the content
    should be retried at a later time.

    If the identified content does not already exist, the content manager should return \l{QMailStore::InvalidId}{InvalidId}.
*/

QMailStore::ErrorCode QMailContentManager::remove(const QList<QString> &identifiers)
{
     QMailStore::ErrorCode returnError(QMailStore::NoError);
    foreach(QString const &identifier, identifiers) {
        QMailStore::ErrorCode err(remove(identifier));
        if (returnError == QMailStore::NoError) // report the first failure, but continue.
            returnError = err;
    }

    return returnError;
}


/*!
    Directs the content manager to perform any initialization tasks required.
    The content manager should return false if unable to perform initialization tasks; otherwise return true.

    This function is called by the mail store after it has been successfully initialized.
*/
bool QMailContentManager::init()
{
    return true;
}

/*!
    Directs the content manager to clear any message content that it is responsible for.

    This function is called by the mail store to remove all existing data, typically in test conditions.
*/
void QMailContentManager::clearContent()
{
}

/*!
    This function is called by the mail store to query the purpose of this particular plugin.
*/
QMailContentManager::ManagerRole QMailContentManager::role() const
{
    return StorageRole;
}


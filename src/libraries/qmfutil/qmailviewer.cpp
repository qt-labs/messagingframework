/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#include "qmailviewer.h"
#include <QApplication>
#include <QIcon>
#include <QMap>
#include <QWidget>
#include <qmaillog.h>
#include <qmailpluginmanager.h>

#define PLUGIN_KEY "viewers"

typedef QMap<QString, QMailViewerInterface*> PluginMap;

// Load all the viewer plugins into a map for quicker reference
static PluginMap initMap(QMailPluginManager& manager)
{
    PluginMap map;

    foreach (const QString &item, manager.list()) {
        QObject *instance = manager.instance(item);
        if (QMailViewerInterface* iface = qobject_cast<QMailViewerInterface*>(instance))
                map.insert(iface->key(), iface);
    }
    return map;
}

// Return a reference to a map containing all loaded plugin objects
static PluginMap& pluginMap()
{
    static QMailPluginManager pluginManager(PLUGIN_KEY);
    static PluginMap map(initMap(pluginManager));

    return map;
}

// Return the viewer plugin object matching the specified ID
static QMailViewerInterface* mapping(const QString& key)
{
    PluginMap::ConstIterator it;
    if ((it = pluginMap().find(key)) != pluginMap().end())
        return it.value();

    return 0;
}

/*!
    \class QMailViewerInterface
    \inpublicgroup QtMessagingModule
    \ingroup qmfutil

    \brief The QMailViewerInterface class defines the interface to objects that can display a mail message.

    Qt Extended uses the QMailViewerInterface interface for displaying mail messages.  A class may implement the
    QMailViewerInterface interface to display a mail message format.

    The message to be displayed is provided to the viewer class using the \l {QMailViewerInterface::setMessage()}
    {setMessage()} function.  If the message refers to external resources, these should be provided using the
    \l {QMailViewerInterface::setResource()}{setResource()} function.  The  \l {QMailViewerInterface::clear()}{clear()}
    function clears any message or resources previously set.

    The viewer object should emit the \l {QMailViewerInterface::anchorClicked()}{anchorClicked()} signal if the user 
    selects a link in the message.  If the message supports a concept of completion, then the 
    \l {QMailViewerInterface::finished()}{finished()} signal should be emitted after the display has been completed.

    Rather than creating objects that implement the QMailViewerInterface directly, clients should create an object
    of an appropriate type by using the QMailViewerFactory class:

    \code
    QString key = QMailViewerFactory::defaultKey( QMailViewerFactory::SmilContent );
    QMailViewerInterface* smilViewer = QMailViewerFactory::create( key, this, "smilViewer" );
    \endcode

    \sa QMailViewerFactory
*/

/*!
    \fn bool QMailViewerInterface::setMessage(const QMailMessage& mail)

    Displays the contents of \a mail.  Returns whether the message could be successfully displayed.
*/

/*!
    \fn void QMailViewerInterface::clear()

    Resets the display to have no content, and removes any resource associations.
*/

/*!
    \fn QWidget* QMailViewerInterface::widget() const

    Returns the widget implementing the display interface.
*/

/*!
    \fn QMailViewerInterface::replyToSender()

    This signal is emitted by the viewer to initiate a reply action.
*/

/*!
    \fn QMailViewerInterface::replyToAll()

    This signal is emitted by the viewer to initiate a reply-to-all action.
*/

/*!
    \fn QMailViewerInterface::forwardMessage()

    This signal is emitted by the viewer to initiate a message forwarding action.
*/

/*!
    \fn QMailViewerInterface::completeMessage()

    This signal is emitted by the viewer to initiate a message completion action.  
    This is only meaningful if the message has not yet been completely retrieved.

    \sa QMailMessage::status(), QMailRetrievalAction::retrieveMessages()
*/

/*!
    \fn QMailViewerInterface::deleteMessage()

    This signal is emitted by the viewer to initiate a message deletion action.
*/

/*!
    \fn QMailViewerInterface::saveSender()

    This signal is emitted by the viewer to request that the sender's address should be saved as a Contact.
*/

/*!
    \fn QMailViewerInterface::contactDetails(const QContact &contact)

    This signal is emitted by the viewer to request a display of \a{contact}'s details.
*/

/*!
    \fn QMailViewerInterface::anchorClicked(const QUrl& link)

    This signal is emitted when the user presses the select key while the display has the 
    anchor \a link selected.
*/

/*!
    \fn QMailViewerInterface::messageChanged(const QMailMessageId &id)

    This signal is emitted by the viewer to report that it is now viewing a different message, 
    identified by \a id.
*/

/*!
    \fn QMailViewerInterface::viewMessage(const QMailMessageId &id, QMailViewerFactory::PresentationType type)

    This signal is emitted by the viewer to request a display of the message identified by \a id, 
    using the presentation style \a type.
*/

/*!
    \fn QMailViewerInterface::sendMessage(const QMailMessage &message)

    This signal is emitted by the viewer to send a new message, whose contents are held by \a message.
*/

/*!
    \fn QMailViewerInterface::finished()

    This signal is emitted when the display of the current mail message is completed.  This signal 
    is emitted only for message types that define a concept of completion, such as SMIL slideshows.
*/

/*!
    \fn QMailViewerInterface::retrieveMessagePart(const QMailMessagePart &part)

    This signal is emitted by the viewer to initiate a message part retrieval action for \a part.
*/

/*!
    Constructs the QMailViewerInterface object with the parent widget \a parent.
*/
QMailViewerInterface::QMailViewerInterface( QWidget *parent )
    : QObject( parent )
{
}

/*! 
    Destructs the QMailViewerInterface object.
*/
QMailViewerInterface::~QMailViewerInterface()
{
}

/*!
    Scrolls the display to position the \a link within the viewable area.
*/
void QMailViewerInterface::scrollToAnchor(const QString& link)
{
    // default implementation does nothing
    Q_UNUSED(link)
}

/*!
    Allows the viewer object to add any relevant actions to the application \a menu supplied.
*/
void QMailViewerInterface::addActions( QMenu* menu ) const
{
    // default implementation does nothing
    Q_UNUSED(menu)
}

/*!
    Allows the viewer object to handle the notification of the arrival of new messages, 
    identified by \a list.

    Return true to indicate that the event has been handled, or false to let the caller handle
    the new message event.
*/
bool QMailViewerInterface::handleIncomingMessages( const QMailMessageIdList &list ) const
{
    // default implementation does nothing
    Q_UNUSED(list)
    return false;
}

/*!
    Allows the viewer object to handle the notification of the transmission of queued messages, 
    identified by \a list.

    Return true to indicate that the event has been handled, or false to let the caller handle
    the new message event.
*/
bool QMailViewerInterface::handleOutgoingMessages( const QMailMessageIdList &list ) const
{
    // default implementation does nothing
    Q_UNUSED(list)
    return false;
}

/*! 
    Supplies the viewer object with a resource that may be referenced by a mail message.  The resource 
    identified as \a name will be displayed as the object \a value.  
*/
void QMailViewerInterface::setResource(const QUrl& name, QVariant value)
{
    // default implementation does nothing
    Q_UNUSED(name)
    Q_UNUSED(value)
}


/*!
    \class QMailViewerFactory
    \inpublicgroup QtMessagingModule
    \ingroup qmfutil

    \brief The QMailViewerFactory class creates objects implementing the QMailViewerInterface interface.

    The QMailViewerFactory class creates objects that are able to display mail messages, and 
    implement the QMailViewerInterface interface.  The factory chooses an implementation
    based on the type of message to be displayed.

    The QMailViewerInterface class describes the interface supported by classes that can be created
    by the QMailViewerFactory class.  To create a new class that can be created via the QMailViewerFactory,
    implement a plug-in that derives from QMailViewerInterface.

    \sa QMailViewerInterface
*/

/*!
    \enum QMailViewerFactory::PresentationType

    This enum defines the types of presentation that message viewer components may implement.

    \value AnyPresentation Do not specify the type of presentation to be employed.
    \value StandardPresentation Present the message in the standard fashion for the relevant content type.
    \value ConversationPresentation Present the message in the context of a conversation with a contact.
    \value UserPresentation The first value that can be used for application-specific purposes.
*/

/*!
    Returns a list of keys identifying classes that can display a message containing \a type content,
    using the presentation type \a pres.
*/
QStringList QMailViewerFactory::keys(QMailMessage::ContentType type, PresentationType pres)
{
    QStringList in;

    foreach (PluginMap::mapped_type plugin, pluginMap())
        if (plugin->isSupported(type, pres))
            in << plugin->key();

    return in;
}

/*!
    Returns the key identifying the first class found that can display message containing \a type content,
    using the presentation type \a pres.
*/
QString QMailViewerFactory::defaultKey(QMailMessage::ContentType type, PresentationType pres)
{
    QStringList list(QMailViewerFactory::keys(type, pres));
    return (list.isEmpty() ? QString() : list.first());
}

/*!
    Creates a viewer object of the class identified by \a key, setting the returned object to
    have the parent widget \a parent.
*/
QMailViewerInterface *QMailViewerFactory::create(const QString &key, QWidget *parent)
{
    Q_UNUSED(parent);
    return mapping(key);
}


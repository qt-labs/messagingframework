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

#include "qmailcomposer.h"
#include <QApplication>
#include <QIcon>
#include <QMap>
#include <QWidget>
#include <qmaillog.h>
#include <qmailpluginmanager.h>
#include <qmailaccount.h>
#include <qmailmessage.h>

#define PLUGIN_KEY "composers"

typedef QMap<QString, QMailComposerInterface*> PluginMap;

// Load all the viewer plugins into a map for quicker reference
static PluginMap initMap(QMailPluginManager& manager)
{
    PluginMap map;

    foreach (const QString &item, manager.list()) {
        QObject *instance = manager.instance(item);
        if (QMailComposerInterface* iface = qobject_cast<QMailComposerInterface*>(instance))
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

// Return the composer plugin object matching the specified ID
static QMailComposerInterface* mapping(const QString& key)
{
    PluginMap::ConstIterator it;
    if ((it = pluginMap().find(key)) != pluginMap().end())
        return it.value();

    qWarning() << "Failed attempt to map composer:" << key;
    return 0;
}

/*!
    \class QMailComposerInterface
    \ingroup qmfutil

    \brief The QMailComposerInterface class defines the interface to objects that can compose a mail message.

    Qt Extended uses the QMailComposerInterface interface for composing mail messages.  A class may implement the
    QMailComposerInterface interface to compose a mail message format.

    The composer class may start composing with no associated message, or it may be provided with an existing
    message to edit, via the \l {QMailComposerInterface::compose()}{compose()} function.
    A client can query whether the composer object is empty with the
    \l {QMailComposerInterface::isEmpty()}{isEmpty()} function, and extract the
    composed message with the \l {QMailComposerInterface::message()}{message()} function.  The current 
    state of composition can be cleared with the \l {QMailComposerInterface::clear()}{clear()} function.

    The composer object should emit the \l {QMailComposerInterface::changed()}{changed()} signal
    whenever the composed message changes. If composition is cancelled, the composer should emit the
    \l {QMailComposerInterface::cancel()}{cancel()} signal. When the message is ready to send, the composer should
    emit the \l {QMailComposerInterface::sendMessage()}{sendMessage()} signal. For composers which need to inform
    of state changes during composition, such as multi-page composers,
    the \l {QMailComposerInterface::statusChanged()}{statusChanged()} signal should be emitted to allow container
    objects to update their view of the \l {QMailComposerInterface::status()}{status()} string.

    Each composer class must export metadata describing itself and the messages it is able to compose.  To do
    this, the composer must implement the
    \l {QMailComposerInterface::key()}{key()},
    \l {QMailComposerInterface::messageTypes()}{messageTypes()},
    \l {QMailComposerInterface::name()}{name()},
    \l {QMailComposerInterface::displayName()}{displayName()} and
    \l {QMailComposerInterface::displayIcon()}{displayIcon()} functions.

    \code
    QString key = QMailComposerFactory::defaultKey( QMailMessage::Email );
    QMailComposerInterface* emailComposer = QMailComposerFactory::create( key, this, "emailComposer" );
    \endcode

    \sa QMailComposerFactory
*/

/*!
    Constructs the QMailComposerInterface object with the parent widget \a parent.
*/
QMailComposerInterface::QMailComposerInterface( QWidget *parent )
    : QWidget( parent )
{
}

/*!
    Returns a string identifying the composer.
*/

QString QMailComposerInterface::key() const
{
    QString val = metaObject()->className();
    val.chop(9); // remove trailing "Interface"
    return val;
}

/*!
    Returns the message types created by the composer.
*/
QList<QMailMessage::MessageType> QMailComposerInterface::messageTypes() const
{
    return mapping(key())->messageTypes();
}

/*!
    Returns the content types created by the composer.
*/
QList<QMailMessage::ContentType> QMailComposerInterface::contentTypes() const
{
    return mapping(key())->contentTypes();
}

/*!
    Returns the translated name of the message type \a type created by the composer.
*/
QString QMailComposerInterface::name(QMailMessage::MessageType type) const
{
    return mapping(key())->name(type);
}

/*!
    Returns the translated name of the message type \a type created by the composer,
    in a form suitable for display on a button or menu.
*/
QString QMailComposerInterface::displayName(QMailMessage::MessageType type) const
{
    return mapping(key())->displayName(type);
}

/*!
    Returns the icon representing the message type \a type created by the composer.
*/
QIcon QMailComposerInterface::displayIcon(QMailMessage::MessageType type) const
{
    return mapping(key())->displayIcon(type);
}

/* !
    Adds \a item as an attachment to the message in the composer. The \a action parameter
    specifies what the composer should do with \a item.
void QMailComposerInterface::attach( const QContent& item, QMailMessage::AttachmentsAction action )
{
    // default implementation does nothing
    Q_UNUSED(item)
    Q_UNUSED(action)
}
*/

/*!
    Sets the composer to append \a signature to the body of the message, when creating a message.
*/
void QMailComposerInterface::setSignature(const QString& signature)
{
    // default implementation does nothing
    Q_UNUSED(signature)
}

/*!
    Sets the composer to use the account identified by \a accountId for outgoing messages.
*/
void QMailComposerInterface::setSendingAccountId(const QMailAccountId &accountId)
{
    // default implementation does nothing
    Q_UNUSED(accountId)
}

/*!
    Returns a list of actions that are exported by the composer.
*/
QList<QAction*> QMailComposerInterface::actions() const
{
    return QList<QAction*>();
}

/*!
    \fn bool QMailComposerInterface::isEmpty() const

    Returns true if the composer contains no message content; otherwise returns false.
*/

/*!
    \fn QMailMessage QMailComposerInterface::message() const

    Returns the current content of the composer.
*/

/*!
    \fn void QMailComposerInterface::compose(QMailMessage::ResponseType type, 
                                             const QMailMessage& source = QMailMessage(),
                                             const QMailMessagePart::Location& sourceLocation = QMailMessagePart::Location(), 
                                             QMailMessage::MessageType messageType = QMailMessage::AnyType)

    Directs the composer to compose a message, of the form required for the response type \a type.
    If \a source is non-empty, then it should be interpreted as preset content to be composed.
    If \a sourceLocation is non-empty, then it should be interpreted as indicating a message part
    that forms preset content for the composition.  \a messageType indicates the type of
    message that the composer should produce.
*/

/*!
    \fn void QMailComposerInterface::clear()

    Clears any message content contained in the composer.
*/

/*!
    \fn QString QMailComposerInterface::title() const

    Returns a string that may be used as the title for the composer presentation.
*/

/*!
    Returns a string description of the current composition state.
*/
QString QMailComposerInterface::status() const
{
    return QString();
}

/*!
    \fn bool QMailComposerInterface::isReadyToSend() const

    Returns true if the composed message is ready to send or \c false otherwise.
*/

/*!
    \fn bool QMailComposerInterface::isSupported(QMailMessage::MessageType t, QMailMessage::ContentType c) const

    Returns true if the composer can produce a message of type \a t, containing data of content type \a c.
*/

/*!
    \fn void QMailComposerInterface::cancel()

    Signal that is emitted when message composition is cancelled.

    \sa changed()
*/

/*!
    \fn void QMailComposerInterface::changed()

    Signal that is emitted when the currently composed message has been changed.

    \sa cancel()
*/

/*!
    \fn void QMailComposerInterface::statusChanged(const QString &status)

    Signal that is emitted when the message composition state has changed, to a new
    state described by \a status. For example, when transitioning from message body 
    composition to message details composition in a multi-page composer.

    \sa status(), cancel(), changed()
*/

/*!
    \fn void QMailComposerInterface::sendMessage()

    Signal that is emitted when message composition has finished and the message is ready to send.

    \sa isReadyToSend()
*/

/*!
    \class QMailComposerFactory
    \ingroup qmfutil

    \brief The QMailComposerFactory class creates objects implementing the QMailComposerInterface interface.

    The QMailComposerFactory class creates objects that are able to compose mail messages, and
    that implement the QMailComposerInterface interface.  The factory chooses an implementation
    based on the type of message to be composed.

    The QMailComposerInterface class describes the interface supported by classes that can be created
    by the QMailComposerFactory class.  To create a new class that can be created via the QMailComposerFactory,
    implement a plug-in that derives from QMailComposerInterface.

    \sa QMailComposerInterface
*/

/*!
    Returns a list of keys identifying classes that can compose messages of type \a type containing \a contentType content.
*/
QStringList QMailComposerFactory::keys( QMailMessage::MessageType type , QMailMessage::ContentType contentType)
{
    QStringList in;

    foreach (PluginMap::mapped_type plugin, pluginMap())
        if (plugin->isSupported(type, contentType))
            in << plugin->key();

    return in;
}

/*!
    Returns the key identifying the first class found that can compose messages of type \a type.
*/
QString QMailComposerFactory::defaultKey( QMailMessage::MessageType type )
{
    QStringList list(QMailComposerFactory::keys(type));
    return (list.isEmpty() ? QString() : list.first());
}

/*!
    Returns the message types created by the composer identified by \a key.

    \sa QMailComposerInterface::messageTypes()
*/
QList<QMailMessage::MessageType> QMailComposerFactory::messageTypes( const QString& key )
{
    return mapping(key)->messageTypes();
}

/*!
    Returns the name for the message type \a type created by the composer identified by \a key.

    \sa QMailComposerInterface::name()
*/
QString QMailComposerFactory::name(const QString &key, QMailMessage::MessageType type)
{
    return mapping(key)->name(type);
}

/*!
    Returns the display name for the message type \a type created by the composer identified by \a key.

    \sa QMailComposerInterface::displayName()
*/
QString QMailComposerFactory::displayName(const QString &key, QMailMessage::MessageType type)
{
    return mapping(key)->displayName(type);
}

/*!
    Returns the display icon for the message type \a type created by the composer identified by \a key.

    \sa QMailComposerInterface::displayIcon()
*/
QIcon QMailComposerFactory::displayIcon(const QString &key, QMailMessage::MessageType type)
{
    return mapping(key)->displayIcon(type);
}

/*!
    Creates a composer object of the class identified by \a key, setting the returned object to
    have the parent widget \a parent.
*/
QMailComposerInterface *QMailComposerFactory::create( const QString& key, QWidget *parent )
{
    Q_UNUSED(parent);
    return mapping(key);
}


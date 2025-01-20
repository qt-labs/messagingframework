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

#include "messageserver.h"
#include "servicehandler.h"
#include <qmailfolder.h>
#include <qmailmessage.h>
#include <qmailstore.h>
#include <QDataStream>
#include <QTimer>
#include <QCoreApplication>
#include <qmaillog.h>
#include <qmailipc.h>
#include <newcountnotifier.h>
#include <qmailmessageserverplugin.h>

#include "qmailservice_adaptor.h"

extern "C" {
#ifndef Q_OS_WIN
#include <sys/socket.h>
#endif
#include <signal.h>
}

#if defined(Q_OS_UNIX)
#include <unistd.h>
int MessageServer::sighupFd[2];
int MessageServer::sigtermFd[2];
int MessageServer::sigintFd[2];
#endif

MessageServer::MessageServer(QObject *parent)
    : QObject(parent),
      handler(0),
      newMessageTotal(0),
      completionAttempted(false)
{
}

MessageServer::~MessageServer()
{
    // Unregister from D-Bus.
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.unregisterObject("/messageserver");
    if (!dbus.unregisterService("org.qt.messageserver")) {
        qWarning() << "Failed to unregister messageserver from D-Bus";
    } else {
        qMailLog(Messaging) << "Unregistered messageserver from D-Bus";
    }
}

bool MessageServer::init()
{
    qMailLog(Messaging) << "MessageServer init begin";

#if defined(Q_OS_UNIX)
    // Unix signal handlers. We use the trick described here: http://doc.qt.io/qt-5/unix-signals.html
    // Looks shocking but the trick has certain reasons stated in Steven's book: http://cr.yp.to/docs/selfpipe.html
    // Use a socket and notifier because signal handlers can't call Qt code

    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sighupFd))
        qFatal("Couldn't create HUP socketpair");
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sigtermFd))
        qFatal("Couldn't create TERM socketpair");
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sigintFd))
        qFatal("Couldn't create TERM socketpair");

    snHup = new QSocketNotifier(sighupFd[1], QSocketNotifier::Read, this);
    connect(snHup, SIGNAL(activated(int)), this, SLOT(handleSigHup()));
    snTerm = new QSocketNotifier(sigtermFd[1], QSocketNotifier::Read, this);
    connect(snTerm, SIGNAL(activated(int)), this, SLOT(handleSigTerm()));
    snInt = new QSocketNotifier(sigintFd[1], QSocketNotifier::Read, this);
    connect(snInt, SIGNAL(activated(int)), this, SLOT(handleSigInt()));

    struct sigaction action;
    action.sa_handler = MessageServer::hupSignalHandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    action.sa_flags |= SA_RESTART;

    if (sigaction(SIGHUP, &action, 0) > 0)
        qFatal("Couldn't register HUP handler");

    action.sa_handler = MessageServer::termSignalHandler;
    if (sigaction(SIGTERM, &action, 0) > 0)
        qFatal("Couldn't register TERM handler");

    action.sa_handler = MessageServer::intSignalHandler;
    if (sigaction(SIGINT, &action, 0) > 0)
        qFatal("Couldn't register INT handler");

#endif // defined(Q_OS_UNIX)

    QMailStore *store = QMailStore::instance();
    if (store->initializationState() != QMailStore::Initialized) {
        qWarning("Messaging DB Invalid: Messaging cannot operate due to database incompatibilty!");
        // Do not close, however, or QPE will start another instance.
        return false;
    } else {
        connect(store, SIGNAL(messagesAdded(QMailMessageIdList)),
                this, SLOT(messagesAdded(QMailMessageIdList)));
        connect(store, SIGNAL(messagesUpdated(QMailMessageIdList)),
                this, SLOT(messagesUpdated(QMailMessageIdList)));
        connect(store, SIGNAL(messagesRemoved(QMailMessageIdList)),
                this, SLOT(messagesRemoved(QMailMessageIdList)));
    }

    // Register our object on the session bus and expose interface to others.
    handler = new ServiceHandler(this);
    new MessageserverAdaptor(handler);

    QDBusConnection dbus = QDBusConnection::sessionBus();
    if (!dbus.registerObject("/messageserver", handler) ||
        !dbus.registerService("org.qt.messageserver")) {
        qWarning() << "Failed to register to D-Bus, aborting start";
        return false;
    }
    qMailLog(Messaging) << "Registered messageserver to D-Bus";

    connect(handler, &ServiceHandler::transmissionReady,
            this, &MessageServer::transmissionCompleted);
    connect(handler, &ServiceHandler::retrievalReady,
            this, &MessageServer::retrievalCompleted);

    connect(handler, SIGNAL(newMessagesAvailable()),
            this, SLOT(reportNewCounts()));

    connect(this, &MessageServer::messageCountUpdated,
            handler, &ServiceHandler::messageCountUpdated);

    //clean up any temporary messages that were not cleaned up by clients
    QTimer::singleShot(0, this, SLOT(cleanupTemporaryMessages()));

    emit handler->actionsListed(QMailActionDataList());

#ifdef MESSAGESERVER_PLUGINS
    qMailLog(Messaging) << "Initiating messageserver plugins.";
    QStringList availablePlugins = QMailMessageServerPluginFactory::keys();

    for (int i = 0; i < availablePlugins.size(); i++) {
        QMailMessageServerPlugin *plugin = QMailMessageServerPluginFactory::createService(availablePlugins.at(i));
        plugin->exec();
    }
#endif

    return true;
}

void MessageServer::retrievalCompleted(quint64 action)
{
    // Ensure the client receives any resulting events before a notification
    QMailStore::instance()->flushIpcNotifications();

    if (!completionList.isEmpty()) {
        if (!completionAttempted) {
            // Complete the messages that we selected for immediate completion
            completionAttempted = true;
            handler->retrieveMessages(action, completionList.values(), QMailRetrievalAction::Content);
            return;
        } else {
            completionList.clear();
        }
    }

    completionAttempted = false;
    emit handler->retrievalCompleted(action);
}

QMap<QMailMessage::MessageType, QString> typeSignatureInit()
{
    QMap<QMailMessage::MessageType, QString> map;

    map.insert(QMailMessage::Sms, "newSmsCount(int)");
    map.insert(QMailMessage::Mms, "newMmsCount(int)");
    map.insert(QMailMessage::Email, "newEmailCount(int)");
    map.insert(QMailMessage::Instant, "newInstantCount(int)");
    map.insert(QMailMessage::System, "newSystemCount(int)");

    return map;
}

static QMap<QMailMessage::MessageType, QString> typeServiceInit()
{
    QMap<QMailMessage::MessageType, QString> map;

    map.insert(QMailMessage::Sms, "NewSmsArrival");
    map.insert(QMailMessage::Mms, "NewMmsArrival");
    map.insert(QMailMessage::Email, "NewEmailArrival");
    map.insert(QMailMessage::Instant, "NewInstantMessageArrival");
    map.insert(QMailMessage::System, "NewSystemMessageArrival");

    return map;
}

QString serviceForType(QMailMessage::MessageType type)
{
    static QMap<QMailMessage::MessageType, QString> typeService(typeServiceInit());
    return typeService[type];
}

int MessageServer::newMessageCount(QMailMessage::MessageType type) const
{
    QMailMessageKey newMessageKey(QMailMessageKey::status(QMailMessage::New, QMailDataComparator::Includes));
    if (type != QMailMessage::AnyType) {
        newMessageKey &= QMailMessageKey::messageType(type);
    }

    return QMailStore::instance()->countMessages(newMessageKey);
}

void MessageServer::reportNewCounts()
{
    static QMap<QMailMessage::MessageType, QString> typeSignature(typeSignatureInit());

    QMailMessageCountMap newCounts;
    foreach (const QMailMessage::MessageType &type, typeSignature.keys()) {
        newCounts[type] = newMessageCount(type);
    }

    newMessageTotal = newMessageCount(QMailMessage::AnyType);

    if (newMessageTotal) {
        // Inform QPE of changes to the new message counts
        foreach (const QMailMessage::MessageType &type, typeSignature.keys()) {
            if ((newCounts[type] > 0) && (newCounts[type] != messageCounts[type]))
               NewCountNotifier::notify(type, newCounts[type]);
        }

        // Request handling of the new message events
        QMailMessageCountMap::const_iterator it = newCounts.begin(), end = newCounts.end();
        for ( ; it != end; ++it) {
            QMailMessage::MessageType type(it.key());
            if (it.value() != messageCounts[type]) {
                // This type's count has changed since last reported

                if( NewCountNotifier* action = new NewCountNotifier(type,it.value())) {
                    actionType[action] = type;

                    connect(action, SIGNAL(response(bool)), this, SLOT(response(bool)));
                    connect(action, SIGNAL(error(QString)), this, SLOT(error(QString)));

                    // Ensure the client receives any generated events before the arrival notification
                    QMailStore::instance()->flushIpcNotifications();
                    if (!action->notify())
                        qWarning() << "Unable to invoke service:" << serviceForType(type);
                }
            }
        }
    }

    messageCounts = newCounts;
}

void MessageServer::response(bool handled)
{
    if (NewCountNotifier* action = static_cast<NewCountNotifier*>(sender())) {
        if (handled) {
            QMailMessage::MessageType type(actionType[action]);
            // No messages of this type are new any longer
            QMailMessageKey newMessages(QMailMessageKey::messageType(type));
            newMessages &= QMailMessageKey(QMailMessageKey::status(QMailMessage::New, QMailDataComparator::Includes));
            QMailStore::instance()->updateMessagesMetaData(newMessages, QMailMessage::New, false);

            if (messageCounts[type] != 0) {
                newMessageTotal -= messageCounts[type];

                messageCounts[type] = 0;
                NewCountNotifier::notify(type, 0);
            }
        }
        actionType.remove(action);
        action->deleteLater();
        if (actionType.isEmpty()) {
            // All outstanding handler events have been processed
            emit messageCountUpdated();
        }
    }
}

void MessageServer::error(const QString &message)
{
    if (NewCountNotifier* action = static_cast<NewCountNotifier*>(sender())) {
        qWarning() << "Unable to complete service:" << serviceForType(actionType[action]) << "-" << message;
        actionType.remove(action);
        action->deleteLater();
    }

    if (actionType.isEmpty()) {
        // No outstanding handler events remain
        emit messageCountUpdated();
    }
}

void MessageServer::transmissionCompleted(quint64 action)
{
    // Ensure the client receives any resulting events before the completion notification
    QMailStore::instance()->flushIpcNotifications();

    emit handler->transmissionCompleted(action);
}

void MessageServer::messagesAdded(const QMailMessageIdList &ids)
{
    if (!QMailStore::instance()->asynchronousEmission()) {
        // Added in our process - from retrieval
        foreach (const QMailMessageId &id, ids) {
            QMailMessageMetaData message(id);

            bool complete(false);
            if (!(message.status() & QMailMessage::ContentAvailable)) {
                // Automatically download voicemail messages
                if (message.content() == QMailMessage::VoicemailContent ||
                    message.content() == QMailMessage::VideomailContent) {
                    complete = true;
                }
            }

            if (complete)
                completionList.insert(message.id());
        }
    }
}

void MessageServer::messagesUpdated(const QMailMessageIdList &ids)
{
    if (QMailStore::instance()->asynchronousEmission()) {
        // Only need to check message counts if the update occurred in another process
        updateNewMessageCounts();
    } else {
        // If we're updating, check whether the messages have been marked as Removed
        foreach (const QMailMessageId &id, ids) {
            if (completionList.contains(id)) {
                QMailMessageMetaData message(id);
                if ((message.status() & QMailMessage::ContentAvailable) || (message.status() & QMailMessage::Removed)) {
                    // This message has been completed (or removed)
                    completionList.remove(id);
                }
            }
        }
    }
}

void MessageServer::messagesRemoved(const QMailMessageIdList &ids)
{
    foreach (const QMailMessageId &id, ids) {
        // No need to complete deleted messages
        completionList.remove(id);
    }

    updateNewMessageCounts();
}

void MessageServer::updateNewMessageCounts()
{
    int newTotal = newMessageCount(QMailMessage::AnyType);
    if (newTotal != newMessageTotal) {
        // The number of messages marked as new has changed, but not via a message arrival event
        static QMap<QMailMessage::MessageType, QString> typeSignature(typeSignatureInit());

        // Update the individual counts
        foreach (const QMailMessage::MessageType &type, typeSignature.keys()) {
            int count(newMessageCount(type));
            if (count != messageCounts[type]) {
                messageCounts[type] = count;
                NewCountNotifier::notify(type, count);
            }
        }

        emit messageCountUpdated();
    }
}

void MessageServer::cleanupTemporaryMessages()
{
    QMailStore::instance()->removeMessages(QMailMessageKey::status(QMailMessage::Temporary), QMailStore::NoRemovalRecord);
}

#if defined(Q_OS_UNIX)
void MessageServer::hupSignalHandler(int)
{
    // Can't call Qt code. Write to the socket and the notifier will fire from the Qt event loop
    char a = 1;
    ::write(sighupFd[0], &a, sizeof(a));
}

void MessageServer::handleSigHup()
{
    snHup->setEnabled(false);
    char tmp;
    ::read(sighupFd[1], &tmp, sizeof(tmp));

    // This is ~/.config/QtProject/Messageserver.conf
    qMailLoggersRecreate("QtProject", "Messageserver", "Msgsrv");

    snHup->setEnabled(true);
}

void MessageServer::termSignalHandler(int)
{
    char a = 1;
    ::write(sigtermFd[0], &a, sizeof(a));
}

void MessageServer::handleSigTerm()
{
    snTerm->setEnabled(false);
    char tmp;
    ::read(sigtermFd[1], &tmp, sizeof(tmp));

    qMailLog(Messaging) << "Received SIGTERM, shutting down.";
    QCoreApplication::exit();

    snTerm->setEnabled(true);
}

void MessageServer::intSignalHandler(int)
{
    char a = 1;
    ::write(sigintFd[0], &a, sizeof(a));
}

void MessageServer::handleSigInt()
{
    snInt->setEnabled(false);
    char tmp;
    ::read(sigintFd[1], &tmp, sizeof(tmp));

    qMailLog(Messaging) << "Received SIGINT, shutting down.";
    QCoreApplication::exit();

    snInt->setEnabled(true);
}

#endif // defined(Q_OS_UNIX)

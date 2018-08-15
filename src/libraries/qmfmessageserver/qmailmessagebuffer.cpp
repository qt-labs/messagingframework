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

#include "qmailmessagebuffer.h"
#include "qmailstore.h"
#include <qmaillog.h>
#include <QTimer>

#include <QSettings>
#include <QFile>


class QMailMessageBufferPrivate
{
    friend class QMailMessageBuffer;
    QList<QMailMessageBuffer::BufferItem*> waitingForCallback;
    QList<QMailMessageBuffer::BufferItem*> waitingForFlush;

    // Limits/Tunables
    int maxPending;
    int idleTimeout;
    int maxTimeout;
    qreal timeoutScale;

    // Flush the buffer periodically
    QTimer messageTimer;
    QTime secondaryTimer;
    int lastFlushTimePerMessage;

};



Q_GLOBAL_STATIC(QMailMessageBuffer, messageBuffer)

QMailMessageBuffer::QMailMessageBuffer(QObject *parent)
    : QObject(parent), d(new QMailMessageBufferPrivate)
{
    d->messageTimer.setSingleShot(true);
    connect(&d->messageTimer, SIGNAL(timeout()), this, SLOT(messageTimeout()));

    d->lastFlushTimePerMessage = 0;

    readConfig();
}

QMailMessageBuffer::~QMailMessageBuffer()
{
}

QMailMessageBuffer *QMailMessageBuffer::instance()
{
    return messageBuffer();
}

bool QMailMessageBuffer::addMessage(QMailMessage *message)
{
    Q_ASSERT(message);
    d->waitingForCallback.append(new BufferItem(true, 0, message));
    return true;
}

bool QMailMessageBuffer::updateMessage(QMailMessage *message)
{
    Q_ASSERT(message);
    d->waitingForCallback.append(new BufferItem(false, 0, message));
    return true;
}

QMailMessageBuffer::BufferItem *QMailMessageBuffer::get_item(QMailMessage *message)
{
    foreach (BufferItem *item, d->waitingForCallback) {
        if (item->message == message) {
            d->waitingForCallback.removeOne(item);
            return item;
        }
    }

    return 0;
}

// We "own" the callback instance (makes the error case simpler for the client to handle)
bool QMailMessageBuffer::setCallback(QMailMessage *message, QMailMessageBufferFlushCallback *callback)
{
    if (!message) {
        // This message was not scheduled for adding or updating
        qWarning() << "Adding null message to buffer";
        delete callback;
        return false;
    }
    BufferItem *item = get_item(message);
    Q_ASSERT(item);

    item->callback = callback;
    item->message = message;
    d->waitingForFlush.append(item);

    if (isFull() || !d->messageTimer.isActive()) {
        // If the buffer is full we flush.
        // If the timer isn't running we flush.
        messageFlush();
    } else if (d->secondaryTimer.elapsed() > d->messageTimer.interval()) {
        // message timer is overdue to fire, force a flush
        messageFlush();
    }

    return true;
}

void QMailMessageBuffer::messageTimeout()
{
    if (messagePending()) {
        messageFlush();
    } else {
        d->lastFlushTimePerMessage = 0;
        d->messageTimer.setInterval(d->idleTimeout);
    }
}

void QMailMessageBuffer::messageFlush()
{
    QMailStore *store = QMailStore::instance();
    QList<QMailMessage*> work;
    QTime commitTimer;
    int processed = messagePending();
    commitTimer.start();

    // Start by processing all the new messages
    foreach (BufferItem *item, d->waitingForFlush) {
        if (item->add)
            work.append(item->message);
    }
    if (work.count())
        store->addMessages(work);
    foreach (BufferItem *item, d->waitingForFlush) {
        if (item->add)
            item->callback->messageFlushed(item->message);
    }

    // Now we process all tne updated messages
    work.clear();
    foreach (BufferItem *item, d->waitingForFlush) {
        if (!item->add)
            work.append(item->message);
    }
    if (work.count())
        store->updateMessages(work);
    foreach (BufferItem *item, d->waitingForFlush) {
        if (!item->add)
            item->callback->messageFlushed(item->message);
    }

    // Delete all the temporarily memory
    foreach (BufferItem *item, d->waitingForFlush) {
        delete item->callback;
        delete item;
    }
    d->waitingForFlush.clear();

    int timePerMessage = commitTimer.elapsed() / processed;
    if (timePerMessage > d->lastFlushTimePerMessage && d->messageTimer.interval() < d->maxTimeout) {
        // increase the timeout
        int interval = d->messageTimer.interval() * d->timeoutScale;
        int actual = (interval > d->maxTimeout)? d->maxTimeout:interval;
        d->messageTimer.setInterval(actual);
    }
    d->lastFlushTimePerMessage = timePerMessage;

    d->messageTimer.start();
    d->secondaryTimer.start();

    if (processed)
        emit flushed();
}

void QMailMessageBuffer::flush()
{
    if (messagePending())
        messageFlush();
}

int QMailMessageBuffer::messagePending() {
    return d->waitingForFlush.size();
}

int QMailMessageBuffer::maximumBufferSize() const
{
    // 1MB buffer = 900KB + 100KB for largest message (default 100KB body limit)
    return 1024*900;
}

bool QMailMessageBuffer::isFull() {
    if (messagePending() >= d->maxPending) {
        return true;
    }
    int totalSize = 0;
    foreach (BufferItem *item, d->waitingForFlush) {
        totalSize += item->message->body().length();
        if (totalSize > maximumBufferSize()) {
            return true;
        }
    }
    return false;
}

void QMailMessageBuffer::readConfig()
{
    QSettings settings(QLatin1String("QtProject"), QLatin1String("QMF"));
    settings.beginGroup(QLatin1String("MessageBuffer"));

    d->maxPending = settings.value(QLatin1String("maxPending"), 1000).toInt();
    d->idleTimeout = settings.value(QLatin1String("idleTimeout"), 1000).toInt();
    d->maxTimeout = settings.value(QLatin1String("maxTimeout"), 8000).toInt();
    d->timeoutScale = settings.value(QLatin1String("timeoutScale"), 2.0f).value<qreal>();

    d->messageTimer.setInterval(d->idleTimeout);
}

void QMailMessageBuffer::removeCallback(QMailMessageBufferFlushCallback *callback)
{
    foreach (BufferItem *item, d->waitingForFlush) {
        if (item->callback == callback) {
            d->waitingForFlush.removeOne(item);
            delete item->callback;
            delete item;
        }
    }
}


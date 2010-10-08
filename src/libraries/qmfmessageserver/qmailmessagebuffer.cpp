#include "qmailmessagebuffer.h"
#include "qmailstore.h"

#include <QSettings>
#include <QDebug>
#include <QTimer>
#include <QFile>



Q_GLOBAL_STATIC(QMailMessageBuffer, messageBuffer)

QMailMessageBuffer::QMailMessageBuffer(QObject *parent)
    : QObject(parent)
{
    m_messageTimer = new QTimer(this);
    m_messageTimer->setSingleShot(true);
    connect(m_messageTimer, SIGNAL(timeout()), this, SLOT(messageTimeout()));

    m_lastFlushTimePerMessage = 0;

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
    m_waitingForCallback.append(new BufferItem(true, 0, message));
    return true;
}

bool QMailMessageBuffer::updateMessage(QMailMessage *message)
{
    m_waitingForCallback.append(new BufferItem(false, 0, message));
    return true;
}

QMailMessageBuffer::BufferItem *QMailMessageBuffer::get_item(QMailMessage *message)
{
    foreach (BufferItem *item, m_waitingForCallback) {
        if (item->message == message) {
            m_waitingForCallback.removeOne(item);
            return item;
        }
    }

    return 0;
}

// We "own" the callback instance (makes the error case simpler for the client to handle)
bool QMailMessageBuffer::setCallback(QMailMessage *message, QMailMessageBufferFlushCallback *callback)
{
    BufferItem *item = get_item(message);
    if (!message) {
        // This message was not scheduled for adding or updating
        qWarning() << "Adding null message to buffer";
        delete callback;
        return false;
    }

    item->callback = callback;
    item->message = new QMailMessage(*message);
    m_waitingForFlush.append(item);

    if (isFull() || !m_messageTimer->isActive()) {
        // If the buffer is full we flush.
        // If the timer isn't running we flush.
        messageFlush();
    }

    return true;
}

void QMailMessageBuffer::messageTimeout()
{
    if (messagePending()) {
        messageFlush();
    } else {
        m_lastFlushTimePerMessage = 0;
        m_messageTimer->setInterval(m_idleTimeout);
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
    foreach (BufferItem *item, m_waitingForFlush) {
        if (item->add)
            work.append(item->message);
    }
    if (work.count())
        store->addMessages(work);
    foreach (BufferItem *item, m_waitingForFlush) {
        if (item->add)
            item->callback->messageFlushed(item->message);
    }

    // Now we process all tne updated messages
    work.clear();
    foreach (BufferItem *item, m_waitingForFlush) {
        if (!item->add)
            work.append(item->message);
    }
    if (work.count())
        store->updateMessages(work);
    foreach (BufferItem *item, m_waitingForFlush) {
        if (!item->add)
            item->callback->messageFlushed(item->message);
    }

    // Delete all the temporarily memory
    foreach (BufferItem *item, m_waitingForFlush) {
        delete item->message;
        delete item->callback;
        delete item;
    }
    m_waitingForFlush.clear();

    int timePerMessage = commitTimer.elapsed() / processed;
    if (timePerMessage > m_lastFlushTimePerMessage && m_messageTimer->interval() < m_maxTimeout) {
        // increase the timeout
        int interval = m_messageTimer->interval() * m_timeoutScale;
        int actual = (interval > m_maxTimeout)?m_maxTimeout:interval;
        m_messageTimer->setInterval(actual);
    }
    m_lastFlushTimePerMessage = timePerMessage;

    m_messageTimer->start();

    if (processed)
        emit flushed();
}

void QMailMessageBuffer::flush()
{
    if (messagePending())
        messageFlush();
}

void QMailMessageBuffer::readConfig()
{
    QSettings settings("Nokia", "QMF");
    settings.beginGroup("MessageBuffer");

    m_maxPending = settings.value("maxPending", 1000).toInt();
    m_idleTimeout = settings.value("idleTimeout", 1000).toInt();
    m_maxTimeout = settings.value("maxTimeout", 8000).toInt();
    m_timeoutScale = settings.value("timeoutScale", 2.0f).value<qreal>();

    m_messageTimer->setInterval(m_idleTimeout);
}

void QMailMessageBuffer::removeCallback(QMailMessageBufferFlushCallback *callback)
{
    foreach (BufferItem *item, m_waitingForFlush) {
        if (item->callback == callback) {
            m_waitingForCallback.removeOne(item);
            delete item->message;
            delete item->callback;
            delete item;
        }
    }
}


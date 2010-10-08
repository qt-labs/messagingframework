#ifndef QMAILMESSAGEBUFFER_H
#define QMAILMESSAGEBUFFER_H

#include <QObject>
#include <QList>
#include <QTime>
#include <QVariant>

class QMailMessage;
class QTimer;

#include <qmailglobal.h>


class MESSAGESERVER_EXPORT QMailMessageBufferFlushCallback
{
public:
    virtual ~QMailMessageBufferFlushCallback() {}
    virtual void messageFlushed(QMailMessage *message) = 0;
};

class MESSAGESERVER_EXPORT QMailMessageBuffer : public QObject
{
    Q_OBJECT
public:
    QMailMessageBuffer(QObject *parent = 0);
    virtual ~QMailMessageBuffer();

    static QMailMessageBuffer *instance();

    bool addMessage(QMailMessage *message);
    bool updateMessage(QMailMessage *message);
    bool setCallback(QMailMessage *message, QMailMessageBufferFlushCallback *callback);

    void flush();

    void removeCallback(QMailMessageBufferFlushCallback *callback);

signals:
    void flushed();

private slots:
    void messageTimeout();
    void readConfig();

private:

    struct BufferItem
    {
        BufferItem(bool _add, QMailMessageBufferFlushCallback *_callback, QMailMessage *_message)
            : add(_add)
            , callback(_callback)
            , message(_message)
        {}

        bool add;
        QMailMessageBufferFlushCallback *callback;
        QMailMessage *message;
    };

    void messageFlush();
    int messagePending() { return m_waitingForFlush.size(); }
    bool isFull() { return messagePending() >= m_maxPending; }
    BufferItem *get_item(QMailMessage *message);

    QList<BufferItem*> m_waitingForCallback;
    QList<BufferItem*> m_waitingForFlush;

    // Limits/Tunables
    int m_maxPending;
    int m_idleTimeout;
    int m_maxTimeout;
    qreal m_timeoutScale;

    // Flush the buffer periodically
    QTimer *m_messageTimer;
    int m_lastFlushTimePerMessage;
};

#endif

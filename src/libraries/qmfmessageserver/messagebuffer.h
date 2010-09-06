#ifndef MESSAGEBUFFER_H
#define MESSAGEBUFFER_H

#include <QObject>
#include <QList>
#include <QTime>
#include <QVariant>

class QMailMessage;
class QMailMessageKey;
class QMailAccountId;
class QMailMessageKey;
class QMailStore;
class BufferItem;
class QTimer;

class MessageBufferFlushCallback
{
public:
    virtual ~MessageBufferFlushCallback() {}
    virtual void messageFlushed(QMailMessage *message) = 0;
};

class MessageBufferProgressCallback
{
protected:
    ~MessageBufferProgressCallback() {}
public:
    virtual void progressChanged(uint progress, uint total) = 0;
};

class MessageBuffer : public QObject
{
    Q_OBJECT
public:
    MessageBuffer(QObject *parent = 0);
    ~MessageBuffer();

    static MessageBuffer *instance();

    bool addMessage(QMailMessage *message);
    bool updateMessage(QMailMessage *message);
    bool setCallback(QMailMessage *message, MessageBufferFlushCallback *callback);

    bool removeMessages(const QMailMessageKey &key);
    int countMessages(const QMailMessageKey &key);

    void progressChanged(MessageBufferProgressCallback *callback, uint progress, uint total);

    void flush();

private slots:
    void messageTimeout();
    void progressTimeout();
    void readConfig();

private:
    void messageFlush();
    void progressFlush();
    int messagePending() { return m_waitingForFlush.size(); }
    bool progressPending() { return m_progressCallback; }
    bool isFull() { return messagePending() >= m_maxPending; }
    BufferItem *get_item(QMailMessage *message);

    QList<BufferItem*> m_waitingForCallback;
    QList<BufferItem*> m_waitingForFlush;
    uint m_progress;
    uint m_total;
    MessageBufferProgressCallback *m_progressCallback;

    // Limits/Tunables
    int m_maxPending;
    int m_idleTimeout;
    int m_maxTimeout;
    qreal m_timeoutScale;
    int m_progressTimeout;

    // Flush the buffer periodically
    QTimer *m_messageTimer;
    int m_lastFlushTimePerMessage;

    // Flush the progress periodically
    QTimer *m_progressTimer;
};

#endif

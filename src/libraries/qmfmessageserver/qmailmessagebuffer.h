#ifndef QMAILMESSAGEBUFFER_H
#define QMAILMESSAGEBUFFER_H

#include <QObject>
#include <QList>
#include <QTime>
#include <QVariant>

class QMailMessage;
class QMailMessageBufferPrivate;
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
    friend class QMailMessageBufferPrivate;
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
    int messagePending();
    bool isFull();
    BufferItem *get_item(QMailMessage *message);

    QScopedPointer<QMailMessageBufferPrivate> d;
};

#endif

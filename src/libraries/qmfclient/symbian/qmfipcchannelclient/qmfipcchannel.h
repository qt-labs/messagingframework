#ifndef SYMBIANQMFIPCCHANNEL_H
#define SYMBIANQMFIPCCHANNEL_H

#include "qmfipcchannelsession.h"
#include <QSharedDataPointer>

class QString;
class SymbianQMFIPCChannelPrivate;

class SymbianQMFIPCChannelObserver
{
public:
    virtual ~SymbianQMFIPCChannelObserver() {}
    virtual void Connected(quintptr connectionId) = 0;
    virtual void ConnectionRefused() = 0;
    virtual void Disconnected() = 0;
    virtual void DataAvailable() = 0;
};

class SymbianQMFIPCChannelIncomingConnectionObserver
{
public:
    virtual ~SymbianQMFIPCChannelIncomingConnectionObserver() {}
    virtual void NewConnection(quintptr connectionId) = 0;
};

class SymbianQMFIPCChannel
{
public:
    SymbianQMFIPCChannel();
    SymbianQMFIPCChannel(const SymbianQMFIPCChannel& other);
    ~SymbianQMFIPCChannel();

    SymbianQMFIPCChannel& operator=(const SymbianQMFIPCChannel& other);

    bool connect();
    bool createChannel(QString name);
    bool destroyChannel(QString name);
    bool waitForIncomingConnection(SymbianQMFIPCChannelIncomingConnectionObserver* observer);

    bool connectClientToChannel(QString name, SymbianQMFIPCChannelObserver* observer);
    bool connectServerToChannel(quintptr socketDescriptor, SymbianQMFIPCChannelObserver* observer);
    bool SendDataL(const char *data, qint64 len);
    bool startWaitingForData();
    qint64 ReadData(char *data, qint64 maxlen);
    qint64 dataSize() const;

private: // Data
    QSharedDataPointer<SymbianQMFIPCChannelPrivate> d;
};

#endif // SYMBIANQMFIPCCHANNEL_H

// End of File

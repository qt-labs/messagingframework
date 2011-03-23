#include "ipcserver.h"

#include <QQueue>
#include <private/qobject_p.h>
#include "qmfipcchannel.h"
#include "ipcsocket.h"

class SymbianIpcServerPrivate : public QObjectPrivate, public SymbianQMFIPCChannelIncomingConnectionObserver
{
    Q_DECLARE_PUBLIC(SymbianIpcServer)

public:
    SymbianIpcServerPrivate();
    ~SymbianIpcServerPrivate();

public: // From SymbianQMFIPCChannelIncomingConnectionObserver
    virtual void NewConnection(quintptr connectionId);

public: // Data
    QString m_channelName;
    quintptr m_connectionId;
    int m_maxPendingConnections;
    QQueue<SymbianIpcSocket*> m_pendingConnections;
    mutable SymbianQMFIPCChannel m_channel;
};

SymbianIpcServerPrivate::SymbianIpcServerPrivate()
    : m_channelName(),
      m_connectionId(0),
      m_maxPendingConnections(30)
{
    m_channel.connect();
}

SymbianIpcServerPrivate::~SymbianIpcServerPrivate()
{
}

void SymbianIpcServerPrivate::NewConnection(quintptr connectionId)
{
    Q_Q(SymbianIpcServer);
    q->incomingConnection(connectionId);
}

SymbianIpcServer::SymbianIpcServer(QObject *parent)
    : QObject(*new SymbianIpcServerPrivate, parent)
{
}

SymbianIpcServer::~SymbianIpcServer()
{
    Q_D(SymbianIpcServer);
    d->m_channel.destroyChannel(d->m_channelName);
    qDeleteAll(d->m_pendingConnections);
    d->m_pendingConnections.clear();
}

bool SymbianIpcServer::listen(const QString &name)
{
    Q_D(SymbianIpcServer);
    if (!d->m_channelName.isEmpty()) {
        d->m_channel.destroyChannel(d->m_channelName);
    }
    d->m_channelName = name;
    if (d->m_channel.createChannel(name)) {
        return d->m_channel.waitForIncomingConnection(d);
    }

    return false;
}

bool SymbianIpcServer::hasPendingConnections() const
{
    Q_D(const SymbianIpcServer);
    return !(d->m_pendingConnections.isEmpty());
}

SymbianIpcSocket *SymbianIpcServer::nextPendingConnection()
{
    Q_D(SymbianIpcServer);

    if (d->m_pendingConnections.isEmpty()) {
        return 0;
    }

    return d->m_pendingConnections.dequeue();
}

void SymbianIpcServer::incomingConnection(quintptr socketDescriptor)
{
    Q_D(SymbianIpcServer);

    SymbianIpcSocket *socket = new SymbianIpcSocket(this);
    socket->setSocketDescriptor(socketDescriptor);
    d->m_pendingConnections.enqueue(socket);

    emit newConnection();

    d->m_channel.waitForIncomingConnection(d);
}

// End of File

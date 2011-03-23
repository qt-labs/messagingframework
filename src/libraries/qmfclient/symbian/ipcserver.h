#ifndef IPCSERVER_H
#define IPCSERVER_H

#include <QtNetwork/qabstractsocket.h>

class SymbianIpcServerPrivate;
class SymbianIpcSocket;

class SymbianIpcServer : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(SymbianIpcServer)

public:
    SymbianIpcServer(QObject *parent = 0);
    ~SymbianIpcServer();

    virtual bool hasPendingConnections() const;
    virtual SymbianIpcSocket *nextPendingConnection();
    bool listen(const QString &name);

protected:
    virtual void incomingConnection(quintptr socketDescriptor);

Q_SIGNALS:
    void newConnection();

private:
    Q_DISABLE_COPY(SymbianIpcServer)
};

#endif // IPCSERVER_H

// End of File

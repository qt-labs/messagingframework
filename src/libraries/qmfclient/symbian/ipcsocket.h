#ifndef IPCSOCKET_H
#define IPCSOCKET_H

#include <QtCore/qiodevice.h>
#include <QtNetwork/qabstractsocket.h>
#include <QSharedDataPointer>

class SymbianIpcSocketPrivate;

class SymbianIpcSocket : public QIODevice
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(SymbianIpcSocket)

public:
    enum SymbianIpcSocketError
    {
        ServerNotFoundError = QAbstractSocket::HostNotFoundError,
        UnknownSocketError = QAbstractSocket::UnknownSocketError
    };

    enum SymbianIpcSocketState
    {
        UnconnectedState = QAbstractSocket::UnconnectedState,
        ConnectingState = QAbstractSocket::ConnectingState,
        ConnectedState = QAbstractSocket::ConnectedState,
        ClosingState = QAbstractSocket::ClosingState
    };

    SymbianIpcSocket(QObject *parent = 0);
    ~SymbianIpcSocket();

    void connectToServer(const QString &name, OpenMode openMode = ReadWrite);

    bool isSequential() const;
    qint64 bytesAvailable() const;

    bool flush();
    bool setSocketDescriptor(quintptr socketDescriptor,
                             SymbianIpcSocketState socketState = ConnectedState,
                             OpenMode openMode = ReadWrite);
    SymbianIpcSocketState state() const;
    bool waitForConnected(int msecs = 30000);

Q_SIGNALS:
    void connected();
    void disconnected();
    void error(SymbianIpcSocket::SymbianIpcSocketError socketError);
    void stateChanged(SymbianIpcSocket::SymbianIpcSocketState socketState);

private: // From QIODevice
    virtual qint64 readData(char *data, qint64 maxlen);
    virtual qint64 writeData(const char *data, qint64 len);

private:
    Q_DISABLE_COPY(SymbianIpcSocket)
};

#endif // IPCSOCKET_H

// End of File

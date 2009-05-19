/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef QMAILTRANSPORT_H
#define QMAILTRANSPORT_H

#include <qmailglobal.h>

#include <QObject>
#include <QAbstractSocket>

class QString;
class QTimer;
#ifndef QT_NO_OPENSSL
class QSslSocket;
class QSslError;
#endif

class MESSAGESERVER_EXPORT QMailTransport : public QObject
{
    Q_OBJECT

public:
    enum EncryptType {
        Encrypt_NONE = 0,
#ifndef QT_NO_OPENSSL
        Encrypt_SSL = 1,
        Encrypt_TLS = 2
#endif
    };

    QMailTransport(const char* name);
    virtual ~QMailTransport();

    // Open a connection to the specified server
    void open(const QString& url, int port, EncryptType encryptionType);

#ifndef QT_NO_OPENSSL
    // If connection is not currently encrypted, switch to encrypted mode
    void switchToEncrypted();
#endif

    // Close the current connection
    void close();

    // True if a connection has been established with the desired enryption type
    bool connected() const;

    bool isEncrypted() const;

    // True if the connection is in use
    bool inUse() const;

    // Access a stream to write to the mail server (must have an open connection)
    QDataStream& stream();
    QAbstractSocket& socket();

    // Read line-oriented data from the transport (must have an open connection)
    bool canReadLine() const;
    QByteArray readLine(qint64 maxSize = 0);

    // Assists with counting bytes written to the device
    void mark();
    qint64 bytesSinceMark() const;

signals:
    void connected(QMailTransport::EncryptType encryptType);
    void readyRead();
    void bytesWritten(qint64 transmitted);

    void errorOccurred(int status, QString);
    void updateStatus(const QString &);

public slots:
    void errorHandling(int errorCode, QString msg);
    void socketError(QAbstractSocket::SocketError error);

protected slots:
    void connectionEstablished();
    void hostConnectionTimeOut();
#ifndef QT_NO_OPENSSL
    void encryptionEstablished();
    void connectionFailed(const QList<QSslError>& errors);
#endif

#ifndef QT_NO_OPENSSL
protected:
    // Override to modify certificate error handling
    virtual bool ignoreCertificateErrors(const QList<QSslError>& errors);
#endif

private:
    void createSocket(EncryptType encryptType);
    EncryptType mailEncryption() const;

private:
    class Socket;

    Socket *mSocket;

#ifndef QT_NO_OPENSSL
    EncryptType encryption;
#endif
    QDataStream *mStream;
    const char *mName;
    QTimer *connectToHostTimeOut;
    bool mConnected;
    bool mInUse;
};

#endif

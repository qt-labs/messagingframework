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

#include "qmailtransport.h"

#include <QFile>
#include <QTimer>
#include <QSslSocket>
#include <QSslError>

#include <QNetworkProxy>
#include <QUrl>

#include <qmaillog.h>
#include <qmailnamespace.h>

class QMailTransport::Socket : public QSslSocket
{
    Q_OBJECT

public:
    Socket(QObject *parent);

    void mark();
    qint64 bytesSinceMark() const;

protected:
    qint64 writeData(const char *data, qint64 maxSize) override
    {
        qint64 rv = QSslSocket::writeData(data, maxSize);
        if (rv > 0)
            written += rv;

        return rv;
    }

private:
    qint64 written;
};

QMailTransport::Socket::Socket(QObject *parent)
    : QSslSocket(parent),
      written(0)
{
    // We'll connect to servers offering any variant of encryption
    setProtocol(QSsl::AnyProtocol);

    // we are library and if application sets proxy somewhere else
    // nothing is done here
    QNetworkProxy appProxy = QNetworkProxy::applicationProxy();
    if (appProxy.type() == QNetworkProxy::NoProxy) {
        QNetworkProxyFactory::setUseSystemConfiguration(true);

        qCDebug(lcMessaging) << "QMailTransport::Socket::Socket SET PROXY"
                             << "host=" << QNetworkProxy::applicationProxy().hostName()
                             << "port=" << QNetworkProxy::applicationProxy().port();
    }
}

void QMailTransport::Socket::mark()
{
    written = 0;
}

qint64 QMailTransport::Socket::bytesSinceMark() const
{
    return written;
}


/*!
    \class QMailTransport
    \inmodule QmfMessageServer

    \preliminary
    \brief The QMailTransport class provides a line-oriented socket for messaging communications.

    QMailTransport implements a TLS and SSL enabled socket, whose incoming data can
    be processed one line of text at a time.

    QMailTransport provides the ability to count the bytes written via the socket,
    which is useful when data is inserted into a stream layered above the socket.
*/

/*!
    \enum QMailTransport::EncryptType

    This enum type is used to describe the encryption types supported by the transport.

    \value Encrypt_NONE     No encryption.
    \value Encrypt_SSL      Transport Layer Security encryption negotiated before application protocol connection.
    \value Encrypt_TLS      Transport Layer Security encryption negotiated after application protocol connection.
*/

/*!
    Creates a transport object with the supplied object \a name.
*/
QMailTransport::QMailTransport(const char* name)
    : mName(name),
      mConnected(false),
      mInUse(false),
      mAcceptUntrustedCertificates(false)
{
    encryption = Encrypt_NONE;
    mSocket = 0;
    mStream = 0;
    connect( &connectToHostTimeOut, SIGNAL(timeout()), this, SLOT(hostConnectionTimeOut()) );
}

/*! \internal */
QMailTransport::~QMailTransport()
{
    delete mStream;
    delete mSocket;
}

/*!
    Resets the byte counter to zero.

    \sa bytesSinceMark()
*/
void QMailTransport::mark()
{
    if (mSocket)
        mSocket->mark();
}

/*!
    Returns the number of bytes written since the counter as reset to zero.

    \sa mark()
*/
qint64 QMailTransport::bytesSinceMark() const
{
    if (mSocket)
        return mSocket->bytesSinceMark();

    return 0;
}

/*!
    Creates a socket for the requested encryption type \a encryptType.
*/
void QMailTransport::createSocket(EncryptType encryptType)
{
    if (mSocket) {
        // Note: socket recycling doesn't seem to work in SSL mode...
        if (mSocket->mode() == QSslSocket::UnencryptedMode
            && (encryptType == Encrypt_NONE || encryptType == Encrypt_TLS)) {
            // The socket already exists in the correct mode
            return;
        } else {
            // We need to create a new socket in the correct mode
            delete mStream;
            mSocket->deleteLater();
        }
    }

    mSocket = new Socket(this);
    encryption = encryptType;
    connect(mSocket, &QSslSocket::encrypted, this, &QMailTransport::encryptionEstablished);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    connect(mSocket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(connectionFailed(QList<QSslError>)));
#else
    connect(mSocket, &QSslSocket::sslErrors, this, &QMailTransport::connectionFailed);
#endif

    const int bufferLimit = 101*1024; // Limit memory used when downloading
    mSocket->setReadBufferSize( bufferLimit );
    mSocket->setObjectName(QString::fromUtf8(mName) + QString::fromLatin1("-socket"));
    connect(mSocket, &QAbstractSocket::connected, this, &QMailTransport::connectionEstablished);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    connect(mSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(socketError(QAbstractSocket::SocketError)));
#else
    connect(mSocket, &QAbstractSocket::errorOccurred, this, &QMailTransport::socketError);
#endif
    connect(mSocket, &QAbstractSocket::readyRead, this, &QMailTransport::readyRead);
    connect(mSocket, &QAbstractSocket::bytesWritten, this, &QMailTransport::bytesWritten);

    mStream = new QDataStream(mSocket);
}

/*!
    Opens a connection to the supplied \a url and \a port, using the specified \a encryptionType.
*/
void QMailTransport::open(const QString& url, int port, EncryptType encryptionType)
{
    if (mSocket && mSocket->isOpen()) {
        qCWarning(lcMessaging) << "Failed to open connection - already open!";
        return;
    }

    mInUse = true;

    const int threeMin = 3 * 60 * 1000;
    connectToHostTimeOut.start(threeMin); // even this seems way too long?
    createSocket(encryptionType);
    emit updateStatus(tr("DNS lookup"));

    qCDebug(lcMessaging) << "Opening connection - " << url << ':' << port
                         << (encryptionType == Encrypt_SSL ? " SSL" : (encryptionType == Encrypt_TLS ? " TLS" : ""));
    if (mailEncryption() == Encrypt_SSL)
        mSocket->connectToHostEncrypted(url, port);
    else
        mSocket->connectToHost(url, port);
}

void QMailTransport::setAcceptUntrustedCertificates(bool accept)
{
    mAcceptUntrustedCertificates = accept;
}

bool QMailTransport::acceptUntrustedCertificates() const
{
    return mAcceptUntrustedCertificates;
}

/*!
    Switches the socket from unencrypted to encrypted mode.
*/
void QMailTransport::switchToEncrypted()
{
    if (mSocket->mode() == QSslSocket::UnencryptedMode)
        mSocket->startClientEncryption();
}

/*!
    Closes the socket, flushing any previously unwritten data.
*/
void QMailTransport::close()
{
    connectToHostTimeOut.stop();

    while ((mSocket->bytesToWrite()) && (mSocket->state() != QAbstractSocket::UnconnectedState)) {
        // Flush any pending write data before closing
        mSocket->flush();
        mSocket->waitForBytesWritten(-1);
    }

    mConnected = false;
    mInUse = false;
    mSocket->close();
}

/*!
    Returns true if a connection has been established.
*/
bool QMailTransport::connected() const
{
    return mConnected;
}

/*!
    Returns true if the connection is established and using encryption.
*/
bool QMailTransport::isEncrypted() const
{
    if (mConnected)
        return (mailEncryption() != Encrypt_NONE);

    return false;
}

/*!
    Returns true if the socket is connected, or in the process of connecting.
*/
bool QMailTransport::inUse() const
{
    return mInUse;
}

/*!
    Returns the stream object allowing data to be read or written across the transport connection.
*/
QDataStream& QMailTransport::stream()
{
    Q_ASSERT(mStream);
    return *mStream;
}

/*!
    Returns the socket object allowing state to be accessed and manipulated.
*/
QAbstractSocket& QMailTransport::socket()
{
    Q_ASSERT(mSocket);
    return *mSocket;
}

/*!
    Returns true if a line of data can be read from the transport; otherwise returns false.
*/
bool QMailTransport::canReadLine() const
{
    return mSocket->canReadLine();
}

/*!
    Returns true if the transport is readable and there are bytes available to read; otherwise returns false.
*/
bool QMailTransport::bytesAvailable() const
{
    return mSocket->isReadable() && mSocket->bytesAvailable();
}

/*!
    Reads a line from the device, but no more than \a maxSize characters, and returns the result as a QByteArray.
*/
QByteArray QMailTransport::readLine(qint64 maxSize)
{
    return mSocket->readLine(maxSize);
}

/*!
    Reads all the data available in the device, and returns the result as a QByteArray.
*/
QByteArray QMailTransport::readAll()
{
    return mSocket->readAll();
}

/*! \internal */
void QMailTransport::connectionEstablished()
{
    connectToHostTimeOut.stop();
    if (mailEncryption() == Encrypt_NONE) {
        mConnected = true;
        emit updateStatus(tr("Connected"));
    }

    qCDebug(lcMessaging) << mName << ": connection established";
    emit connected(Encrypt_NONE);
}

/*! \internal */
void QMailTransport::hostConnectionTimeOut()
{
    connectToHostTimeOut.stop();
    errorHandling(QAbstractSocket::SocketTimeoutError, tr("Connection timed out"));
}

/*! \internal */
void QMailTransport::encryptionEstablished()
{
    if (mailEncryption() != Encrypt_NONE) {
        mConnected = true;
        emit updateStatus(tr("Connected"));
    }

    qCDebug(lcMessaging) << mName << ": Secure connection established";
    emit connected(mailEncryption());
}

/*! \internal */
void QMailTransport::connectionFailed(const QList<QSslError>& errors)
{
    QMailServiceAction::Status::ErrorCode errorCode = classifyCertificateErrors(errors);

    if (errorCode == QMailServiceAction::Status::ErrNoError) {
        qCWarning(lcMessaging) << "Accepting untrusted certificates";
        mSocket->ignoreSslErrors();
    } else {
        connectToHostTimeOut.stop();
        mConnected = false;
        mInUse = false;
        mSocket->abort();

        emit updateStatus(tr("Error occurred"));
        emit sslErrorOccured(errorCode, tr("Socket error"));
    }
}

/*! \internal */
QMailServiceAction::Status::ErrorCode QMailTransport::classifyCertificateErrors(const QList<QSslError>& errors)
{
    QMailServiceAction::Status::ErrorCode rv(QMailServiceAction::Status::ErrNoError);
    QString text;

    for (const QSslError& error : errors) {
        text += (text.isEmpty() ? QLatin1String("'") : QLatin1String(", '"));
        text += error.errorString();
        text += QLatin1String("'");

        if (error.error() == QSslError::NoSslSupport) {
            rv = QMailServiceAction::Status::ErrNoSslSupport;
        } else if (error.error() == QSslError::CertificateBlacklisted) {
            // Refuse the connection on more serious errors
            if (rv == QMailServiceAction::Status::ErrNoError) {
                rv = QMailServiceAction::Status::ErrUntrustedCertificates;
            }
        } else {
            // Only classify remainder as an error if untrusted certificates are not accepted
            if (!mAcceptUntrustedCertificates && rv == QMailServiceAction::Status::ErrNoError) {
                rv = QMailServiceAction::Status::ErrUntrustedCertificates;
            }
        }
    }

    bool failed(rv != QMailServiceAction::Status::ErrNoError);
    qCWarning(lcMessaging) << "Encrypted connect"
                           << (failed ? "failed:" : "warnings:") << text;
    return rv;
}

/*! \internal */
void QMailTransport::errorHandling(int status, QString msg)
{
    connectToHostTimeOut.stop();
    mConnected = false;
    mInUse = false;
    mSocket->abort();

    emit updateStatus(tr("Error occurred"));

    // Socket errors run from -1; offset this value by +2
    emit errorOccurred(status + 2, msg);
}

/*! \internal */
void QMailTransport::socketError(QAbstractSocket::SocketError status)
{
    qCWarning(lcMessaging) << "socketError:" << static_cast<int>(status)
                           << ':' << mSocket->errorString();
    errorHandling(static_cast<int>(status), tr("Socket error"));
}

/*!
    Returns the type of encryption in use by the transport.
*/
QMailTransport::EncryptType QMailTransport::mailEncryption() const
{
    return encryption;
}

/*!
    \fn void QMailTransport::connected(QMailTransport::EncryptType encryptType);

    This signal is emitted when a connection is achieved, with the encryption type \a encryptType.
*/

/*!
    \fn void QMailTransport::readyRead();

    This signal is emitted once every time new data is available for reading from the device.
    It will only be emitted again once new data is available, such as when a new payload of
    network data has arrived on your network socket, or when a new block of data has been
    appended to your device.
*/

/*!
    \fn void QMailTransport::bytesWritten(qint64 transmitted);

    This signal is emitted every time a payload of data has been written to the device.
    The \a transmitted argument is set to the number of bytes that were written in this payload.
*/

/*!
    \fn void QMailTransport::errorOccurred(int status, QString text);

    This signal is emitted when an error is encountered.
    The value of \a status corresponds to a value of QSslSocket::SocketError, and \a text
    contains a textual annotation where possible.
*/

/*!
    \fn void QMailTransport::updateStatus(const QString &status);

    This signal is emitted when a change in status is reported.  The new status is described by \a status.
*/

#include "qmailtransport.moc"

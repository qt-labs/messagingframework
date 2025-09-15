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

#ifndef QMAILTRANSPORT_H
#define QMAILTRANSPORT_H

#include <qmailglobal.h>
#include <qmailserviceaction.h>

#include <QObject>
#include <QAbstractSocket>
#include <QTimer>
#include <QSslError>

QT_BEGIN_NAMESPACE

class QString;
class QSslSocket;

QT_END_NAMESPACE

class MESSAGESERVER_EXPORT QMailTransport : public QObject
{
    Q_OBJECT

public:
    enum EncryptType {
        Encrypt_NONE = 0,
        Encrypt_SSL = 1,
        Encrypt_TLS = 2
    };

    QMailTransport(const char* name);
    virtual ~QMailTransport();

    // Open a connection to the specified server
    void open(const QString& url, int port, EncryptType encryptionType);

    void setAcceptUntrustedCertificates(bool accept);
    bool acceptUntrustedCertificates() const;

    // If connection is not currently encrypted, switch to encrypted mode
    void switchToEncrypted();

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
    bool bytesAvailable() const;
    QByteArray readLine(qint64 maxSize = 0);
    QByteArray readAll();

    // Assists with counting bytes written to the device
    void mark();
    qint64 bytesSinceMark() const;

Q_SIGNALS:
    void connected(QMailTransport::EncryptType encryptType);
    void readyRead();
    void bytesWritten(qint64 transmitted);

    void errorOccurred(int status, QString);
    void updateStatus(const QString &);
    void sslErrorOccured(QMailServiceAction::Status::ErrorCode, QString);

public Q_SLOTS:
    void errorHandling(int errorCode, QString msg);
    void socketError(QAbstractSocket::SocketError error);

protected Q_SLOTS:
    void connectionEstablished();
    void hostConnectionTimeOut();
    void encryptionEstablished();
    void connectionFailed(const QList<QSslError>& errors);

protected:
    // Override to modify certificate error handling
    virtual QMailServiceAction::Status::ErrorCode classifyCertificateErrors(const QList<QSslError>& errors);

private:
    void createSocket(EncryptType encryptType);
    EncryptType mailEncryption() const;

private:
    class Socket;

    Socket *mSocket;
    EncryptType encryption;
    QDataStream *mStream;
    const char *mName;
    QTimer connectToHostTimeOut;
    bool mConnected;
    bool mInUse;
    bool mAcceptUntrustedCertificates;
};

#endif

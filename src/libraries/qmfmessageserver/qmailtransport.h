/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QMAILTRANSPORT_H
#define QMAILTRANSPORT_H

#include <qmailglobal.h>

#include <QObject>
#include <QAbstractSocket>
#include <QTimer>

QT_BEGIN_NAMESPACE

class QString;
#ifndef QT_NO_OPENSSL
class QSslSocket;
class QSslError;
#endif

QT_END_NAMESPACE

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
    QTimer connectToHostTimeOut;
    bool mConnected;
    bool mInUse;
};

#endif

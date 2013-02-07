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

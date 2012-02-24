/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
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

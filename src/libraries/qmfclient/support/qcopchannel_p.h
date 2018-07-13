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

#ifndef QCOPCHANNEL_H
#define QCOPCHANNEL_H

#include <qobject.h>

#if !defined(Q_QCOP_EXPORT)
#if defined(QT_BUILD_QCOP_LIB)
#define Q_QCOP_EXPORT Q_DECL_EXPORT
#else
#define Q_QCOP_EXPORT Q_DECL_IMPORT
#endif
#endif

class QCopChannelPrivate;
class QCopClient;

class Q_QCOP_EXPORT QCopChannel : public QObject
{
    Q_OBJECT
public:
    explicit QCopChannel(const QString& channel, QObject *parent = Q_NULLPTR);
    virtual ~QCopChannel();

    QString channel() const;

    static bool isRegistered(const QString&  channel);
    static bool send(const QString& channel, const QString& msg);
    static bool send(const QString& channel, const QString& msg,
                      const QByteArray &data);

    static bool flush();

    static void sendLocally(const QString& ch, const QString& msg,
                            const QByteArray &data);
    static void reregisterAll();

    virtual void receive(const QString& msg, const QByteArray &data);

    bool isConnected() const;
    void connectRepeatedly();
    void disconnectFromServer();

Q_SIGNALS:
    void received(const QString& msg, const QByteArray &data);
    void forwarded(const QString& msg, const QByteArray &data, const QString& channel);
    void connected();
    void connectionFailed();
    void reconnectionTimeout();
    void connectionDown();

protected:
    void connectNotify(const QMetaMethod &);

protected Q_SLOTS:
    void connectClientSignals();

private:
    QCopChannelPrivate* d;

    friend class QCopClient;
};

#endif

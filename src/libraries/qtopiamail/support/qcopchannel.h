/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
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
    explicit QCopChannel(const QString& channel, QObject *parent=0);
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

Q_SIGNALS:
    void received(const QString& msg, const QByteArray &data);
    void forwarded(const QString& msg, const QByteArray &data, const QString& channel);

protected:
    void connectNotify(const char *);

private:
    QCopChannelPrivate* d;

    friend class QCopClient;
};

#endif

/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef QCOPADAPTOR_H
#define QCOPADAPTOR_H

#include <QtCore/qobject.h>
#include <QtCore/qstring.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qvariant.h>
#include <QtCore/qstringlist.h>

#if !defined(Q_QCOP_EXPORT)
#if defined(QT_BUILD_QCOP_LIB)
#define Q_QCOP_EXPORT Q_DECL_EXPORT
#else
#define Q_QCOP_EXPORT Q_DECL_IMPORT
#endif
#endif

class QCopAdaptorPrivate;
class QCopAdaptorEnvelopePrivate;

class Q_QCOP_EXPORT QCopAdaptorEnvelope
{
    friend class QCopAdaptor;
private:
    QCopAdaptorEnvelope(const QStringList& channels, const QString& message);

public:
    QCopAdaptorEnvelope();
    QCopAdaptorEnvelope(const QCopAdaptorEnvelope& value);
    ~QCopAdaptorEnvelope();

    QCopAdaptorEnvelope& operator=(const QCopAdaptorEnvelope& value);
    template <class T>
    QCopAdaptorEnvelope& operator<<(const T &value);

    inline QCopAdaptorEnvelope& operator<<(const char *value)
    {
        addArgument(QVariant(QString(value)));
        return *this;
    }

private:
    QCopAdaptorEnvelopePrivate *d;

    void addArgument(const QVariant& value);
};

class Q_QCOP_EXPORT QCopAdaptor : public QObject
{
    Q_OBJECT
    friend class QCopAdaptorPrivate;
    friend class QCopAdaptorEnvelope;
    friend class QCopAdaptorChannel;
public:
    explicit QCopAdaptor(const QString& channel, QObject *parent = 0);
    ~QCopAdaptor();

    QString channel() const;

    static bool connect(QObject *sender, const QByteArray& signal,
                        QObject *receiver, const QByteArray& member);

    QCopAdaptorEnvelope send(const QByteArray& member);
    void send(const QByteArray& member, const QVariant &arg1);
    void send(const QByteArray& member, const QVariant &arg1,
              const QVariant &arg2);
    void send(const QByteArray& member, const QVariant &arg1,
              const QVariant &arg2, const QVariant &arg3);
    void send(const QByteArray& member, const QList<QVariant>& args);

    bool isConnected(const QByteArray& signal);

protected:
    enum PublishType
    {
        Signals,
        Slots,
        SignalsAndSlots
    };

    bool publish(const QByteArray& member);
    void publishAll(QCopAdaptor::PublishType type);
    virtual QString memberToMessage(const QByteArray& member);
    virtual QStringList sendChannels(const QString& channel);
    virtual QString receiveChannel(const QString& channel);

private slots:
    void received(const QString& msg, const QByteArray& data);
    void receiverDestroyed();

private:
    QCopAdaptorPrivate *d;

    bool connectLocalToRemote(QObject *sender, const QByteArray& signal,
                              const QByteArray& member);
    bool connectRemoteToLocal(const QByteArray& signal, QObject *receiver,
                              const QByteArray& member);
    void sendMessage(const QString& msg, const QList<QVariant>& args);
    static void send(const QStringList& channels,
                     const QString& msg, const QList<QVariant>& args);
};

template<class T>
QCopAdaptorEnvelope& QCopAdaptorEnvelope::operator<<(const T &value)
{
    addArgument(qVariantFromValue(value));
    return *this;
}

// Useful alias to make it clearer when connecting to messages on a channel.
#define MESSAGE(x)      "3"#x
#define QMESSAGE_CODE   3

#endif

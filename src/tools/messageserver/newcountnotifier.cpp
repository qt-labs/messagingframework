/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#include <newcountnotifier.h>

#ifdef QMAIL_QTOPIA

#include <QDataStream>
#include <QByteArray>
#include <QMimeType>
#include <QDSAction>
#include <QDSData>

extern QString serviceForType(QMailMessage::MessageType);
extern QMap<QMailMessage::MessageType, QString> typeSignatureInit();

class NewCountNotifierPrivate : public QObject
{
    Q_OBJECT
public:
    NewCountNotifierPrivate(QMailMessage::MessageType mtype, int mcount);
    int count;
    QDSAction action;

signals:
    void response(bool handled);
    void error(const QString& message);

public slots:
    void actionResponse(const QUniqueId &uid, const QDSData &responseData);
    void actionError(const QUniqueId &uid, const QString &message);
};

void NewCountNotifierPrivate::actionResponse(const QUniqueId&, const QDSData& responseData)
{
    const QByteArray &data(responseData.data());

    bool handled(false);
    {
        QDataStream extractor(data);
        extractor >> handled;
    }

    emit response(handled);
}

void NewCountNotifierPrivate::actionError(const QUniqueId&, const QString& message)
{
    emit error(message);
}

NewCountNotifierPrivate::NewCountNotifierPrivate(QMailMessage::MessageType mtype, int mcount)
:
    QObject(),
    count(mcount),
    action(QDSAction("arrived",serviceForType(mtype)))
{
    connect(&action, SIGNAL(response(QUniqueId, QDSData)), this, SLOT(actionResponse(QUniqueId, QDSData)));
    connect(&action, SIGNAL(error(QUniqueId, QString)), this, SLOT(actionError(QUniqueId, QString)));
}

NewCountNotifier::NewCountNotifier(QMailMessage::MessageType type, int count)
:
    QObject(),
    d(new NewCountNotifierPrivate(type,count))
{
    connect(d, SIGNAL(response(bool)), this, SIGNAL(response(bool)));
    connect(d, SIGNAL(error(const QString&)), this, SIGNAL(error(const QString&)));
}

NewCountNotifier::~NewCountNotifier()
{
    delete d; d=0;
}

bool NewCountNotifier::notify()
{
    QByteArray data;
    {
        QDataStream insertor(&data, QIODevice::WriteOnly);
        insertor << d->count;
    }

    return (d->action.invoke(QDSData(data, QMimeType())));
}

void NewCountNotifier::notify(QMailMessage::MessageType type, int count)
{
    static QMap<QMailMessage::MessageType, QString> typeSignature(typeSignatureInit());

    if (typeSignature.contains(type)){
#ifdef QMAIL_QTOPIA
        QtopiaIpcEnvelope e("QPE/MessageControl", typeSignature[type]);
#else
        QCopAdaptor a("QPE/MessageControl");
        QCopAdaptorEnvelope e = a.send(typeSignature[type].toLatin1());
#endif
    e << static_cast<int>(count);
    }
}

#else

//stub for alternate handling, currently does nothing

NewCountNotifier::NewCountNotifier(QMailMessage::MessageType, int)
:
    QObject()
{
}

NewCountNotifier::~NewCountNotifier()
{
}

bool NewCountNotifier::notify()
{
    //ensure a response is returned
    emit response(true);
    return true;
}

void NewCountNotifier::notify(QMailMessage::MessageType, int)
{
}

#endif //QMAIL_QTOPIA


#include <newcountnotifier.moc>


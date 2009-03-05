/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef QMAILCOMPOSER_H
#define QMAILCOMPOSER_H

#include <QWidget>
#include <QList>
#include <QString>
#include <QIconSet>
#include <qmailglobal.h>
#include <qmailmessage.h>

class QMenu;
class QMailAccount;

class QTOPIAMAIL_EXPORT QMailComposerInterface : public QWidget
{
    Q_OBJECT

public:
    enum ComposeContext { Create = 0, Reply = 1, ReplyToAll = 2, Forward = 3 };

public:
    QMailComposerInterface( QWidget *parent = 0 );
    virtual ~QMailComposerInterface();

    virtual QString key() const = 0;
    virtual QList<QMailMessage::MessageType> messageTypes() const = 0;
    virtual QList<QMailMessage::ContentType> contentTypes() const = 0;
    virtual QString name(QMailMessage::MessageType type) const = 0;
    virtual QString displayName(QMailMessage::MessageType type) const = 0;
    virtual QIcon displayIcon(QMailMessage::MessageType type) const = 0;

    virtual bool isSupported(QMailMessage::MessageType t, QMailMessage::ContentType c = QMailMessage::NoContent) const
    {
        bool supportsMessageType(t == QMailMessage::AnyType || messageTypes().contains(t));
        bool supportsContentType(c == QMailMessage::NoContent || contentTypes().contains(c));

        return (supportsMessageType && supportsContentType);
    }

    virtual bool isEmpty() const = 0;
    virtual bool isReadyToSend() const = 0;

    virtual QMailMessage message() const = 0;

    virtual void setTo(const QString& toAddress) = 0;
    virtual QString to() const = 0;

    virtual void setFrom(const QString& fromAddress) = 0;
    virtual QString from() const = 0;

    virtual void setSubject(const QString& subject) = 0;

    virtual void setMessageType(QMailMessage::MessageType type);

    virtual bool isDetailsOnlyMode() const = 0;
    virtual void setDetailsOnlyMode(bool val) = 0;

    virtual QString contextTitle() const = 0;

    virtual QMailAccount fromAccount() const = 0;

public slots:
    virtual void setMessage( const QMailMessage& mail ) = 0;

    virtual void clear() = 0;

    virtual void setBody( const QString &text, const QString &type );
    //virtual void attach( const QContent &lnk, QMailMessage::AttachmentsAction action = QMailMessage::LinkToAttachments );

    virtual void setSignature( const QString &sig );

    virtual void reply(const QMailMessage& source, int type) = 0;

signals:
    void sendMessage();
    void cancel();
    void changed();
    void contextChanged();
};

class QTOPIAMAIL_EXPORT QMailComposerFactory
{
public:
    // Yield the key for each interface supporting the supplied type
    static QStringList keys(QMailMessage::MessageType type = QMailMessage::AnyType,
                            QMailMessage::ContentType contentType = QMailMessage::NoContent);

    // Yield the default key for the supplied type
    static QString defaultKey( QMailMessage::MessageType type = QMailMessage::AnyType );

    // Properties available for each interface
    static QList<QMailMessage::MessageType> messageTypes(const QString &key);
    //static QList<QMailMessage::ContentType> contentTypes(const QString& key);
    static QString name(const QString &key, QMailMessage::MessageType type);
    static QString displayName(const QString &key, QMailMessage::MessageType type);
    static QIcon displayIcon(const QString &key, QMailMessage::MessageType type);

    // Use the interface identified by the supplied key to create a composer
    static QMailComposerInterface *create( const QString& key, QWidget *parent = 0 );
};

#endif

/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef QMAILVIEWER_H
#define QMAILVIEWER_H

#include <qmailmessage.h>
#include <QObject>
#include <QString>
#include <QVariant>
#include <qmailglobal.h>

class QContact;
class QMenu;
class QUrl;
class QWidget;

class QMailViewerInterface;

class QTOPIAMAIL_EXPORT QMailViewerFactory
{
public:
    enum PresentationType
    {
        AnyPresentation = 0,
        StandardPresentation = 1,
        ConversationPresentation = 2,
        UserPresentation = 64
    };

    // Yield the ID for each interface supporting the supplied type, where the
    // value is interpreted as a ContentType value
    static QStringList keys(QMailMessage::ContentType type = QMailMessage::UnknownContent, PresentationType pres = AnyPresentation);

    // Yield the default ID for the supplied type
    static QString defaultKey(QMailMessage::ContentType type = QMailMessage::UnknownContent, PresentationType pres = AnyPresentation);

    // Use the interface identified by the supplied ID to create a viewer
    static QMailViewerInterface *create(const QString &key, QWidget *parent = 0);
};

// The interface for objects able to view mail messages
class QTOPIAMAIL_EXPORT QMailViewerInterface : public QObject
{
    Q_OBJECT

public:
    QMailViewerInterface( QWidget* parent = 0 );
    virtual ~QMailViewerInterface();

    virtual QWidget *widget() const = 0;

    virtual void scrollToAnchor(const QString& link);

    virtual void addActions(QMenu* menu) const;

    virtual bool handleIncomingMessages(const QMailMessageIdList &list) const;
    virtual bool handleOutgoingMessages(const QMailMessageIdList &list) const;

    virtual QString key() const = 0;
    virtual QMailViewerFactory::PresentationType presentation() const = 0;
    virtual QList<int> types() const = 0;

    bool isSupported(QMailMessage::ContentType t, QMailViewerFactory::PresentationType pres) const
    {
        if ((pres != QMailViewerFactory::AnyPresentation) && (pres != presentation()))
            return false;

        return types().contains(t);
    }


public slots:
    virtual bool setMessage(const QMailMessage& mail) = 0;
    virtual void setResource(const QUrl& name, QVariant value);
    virtual void clear() = 0;

signals:
    void replyToSender();
    void replyToAll();
    void forwardMessage();
    void deleteMessage();
    void saveSender();
    void contactDetails(const QContact &contact);
    void anchorClicked(const QUrl &link);
    void messageChanged(const QMailMessageId &id);
    void viewMessage(const QMailMessageId &id, QMailViewerFactory::PresentationType);
    void sendMessage(const QMailMessage &message);
    void finished();

    void retrieveMessage();
    void retrieveMessagePortion(uint bytes);

    void retrieveMessagePart(const QMailMessagePart &part);
    void retrieveMessagePartPortion(const QMailMessagePart &part, uint bytes);
};

#endif

/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef MESSAGESSERVICE_H
#define MESSAGESSERVICE_H

#include <QContent>
#include <QMailMessageId>
#include <QMailMessage>
#include <QtopiaAbstractService>

class MessagesService : public QtopiaAbstractService
{
    Q_OBJECT

public:
    MessagesService(QObject* parent);
    ~MessagesService();

signals:
    void view();
    void viewNew(bool);
    void view(const QMailMessageId&);
    void replyTo(const QMailMessageId&);
    void compose(QMailMessage::MessageType,
                 const QMailAddressList&,
                 const QString&,
                 const QString&,
                 const QContentList&,
                 QMailMessage::AttachmentsAction);
    void compose(const QMailMessage&);

public slots:
    void viewMessages();
    void viewNewMessages(bool userRequest);
    void viewMessage(QMailMessageId id);
    void replyToMessage(QMailMessageId id);
    void composeMessage(QMailMessage::MessageType,
                        QMailAddressList,
                        QString,
                        QString);
    void composeMessage(QMailMessage::MessageType,
                        QMailAddressList,
                        QString,
                        QString,
                        QContentList,
                        QMailMessage::AttachmentsAction);
    void composeMessage(QMailMessage);
};

#endif

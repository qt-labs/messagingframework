/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef QMAILNEWMESSAGEHANDLER_H
#define QMAILNEWMESSAGEHANDLER_H

#include "qmailglobal.h"
#include "qmailmessage.h"
#include <QtopiaAbstractService>
#include <QDSActionRequest>

class QTOPIAMAIL_EXPORT QMailNewMessageHandler : public QtopiaAbstractService
{
    Q_OBJECT

public:
    QMailNewMessageHandler(const QString &service, QObject* parent);
    ~QMailNewMessageHandler();

    virtual QMailMessage::MessageType messageType() const = 0;

signals:
    void newCountChanged(uint newCount);
    
public slots:
    void arrived(const QDSActionRequest &request);
    void setHandled(bool b);

private:
    QDSActionRequest request;
    bool pending;
};

class QTOPIAMAIL_EXPORT QMailNewSmsHandler : public QMailNewMessageHandler
{
public:
    QMailNewSmsHandler(QObject* parent = 0);

    virtual QMailMessage::MessageType messageType() const;
};

class QTOPIAMAIL_EXPORT QMailNewMmsHandler : public QMailNewMessageHandler
{
public:
    QMailNewMmsHandler(QObject* parent = 0);

    virtual QMailMessage::MessageType messageType() const;
};

class QTOPIAMAIL_EXPORT QMailNewEmailHandler : public QMailNewMessageHandler
{
public:
    QMailNewEmailHandler(QObject* parent = 0);

    virtual QMailMessage::MessageType messageType() const;
};

class QTOPIAMAIL_EXPORT QMailNewInstantMessageHandler : public QMailNewMessageHandler
{
public:
    QMailNewInstantMessageHandler(QObject* parent = 0);

    virtual QMailMessage::MessageType messageType() const;
};

class QTOPIAMAIL_EXPORT QMailNewSystemMessageHandler : public QMailNewMessageHandler
{
public:
    QMailNewSystemMessageHandler(QObject* parent = 0);

    virtual QMailMessage::MessageType messageType() const;
};

#endif

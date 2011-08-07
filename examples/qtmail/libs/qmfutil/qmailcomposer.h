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

#ifndef QMAILCOMPOSER_H
#define QMAILCOMPOSER_H

#include <QWidget>
#include <QList>
#include <QString>
#include <QIconSet>
#include <qmailglobal.h>
#include <qmailmessage.h>
#include <QAction>

class QMailAccount;

QT_BEGIN_NAMESPACE

class QMenu;

QT_END_NAMESPACE

class QMFUTIL_EXPORT QMailComposerInterface : public QWidget
{
    Q_OBJECT

public:
    QMailComposerInterface( QWidget *parent = 0 );

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

    virtual QString title() const = 0;
    virtual void compose(QMailMessage::ResponseType type,
                         const QMailMessage& source = QMailMessage(),
                         const QMailMessagePart::Location& sourceLocation = QMailMessagePart::Location(),
                         QMailMessage::MessageType = QMailMessage::AnyType) = 0;

    virtual QMailMessage message() const = 0;
    virtual QList<QAction*> actions() const;
    virtual bool isEmpty() const = 0;
    virtual bool isReadyToSend() const = 0;
    virtual QString status() const;

public slots:
    virtual void clear() = 0;
    //virtual void attach( const QContent &lnk, QMailMessage::AttachmentsAction action = QMailMessage::LinkToAttachments );
    virtual void setSignature(const QString &sig);
    virtual void setSendingAccountId(const QMailAccountId &accountId);

signals:
    void sendMessage();
    void cancel();
    void changed();
    void statusChanged(const QString& status);
};

class QMFUTIL_EXPORT QMailComposerFactory
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

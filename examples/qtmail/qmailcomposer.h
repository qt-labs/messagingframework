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

#ifndef QMAILCOMPOSER_H
#define QMAILCOMPOSER_H

#include <QWidget>
#include <QList>
#include <QString>
#include <qmailglobal.h>
#include <qmailmessage.h>
#include <QAction>

class QMailAccount;

QT_BEGIN_NAMESPACE

class QMenu;

QT_END_NAMESPACE

class QMailComposerInterface : public QWidget
{
    Q_OBJECT

public:
    QMailComposerInterface( QWidget *parent = Q_NULLPTR );

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

class QMailComposerFactory
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
    static QMailComposerInterface *create( const QString& key, QWidget *parent = Q_NULLPTR );
};

#endif

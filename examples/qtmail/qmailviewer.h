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

#ifndef QMAILVIEWER_H
#define QMAILVIEWER_H

#include <qmailmessage.h>
#include <QObject>
#include <QString>
#include <QVariant>
#include <qmailglobal.h>
#include <QWidget>

QT_BEGIN_NAMESPACE

class QContact;
class QMenu;
class QUrl;

QT_END_NAMESPACE

class QMailViewerInterface;

class QMailViewerFactory
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
    static QMailViewerInterface *create(const QString &key, QWidget *parent = Q_NULLPTR);
};

// The interface for objects able to view mail messages
class QMailViewerInterface : public QObject
{
    Q_OBJECT

public:
    QMailViewerInterface( QWidget* parent = Q_NULLPTR );
    virtual ~QMailViewerInterface();

    virtual QWidget* widget() const = 0;

    virtual void scrollToAnchor(const QString& link);

    virtual void addActions(const QList<QAction*>& actions) = 0;
    virtual void removeAction(QAction* action) = 0;

    virtual bool handleIncomingMessages(const QMailMessageIdList &list) const;
    virtual bool handleOutgoingMessages(const QMailMessageIdList &list) const;

    virtual QString key() const = 0;
    virtual QMailViewerFactory::PresentationType presentation() const = 0;
    virtual QList<QMailMessage::ContentType> types() const = 0;

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
    void anchorClicked(const QUrl &link);
    void contactDetails(const QContact &contact);
    void messageChanged(const QMailMessageId &id);
    void viewMessage(const QMailMessageId &id, QMailViewerFactory::PresentationType type);
    void sendMessage(QMailMessage &message);
    void retrieveMessage();
    void retrieveMessagePortion(uint bytes);
    void retrieveMessagePart(const QMailMessagePart &part);
    void retrieveMessagePartPortion(const QMailMessagePart &part, uint bytes);
    void respondToMessage(QMailMessage::ResponseType type);
    void respondToMessagePart(const QMailMessagePart::Location &partLocation, QMailMessage::ResponseType type);
    void finished();
};

#endif

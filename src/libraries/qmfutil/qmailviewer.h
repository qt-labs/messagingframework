/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Qt Software Information (qt-info@nokia.com)
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the either Technology Preview License Agreement or the
** Beta Release License Agreement.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
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

class QContact;
class QMenu;
class QUrl;

class QMailViewerInterface;

class QMFUTIL_EXPORT QMailViewerFactory
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
class QMFUTIL_EXPORT QMailViewerInterface : public QObject
{
    Q_OBJECT

public:
    QMailViewerInterface( QWidget* parent = 0 );
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

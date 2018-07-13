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

#ifndef READMAIL_H
#define READMAIL_H

#include <QMap>
#include <QStack>
#include <qmailmessage.h>
#include <qmailviewer.h>
#include <QFrame>

class QMailViewerInterface;

QT_BEGIN_NAMESPACE

class QAction;
class QLabel;
class QMenu;
class QStackedWidget;
class QUrl;

QT_END_NAMESPACE

class ReadMail : public QFrame
{
    Q_OBJECT

public:
    ReadMail( QWidget* parent = Q_NULLPTR, Qt::WindowFlags fl = 0 );

    void displayMessage(const QMailMessageId& message, QMailViewerFactory::PresentationType, bool nextAvailable, bool previousAvailable);
    QMailMessageId displayedMessage() const;

    bool handleIncomingMessages(const QMailMessageIdList &list) const;
    bool handleOutgoingMessages(const QMailMessageIdList &list) const;

private slots:
    void updateView(QMailViewerFactory::PresentationType);
    void messageContentsModified(const QMailMessageIdList& list);
    void messagesRemoved(const QMailMessageIdList& list);

signals:
    void responseRequested(const QMailMessage& message,QMailMessage::ResponseType type);
    void responseRequested(const QMailMessagePart::Location &partLocation, QMailMessage::ResponseType type);
    void sendMessageTo(const QMailAddress&, QMailMessage::MessageType);
    void viewingMail(const QMailMessageMetaData&);
    void getMailRequested(const QMailMessageMetaData&);
    void readReplyRequested(const QMailMessageMetaData&);
    void viewMessage(const QMailMessageId &id, QMailViewerFactory::PresentationType);
    void sendMessage(QMailMessage &message);
    void retrieveMessagePortion(const QMailMessageMetaData &message, uint bytes);
    void retrieveMessagePart(const QMailMessagePart::Location &partLocation);
    void retrieveMessagePartPortion(const QMailMessagePart::Location &partLocation, uint bytes);
    void flagMessage(const QMailMessageId &id, quint64 setMask, quint64 unsetMask);

public slots:
    void setSendingInProgress(bool);
    void setRetrievalInProgress(bool);

protected slots:
    void linkClicked(const QUrl &lnk);
    void messageChanged(const QMailMessageId &id);

    void getThisMail();
    void retrieveMessagePortion(uint bytes);
    void retrieveMessagePart(const QMailMessagePart &part);
    void retrieveMessagePartPortion(const QMailMessagePart &part, uint bytes);
    void respondToMessage(QMailMessage::ResponseType type);
    void respondToMessagePart(const QMailMessagePart::Location &partLocation, QMailMessage::ResponseType type);

protected:
    void keyPressEvent(QKeyEvent *);

private:
    void init();
    void showMessage(const QMailMessageId &id, QMailViewerFactory::PresentationType);
    void loadMessage(const QMailMessageId &id);
    void updateButtons();
    void buildMenu(const QString &mailbox);
    void initImages(QMailViewerInterface* view);

    void switchView(QMailViewerInterface* viewer, const QString& title);

    QMailViewerInterface* currentViewer() const;

    QMailViewerInterface* viewer(QMailMessage::ContentType content, QMailViewerFactory::PresentationType type = QMailViewerFactory::StandardPresentation);

    static QString displayName(const QMailMessage &);

    void updateReadStatus();

private:
    QStackedWidget *views;
    bool sending, receiving;
    QMailMessage mail;
    bool firstRead;
    bool hasNext, hasPrevious;
    QMenu *context;
    QAction *attachmentsButton;
    QAction *getThisMailButton;
    QAction *storeButton;
    bool modelUpdatePending;
    QString lastTitle;

    typedef QStack<QPair<QMailViewerInterface*, QString> > ViewerStack;
    ViewerStack currentView;

    typedef QMap<QPair<QMailMessage::ContentType, QMailViewerFactory::PresentationType>, QMailViewerInterface*> ViewerMap;
    ViewerMap contentViews;
};

#endif

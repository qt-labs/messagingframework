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

#ifndef READMAIL_H
#define READMAIL_H

#include <QMap>
#include <QStack>
#include <qmailmessage.h>
#include <qmailviewer.h>
#include <QFrame>

class QAction;
class QLabel;
class QMailViewerInterface;
class QMenu;
class QStackedWidget;
class QUrl;

class ReadMail : public QFrame
{
    Q_OBJECT

public:
    ReadMail( QWidget* parent = 0, Qt::WFlags fl = 0 );

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
    bool initialized;
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

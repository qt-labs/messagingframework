/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
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
    enum ResendAction { Reply = 1, ReplyToAll = 2, Forward = 3 };

    ReadMail( QWidget* parent = 0, Qt::WFlags fl = 0 );
    ~ReadMail();

    void displayMessage(const QMailMessageId& message, QMailViewerFactory::PresentationType, bool nextAvailable, bool previousAvailable);
    QMailMessageId displayedMessage() const;

    bool handleIncomingMessages(const QMailMessageIdList &list) const;
    bool handleOutgoingMessages(const QMailMessageIdList &list) const;

private slots:
    void updateView(QMailViewerFactory::PresentationType);
    void messageContentsModified(const QMailMessageIdList& list);

signals:
    void resendRequested(const QMailMessage&, int);
    void sendMessageTo(const QMailAddress&, QMailMessage::MessageType);
    void modifyRequested(const QMailMessage&);
    void removeMessage(const QMailMessageId& id, bool userRequest);
    void viewingMail(const QMailMessageMetaData&);
    void getMailRequested(const QMailMessageMetaData&);
    void sendMailRequested(QMailMessageMetaData&);
    void readReplyRequested(const QMailMessageMetaData&);
    void viewMessage(const QMailMessageId &id, QMailViewerFactory::PresentationType);
    void sendMessage(const QMailMessage &message);
    void retrieveMessagePortion(const QMailMessageMetaData &message, uint bytes);
    void retrieveMessagePart(const QMailMessagePart::Location &partLocation);
    void retrieveMessagePartPortion(const QMailMessagePart::Location &partLocation, uint bytes);

public slots:
    void setSendingInProgress(bool);
    void setRetrievalInProgress(bool);

protected slots:
    void linkClicked(const QUrl &lnk);
    void messageChanged(const QMailMessageId &id);

    void deleteItem();

    void reply();
    void replyAll();
    void forward();
    void modify();

    void setStatus(int);
    void getThisMail();
    void sendThisMail();
    void retrieveMessagePortion(uint bytes);
    void retrieveMessagePart(const QMailMessagePart &part);
    void retrieveMessagePartPortion(const QMailMessagePart &part, uint bytes);

protected:
    void keyPressEvent(QKeyEvent *);

private:
    void viewMms();

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
    bool isMms;
    bool isSmil;
    bool firstRead;
    bool hasNext, hasPrevious;
    QMenu *context;
    QAction *deleteButton;
    bool initialized;
    QAction *attachmentsButton;
    QAction *replyButton;
    QAction *replyAllAction;
    QAction *forwardAction;
    QAction *getThisMailButton;
    QAction *sendThisMailButton;
    QAction *modifyButton;
    QAction *storeButton;
    bool modelUpdatePending;
    QString lastTitle;

    typedef QStack<QPair<QMailViewerInterface*, QString> > ViewerStack;
    ViewerStack currentView;

    typedef QMap<QPair<QMailMessage::ContentType, QMailViewerFactory::PresentationType>, QMailViewerInterface*> ViewerMap;
    ViewerMap contentViews;
};

#endif

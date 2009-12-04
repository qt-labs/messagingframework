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

#include "emailclient.h"
#include "selectfolder.h"
#include "emailfoldermodel.h"
#include "emailfolderview.h"
#include <longstream_p.h>
#include <qmaillog.h>
#include <QApplication>
#include <QDebug>
#include <QDesktopWidget>
#include <QFile>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <qmailaccount.h>
#include <qmailaddress.h>
#include <qmailcomposer.h>
#include <qmailstore.h>
#include <qmailtimestamp.h>
#include <QMessageBox>
#include <QStack>
#include <QStackedWidget>
#include <QThread>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QSettings>
#include "searchview.h"
#include "readmail.h"
#include "writemail.h"
#include <QMenuBar>
#include <qmailnamespace.h>
#include <QSplitter>
#include <QListView>
#include <QToolBar>
#include <QMovie>
#include <QStatusBar>
#include <statusdisplay.h>

class ActivityIcon : public QLabel
{
    Q_OBJECT

public:
    ActivityIcon(QList<const QMailServiceAction*> actions,
                 QWidget* parent = 0);

private slots:
    void activityChanged(QMailServiceAction::Activity a);
    void showActivity(bool val);

private:
    QMovie m_activeIcon;
    QPixmap m_inactiveIcon;
    QList<QObject*> m_activeActions;
};

ActivityIcon::ActivityIcon(const QList<const QMailServiceAction*> actions,
                           QWidget* parent)
:
QLabel(parent),
m_activeIcon(":icon/activity_working"),
m_inactiveIcon(":icon/activity_idle")
{
    foreach(const QMailServiceAction* a, actions)
        connect(a,SIGNAL(activityChanged(QMailServiceAction::Activity)),
                this,SLOT(activityChanged(QMailServiceAction::Activity)));

    setPixmap(m_inactiveIcon);
}

void ActivityIcon::activityChanged(QMailServiceAction::Activity a)
{
    bool active = (a == QMailServiceAction::InProgress);
    if(active && !m_activeActions.contains(sender()))
        m_activeActions.append(sender());
    else if(!active)
        m_activeActions.removeAll(sender());

    showActivity(!m_activeActions.isEmpty());
}

void ActivityIcon::showActivity(bool val)
{
    if(val)
    {
        if(m_activeIcon.state() == QMovie::Running)
            return;
        setMovie(&m_activeIcon);
        m_activeIcon.start();
    }
    else
    {
        if(m_activeIcon.state() == QMovie::NotRunning)
            return;
        m_activeIcon.stop();
        setPixmap(m_inactiveIcon);
    }
}

static const int defaultWidth = 1024;
static const int defaultHeight = 768;

enum ActivityType {
    Inactive = 0,
    Retrieving = 1,
    Sending = 2
};

static bool confirmDelete( QWidget *parent, const QString & caption, const QString & object ) {
    QString msg = "<qt>" + QString("Are you sure you want to delete: %1?").arg( object ) + "</qt>";
    int r = QMessageBox::warning( parent, caption, msg, QMessageBox::Yes, QMessageBox::No|QMessageBox::Default| QMessageBox::Escape, 0 );
    return r == QMessageBox::Yes;
}

// Number of new messages to request per increment
static const int MoreMessagesIncrement = 20;

// Time in ms to show new message dialog.  0 == Indefinate
static const int NotificationVisualTimeout = 0;

// This is used regularly:
static const QMailMessage::MessageType nonEmailType = static_cast<QMailMessage::MessageType>(QMailMessage::Mms |
                                                                                             QMailMessage::Sms |
                                                                                             QMailMessage::Instant |
                                                                                             QMailMessage::System);
class SearchProgressDialog : public QDialog
{
    Q_OBJECT

public:
    SearchProgressDialog(QMailSearchAction* action);

public slots:
    void progressChanged(uint progress, uint max);
};

SearchProgressDialog::SearchProgressDialog(QMailSearchAction* action)
:
    QDialog()
{
    setWindowTitle(tr("Searching"));
    connect(action,SIGNAL(progressChanged(uint,uint)),this,SLOT(progressChanged(uint,uint)));
    connect(this,SIGNAL(cancelled()),action,SLOT(cancelOperation()));
}

void SearchProgressDialog::progressChanged(uint value, uint max)
{
    QString percentageText;

    if((value + max) > 0)
    {
        float percentage = (max == 0) ? 0 : static_cast<float>(value)/max*100;
        percentageText = QString::number(percentage,'f',0) + "%";
    }

    QString text = tr("Searching") + "\n" + percentageText;
    setWindowTitle(text);
}

class AcknowledgmentBox : public QMessageBox
{
    Q_OBJECT

public:
    static void show(const QString& title, const QString& text);

private:
    AcknowledgmentBox(const QString& title, const QString& text);

    virtual void keyPressEvent(QKeyEvent* event);

    static const int _timeout = 3 * 1000;
};

AcknowledgmentBox::AcknowledgmentBox(const QString& title, const QString& text)
    : QMessageBox(0)
{
    setWindowTitle(title);
    setText(text);
    setIcon(QMessageBox::Information);
    setAttribute(Qt::WA_DeleteOnClose);

    QDialog::show();
    QTimer::singleShot(_timeout, this, SLOT(accept()));
}

void AcknowledgmentBox::show(const QString& title, const QString& text)
{
    (void)new AcknowledgmentBox(title, text);
}

void AcknowledgmentBox::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Select) {
        event->accept();
        accept();
    } else {
        QMessageBox::keyPressEvent(event);
    }
}

MessageUiBase::MessageUiBase(QWidget *parent, Qt::WindowFlags f)
    : QMainWindow(parent,f),
      appTitle(tr("Messages")),
      suspendMailCount(true),
      markingMode(false),
      threaded(true),
      selectionCount(0),
      emailCountSuspended(false)
{
    setWindowTitle( appTitle );
}

void MessageUiBase::viewSearchResults(const QMailMessageKey&, const QString& title)
{
    //TODO
    QString caption(title);
    if (caption.isNull())
        caption = tr("Search Results");
}

void MessageUiBase::viewComposer()
{
    writeMailWidget()->raise();
    writeMailWidget()->activateWindow();
    writeMailWidget()->show();
}

WriteMail* MessageUiBase::writeMailWidget() const
{
    static WriteMail* writeMail = const_cast<MessageUiBase*>(this)->createWriteMailWidget();
    return writeMail;
}

ReadMail* MessageUiBase::readMailWidget() const
{
    static ReadMail* readMail = const_cast<MessageUiBase*>(this)->createReadMailWidget();
    return readMail;
}

EmailFolderView* MessageUiBase::folderView() const
{
    static EmailFolderView* view = const_cast<MessageUiBase*>(this)->createFolderView();
    return view;
}

MessageListView* MessageUiBase::messageListView() const
{
    static MessageListView* view = const_cast<MessageUiBase*>(this)->createMessageListView();
    return view;
}

EmailFolderModel* MessageUiBase::emailFolderModel() const
{
    static EmailFolderModel* model = const_cast<MessageUiBase*>(this)->createEmailFolderModel();
    return model;
}

SearchView* MessageUiBase::searchView() const
{
    static SearchView* searchview = const_cast<MessageUiBase*>(this)->createSearchView();
    return searchview;
}

void MessageUiBase::showFolderStatus(QMailMessageSet* item)
{
    if (item)
        emit updateStatus(item->data(EmailFolderModel::FolderStatusDetailRole).value<QString>());
}

void MessageUiBase::contextStatusUpdate()
{
    if (suspendMailCount)
        return;
    showFolderStatus(folderView()->currentItem());
}

void MessageUiBase::suspendMailCounts()
{
    suspendMailCount = true;

    if (!emailFolderModel()->ignoreMailStoreUpdates()) {
        emailFolderModel()->setIgnoreMailStoreUpdates(true);
        emailCountSuspended = true;
    }
}

void MessageUiBase::resumeMailCounts()
{
    suspendMailCount = false;

    if (emailCountSuspended) {
        emailFolderModel()->setIgnoreMailStoreUpdates(false);
        emailCountSuspended = false;
    }

    contextStatusUpdate();
}

void MessageUiBase::messageSelectionChanged()
{
    selectionCount = messageListView()->selected().count();
    contextStatusUpdate();
}

void MessageUiBase::setMarkingMode(bool set)
{
    markingMode = set;

    messageListView()->setMarkingMode(markingMode);
    if (!markingMode) {
        // Clear whatever selections were previously made
        messageListView()->clearSelection();

    }
    contextStatusUpdate();
}

void MessageUiBase::setThreaded(bool set)
{
    threaded = set;

    messageListView()->setThreaded(threaded);
    contextStatusUpdate();
}

void MessageUiBase::clearStatusText()
{
    emit clearStatus();
}

void MessageUiBase::presentMessage(const QMailMessageId &id, QMailViewerFactory::PresentationType type)
{
    readMailWidget()->displayMessage(id, type, false, false);
}

WriteMail* MessageUiBase::createWriteMailWidget()
{
    WriteMail* writeMail = new WriteMail(this);
    writeMail->setObjectName("write-mail");

    connect(writeMail, SIGNAL(enqueueMail(QMailMessage&)), this, SLOT(enqueueMail(QMailMessage&)));
    connect(writeMail, SIGNAL(discardMail()), this, SLOT(discardMail()));
    connect(writeMail, SIGNAL(saveAsDraft(QMailMessage&)), this, SLOT(saveAsDraft(QMailMessage&)));
    connect(writeMail, SIGNAL(noSendAccount(QMailMessage::MessageType)), this, SLOT(noSendAccount(QMailMessage::MessageType)));
    connect(writeMail, SIGNAL(editAccounts()), this, SLOT(settings()));

    return writeMail;
}

ReadMail* MessageUiBase::createReadMailWidget()
{
    ReadMail* readMail = new ReadMail(this);

    readMail->setObjectName("read-message");

    readMail->setGeometry(geometry());

    connect(readMail, SIGNAL(responseRequested(QMailMessage,QMailMessage::ResponseType)), this, SLOT(respond(QMailMessage,QMailMessage::ResponseType)));
    connect(readMail, SIGNAL(responseRequested(QMailMessagePart::Location,QMailMessage::ResponseType)), this, SLOT(respond(QMailMessagePart::Location,QMailMessage::ResponseType)));
    connect(readMail, SIGNAL(getMailRequested(QMailMessageMetaData)), this, SLOT(getSingleMail(QMailMessageMetaData)));
    connect(readMail, SIGNAL(readReplyRequested(QMailMessageMetaData)), this, SLOT(readReplyRequested(QMailMessageMetaData)));
    connect(readMail, SIGNAL(sendMessageTo(QMailAddress,QMailMessage::MessageType)), this, SLOT(sendMessageTo(QMailAddress,QMailMessage::MessageType)));
    connect(readMail, SIGNAL(viewMessage(QMailMessageId,QMailViewerFactory::PresentationType)), this, SLOT(presentMessage(QMailMessageId,QMailViewerFactory::PresentationType)));
    connect(readMail, SIGNAL(sendMessage(QMailMessage&)), this, SLOT(enqueueMail(QMailMessage&)));
    connect(readMail, SIGNAL(retrieveMessagePortion(QMailMessageMetaData, uint)), this, SLOT(retrieveMessagePortion(QMailMessageMetaData, uint)));
    connect(readMail, SIGNAL(retrieveMessagePart(QMailMessagePart::Location)), this, SLOT(retrieveMessagePart(QMailMessagePart::Location)));
    connect(readMail, SIGNAL(retrieveMessagePartPortion(QMailMessagePart::Location, uint)), this, SLOT(retrieveMessagePartPortion(QMailMessagePart::Location, uint)));
    connect(readMail, SIGNAL(flagMessage(QMailMessageId, quint64, quint64)), this, SLOT(flagMessage(QMailMessageId, quint64, quint64)));

    return readMail;
}

EmailFolderView* MessageUiBase::createFolderView()
{
    EmailFolderView* view = new EmailFolderView(this);

    view->setObjectName("read-email");
    view->setModel(emailFolderModel());

    connect(view, SIGNAL(selected(QMailMessageSet*)), this, SLOT(folderSelected(QMailMessageSet*)));

    return view;
}

MessageListView* MessageUiBase::createMessageListView()
{
    MessageListView* view = new MessageListView(this);

    // Default sort is by descending send timestamp
    view->setSortKey(QMailMessageSortKey::timeStamp(Qt::DescendingOrder));

    connect(view, SIGNAL(clicked(QMailMessageId)), this, SLOT(messageActivated()));
    connect(view, SIGNAL(selectionChanged()), this, SLOT(messageSelectionChanged()));
    connect(view, SIGNAL(rowCountChanged()), this, SLOT(messageSelectionChanged()));
    connect(view, SIGNAL(responseRequested(QMailMessage,QMailMessage::ResponseType)), this, SLOT(respond(QMailMessage,QMailMessage::ResponseType)) );
    connect(view, SIGNAL(moreClicked()), this, SLOT(retrieveMoreMessages()) );
    connect(view, SIGNAL(visibleMessagesChanged()), this, SLOT(retrieveVisibleMessagesFlags()) );
    connect(view, SIGNAL(fullSearchRequested()),this,SLOT(search()));

    return view;
}

EmailFolderModel* MessageUiBase::createEmailFolderModel()
{
    EmailFolderModel* model = new EmailFolderModel(this);
    model->init();
    return model;
}

SearchView* MessageUiBase::createSearchView()
{
    SearchView* searchview = new SearchView(this);
    searchview->setObjectName("searchview");
    return searchview;
}

EmailClient::EmailClient(QWidget *parent, Qt::WindowFlags f)
    : MessageUiBase( parent, f ),
      filesRead(false),
      transferStatus(Inactive),
      primaryActivity(Inactive),
      enableMessageActions(false),
      closeAfterTransmissions(false),
      closeAfterWrite(false),
      fetchTimer(this),
      autoGetMail(false),
      initialAction(None),
      preSearchWidgetId(-1),
      m_messageServerProcess(0),
      syncState(ExportUpdates),
      m_contextMenu(0)
{
    setObjectName( "EmailClient" );

    //start messageserver if it's not running
    if (!isMessageServerRunning() && !startMessageServer())
        qFatal("Unable to start messageserver!");

    //run account setup if we don't have any defined yet
    bool haveAccounts = QMailStore::instance()->countAccounts() > 0;
    if(!haveAccounts)
        settings();

    init();

    setupUi();
}

EmailClient::~EmailClient()
{
    clearNewMessageStatus(messageListView()->key());
    waitForMessageServer();
}


void EmailClient::openFiles()
{
    delayedInit();

    if ( filesRead ) {
        if ( cachedDisplayMailId.isValid() )
            displayCachedMail();

        return;
    }

    filesRead = true;

    QMailMessageKey outboxFilter(QMailMessageKey::status(QMailMessage::Outbox));
    if (QMailStore::instance()->countMessages(outboxFilter)) {
        // There are messages ready to be sent
        QTimer::singleShot( 0, this, SLOT(sendAllQueuedMail()) );
    }

    if ( cachedDisplayMailId.isValid() ) {
        displayCachedMail();
    }
}

void EmailClient::displayCachedMail()
{
    presentMessage(cachedDisplayMailId,QMailViewerFactory::AnyPresentation);
    cachedDisplayMailId = QMailMessageId();
}

void EmailClient::resumeInterruptedComposition()
{
    QSettings mailconf("Trolltech", "qtmail");
    mailconf.beginGroup("restart");

    QVariant var = mailconf.value("lastDraftId");
    if (!var.isNull()) {
        lastDraftId = QMailMessageId(var.toULongLong());
        mailconf.remove("lastDraftId");
    }

    mailconf.endGroup();

    if (lastDraftId.isValid()) {
        if (QMessageBox::information(0,
                                     tr("Incomplete message"),
                                     tr("Messages was previously interrupted while composing a message.\n"
                                        "Do you want to resume composing the message?"),
                                     QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
            QMailMessage message(lastDraftId);
            modify(message);
        }
    }
}

bool EmailClient::startMessageServer()
{
    qMailLog(Messaging) << "Starting messageserver child process...";
    if(m_messageServerProcess) delete m_messageServerProcess;
    m_messageServerProcess = new QProcess(this);
    connect(m_messageServerProcess,SIGNAL(error(QProcess::ProcessError)),
            this,SLOT(messageServerProcessError(QProcess::ProcessError)));

#ifdef Q_OS_WIN
	const QString binary("/messageserver.exe");
#else
	const QString binary("/messageserver");
#endif

	m_messageServerProcess->start(QMail::messageServerPath() + binary);
    return m_messageServerProcess->waitForStarted();
}

bool EmailClient::waitForMessageServer()
{
    if(m_messageServerProcess)
    {
        qMailLog(Messaging) << "Shutting down messageserver child process..";
        bool result = m_messageServerProcess->waitForFinished();
        delete m_messageServerProcess; m_messageServerProcess = 0;
        return result;
    }
    return true;
}

void EmailClient::messageServerProcessError(QProcess::ProcessError e)
{
    QString errorMsg = QString("The Message server child process encountered an error (%1). Qtmail will now exit.").arg(static_cast<int>(e));
    QMessageBox::critical(this,"Message Server",errorMsg);
    qFatal(errorMsg.toLatin1());
}

bool EmailClient::isMessageServerRunning() const
{
    QString lockfile = "messageserver-instance.lock";
    int lockid = QMail::fileLock(lockfile);
	if (lockid == -1)
		return true;

    QMail::fileUnlock(lockid);
    return false;
}

bool EmailClient::cleanExit(bool force)
{
    bool result = true;

    if (isTransmitting()) {
        if (force) {
            qMailLog(Messaging) << "EmailClient::cleanExit: forcing cancel to exit";
            cancelOperation();   //abort all transfer
        }
        result = false;
    }

    saveSettings();
    return result;
}

bool EmailClient::closeImmediately()
{
    if (writeMailWidget()->isVisible() && (writeMailWidget()->hasContent())) {
        // We need to save whatever is currently being worked on
        writeMailWidget()->forcedClosure();

        if (lastDraftId.isValid()) {
            // Store this value to remind the user on next startup
            QSettings mailconf("Trolltech", "qtmail");
            mailconf.beginGroup("restart");
            mailconf.setValue("lastDraftId", lastDraftId.toULongLong() );
            mailconf.endGroup();
        }
    }

    if (isTransmitting()) {
        closeAfterTransmissionsFinished();
        return false;
    }

    return true;
}

void EmailClient::setVisible(bool visible)
{
    if(visible)
    {
        QPoint p(0, 0);
        int extraw = 0, extrah = 0, scrn = 0;
        QWidget* w = 0;
        if (w)
            w = w->window();
        QRect desk;
        if (w) {
            scrn = QApplication::desktop()->screenNumber(w);
        } else if (QApplication::desktop()->isVirtualDesktop()) {
            scrn = QApplication::desktop()->screenNumber(QCursor::pos());
        } else {
            scrn = QApplication::desktop()->screenNumber(this);
        }
        desk = QApplication::desktop()->availableGeometry(scrn);

        QWidgetList list = QApplication::topLevelWidgets();
        for (int i = 0; (extraw == 0 || extrah == 0) && i < list.size(); ++i) {
            QWidget * current = list.at(i);
            if (current->isVisible()) {
                int framew = current->geometry().x() - current->x();
                int frameh = current->geometry().y() - current->y();

                extraw = qMax(extraw, framew);
                extrah = qMax(extrah, frameh);
            }
        }

        // sanity check for decoration frames. With embedding, we
        // might get extraordinary values
        if (extraw == 0 || extrah == 0 || extraw >= 10 || extrah >= 40) {
            extrah = 40;
            extraw = 10;
        }

        p = QPoint(desk.x() + desk.width()/2, desk.y() + desk.height()/2);

        // p = origin of this
        p = QPoint(p.x()-width()/2 - extraw,
                p.y()-height()/2 - extrah);


        if (p.x() + extraw + width() > desk.x() + desk.width())
            p.setX(desk.x() + desk.width() - width() - extraw);
        if (p.x() < desk.x())
            p.setX(desk.x());

        if (p.y() + extrah + height() > desk.y() + desk.height())
            p.setY(desk.y() + desk.height() - height() - extrah);
        if (p.y() < desk.y())
            p.setY(desk.y());

        move(p);
    }
    QMainWindow::setVisible(visible);
}


void EmailClient::closeAfterTransmissionsFinished()
{
    closeAfterWrite = false;
    closeAfterTransmissions = true;
}

void EmailClient::closeApplication()
{
    cleanExit(false);

    // If we're still transmitting, just hide until it completes
    if (isTransmitting())
        hide();
    else
        close();
}

void EmailClient::allWindowsClosed()
{
    closeAfterTransmissionsFinished();
    closeApplication();
}

bool EmailClient::isTransmitting()
{
    return (transferStatus != Inactive);
}

bool EmailClient::isSending()
{
    return (transferStatus & Sending);
}

bool EmailClient::isRetrieving()
{
    return (transferStatus & Retrieving);
}


void EmailClient::initActions()
{
    if (!getMailButton) {
        getMailButton = new QAction( QIcon(":icon/sync"), tr("Get all mail"), this );
        connect(getMailButton, SIGNAL(triggered()), this, SLOT(getAllNewMail()) );
        getMailButton->setWhatsThis( tr("Get new mail from all your accounts.") );
        setActionVisible(getMailButton, false);

        getAccountButton = new QAction( QIcon(":icon/account"), QString(), this );
        connect(getAccountButton, SIGNAL(triggered()), this, SLOT(getAccountMail()) );
        getAccountButton->setWhatsThis( tr("Get new mail from current account.") );
        setActionVisible(getAccountButton, false);

        cancelButton = new QAction( QIcon(":icon/reset"), tr("Cancel transfer"), this );
        connect(cancelButton, SIGNAL(triggered()), this, SLOT(cancelOperation()) );
        cancelButton->setWhatsThis( tr("Abort all transfer of mail.") );
        setActionVisible(cancelButton, false);

        composeButton = new QAction( QIcon(":icon/new"), tr("New"), this );
        connect(composeButton, SIGNAL(triggered()), this, SLOT(composeActivated()) );
        composeButton->setWhatsThis( tr("Write a new message.") );

        searchButton = new QAction( QIcon(":icon/find"), tr("Search"), this );
        connect(searchButton, SIGNAL(triggered()), this, SLOT(search()) );
        searchButton->setWhatsThis( tr("Search for messages in your folders.") );

        synchronizeAction = new QAction( this );
        connect(synchronizeAction, SIGNAL(triggered()), this, SLOT(synchronizeFolder()) );
        synchronizeAction->setWhatsThis( tr("Decide whether messages in this folder should be retrieved.") );
        setActionVisible(synchronizeAction, false);

        settingsAction = new QAction( QIcon(":icon/settings"), tr("Account settings..."), this );
        connect(settingsAction, SIGNAL(triggered()), this, SLOT(settings()));

        emptyTrashAction = new QAction( QIcon(":icon/trash"), tr("Empty trash"), this );
        connect(emptyTrashAction, SIGNAL(triggered()), this, SLOT(emptyTrashFolder()));
        setActionVisible(emptyTrashAction, false);

        moveAction = new QAction( this );
        connect(moveAction, SIGNAL(triggered()), this, SLOT(moveSelectedMessages()));
        setActionVisible(moveAction, false);

        copyAction = new QAction( this );
        connect(copyAction, SIGNAL(triggered()), this, SLOT(copySelectedMessages()));
        setActionVisible(copyAction, false);

        restoreAction = new QAction( this );
        connect(restoreAction, SIGNAL(triggered()), this, SLOT(restoreSelectedMessages()));
        setActionVisible(restoreAction, false);

        selectAllAction = new QAction( tr("Select all"), this );
        connect(selectAllAction, SIGNAL(triggered()), this, SLOT(selectAll()));
        setActionVisible(selectAllAction, false);

        deleteMailAction = new QAction( this );
        deleteMailAction->setIcon( QIcon(":icon/trash") );
        connect(deleteMailAction, SIGNAL(triggered()), this, SLOT(deleteSelectedMessages()));
        setActionVisible(deleteMailAction, false);

        detachThreadAction = new QAction( tr("Detach from thread"), this );
        connect(detachThreadAction, SIGNAL(triggered()), this, SLOT(detachThread()));
        setActionVisible(detachThreadAction, false);

        markAction = new QAction( tr("Mark messages"), this );
        connect(markAction, SIGNAL(triggered()), this, SLOT(markMessages()));
        setActionVisible(markAction, true);

        threadAction = new QAction( tr("Unthread messages"), this );
        connect(threadAction, SIGNAL(triggered()), this, SLOT(threadMessages()));
        setActionVisible(threadAction, true);

        replyAction= new QAction( QIcon(":icon/reply"), tr("Reply"), this );
        connect(replyAction, SIGNAL(triggered()), this, SLOT(replyClicked()));
        replyAction->setWhatsThis( tr("Reply to sender only.  Select Reply all from the menu if you want to reply to all recipients.") );

        replyAllAction = new QAction( QIcon(":icon/replyall"), tr("Reply all"), this );
        connect(replyAllAction, SIGNAL(triggered()), this, SLOT(replyAllClicked()));

        forwardAction = new QAction(QIcon(":icon/forward"),tr("Forward"), this );
        connect(forwardAction, SIGNAL(triggered()), this, SLOT(forwardClicked()));

        QMenu* fileMenu = m_contextMenu;
        fileMenu->addAction( composeButton );
        fileMenu->addAction( getMailButton );
        fileMenu->addAction( getAccountButton );
        fileMenu->addAction( searchButton );
        fileMenu->addAction( cancelButton );
        fileMenu->addAction( emptyTrashAction );
        fileMenu->addAction( settingsAction );
        fileMenu->addSeparator();

        QAction* quitAction = fileMenu->addAction(QIcon(":icon/quit"),"Quit");
        connect(quitAction,SIGNAL(triggered(bool)),
                this,SLOT(quit()));
        connect(fileMenu, SIGNAL(aboutToShow()), this, SLOT(updateActions()));

        QToolBar* toolBar = m_toolBar;
        toolBar->addAction( composeButton );
        toolBar->addAction( getMailButton );
        toolBar->addAction( cancelButton );
        toolBar->addAction( searchButton );
        toolBar->addSeparator();
        toolBar->addAction( settingsAction );
        toolBar->addSeparator();
        toolBar->addAction(replyAction);
        toolBar->addAction(forwardAction);
        toolBar->addSeparator();
        toolBar->addAction(deleteMailAction);

        updateGetMailButton();

        folderView()->addAction( synchronizeAction );
        folderView()->addAction( emptyTrashAction);
        folderView()->setContextMenuPolicy(Qt::ActionsContextMenu);

        messageListView()->addAction(replyAction);
        messageListView()->addAction(replyAllAction);
        messageListView()->addAction(forwardAction);
        messageListView()->addAction(createSeparator());
        messageListView()->addAction( copyAction );
        messageListView()->addAction( moveAction );
        messageListView()->addAction( deleteMailAction );
        messageListView()->addAction( restoreAction );
        messageListView()->addAction(createSeparator());
        messageListView()->addAction( selectAllAction );
        messageListView()->addAction(createSeparator());
        messageListView()->addAction( markAction );
        messageListView()->addAction( threadAction );
        messageListView()->addAction( detachThreadAction );
        messageListView()->setContextMenuPolicy(Qt::ActionsContextMenu);

        readMailWidget()->addAction(replyAction);
        readMailWidget()->addAction(replyAllAction);
        readMailWidget()->addAction(forwardAction);
        readMailWidget()->addAction(createSeparator());
        readMailWidget()->addAction(deleteMailAction);
    }
}

void EmailClient::updateActions()
{
    openFiles();

    // Ensure that the actions have been initialised
    initActions();

    //Enable marking and selectAll actions only if we have messages.
    int messageCount = messageListView()->rowCount();
    setActionVisible(markAction, messageCount > 0);
    setActionVisible(selectAllAction, (messageCount > 1 && messageCount != selectionCount));

    // Only enable empty trash action if the trash has messages in it
    QMailMessageKey typeFilter(QMailMessageKey::messageType(QMailMessage::Email));
    QMailMessageKey trashFilter(QMailMessageKey::status(QMailMessage::Trash));

    messageCount = QMailStore::instance()->countMessages(typeFilter & trashFilter);

    setActionVisible(emptyTrashAction, (messageCount > 0) && !markingMode);
    setActionVisible(threadAction, (messageCount > 0) && !markingMode);

    // Set the visibility for each action to whatever was last configured
    QMap<QAction*, bool>::iterator it = actionVisibility.begin(), end = actionVisibility.end();
    for ( ; it != end; ++it)
        it.key()->setVisible(it.value());
}

void EmailClient::delayedInit()
{
    if (moveAction)
        return; // delayedInit already done

    QMailStore* store = QMailStore::instance();

    // Whenever these actions occur, we need to reload accounts that may have changed
    connect(store, SIGNAL(accountsAdded(QMailAccountIdList)), this, SLOT(accountsAdded(QMailAccountIdList)));
    connect(store, SIGNAL(accountsAdded(QMailAccountIdList)), this, SLOT(updateActions()));
    connect(store, SIGNAL(accountsRemoved(QMailAccountIdList)), this, SLOT(accountsRemoved(QMailAccountIdList)));
    connect(store, SIGNAL(accountsRemoved(QMailAccountIdList)), this, SLOT(updateActions()));
    connect(store, SIGNAL(accountsUpdated(QMailAccountIdList)), this, SLOT(accountsUpdated(QMailAccountIdList)));

    // We need to detect when messages are marked as deleted during downloading
    connect(store, SIGNAL(messagesUpdated(QMailMessageIdList)), this, SLOT(messagesUpdated(QMailMessageIdList)));

    connect(&fetchTimer, SIGNAL(timeout()), this, SLOT(automaticFetch()) );

    // Ideally would make actions functions methods and delay their
    // creation until context menu is shown.
    initActions();
    updateActions();

    QTimer::singleShot(0, this, SLOT(openFiles()) );
}

EmailFolderView* EmailClient::createFolderView()
{
    EmailFolderView* view = MessageUiBase::createFolderView();
    return view;
}

MessageListView* EmailClient::createMessageListView()
{
    MessageListView* view = MessageUiBase::createMessageListView();
    return view;
}

void EmailClient::init()
{
    getMailButton = 0;
    getAccountButton = 0;
    cancelButton = 0;
    composeButton = 0;
    searchButton = 0;
    synchronizeAction = 0;
    settingsAction = 0;
    emptyTrashAction = 0;
    moveAction = 0;
    copyAction = 0;
    restoreAction = 0;
    selectAllAction = 0;
    deleteMailAction = 0;

    // Connect our service action signals
    retrievalAction = new QMailRetrievalAction(this);
    storageAction = new QMailStorageAction(this);
    transmitAction = new QMailTransmitAction(this);

    foreach (QMailServiceAction *action, QList<QMailServiceAction*>() << retrievalAction
                                                                      << storageAction
                                                                      << transmitAction) {
        connect(action, SIGNAL(connectivityChanged(QMailServiceAction::Connectivity)), this, SLOT(connectivityChanged(QMailServiceAction::Connectivity)));
        connect(action, SIGNAL(activityChanged(QMailServiceAction::Activity)), this, SLOT(activityChanged(QMailServiceAction::Activity)));
        connect(action, SIGNAL(statusChanged(QMailServiceAction::Status)), this, SLOT(statusChanged(QMailServiceAction::Status)));
        connect(action, SIGNAL(progressChanged(uint, uint)), this, SLOT(progressChanged(uint, uint)));
    }

    // Use a separate action for flag updates, which are not directed by the user
    flagRetrievalAction = new QMailRetrievalAction(this);
    connect(flagRetrievalAction, SIGNAL(activityChanged(QMailServiceAction::Activity)), this, SLOT(flagRetrievalActivityChanged(QMailServiceAction::Activity)));

    // We need to load the settings in case they affect our service handlers
    readSettings();
}

void EmailClient::cancelOperation()
{
    if ( !cancelButton->isEnabled() )
        return;

    clearStatusText();

    retrievalAccountIds.clear();

    if (isSending()) {
        transmitAction->cancelOperation();
        setSendingInProgress( false );
    }
    if (isRetrieving()) {
        retrievalAction->cancelOperation();
        setRetrievalInProgress( false );
    }

    if (flagRetrievalAction->activity() == QMailServiceAction::InProgress) {
        flagRetrievalAction->cancelOperation();
    }
}

/*  Enqueue mail must always store the mail in the outbox   */
void EmailClient::enqueueMail(QMailMessage& mail)
{
    // Does this account support sending a message by reference from an external sent folder?
    QMailAccount account(mail.parentAccountId());
    if ((account.status() & (QMailAccount::CanReferenceExternalData | QMailAccount::CanTransmitViaReference)) &&
        account.standardFolder(QMailFolder::SentFolder).isValid()) {
        mail.setStatus(QMailMessage::TransmitFromExternal, true);
    }

    // This message should be marked as a draft, and flagged for transmission
    // Don't use flagMessages - we don't want this to be transmitted to a server 
    // as a draft unless transmission fails
    mail.setStatus((QMailMessage::Outbox | QMailMessage::Draft), true);

    bool inserted(false);
    if (!mail.id().isValid()) {
        // This message is present only on the local device until we externalise or send it
        mail.setStatus(QMailMessage::LocalOnly, true);
        inserted = QMailStore::instance()->addMessage(&mail);
    } else {
        inserted = QMailStore::instance()->updateMessage(&mail);
    }

    if (inserted) {
        sendAllQueuedMail(true);

        if (closeAfterWrite) {
            closeAfterTransmissionsFinished();
            closeApplication();
        }
    } else {
        QMailFolder folder(mail.parentFolderId());
        accessError(folder.displayName());
        return;
    }
}

/*  Simple, do nothing  */
void EmailClient::discardMail()
{
    // Reset these in case user chose reply but discarded message
    repliedFromMailId = QMailMessageId();
    repliedFlags = 0;

    if (closeAfterWrite) {
        closeAfterTransmissionsFinished();
        closeApplication();
    }
}

void EmailClient::saveAsDraft(QMailMessage& mail)
{
    // Mark the message as a draft so that it is presented correctly
    mail.setStatus(QMailMessage::Draft, true);

    bool inserted(false);
    if (!mail.id().isValid()) {
        // This message is present only on the local device until we externalise or send it
        mail.setStatus(QMailMessage::LocalOnly, true);
        inserted = QMailStore::instance()->addMessage(&mail);
    } else {
        inserted = QMailStore::instance()->updateMessage(&mail);
    }

    if (inserted) {
        // Inform the responsible service that it is a draft
        storageAction->flagMessages(QMailMessageIdList() << mail.id(), QMailMessage::Draft, 0);

        lastDraftId = mail.id();
    } else {
        QMailFolder folder(mail.parentFolderId());
        accessError(folder.displayName());
    }
}

/*  Mark a message as replied/repliedall/forwarded  */
void EmailClient::mailResponded()
{
    if (repliedFromMailId.isValid()) {
        storageAction->flagMessages(QMailMessageIdList() << repliedFromMailId, repliedFlags, 0);
        repliedFromMailId = QMailMessageId();
        repliedFlags = 0;
    }
}

// each message that belongs to the current found account
void EmailClient::sendAllQueuedMail(bool userRequest)
{
    QMailMessageKey outboxFilter(QMailMessageKey::status(QMailMessage::Outbox));

    if (transmitAccountIds.isEmpty()) {
        // Find which accounts have messages to transmit in the outbox
        foreach (const QMailMessageMetaData &metaData, QMailStore::instance()->messagesMetaData(outboxFilter, QMailMessageKey::ParentAccountId, QMailStore::ReturnDistinct)) {
            transmitAccountIds.append(metaData.parentAccountId());
        }
        if (transmitAccountIds.isEmpty())
            return;
    }

    if (userRequest) {
        // See if the message viewer wants to suppress the 'Sending messages' notification
        QMailMessageIdList outgoingIds = QMailStore::instance()->queryMessages(outboxFilter);
        if (!readMailWidget()->handleOutgoingMessages(outgoingIds)) {
            // Tell the user we're responding
            QString detail;
            if (outgoingIds.count() == 1) {
                QMailMessageMetaData mail(outgoingIds.first());
                detail = mailType(mail.messageType());
            } else {
                detail = tr("%n message(s)", "%1: number of messages", outgoingIds.count());
            }

            AcknowledgmentBox::show(tr("Sending"), tr("Sending:") + " " + detail);
        }
    }

    while (!transmitAccountIds.isEmpty()) {
        QMailAccountId transmitId(transmitAccountIds.first());
        transmitAccountIds.removeFirst();

        if (verifyAccount(transmitId, true)) {
            setSendingInProgress(true);

            transmitAction->transmitMessages(transmitId);
            return;
        } else {
            // Move this account's outbox messages to Drafts
            QMailMessageKey accountFilter(QMailMessageKey::parentAccountId(transmitId));
            QMailMessageIdList unsentIds(QMailStore::instance()->queryMessages(accountFilter & outboxFilter));
            storageAction->flagMessages(unsentIds, QMailMessage::Draft, QMailMessage::Outbox);
        }
    }
}

void EmailClient::flagMessage(const QMailMessageId &id, quint64 setMask, quint64 unsetMask)
{
    storageAction->flagMessages(QMailMessageIdList() << id, setMask, unsetMask);
}

bool EmailClient::verifyAccount(const QMailAccountId &accountId, bool outgoing)
{
    QMailAccount account(accountId);

    if ((outgoing && ((account.status() & QMailAccount::CanTransmit) == 0)) ||
        (!outgoing && ((account.status() & QMailAccount::CanRetrieve) == 0))) {
        QString caption(outgoing ? tr("Cannot transmit") : tr("Cannot retrieve"));
        QString text(tr("Account configuration is incomplete."));
        QMessageBox box(caption, text, QMessageBox::Warning, QMessageBox::Ok | QMessageBox::Default, QMessageBox::NoButton, QMessageBox::NoButton);
        box.exec();
        return false;
    }

    return true;
}

void EmailClient::transmitCompleted()
{
    // If there are more SMTP accounts to service, continue
    if (!transmitAccountIds.isEmpty()) {
        sendAllQueuedMail();
    } else {
        if (primaryActivity == Sending)
            clearStatusText();

        setSendingInProgress(false);

        // If the sent message was a response, we have modified the original message's status
        mailResponded();
    }
}

void EmailClient::retrievalCompleted()
{
    if (mailAccountId.isValid()) {
        if (syncState == ExportUpdates) {
            // Find any changes to the folder list of the server
            syncState = RetrieveFolders;
            retrievalAction->retrieveFolderList(mailAccountId, QMailFolderId());
        } else if (syncState == RetrieveFolders) {
            // Now we need to retrieve the message lists for the folders
            syncState = RetrieveMessages;
            retrievalAction->retrieveMessageList(mailAccountId, QMailFolderId(), MoreMessagesIncrement);
        } else {
            // See if there are more accounts to process
            getNextNewMail();
        }
    } else {
        autoGetMail = false;

        if (primaryActivity == Retrieving)
            clearStatusText();

        setRetrievalInProgress(false);
    }
}

void EmailClient::storageActionCompleted()
{
    clearStatusText();
}

void EmailClient::getNewMail()
{
    // Try to preserve the message list selection
    selectedMessageId = messageListView()->current();
    if (!selectedMessageId.isValid())
        selectedMessageId = QMailMessageId();

    setRetrievalInProgress(true);
    syncState = ExportUpdates;

    retrievalAction->exportUpdates(mailAccountId);
}

void EmailClient::getAllNewMail()
{
    retrievalAccountIds.clear();

    QMailAccountKey retrieveKey(QMailAccountKey::status(QMailAccount::CanRetrieve, QMailDataComparator::Includes));
    QMailAccountKey enabledKey(QMailAccountKey::status(QMailAccount::Enabled, QMailDataComparator::Includes));
    retrievalAccountIds = QMailStore::instance()->queryAccounts(retrieveKey & enabledKey);

    if (!retrievalAccountIds.isEmpty())
        getNextNewMail();
}

void EmailClient::getAccountMail()
{
    retrievalAccountIds.clear();

    if (const QAction* action = static_cast<const QAction*>(sender())) {
        QMailAccountId accountId(action->data().value<QMailAccountId>());
        retrievalAccountIds.append(accountId);
        getNextNewMail();
    }
}

void EmailClient::getSingleMail(const QMailMessageMetaData& message)
{
    if (isRetrieving()) {
        QString msg(tr("Cannot retrieve message because a retrieval operation is currently in progress"));
        QMessageBox::warning(0, tr("Retrieval in progress"), msg, tr("OK") );
        return;
    }

    mailAccountId = message.parentAccountId();
    setRetrievalInProgress(true);

    retrievalAction->retrieveMessages(QMailMessageIdList() << message.id(), QMailRetrievalAction::Content);
}

void EmailClient::retrieveMessagePortion(const QMailMessageMetaData& message, uint bytes)
{
    if (isRetrieving()) {
        QString msg(tr("Cannot retrieve message portion because a retrieval operation is currently in progress"));
        QMessageBox::warning(0, tr("Retrieval in progress"), msg, tr("OK") );
        return;
    }

    mailAccountId = message.parentAccountId();
    setRetrievalInProgress(true);

    QMailMessage msg(message.id());
    retrievalAction->retrieveMessageRange(message.id(), msg.body().length() + bytes);
}

void EmailClient::retrieveMessagePart(const QMailMessagePart::Location &partLocation)
{
    if (isRetrieving()) {
        QString msg(tr("Cannot retrieve message part because a retrieval operation is currently in progress"));
        QMessageBox::warning(0, tr("Retrieval in progress"), msg, tr("OK") );
        return;
    }

    if (!partLocation.isValid(true)) {
        QString msg(tr("Cannot retrieve message part without a valid message ID"));
        QMessageBox::warning(0, tr("Invalid part location"), msg, tr("OK") );
    } else {
        QMailMessageMetaData metaData(partLocation.containingMessageId());

        mailAccountId = metaData.parentAccountId();
        setRetrievalInProgress(true);

        retrievalAction->retrieveMessagePart(partLocation);
    }
}

void EmailClient::retrieveMessagePartPortion(const QMailMessagePart::Location &partLocation, uint bytes)
{
    if (isRetrieving()) {
        QString msg(tr("Cannot retrieve message part portion because a retrieval operation is currently in progress"));
        QMessageBox::warning(0, tr("Retrieval in progress"), msg, tr("OK") );
        return;
    }

    if (!partLocation.isValid(true)) {
        QString msg(tr("Cannot retrieve message part without a valid message ID"));
        QMessageBox::warning(0, tr("Invalid part location"), msg, tr("OK") );
    } else {
        QMailMessage messsage(partLocation.containingMessageId());

        mailAccountId = messsage.parentAccountId();
        setRetrievalInProgress(true);

        const QMailMessagePart &part(messsage.partAt(partLocation));
        retrievalAction->retrieveMessagePartRange(partLocation, part.body().length() + bytes);
    }
}

void EmailClient::readReplyRequested(const QMailMessageMetaData& mail)
{
    if (mail.messageType() == QMailMessage::Mms) {
    }
}

void EmailClient::getNextNewMail()
{
    if (!retrievalAccountIds.isEmpty()) {
        mailAccountId = retrievalAccountIds.takeFirst();
        getNewMail();
    } else {
        // We have processed all accounts
        autoGetMail = false;
        mailAccountId = QMailAccountId();

        if (primaryActivity == Retrieving)
            clearStatusText();

        setRetrievalInProgress(false);
    }
}

void EmailClient::sendFailure(const QMailAccountId &accountId)
{
    setSendingInProgress(false);

    Q_UNUSED(accountId)
}

void EmailClient::receiveFailure(const QMailAccountId &accountId)
{
    setRetrievalInProgress(false);

    autoGetMail = false;

    // Try the next account if we're working through a set of accounts
    if (!retrievalAccountIds.isEmpty())
        getNextNewMail();

    Q_UNUSED(accountId)
}

void EmailClient::transferFailure(const QMailAccountId& accountId, const QString& text, int code)
{
    QString caption, action;
    if (isSending()) {
        caption = tr("Send Failure");
        action = tr("Error sending %1: %2", "%1: message type, %2: error text");
    } else if (isRetrieving()) {
        caption = autoGetMail ? tr("Automatic Fetch Failure") : tr("Retrieve Failure");
        action = tr("Error retrieving %1: %2", "%1: message type, %2: error text");
    }

    if (!action.isEmpty()) {
        if (accountId.isValid()) {
            QMailAccount account(accountId);
            QMailMessage::MessageType type(account.messageType());
            action = action.arg(mailType(type)).arg(text);

            // If we could have multiple accounts, name the relevant one
            if (type == QMailMessage::Email)
                action.prepend(" - ").prepend(account.name());
        } else {
            action = action.arg(tr("message")).arg(text);
        }

        qMailLog(Messaging) << "transferFailure:" << caption << '-' << action;
        if (code != QMailServiceAction::Status::ErrCancel) {
            clearStatusText();
            QMessageBox::warning(0, caption, action, QMessageBox::Ok);
        } else {
            emit updateStatus(tr("Transfer cancelled"));
        }

        if (isSending()) {
            sendFailure(accountId);
        } else {
            receiveFailure(accountId);
        }
    }
}

void EmailClient::storageActionFailure(const QMailAccountId& accountId, const QString& text)
{
    QString caption(tr("Storage Failure"));
    QString action(tr("Unable to perform requested action %1", "%1: error text"));

    if (accountId.isValid()) {
        QMailAccount account(accountId);
        action.prepend(" - ").prepend(account.name());
    }

    clearStatusText();
    QMessageBox::warning(0, caption, action.arg(text), QMessageBox::Ok);
}

QString EmailClient::mailType(QMailMessage::MessageType type)
{
    QString key(QMailComposerFactory::defaultKey(type));
    if (!key.isEmpty())
        return QMailComposerFactory::displayName(key, type);

    return tr("Message");
}

void EmailClient::messageActivated()
{
    QMailMessageId currentId = messageListView()->current();
    if(!currentId.isValid())
        return;

    QMailMessage message(currentId);
    if (message.status() & QMailMessage::Draft) {
        modify(message);
    } else {
        bool hasNext = false;
        bool hasPrevious = false;
        if (readMailWidget()->displayedMessage() != currentId) {
            readMailWidget()->displayMessage(currentId, QMailViewerFactory::AnyPresentation, hasNext, hasPrevious);
        }
    }
}

void EmailClient::searchResultSelected(const QMailMessageId& id)
{
    readMailWidget()->displayMessage(id,QMailViewerFactory::AnyPresentation,false,false);
}

void EmailClient::accessError(const QString &folderName)
{
    QString msg = tr("Cannot access %1. Either there is insufficient space, or another program is accessing the mailbox.").arg(folderName);
    QMessageBox::critical( 0, tr("Access error"), msg );
}

void EmailClient::readSettings()
{
    QSettings mailconf("Trolltech","qtmail");
    mailconf.beginGroup("qtmailglobal");
    mailconf.endGroup();

    mailconf.beginGroup("settings");
    int val = mailconf.value("interval", -1 ).toInt();
    if ( val == -1 ) {
        fetchTimer.stop();
    } else {
        fetchTimer.start( val * 60 * 1000);
    }
    mailconf.endGroup();
}

bool EmailClient::saveSettings()
{
    const int QTMAIL_CONFIG_VERSION = 100;
    QSettings mailconf("Trolltech","qtmail");

    mailconf.beginGroup("qtmailglobal");
    mailconf.remove("");
    mailconf.setValue("version", QTMAIL_CONFIG_VERSION );
    mailconf.endGroup();

    mailconf.beginGroup("qtmailglobal");
    mailconf.endGroup();
    return true;
}

void EmailClient::updateGetMailButton()
{
    bool visible(false);

    // We can get only mail if we're currently inactive
    if (!isTransmitting()) {
        // At least one account must be able to retrieve mail
        QMailAccountKey retrieveKey(QMailAccountKey::status(QMailAccount::CanRetrieve, QMailDataComparator::Includes));
        visible = (QMailStore::instance()->countAccounts(retrieveKey) != 0);
    }

    setActionVisible(getMailButton, visible);

    updateGetAccountButton();
}

void EmailClient::updateGetAccountButton()
{
    // We can get only mail if we're currently inactive
    bool inactive(!isTransmitting());

    if (QMailMessageSet* item = folderView()->currentItem()) {
        QMailAccountId accountId(item->data(EmailFolderModel::ContextualAccountIdRole).value<QMailAccountId>());
        bool accountContext(accountId.isValid());

        // Only show the get mail for account button if there are multiple accounts to retrieve from
        bool multipleMailAccounts = (emailAccounts().count() > 1);
        setActionVisible(getAccountButton, (inactive && accountContext && multipleMailAccounts));
    }
}

void EmailClient::updateAccounts()
{
    queuedAccountIds.clear();
    updateGetMailButton();
}

void EmailClient::deleteSelectedMessages()
{
    QMailMessageIdList deleteList = messageListView()->selected();
    int deleteCount = deleteList.count();
    if (deleteCount == 0)
        return;

    if (isSending()) {
        // Do not delete messages from the outbox folder while we're sending
        QMailMessageKey outboxFilter(QMailMessageKey::status(QMailMessage::Outbox));
        if (QMailStore::instance()->countMessages(QMailMessageKey::id(deleteList) & outboxFilter)) {
            AcknowledgmentBox::show(tr("Cannot delete"), tr("Message transmission is in progress"));
            return;
        }
    }

    // If any of these messages are not yet trash, then we're only moving to trash
    QMailMessageKey idFilter(QMailMessageKey::id(deleteList));
    QMailMessageKey notTrashFilter(QMailMessageKey::status(QMailMessage::Trash, QMailDataComparator::Excludes));

    const bool deleting(QMailStore::instance()->countMessages(idFilter & notTrashFilter) == 0);

    // Tell the user we're doing what they asked for
    QString action;
    QString actionDetails;
    if ( deleting ) {
        QString item(tr("%n message(s)", "%1: number of messages", deleteCount));
        if ( !confirmDelete( this, tr("Delete"), item ) )
            return;

        action = tr("Deleting");
        actionDetails = tr("Deleting %n message(s)", "%1: number of messages", deleteCount);
    } else {
        // Received messages will be removed from SIM on move to Trash
        action = tr("Moving");
        actionDetails = tr("Moving %n message(s) to Trash", "%1: number of messages", deleteCount);
    }

    AcknowledgmentBox::show(action, actionDetails);

    clearNewMessageStatus(QMailMessageKey::id(messageListView()->selected()));

    if (deleting) {
        storageAction->deleteMessages(deleteList);
    } else {
        storageAction->flagMessages(deleteList, QMailMessage::Trash, 0);
    }

    if (markingMode) {
        // After deleting the messages, clear marking mode
        setMarkingMode(false);
    }
}

void EmailClient::moveSelectedMessagesTo(const QMailFolderId &destination)
{
    QMailMessageIdList moveList = messageListView()->selected();
    if (moveList.isEmpty())
        return;

    clearNewMessageStatus(QMailMessageKey::id(moveList));
    storageAction->moveMessages(moveList, destination);

    AcknowledgmentBox::show(tr("Moving"), tr("Moving %n message(s)", "%1: number of messages", moveList.count()));
}

void EmailClient::copySelectedMessagesTo(const QMailFolderId &destination)
{
    QMailMessageIdList copyList = messageListView()->selected();
    if (copyList.isEmpty())
        return;

    clearNewMessageStatus(QMailMessageKey::id(copyList));
    storageAction->copyMessages(copyList, destination);

    AcknowledgmentBox::show(tr("Copying"), tr("Copying %n message(s)", "%1: number of messages", copyList.count()));
}

bool EmailClient::applyToSelectedFolder(void (EmailClient::*function)(const QMailFolderId&))
{
    locationSet.clear();

    // Find the current locations for each of the selected messages
    QMailMessageKey key(QMailMessageKey::id(messageListView()->selected()));
    foreach (const QMailMessageMetaData &message, QMailStore::instance()->messagesMetaData(key, QMailMessageKey::ParentFolderId)) {
        locationSet.insert(message.parentFolderId());
    }

    if (!locationSet.isEmpty()) {
        QSet<QMailAccountId> locationAccountIds;
        foreach (const QMailFolderId &folderId, locationSet) {
            QMailFolder folder(folderId);
            if (folder.parentAccountId().isValid())
                locationAccountIds.insert(folder.parentAccountId());
        }

        QMailFolderIdList list;

        if (locationAccountIds.count() == 1) {
            AccountFolderModel model(*locationAccountIds.begin());
            model.init();

            QList<QMailMessageSet*> invalidItems;
            invalidItems.append(model.itemFromIndex(model.indexFromAccountId(*locationAccountIds.begin())));

            // If the message(s) are in a single location, do not permit that as a destination
            if (locationSet.count() == 1) {
                invalidItems.append(model.itemFromIndex(model.indexFromFolderId(*locationSet.begin())));
            }

            SelectFolderDialog selectFolderDialog(&model);
            selectFolderDialog.setInvalidSelections(invalidItems);
            selectFolderDialog.exec();

            if (selectFolderDialog.result() == QDialog::Accepted) {
                // Apply the function to the selected messages, with the selected folder argument
                (this->*function)(model.folderIdFromIndex(model.indexFromItem(selectFolderDialog.selectedItem())));
                return true;
            }
        } else {
            // TODO:
            AcknowledgmentBox::show(tr("Whoops"), tr("Cannot handle messages from multiple accounts..."));
        }


    }

    return false;
}

void EmailClient::moveSelectedMessages()
{
    QMailMessageIdList moveList = messageListView()->selected();
    if (moveList.isEmpty())
        return;

    if (isSending()) {
        // Do not move outbox messages while we're sending
        QMailMessageKey outboxFilter(QMailMessageKey::status(QMailMessage::Outbox));
        if (QMailStore::instance()->countMessages(QMailMessageKey::id(moveList) & outboxFilter)) {
            AcknowledgmentBox::show(tr("Cannot move"), tr("Message transmission is in progress"));
            return;
        }
    }

    if (applyToSelectedFolder(&EmailClient::moveSelectedMessagesTo)) {
        if (markingMode) {
            // After moving the messages, clear marking mode
            setMarkingMode(false);
        }
    }
}

void EmailClient::copySelectedMessages()
{
    if (applyToSelectedFolder(&EmailClient::copySelectedMessagesTo)) {
        if (markingMode) {
            // After copying the messages, clear marking mode
            setMarkingMode(false);
        }
    }
}

void EmailClient::restoreSelectedMessages()
{
    QMailMessageKey selectedFilter(QMailMessageKey::id(messageListView()->selected()));
    QMailMessageKey trashFilter(QMailMessageKey::status(QMailMessage::Trash));

    // Only messages currently in the trash folder should be restored
    QMailMessageIdList restoreIds = QMailStore::instance()->queryMessages(selectedFilter & trashFilter);
    if (restoreIds.isEmpty())
        return;

    AcknowledgmentBox::show(tr("Restoring"), tr("Restoring %n message(s)", "%1: number of messages", restoreIds.count()));
    storageAction->flagMessages(restoreIds, 0, QMailMessage::Trash);
}

void EmailClient::selectAll()
{
    if (!markingMode) {
        // No point selecting messages unless we're in marking mode
        setMarkingMode(true);
    }

    messageListView()->selectAll();
}

void EmailClient::emptyTrashFolder()
{
    QMailMessageKey trashFilter(EmailStandardFolderMessageSet::contentKey(QMailFolder::TrashFolder));

    QMailMessageIdList trashIds = QMailStore::instance()->queryMessages(trashFilter);
    if (trashIds.isEmpty())
        return;

    if (confirmDelete(this, "Empty trash", tr("all messages in the trash"))) {
        AcknowledgmentBox::show(tr("Deleting"), tr("Deleting %n message(s)", "%1: number of messages", trashIds.count()));
        storageAction->deleteMessages(trashIds);
    }
}

void EmailClient::detachThread()
{
    QMailMessageIdList ids(messageListView()->selected());
    if (ids.count() == 1) {
        QString caption(tr("Detach"));
        QString msg(tr("Are you sure you want to detach this message from its current thread?"));

        if (QMessageBox::warning(this, caption, msg, QMessageBox::Yes, QMessageBox::No|QMessageBox::Default|QMessageBox::Escape, 0) == QMessageBox::Yes) {
            QMailMessageMetaData metaData(ids.first());
            metaData.setInResponseTo(QMailMessageId());

            QMailStore::instance()->updateMessage(&metaData);
        }
    }
}

void EmailClient::setStatusText(QString &txt)
{
    emit updateStatus(txt);
}

void EmailClient::connectivityChanged(QMailServiceAction::Connectivity /*connectivity*/)
{
}

void EmailClient::activityChanged(QMailServiceAction::Activity activity)
{
    if (QMailServiceAction *action = static_cast<QMailServiceAction*>(sender())) {
        if (activity == QMailServiceAction::Successful) {
            if (action == transmitAction) {
                transmitCompleted();
            } else if (action == retrievalAction) {
                retrievalCompleted();
            } else if (action == storageAction) {
                storageActionCompleted();
            }
        } else if (activity == QMailServiceAction::Failed) {
            const QMailServiceAction::Status status(action->status());
            if (action == storageAction) {
                storageActionFailure(status.accountId, status.text);
            } else {
                transferFailure(status.accountId, status.text, status.errorCode);
            }
        }
    }
}

void EmailClient::statusChanged(const QMailServiceAction::Status &status)
{
    if (QMailServiceAction *action = static_cast<QMailServiceAction*>(sender())) {
        // If we have completed, don't show the status info
        if (action->activity() == QMailServiceAction::InProgress) {
            QString text = status.text;
            if (status.accountId.isValid()) {
                QMailAccount account(status.accountId);
                text.prepend(account.name() + " - ");
            }
            emit updateStatus(text);
        }
    }
}

void EmailClient::progressChanged(uint progress, uint total)
{
    emit updateProgress(progress, total);
}

void EmailClient::flagRetrievalActivityChanged(QMailServiceAction::Activity activity)
{
    if (QMailServiceAction *action = static_cast<QMailServiceAction*>(sender())) {
        if (activity == QMailServiceAction::Failed) {
            // Report failure
            const QMailServiceAction::Status status(action->status());
            qMailLog(Messaging) << "Failed to update message flags -" << status.text << "(" << status.errorCode << ")";
        } else if (activity != QMailServiceAction::Successful) {
            return;
        }

        // Are there pending message IDS to be checked?
        if (!flagMessageIds.isEmpty()) {
            flagRetrievalAction->retrieveMessages(flagMessageIds.toList(), QMailRetrievalAction::Flags);
            flagMessageIds.clear();
        }
    }
}

void EmailClient::folderSelected(QMailMessageSet *item)
{
    if (item) {
        contextStatusUpdate();

        bool synchronizeAvailable(false);

        QMailAccountId accountId(item->data(EmailFolderModel::ContextualAccountIdRole).value<QMailAccountId>());
        QMailFolderId folderId(item->data(EmailFolderModel::FolderIdRole).value<QMailFolderId>());

        if (accountId.isValid()) {
            QMailAccount account(accountId);
            getAccountButton->setText(tr("Get mail for %1", "%1:account name").arg(account.name()));
            getAccountButton->setData(accountId);

            // See if this is a folder that can be included/excluded
            if (folderId.isValid()) {
                synchronizeAction->setData(folderId);
                synchronizeAvailable = true;

                if (item->data(EmailFolderModel::FolderSynchronizationEnabledRole).value<bool>())
                    synchronizeAction->setText(tr("Exclude folder"));
                else
                    synchronizeAction->setText(tr("Include folder"));
            }
        }

        setActionVisible(synchronizeAction, synchronizeAvailable);

        updateGetAccountButton();

        bool contentsChanged = (item->messageKey() != messageListView()->key());
        if (contentsChanged)
            messageListView()->setKey(item->messageKey());

        // We need to see if the folder status has changed
        QMailFolder folder;
        if (folderId.isValid()) {
            folder = QMailFolder(folderId);

            // Are there more messages to be retrieved for this folder?
            if ((folder.status() & QMailFolder::PartialContent) == 0) {
                // No more messages to retrieve
                folder = QMailFolder();
            }
        }
        messageListView()->setFolderId(folder.id());
        updateActions();
    }
}

void EmailClient::search()
{
    static bool init = false;
    if(!init)
    {
        connect(searchView(),SIGNAL(searchResultSelected(const QMailMessageId&)),this,SLOT(searchResultSelected(const QMailMessageId&)));
        init = true;
    }
    searchView()->raise();
    searchView()->activateWindow();
    searchView()->reset();
    searchView()->show();
}

void EmailClient::automaticFetch()
{
    if (isRetrieving())
        return;

    // TODO: remove this code - obsoleted by messageserver interval checking

    qWarning("get all new mail automatic");
    autoGetMail = true;
    getAllNewMail();
}

/*  TODO: Is external edit still relevant?

    Someone external are making changes to the mailboxes.  By this time
    we won't know what changes has been made (nor is it feasible to try
    to determine it).  Close all actions which can have become
    invalid due to the external edit.  A writemail window will as such close, but
    the information will be kept in memory (pasted when you reenter the
    writemail window (hopefully the external edit is done by then)
*/
void EmailClient::externalEdit(const QString &mailbox)
{
    cancelOperation();
    QString msg = mailbox + " "; //no tr
    msg += tr("was edited externally");
    emit updateStatus(msg);
}

bool EmailClient::checkMailConflict(const QString& msg1, const QString& msg2)
{
    if ( writeMailWidget()->isVisible()) {
        QString message = tr("<qt>You are currently editing a message:<br>%1</qt>").arg(msg1);
        switch( QMessageBox::warning( 0, tr("Messages conflict"), message,
                                      tr("Yes"), tr("No"), 0, 0, 1 ) ) {

            case 0:
            {
                if ( !writeMailWidget()->saveChangesOnRequest() ) {
                    QMessageBox::warning(0,
                                        tr("Autosave failed"),
                                        tr("<qt>Autosave failed:<br>%1</qt>").arg(msg2));
                    return true;
                }
                break;
            }
            case 1: break;
        }
    }
    return false;
}

void EmailClient::replyClicked()
{
    QMailMessageId currentId = messageListView()->current();
    if(currentId.isValid())
        respond(QMailMessage(currentId),QMailMessage::Reply);
}

void EmailClient::replyAllClicked()
{
    QMailMessageId currentId = messageListView()->current();
    if(currentId.isValid())
        respond(QMailMessage(currentId),QMailMessage::ReplyToAll);
}

void EmailClient::forwardClicked()
{
    QMailMessageId currentId = messageListView()->current();
    if(currentId.isValid())
        respond(QMailMessage(currentId),QMailMessage::Forward);
}

void EmailClient::respond(const QMailMessage& message, QMailMessage::ResponseType type)
{
    if ((type == QMailMessage::NoResponse) ||
        (type == QMailMessage::ForwardPart)) {
        qWarning() << "Invalid responseType:" << type;
        return;
    }

    repliedFromMailId = message.id();

    if (type == QMailMessage::Reply) {
        repliedFlags = QMailMessage::Replied;
    } else if (type == QMailMessage::ReplyToAll) {
        repliedFlags = QMailMessage::RepliedAll;
    } else if (type == QMailMessage::Forward) {
        repliedFlags = QMailMessage::Forwarded;
    }

    writeMailWidget()->respond(message, type);
    if (!writeMailWidget()->composer().isEmpty()) {
        viewComposer();
    }
}

void EmailClient::respond(const QMailMessagePart::Location& partLocation, QMailMessage::ResponseType type)
{
    if (type != QMailMessage::ForwardPart) {
        qWarning() << "Invalid responseType:" << type;
        return;
    }

    repliedFromMailId = partLocation.containingMessageId();

    writeMailWidget()->respond(partLocation, type);
    if (!writeMailWidget()->composer().isEmpty()) {
        viewComposer();
    }
}

void EmailClient::modify(const QMailMessage& message)
{
    // Is this type editable?
    QString key(QMailComposerFactory::defaultKey(message.messageType()));
    if (!key.isEmpty()) {
        writeMailWidget()->modify(message);
        if ( writeMailWidget()->composer().isEmpty() ) {
            // failed to create new composer, maybe due to no email account
            // being present.
            return;
        }
        viewComposer();
    } else {
        QMessageBox::warning(0,
                             tr("Error"),
                             tr("Cannot edit a message of this type."),
                             tr("OK"));
    }
}

void EmailClient::retrieveMoreMessages()
{
    if (isRetrieving()) {
        qWarning() << "retrieveMoreMessages called while retrieval in progress";
        return;
    }

    QMailFolderId folderId(messageListView()->folderId());
    if (folderId.isValid()) {
        QMailFolder folder(folderId);

        // Find how many messages we have requested for this folder
        int retrievedMinimum = folder.customField("qtmail-retrieved-minimum").toInt();
        if (retrievedMinimum == 0) {
            retrievedMinimum = QMailStore::instance()->countMessages(QMailMessageKey::parentFolderId(folderId));
        }

        // Request more messages
        retrievedMinimum += MoreMessagesIncrement;

        setRetrievalInProgress(true);

        retrievalAction->retrieveMessageList(folder.parentAccountId(), folderId, retrievedMinimum);

        folder.setCustomField("qtmail-retrieved-minimum", QString::number(retrievedMinimum));
        QMailStore::instance()->updateFolder(&folder);
    }
}

void EmailClient::retrieveVisibleMessagesFlags()
{
    // This code to detect flag changes is required to address a limitation 
    // of IMAP servers that do not support NOTIFY+CONDSTORE functionality.
    QMailMessageIdList ids(messageListView()->visibleMessagesIds());
    if (ids.isEmpty())
        return;

    // Ensure that we only ask for flag updates for messages that are in account folders
    QMailMessageKey idKey(QMailMessageKey::id(ids));
    QMailFolderKey accountFolderKey(QMailFolderKey::parentAccountId(QMailAccountId(), QMailDataComparator::NotEqual));

    ids = QMailStore::instance()->queryMessages(idKey & QMailMessageKey::parentFolderId(accountFolderKey));
    if (ids.isEmpty())
        return;

    QMailServiceAction::Activity activity(flagRetrievalAction->activity());
    if ((activity == QMailServiceAction::Pending) || (activity == QMailServiceAction::InProgress)) {
        // There is a flag retrieval already ocurring; save these IDs to be checked afterwards
        flagMessageIds += ids.toSet();
    } else {
        flagRetrievalAction->retrieveMessages(ids, QMailRetrievalAction::Flags);
    }
}

void EmailClient::composeActivated()
{
    delayedInit();
    if (writeMailWidget()->prepareComposer(QMailMessage::Email))
        viewComposer();
}

void EmailClient::sendMessageTo(const QMailAddress &address, QMailMessage::MessageType type)
{
    if (type == QMailMessage::AnyType)
        type = QMailMessage::Email;

    // Some address types imply message types
    if (address.isEmailAddress() && ((type != QMailMessage::Email) && (type != QMailMessage::Mms))) {
        type = QMailMessage::Email;
    }

    if (writeMailWidget()->prepareComposer(type)) {
        QMailMessage newMessage;
        newMessage.setTo(QMailAddressList() << address);
        newMessage.setMessageType(type);
        writeMailWidget()->create(newMessage);
        viewComposer();
    }
}

void EmailClient::quit()
{
    if(m_messageServerProcess)
    {
        //we started the messageserver, direct it to shut down
        //before we quit ourselves
        QMailMessageServer server;
        server.shutdown();
        QTimer::singleShot(0,qApp,SLOT(quit()));
    }
    else QApplication::quit();
}

void EmailClient::closeEvent(QCloseEvent *e)
{
    if (closeImmediately())
        quit();
    e->ignore();
}

void EmailClient::setupUi()
{
    QWidget* f = new QWidget(this);
    setCentralWidget(f);
    QVBoxLayout* vb = new QVBoxLayout(f);
    vb->setContentsMargins( 0, 0, 0, 0 );
    vb->setSpacing( 0 );

    QSplitter* horizontalSplitter = new QSplitter(this);
    horizontalSplitter->setOrientation(Qt::Vertical);

    QWidget* messageList = new QWidget(this);
    QVBoxLayout* messageListLayout = new QVBoxLayout(messageList);
    messageListLayout->setSpacing(0);
    messageListLayout->setContentsMargins(0,0,0,0);

    horizontalSplitter->addWidget(messageListView());
    horizontalSplitter->addWidget(readMailWidget());

    QSplitter* verticalSplitter = new QSplitter(this);
    verticalSplitter->addWidget(folderView());
    verticalSplitter->addWidget(horizontalSplitter);

    vb->addWidget( verticalSplitter );

    setGeometry(0,0,defaultWidth,defaultHeight);
    int thirdHeight = height() /3;
    horizontalSplitter->setSizes(QList<int>() << thirdHeight << (height()- thirdHeight));
    int quarterWidth = width() /4;
    verticalSplitter->setSizes(QList<int>() << quarterWidth << (width()- quarterWidth));

    QStatusBar* mainStatusBar = new QStatusBar();
    StatusDisplay* status = new StatusDisplay;
    mainStatusBar->addPermanentWidget(status,0);
    setStatusBar(status);
    connect(this, SIGNAL(statusVisible(bool)),
            status, SLOT(showStatus(bool)) );
    connect(this, SIGNAL(updateStatus(QString)),
            status, SLOT(displayStatus(QString)) );
    connect(this, SIGNAL(updateProgress(uint,uint)),
            status, SLOT(displayProgress(uint,uint)) );
    connect(this, SIGNAL(clearStatus()),
            status, SLOT(clearStatus()) );

    QMenuBar* mainMenuBar = new QMenuBar();
    QMenu* file = mainMenuBar->addMenu("File");
    QMenu* help = mainMenuBar->addMenu("Help");
    QAction* aboutQt = help->addAction("About Qt");
    connect(aboutQt,SIGNAL(triggered()),qApp,SLOT(aboutQt()));

    QWidget* menuWidget = new QWidget(this);
    QHBoxLayout*  menuLayout = new QHBoxLayout(menuWidget);
    menuLayout->setSpacing(0);
    menuLayout->setContentsMargins(0,0,5,0);
    menuLayout->addWidget(mainMenuBar);

    QLabel* statusIcon = new ActivityIcon(QList<const QMailServiceAction*>() << retrievalAction
                                                                             << transmitAction
                                                                             << flagRetrievalAction,this);
    menuLayout->addWidget(statusIcon);
    setMenuWidget(menuWidget);
    m_contextMenu = file;

    m_toolBar = new QToolBar(this);
    addToolBar(m_toolBar);
}

void EmailClient::showEvent(QShowEvent* e)
{
    Q_UNUSED(e);

    clearStatusText();

    closeAfterTransmissions = false;

    // We have been launched and raised by QPE in response to a user request
    userInvocation();

    suspendMailCount = false;

    QTimer::singleShot(0, this, SLOT(delayedInit()) );
}

void EmailClient::userInvocation()
{
    // Since the action list hasn't been created until now, it wasn't given focus
    // before the application was shown.  Give it focus now.
    folderView()->setFocus();

    // See if there is a draft whose composition was interrupted by the Red Key (tm)
    QTimer::singleShot(0, this, SLOT(resumeInterruptedComposition()));
}

void EmailClient::setSendingInProgress(bool set)
{
    readMailWidget()->setSendingInProgress(set);

    if (set) {
        if (!isRetrieving())
            primaryActivity = Sending;
    } else {
        if (primaryActivity == Sending)
            primaryActivity = Inactive;

        // Anything we could not send should move back to the drafts folder
        clearOutboxFolder();
    }

    if (isSending() != set) {
        int newStatus = (set ? transferStatus | Sending : transferStatus & ~Sending);
        transferStatusUpdate(newStatus);
    }
}

void EmailClient::setRetrievalInProgress(bool set)
{
    readMailWidget()->setRetrievalInProgress(set);

    if (set) {
        if (!isSending())
            primaryActivity = Retrieving;
    } else {
        if (primaryActivity == Retrieving)
            primaryActivity = Inactive;
    }

    if (isRetrieving() != set) {
        int newStatus = (set ? transferStatus | Retrieving : transferStatus & ~Retrieving);
        transferStatusUpdate(newStatus);
    }
}

void EmailClient::transferStatusUpdate(int status)
{
    if (status != transferStatus) {
        transferStatus = status;

        if (transferStatus == Inactive) {
            if (closeAfterTransmissions)
                close();
        }

        // UI updates
        setActionVisible(cancelButton, transferStatus != Inactive);
        updateGetMailButton();
        updateActions();
    }
}


void EmailClient::clearOutboxFolder()
{
    QMailMessageKey outboxFilter(QMailMessageKey::status(QMailMessage::Outbox));
    QMailMessageIdList unsentIds(QMailStore::instance()->queryMessages(outboxFilter));

    // Set any unsent messages back to draft, and remove Outbox status
    if (!unsentIds.isEmpty()) {
        storageAction->flagMessages(unsentIds, QMailMessage::Draft, QMailMessage::Outbox);
    }
}

void EmailClient::contextStatusUpdate()
{
    if (isTransmitting())
        return;

    MessageUiBase::contextStatusUpdate();
}

void EmailClient::settings()
{
    clearStatusText();
    contextStatusUpdate();

#ifdef Q_OS_WIN
    const QString binary("/messagingaccounts.exe");
#else
    const QString binary("/messagingaccounts");
#endif

    qMailLog(Messaging) << "Starting messagingaccounts process...";
    QProcess settingsAppProcess(this);
    settingsAppProcess.startDetached(QMail::messageSettingsPath() + binary);
}

void EmailClient::accountsAdded(const QMailAccountIdList&)
{
    updateGetAccountButton();
    updateAccounts();
}

void EmailClient::accountsRemoved(const QMailAccountIdList&)
{
    updateGetAccountButton();
    updateAccounts();
}

void EmailClient::accountsUpdated(const QMailAccountIdList&)
{
    updateGetAccountButton();
    updateAccounts();
}

void EmailClient::messagesUpdated(const QMailMessageIdList& ids)
{
    if (isRetrieving()) {
        if (ids.contains(readMailWidget()->displayedMessage())) {
            QMailMessageMetaData updatedMessage(readMailWidget()->displayedMessage());
            if (updatedMessage.status() & QMailMessage::Removed) {
                // This message has been removed
                QMessageBox::warning(0,
                                    tr("Message deleted"),
                                    tr("Message cannot be downloaded, because it has been deleted from the server."),
                                    QMessageBox::Ok);
            }
        }
    }
}

void EmailClient::messageSelectionChanged()
{
    if (!moveAction)
        return; // initActions hasn't been called yet

    MessageUiBase::messageSelectionChanged();

    const bool messagesSelected(selectionCount != 0);

    // We can delete only if all selected messages are in the Trash folder
    QMailMessageKey selectedFilter(QMailMessageKey::id(messageListView()->selected()));
    QMailMessageKey trashFilter(QMailMessageKey::status(QMailMessage::Trash));

    const int trashCount(QMailStore::instance()->countMessages(selectedFilter & trashFilter));
    const bool deleting(trashCount == selectionCount);

    int count = messageListView()->rowCount();
    if ((count > 0) && (selectionCount > 0)) {
        if (deleting) {
            deleteMailAction->setText(tr("Delete message(s)", "", selectionCount));
        } else {
            deleteMailAction->setText(tr("Move to Trash"));
        }
        moveAction->setText(tr("Move message(s)...", "", selectionCount));
        copyAction->setText(tr("Copy message(s)...", "", selectionCount));
        restoreAction->setText(tr("Restore message(s)", "", selectionCount));
    }

    // Ensure that the per-message actions are hidden, if not usable
    setActionVisible(deleteMailAction, messagesSelected);

    // We cannot move/copy messages in the trash
    setActionVisible(moveAction, (messagesSelected && !(trashCount > 0)));
    setActionVisible(copyAction, (messagesSelected && !(trashCount > 0)));
    setActionVisible(restoreAction, (messagesSelected && (trashCount > 0)));

    if(messageListView()->current().isValid())
    {
        QMailMessage mail(messageListView()->current());
        bool incoming(mail.status() & QMailMessage::Incoming);
        bool downloaded(mail.status() & QMailMessage::ContentAvailable);
        bool system(mail.messageType() == QMailMessage::System);

        if (!downloaded || system) {
            // We can't really forward/reply/reply-to-all without the message content
            setActionVisible(replyAction,false);
            setActionVisible(replyAllAction,false);
            setActionVisible(forwardAction,false);
        } else {
            bool otherReplyTarget(!mail.cc().isEmpty() || mail.to().count() > 1);
            setActionVisible(replyAction,incoming);
            setActionVisible(replyAllAction,incoming & otherReplyTarget);
            setActionVisible(forwardAction,true);
        }
    }

    // We can detach only if a single non-root message is selected
    setActionVisible(detachThreadAction, ((selectionCount == 1) && messageListView()->hasParent()));

    updateActions();
}

void EmailClient::noSendAccount(QMailMessage::MessageType type)
{
    QString key(QMailComposerFactory::defaultKey(type));
    QString name(QMailComposerFactory::name(key, type));

    QMessageBox::warning(0,
                         tr("Send Error"),
                         tr("%1 cannot be sent, because no account has been configured to send with.","%1=MMS/Email/TextMessage").arg(name),
                         QMessageBox::Ok);
}

void EmailClient::setActionVisible(QAction* action, bool visible)
{
    if (action)
        actionVisibility[action] = visible;
}

QMailFolderId EmailClient::containingFolder(const QMailMessageId& id)
{
    QMailMessageMetaData metaData(id);
    return metaData.parentFolderId();
}

QMailAccountIdList EmailClient::emailAccounts() const
{
    QMailAccountKey typeKey(QMailAccountKey::messageType(QMailMessage::Email));
    QMailAccountKey enabledKey(QMailAccountKey::status(QMailAccount::Enabled, QMailDataComparator::Includes));
    return QMailStore::instance()->queryAccounts(typeKey & enabledKey);
}

void EmailClient::clearNewMessageStatus(const QMailMessageKey& key)
{
    QMailMessageKey clearNewKey = key & QMailMessageKey::status(QMailMessage::New, QMailDataComparator::Includes);

    int count = QMailStore::instance()->countMessages(clearNewKey);
    if (count) {
        QMailStore::instance()->updateMessagesMetaData(clearNewKey, QMailMessage::New, false);
    }
}

QAction* EmailClient::createSeparator()
{
    QAction* sep = new QAction(this);
    sep->setSeparator(true);
    return sep;
}

void EmailClient::setMarkingMode(bool set)
{
    MessageUiBase::setMarkingMode(set);

    if (markingMode) {
        markAction->setText(tr("Cancel"));
    } else {
        markAction->setText(tr("Mark messages"));
    }
}

void EmailClient::markMessages()
{
    setMarkingMode(!markingMode);
}

void EmailClient::setThreaded(bool set)
{
    MessageUiBase::setThreaded(set);

    if (threaded) {
        threadAction->setText(tr("Unthread messages"));
    } else {
        threadAction->setText(tr("Thread messages"));
    }
}

void EmailClient::threadMessages()
{
    setThreaded(!threaded);
}

void EmailClient::synchronizeFolder()
{
    if (const QAction* action = static_cast<const QAction*>(sender())) {
        QMailFolderId folderId(action->data().value<QMailFolderId>());

        if (folderId.isValid()) {
            QMailFolder folder(folderId);
            bool excludeFolder = (folder.status() & QMailFolder::SynchronizationEnabled);

            if (QMailStore *store = QMailStore::instance()) {
                if (excludeFolder) {
                    // Delete any messages which are in this folder or its sub-folders
                    QMailMessageKey messageKey(QMailMessageKey::parentFolderId(folderId, QMailDataComparator::Equal));
                    QMailMessageKey descendantKey(QMailMessageKey::ancestorFolderIds(folderId, QMailDataComparator::Includes));
                    store->removeMessages(messageKey | descendantKey, QMailStore::NoRemovalRecord);
                }

                // Find any subfolders of this folder
                QMailFolderKey subfolderKey(QMailFolderKey::ancestorFolderIds(folderId, QMailDataComparator::Includes));
                QMailFolderIdList folderIds = QMailStore::instance()->queryFolders(subfolderKey);

                // Mark all of these folders as {un}synchronized
                folderIds.append(folderId);
                foreach (const QMailFolderId &id, folderIds) {
                    QMailFolder folder(id);
                    folder.setStatus(QMailFolder::SynchronizationEnabled, !excludeFolder);
                    store->updateFolder(&folder);
                }
            }

            // Update the action to reflect the change
            folderSelected(folderView()->currentItem());
        }
    }
}

#include <emailclient.moc>


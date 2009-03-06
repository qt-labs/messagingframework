/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#include "emailclient.h"
#include "selectfolder.h"
#include "messagefolder.h"
#include "messagestore.h"
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
#include "qtmailwindow.h"
#include "searchview.h"
#include "readmail.h"
#include "writemail.h"
#include <QMenuBar>
#include <qmailnamespace.h>
#include <QSplitter>
#include <QListView>

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
    ~AcknowledgmentBox();

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

    //QSoftMenuBar::setLabel(this, Qt::Key_Back, QSoftMenuBar::Cancel);

    QDialog::show();

    QTimer::singleShot(_timeout, this, SLOT(accept()));
}

AcknowledgmentBox::~AcknowledgmentBox()
{
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

MessageUiBase::MessageUiBase(QWidget* parent)
    : QWidget(parent),
      appTitle(tr("Messages")),
      suspendMailCount(true),
      markingMode(false),
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

void MessageUiBase::viewComposer(const QString& title)
{
    writeMailWidget()->setWindowTitle(title);
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

MessageStore* MessageUiBase::messageStore() const
{
    static MessageStore* list = const_cast<MessageUiBase*>(this)->createMessageStore();
    return list;
}

EmailFolderModel* MessageUiBase::emailFolderModel() const
{
    static EmailFolderModel* model = const_cast<MessageUiBase*>(this)->createEmailFolderModel();
    return model;
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
    writeMail->setGeometry(0,0,400,400);
    writeMail->setObjectName("write-mail");

    connect(writeMail, SIGNAL(enqueueMail(QMailMessage)), this, SLOT(enqueueMail(QMailMessage)));
    connect(writeMail, SIGNAL(discardMail()), this, SLOT(discardMail()));
    connect(writeMail, SIGNAL(saveAsDraft(QMailMessage)), this, SLOT(saveAsDraft(QMailMessage)));
    connect(writeMail, SIGNAL(noSendAccount(QMailMessage::MessageType)), this, SLOT(noSendAccount(QMailMessage::MessageType)));
    connect(writeMail, SIGNAL(editAccounts()), this, SLOT(settings()));

    return writeMail;
}

ReadMail* MessageUiBase::createReadMailWidget()
{
    ReadMail* readMail = new ReadMail(this);

    readMail->setObjectName("read-message");

    readMail->setGeometry(geometry());

    connect(readMail, SIGNAL(resendRequested(QMailMessage,int)), this, SLOT(resend(QMailMessage,int)) );
    connect(readMail, SIGNAL(modifyRequested(QMailMessage)),this, SLOT(modify(QMailMessage)));
    connect(readMail, SIGNAL(removeMessage(QMailMessageId, bool)), this, SLOT(removeMessage(QMailMessageId, bool)) );
    connect(readMail, SIGNAL(getMailRequested(QMailMessageMetaData)),this, SLOT(getSingleMail(QMailMessageMetaData)) );
    connect(readMail, SIGNAL(sendMailRequested(QMailMessageMetaData&)),this, SLOT(sendSingleMail(QMailMessageMetaData&)));
    connect(readMail, SIGNAL(readReplyRequested(QMailMessageMetaData)),this, SLOT(readReplyRequested(QMailMessageMetaData)));
    connect(readMail, SIGNAL(sendMessageTo(QMailAddress,QMailMessage::MessageType)), this, SLOT(sendMessageTo(QMailAddress,QMailMessage::MessageType)) );
    connect(readMail, SIGNAL(viewMessage(QMailMessageId,QMailViewerFactory::PresentationType)), this, SLOT(presentMessage(QMailMessageId,QMailViewerFactory::PresentationType)) );
    connect(readMail, SIGNAL(sendMessage(QMailMessage)), this, SLOT(enqueueMail(QMailMessage)) );
    connect(readMail, SIGNAL(retrieveMessagePart(QMailMessagePart::Location)),this, SLOT(retrieveMessagePart(QMailMessagePart::Location)) );

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
    connect(view, SIGNAL(selectionChanged()), this, SLOT(messageSelectionChanged()) );
    connect(view, SIGNAL(resendRequested(QMailMessage,int)), this, SLOT(resend(QMailMessage,int)) );
    connect(view, SIGNAL(moreClicked()), this, SLOT(retrieveMoreMessages()) );

    return view;
}

MessageStore* MessageUiBase::createMessageStore()
{
    MessageStore* list = new MessageStore(this);

    connect(list, SIGNAL(stringStatus(QString&)), this, SLOT(setStatusText(QString&)) );
    connect(list, SIGNAL(externalEdit(QString)), this,SLOT(externalEdit(QString)) );

    return list;
}

EmailFolderModel* MessageUiBase::createEmailFolderModel()
{
    EmailFolderModel* model = new EmailFolderModel(this);
    return model;
}

class SleepFor : public QThread
{
public:
    SleepFor(uint msecs)
        : QThread()
    {
        msleep(msecs);
    }
};

EmailClient::EmailClient( QWidget* parent )
    : MessageUiBase( parent ),
      filesRead(false),
      transferStatus(Inactive),
      primaryActivity(Inactive),
      enableMessageActions(false),
      closeAfterTransmissions(false),
      closeAfterWrite(false),
      fetchTimer(this),
      autoGetMail(false),
      initialAction(None),
      searchView(0),
      preSearchWidgetId(-1),
      searchAction(0),
      m_messageServerProcess(0),
      retrievingFolders(false)
{
    setObjectName( "EmailClient" );

    if (!isMessageServerRunning() && !startMessageServer()) {
        qFatal("Unable to start messageserver!");
    } else {
        QTime start(QTime::currentTime());
        int wait = 0;

        // We need to wait until the mail store is initialized
        QMailStore* store = QMailStore::instance();
        while (!store->initialized()) {
            if (start.secsTo(QTime::currentTime()) > 5) {
                // The mailstore isn't working - abort
                qFatal("QMF database failed to initialize");
            } else {
                if (++wait == 5) {
                    wait = 0;
                    qWarning() << "Waiting for mail store initialization...";
                }
                SleepFor(200);
            }
        }
    }

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

    messageStore()->openMailboxes();

    MessageFolder* outbox = messageStore()->mailbox(QMailFolder::OutboxFolder);
    if (outbox->messageCount(MessageFolder::All)) {
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
    qDebug() << "Starting messageserver child process...";
    if(m_messageServerProcess) delete m_messageServerProcess;
    m_messageServerProcess = new QProcess(this);
    connect(m_messageServerProcess,SIGNAL(error(QProcess::ProcessError)),
            this,SLOT(messageServerProcessError(QProcess::ProcessError)));
    m_messageServerProcess->start(QApplication::applicationDirPath() + "/messageserver");
    return m_messageServerProcess->waitForStarted();
}

bool EmailClient::waitForMessageServer()
{
    if(m_messageServerProcess)
    {
        qDebug() << "Shutting down messageserver child process..";
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
    QMail::fileUnlock(lockid);
    return (lockid == -1);
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

void EmailClient::closeAfterTransmissionsFinished()
{
    closeAfterWrite = false;
    closeAfterTransmissions = true;
}

void EmailClient::closeApplication()
{
    cleanExit(false);

    // If we're still transmitting, just hide until it completes
    if (isTransmitting()) {
        QTMailWindow::singleton()->hide();
    } else {
        QTMailWindow::singleton()->close();
    }
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
        getMailButton = new QAction( QIcon(":icon/getmail"), tr("Get all mail"), this );
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
        searchButton->setEnabled(false);

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

        markAction = new QAction( tr("Mark messages"), this );
        connect(markAction, SIGNAL(triggered()), this, SLOT(markMessages()));
        setActionVisible(markAction, true);

    QTMailWindow* mw = QTMailWindow::singleton();
        QMenu* fileMenu = mw->contextMenu();
        fileMenu->addAction( composeButton );
        fileMenu->addAction( getMailButton );
        fileMenu->addAction( getAccountButton );
        fileMenu->addAction( searchButton );
        fileMenu->addAction( cancelButton );
        fileMenu->addAction( emptyTrashAction );
        fileMenu->addAction( settingsAction );
        fileMenu->addSeparator();

        QAction* quitAction = fileMenu->addAction("Quit");
        connect(quitAction,SIGNAL(triggered(bool)),
                this,SLOT(quit()));
        connect(fileMenu, SIGNAL(aboutToShow()), this, SLOT(updateActions()));

        updateGetMailButton();

        folderView()->addAction( synchronizeAction );
        folderView()->addAction( emptyTrashAction);
        folderView()->setContextMenuPolicy(Qt::ActionsContextMenu);

        messageListView()->addAction( deleteMailAction );
        messageListView()->addAction( moveAction );
        messageListView()->addAction( copyAction );
        messageListView()->addAction( restoreAction );
        messageListView()->addAction( selectAllAction );
        messageListView()->addAction( markAction );
        messageListView()->setContextMenuPolicy(Qt::ActionsContextMenu);
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
    QMailMessage::MessageType type = QMailMessage::Email;

    static MessageFolder const* trashFolder = messageStore()->mailbox(QMailFolder::TrashFolder);

    messageCount = trashFolder->messageCount(MessageFolder::All, type);

    setActionVisible(emptyTrashAction, (messageCount > 0) && !markingMode);

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
    connect(store, SIGNAL(accountsRemoved(QMailAccountIdList)), this, SLOT(accountsRemoved(QMailAccountIdList)));
    connect(store, SIGNAL(accountsUpdated(QMailAccountIdList)), this, SLOT(accountsUpdated(QMailAccountIdList)));

    // We need to detect when messages are marked as deleted during downloading
    connect(store, SIGNAL(messagesUpdated(QMailMessageIdList)), this, SLOT(messagesUpdated(QMailMessageIdList)));

    connect(&fetchTimer, SIGNAL(timeout()), this, SLOT(automaticFetch()) );

    // Ideally would make actions functions methods and delay their
    // creation until context menu is shown.
    initActions();

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
    searchAction = new QMailSearchAction(this);

    foreach (QMailServiceAction *action, QList<QMailServiceAction*>() << retrievalAction
                                                                      << storageAction
                                                                      << transmitAction
                                                                      << searchAction) {
        connect(action, SIGNAL(connectivityChanged(QMailServiceAction::Connectivity)), this, SLOT(connectivityChanged(QMailServiceAction::Connectivity)));
        connect(action, SIGNAL(activityChanged(QMailServiceAction::Activity)), this, SLOT(activityChanged(QMailServiceAction::Activity)));
        connect(action, SIGNAL(statusChanged(QMailServiceAction::Status)), this, SLOT(statusChanged(QMailServiceAction::Status)));
        if (action != searchAction)
            connect(action, SIGNAL(progressChanged(uint, uint)), this, SLOT(progressChanged(uint, uint)));
    }


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
}

/*  Enqueue mail must always store the mail in the outbox   */
void EmailClient::enqueueMail(const QMailMessage& mailIn)
{
    static MessageFolder* const outboxFolder = messageStore()->mailbox(QMailFolder::OutboxFolder);

    QMailMessage mail(mailIn);

    // if uuid is not valid , it's a new mail
    bool isNew = !mail.id().isValid();
    if ( isNew ) {
        mailResponded();
    }

    if ( !outboxFolder->insertMessage(mail) ) {
        accessError(*outboxFolder);
        return;
    }

    sendAllQueuedMail(true);

    if (closeAfterWrite) {
        closeAfterTransmissionsFinished();
        closeApplication();
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

void EmailClient::saveAsDraft(const QMailMessage& mailIn)
{
    static MessageFolder* const draftsFolder = messageStore()->mailbox(QMailFolder::DraftsFolder);

    QMailMessage mail(mailIn);

    // if uuid is not valid, it's a new mail
    bool isNew = !mail.id().isValid();
    if ( isNew ) {
        mailResponded();
    }

    if ( !draftsFolder->insertMessage(mail) ) {
        accessError(*draftsFolder);
    } else {
        lastDraftId = mail.id();
    }
}

/*  Mark a message as replied/repliedall/forwarded  */
void EmailClient::mailResponded()
{
    if ( repliedFromMailId.isValid() ) {
        QMailMessageMetaData replyMail(repliedFromMailId);
        replyMail.setStatus(replyMail.status() | repliedFlags);
        QMailStore::instance()->updateMessage(&replyMail);
    }

    repliedFromMailId = QMailMessageId();
    repliedFlags = 0;
}

// send all messages in outbox, by looping through the outbox, sending
// each message that belongs to the current found account
void EmailClient::sendAllQueuedMail(bool userRequest)
{
    static MessageFolder* const outboxFolder = messageStore()->mailbox(QMailFolder::OutboxFolder);
    static MessageFolder* const draftsFolder = messageStore()->mailbox(QMailFolder::DraftsFolder);

    if (transmitAccountIds.isEmpty()) {
        // Find which accounts have messages to transmit in the outbox
        QMailMessageKey key(QMailMessageKey::parentFolderId(outboxFolder->id()));
        foreach (const QMailMessageMetaData &metaData, QMailStore::instance()->messagesMetaData(key, QMailMessageKey::ParentAccountId, QMailStore::ReturnDistinct)) {
            transmitAccountIds.append(metaData.parentAccountId());
        }
        if (transmitAccountIds.isEmpty())
            return;
    }

    if (userRequest) {
        // See if the message viewer wants to suppress the 'Sending messages' notification
        QMailMessageIdList outgoingIds = outboxFolder->messages();
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
            QMailMessageIdList messageIds = QMailStore::instance()->queryMessages(QMailMessageKey::parentAccountId(transmitId) &
                                                                                  QMailMessageKey::parentFolderId(outboxFolder->id()));
            storageAction->moveMessages(messageIds, draftsFolder->id());
        }
    }
}

void EmailClient::sendSingleMail(QMailMessageMetaData& message)
{
    static MessageFolder* const outboxFolder = messageStore()->mailbox(QMailFolder::OutboxFolder);

    if (isSending()) {
        qWarning("sending in progress, no action performed");
        return;
    }

    // Ensure the message is in the Outbox
    if (message.parentFolderId() != outboxFolder->id()) {
        if (!outboxFolder->insertMessage(message)) {
            accessError(*outboxFolder);
            return;
        }
    }

    if (!message.parentAccountId().isValid() || !verifyAccount(message.parentAccountId(), true)) {
        qWarning("Mail requires valid account but none available.");
        clearOutboxFolder();
        return;
    }

    transmitAccountIds.clear();
    setSendingInProgress(true);

    transmitAction->transmitMessages(message.parentAccountId());
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
    }
}

void EmailClient::retrievalCompleted()
{
    if (mailAccountId.isValid()) {
        if (retrievingFolders) {
            retrievingFolders = false;

            // Now we need to retrieve the message lists for the folders
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

void EmailClient::searchCompleted()
{
    clearStatusText();
    viewSearchResults(QMailMessageKey::id(searchAction->matchingMessageIds()));
    searchProgressDialog()->hide();
}

void EmailClient::getNewMail()
{
    // Try to preserve the message list selection
    selectedMessageId = messageListView()->current();
    if (!selectedMessageId.isValid())
        selectedMessageId = QMailMessageId();

    setRetrievalInProgress(true);
    retrievingFolders = true;

    retrievalAction->retrieveFolderList(mailAccountId, QMailFolderId(), true);
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
    static const QMailFolderId draftsFolderId = messageStore()->mailbox(QMailFolder::DraftsFolder)->mailFolder().id();

    QMailMessageId currentId = messageListView()->current();
    if(!currentId.isValid())
        return;

    MessageFolder* source(containingFolder(currentId));
    if (source && (source->id() == draftsFolderId)) {
        QMailMessage message(currentId);
        modify(message);
    } else {

    bool hasNext = false;
    bool hasPrevious = false;
    if (readMailWidget()->displayedMessage() != currentId)
        readMailWidget()->displayMessage(currentId, QMailViewerFactory::AnyPresentation, hasNext, hasPrevious);

        //viewMessage(currentId, true);
    }
}

void EmailClient::accessError(const MessageFolder &box)
{
    QString msg = tr("<qt>Cannot access %1. Either there is insufficient space, or another program is accessing the mailbox.</qt>").arg(box.mailbox());

    QMessageBox::critical( 0, tr("Save error"), msg );
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

bool EmailClient::restoreMessages(const QMailMessageIdList& ids, MessageFolder*)
{
    QMailStore::instance()->restoreToPreviousFolder(QMailMessageKey::id(ids));
    return true;
}

void EmailClient::deleteSelectedMessages()
{
    static MessageFolder* const outboxFolder = messageStore()->mailbox( QMailFolder::OutboxFolder );
    static MessageFolder* const trashFolder = messageStore()->mailbox( QMailFolder::TrashFolder );

    // Do not delete messages from the outbox folder while we're sending
    if (locationSet.contains(outboxFolder->id()) && isSending())
        return;

    QMailMessageIdList deleteList = messageListView()->selected();
    int deleteCount = deleteList.count();
    if (deleteCount == 0)
        return;

    const bool deleting((locationSet.count() == 1) && (*locationSet.begin() == trashFolder->id()));

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

    if (deleting)
        storageAction->deleteMessages(deleteList);
    else
        storageAction->moveMessages(deleteList, trashFolder->id());

    if (markingMode) {
        // After deleting the messages, clear marking mode
        setMarkingMode(false);
    }
}

void EmailClient::moveSelectedMessagesTo(MessageFolder* destination)
{
    QMailMessageIdList moveList = messageListView()->selected();
    if (moveList.isEmpty())
        return;

    clearNewMessageStatus(QMailMessageKey::id(moveList));
    storageAction->moveMessages(moveList, destination->id());

    AcknowledgmentBox::show(tr("Moving"), tr("Moving %n message(s)", "%1: number of messages", moveList.count()));
}

void EmailClient::copySelectedMessagesTo(MessageFolder* destination)
{
    QMailMessageIdList copyList = messageListView()->selected();
    if (copyList.isEmpty())
        return;

    clearNewMessageStatus(QMailMessageKey::id(copyList));
    storageAction->copyMessages(copyList, destination->id());

    AcknowledgmentBox::show(tr("Copying"), tr("Copying %n message(s)", "%1: number of messages", copyList.count()));
}

bool EmailClient::applyToSelectedFolder(void (EmailClient::*function)(MessageFolder*))
{
    static MessageFolder* const outboxFolder = messageStore()->mailbox( QMailFolder::OutboxFolder );

    if (!locationSet.isEmpty()) {
        QSet<QMailAccountId> locationAccountIds;
        foreach (const QMailFolderId &folderId, locationSet) {
            QMailFolder folder(folderId);
            if (folder.parentAccountId().isValid())
                locationAccountIds.insert(folder.parentAccountId());
        }

        QMailFolderIdList list = messageStore()->standardFolders();
        if (locationAccountIds.count() == 1) {
            // Add folders for the account
            QMailFolderKey accountFoldersKey(QMailFolderKey::parentAccountId(*locationAccountIds.begin()));
            list += QMailStore::instance()->queryFolders(accountFoldersKey, QMailFolderSortKey::id());
        }

        // If the message(s) are in a single location, do not permit that as a destination
        if (locationSet.count() == 1) {
            QMailFolder folder(*locationSet.begin());
            list.removeAll(folder.id());
        }

        // Also, do not permit messages to be copied/moved to the Outbox manually
        list.removeAll(outboxFolder->id());

        SelectFolderDialog selectFolderDialog(list);
        selectFolderDialog.exec();

        if (selectFolderDialog.result() == QDialog::Accepted) {
            // Apply the function to the selected messages, with the selected folder argument
            MessageFolder *folder = messageStore()->mailbox(selectFolderDialog.selectedFolderId());
            if (folder) {
                (this->*function)(folder);
            } else {
                MessageFolder accountFolder(selectFolderDialog.selectedFolderId());
                (this->*function)(&accountFolder);
            }
            return true;
        }
    }

    return false;
}

void EmailClient::moveSelectedMessages()
{
    static MessageFolder* const outboxFolder = messageStore()->mailbox( QMailFolder::OutboxFolder );

    // Do not move messages from the outbox folder while we're sending
    if (locationSet.contains(outboxFolder->id()) && isSending())
        return;

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
    static const QMailFolderId trashFolderId = messageStore()->mailbox(QMailFolder::TrashFolder)->id();

    QMailMessageIdList restoreList;
    foreach (const QMailMessageId &id, messageListView()->selected()) {
        // Only messages currently in the trash folder should be restored
        QMailMessageMetaData message(id);
        if (message.parentFolderId() == trashFolderId)
            restoreList.append(id);
    }

    if (restoreList.isEmpty())
        return;

    AcknowledgmentBox::show(tr("Restoring"), tr("Restoring %n message(s)", "%1: number of messages", restoreList.count()));
    QMailStore::instance()->restoreToPreviousFolder(QMailMessageKey::id(restoreList));
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
    QMailMessage::MessageType type = QMailMessage::Email;

    static MessageFolder* const trashFolder = messageStore()->mailbox(QMailFolder::TrashFolder);

    QMailMessageIdList trashIds = trashFolder->messages(type);
    if (trashIds.isEmpty())
        return;

    if (confirmDelete(this, "Empty trash", tr("all messages in the trash"))) {
        AcknowledgmentBox::show(tr("Deleting"), tr("Deleting %n message(s)", "%1: number of messages", trashIds.count()));
        storageAction->deleteMessages(trashIds);
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
            } else if (action == searchAction) {
                searchCompleted();
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
    if (!searchView) {
        searchView = new SearchView(this);
        searchView->setObjectName("search"); // No tr
        connect(searchView, SIGNAL(accepted()), this, SLOT(searchRequested()));
    }

    searchView->reset();
    searchView->exec();
}

void EmailClient::searchRequested()
{
    QString bodyText(searchView->bodyText());

    QMailMessageKey searchKey(searchView->searchKey());
    QMailMessageKey emailKey(QMailMessageKey::messageType(QMailMessage::Email, QMailDataComparator::Includes));
    searchKey &= emailKey;
    searchProgressDialog()->show();
    searchAction->searchMessages(searchKey, bodyText, QMailSearchAction::Local);
    emit updateStatus(tr("Searching"));
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

void EmailClient::resend(const QMailMessage& message, int replyType)
{
    repliedFromMailId = message.id();

    if (replyType == ReadMail::Reply) {
        repliedFlags = QMailMessage::Replied;
    } else if (replyType == ReadMail::ReplyToAll) {
        repliedFlags = QMailMessage::RepliedAll;
    } else if (replyType == ReadMail::Forward) {
        repliedFlags = QMailMessage::Forwarded;
    } else {
        return;
    }

    writeMailWidget()->reply(message, replyType);

    if ( writeMailWidget()->composer().isEmpty() ) {
        // failed to create new composer, maybe due to no email account
        // being present.
        return;
    }
    writeMailWidget()->show();
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


void EmailClient::composeActivated()
{
    delayedInit();
    if(writeMailWidget()->prepareComposer())
        writeMailWidget()->show();
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
        writeMailWidget()->setRecipient(address.address());
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

bool EmailClient::removeMessage(const QMailMessageId& id, bool userRequest)
{
    Q_UNUSED(userRequest);
    static const QMailFolderId outboxFolderId = messageStore()->mailbox(QMailFolder::OutboxFolder)->id();
    static MessageFolder* const trashFolder = messageStore()->mailbox(QMailFolder::TrashFolder);

    MessageFolder* source(containingFolder(id));
    if (isSending()) {
        // Don't delete from Outbox when sending
        if (source && (source->id() == outboxFolderId))
            return false;
    }

    QMailMessageMetaData message(id);

    bool deleting(false);
    QString type = mailType(message.messageType());
    if (!deleting) {
        if (source && (source->id() == trashFolder->id())) {
            if (!confirmDelete( this, "Delete", type ))
                return false;

            deleting = true;
        }
    }

    if ( deleting ) {
        AcknowledgmentBox::show(tr("Deleting"), tr("Deleting: %1","%1=Email/Message/MMS").arg(type));
        storageAction->deleteMessages(QMailMessageIdList() << id);
    } else {
        AcknowledgmentBox::show(tr("Moving"), tr("Moving to Trash: %1", "%1=Email/Message/MMS").arg(type));
        storageAction->moveMessages(QMailMessageIdList() << id, trashFolder->id());
    }

    return true;
}

void EmailClient::setupUi()
{
    QVBoxLayout* vb = new QVBoxLayout( this );
    vb->setContentsMargins( 0, 0, 0, 0 );
    vb->setSpacing( 0 );

    QSplitter* horizontalSplitter = new QSplitter(this);
    horizontalSplitter->setOrientation(Qt::Vertical);
    horizontalSplitter->addWidget(messageListView());
    horizontalSplitter->addWidget(readMailWidget());

    QSplitter* verticalSplitter = new QSplitter(this);
    verticalSplitter->addWidget(folderView());
    verticalSplitter->addWidget(horizontalSplitter);

    vb->addWidget( verticalSplitter );

    QTMailWindow* qmw = QTMailWindow::singleton();
    qmw->setGeometry(0,0,defaultWidth,defaultHeight);
    int thirdHeight = qmw->height() /3;
    horizontalSplitter->setSizes(QList<int>() << thirdHeight << (qmw->height()- thirdHeight));
    int quarterWidth = qmw->width() /4;
    verticalSplitter->setSizes(QList<int>() << quarterWidth << (qmw->width()- quarterWidth));
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
                QTMailWindow::singleton()->close();
        }

        // UI updates
        setActionVisible(cancelButton, transferStatus != Inactive);
        updateGetMailButton();
    }
}


void EmailClient::clearOutboxFolder()
{
    static MessageFolder* const outbox = messageStore()->mailbox(QMailFolder::OutboxFolder);
    static MessageFolder* const sent = messageStore()->mailbox(QMailFolder::SentFolder);
    static MessageFolder* const drafts = messageStore()->mailbox(QMailFolder::DraftsFolder);

    QMailMessageIdList sentIds(outbox->messages(QMailMessage::Sent, true));
    QMailMessageIdList unsentIds(outbox->messages(QMailMessage::Sent, false));

    // Move any sent messages to the sent folder
    if (!sentIds.isEmpty())
        storageAction->moveMessages(sentIds, sent->id());

    // Move any messages stuck in the outbox to the drafts folder
    if (!unsentIds.isEmpty())
        storageAction->moveMessages(unsentIds, drafts->id());
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

    qDebug() << "Starting messagingaccounts process...";
    QProcess settingsAppProcess(this);
    settingsAppProcess.startDetached(QApplication::applicationDirPath() + "/messagingaccounts");
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
    static MessageFolder* const trashFolder = messageStore()->mailbox( QMailFolder::TrashFolder );

    if (!moveAction)
        return; // initActions hasn't been called yet

    MessageUiBase::messageSelectionChanged();

    locationSet.clear();

    int count = messageListView()->rowCount();
    if ((count > 0) && (selectionCount > 0)) {
        // Find the locations for each of the selected messages
        QMailMessageKey key(QMailMessageKey::id(messageListView()->selected()));
        QMailMessageMetaDataList messages = QMailStore::instance()->messagesMetaData(key, QMailMessageKey::ParentFolderId);

        foreach (const QMailMessageMetaData &message, messages)
            locationSet.insert(message.parentFolderId());

        // We can delete only if all selected messages are in the Trash folder
        if ((locationSet.count() == 1) && (*locationSet.begin() == trashFolder->id())) {
            deleteMailAction->setText(tr("Delete message(s)", "", selectionCount));
        } else {
            deleteMailAction->setText(tr("Move to Trash"));
        }
        moveAction->setText(tr("Move message(s)...", "", selectionCount));
        copyAction->setText(tr("Copy message(s)...", "", selectionCount));
        restoreAction->setText(tr("Restore message(s)", "", selectionCount));
    }

    // Ensure that the per-message actions are hidden, if not usable
    const bool messagesSelected(selectionCount != 0);
    setActionVisible(deleteMailAction, messagesSelected);

    // We cannot move/copy messages in the trash
    const bool trashMessagesSelected(locationSet.contains(trashFolder->id()));
    setActionVisible(moveAction, (messagesSelected && !trashMessagesSelected));
    setActionVisible(copyAction, (messagesSelected && !trashMessagesSelected));
    setActionVisible(restoreAction, (messagesSelected && trashMessagesSelected));
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

MessageFolder* EmailClient::containingFolder(const QMailMessageId& id)
{
    return messageStore()->owner(id);
}

QMailAccountIdList EmailClient::emailAccounts() const
{
    QMailAccountKey typeKey(QMailAccountKey::messageType(QMailMessage::Email));
    QMailAccountKey enabledKey(QMailAccountKey::status(QMailAccount::Enabled, QMailDataComparator::Includes));
    return QMailStore::instance()->queryAccounts(typeKey & enabledKey);
}

SearchProgressDialog* EmailClient::searchProgressDialog() const
{
    Q_ASSERT(searchAction);
    static SearchProgressDialog* spd = new SearchProgressDialog(searchAction);
    return spd;
}

void EmailClient::clearNewMessageStatus(const QMailMessageKey& key)
{
    QMailMessageKey clearNewKey = key & QMailMessageKey::status(QMailMessage::New, QMailDataComparator::Includes);

    int count = QMailStore::instance()->countMessages(clearNewKey);
    if (count) {
        QMailStore::instance()->updateMessagesMetaData(clearNewKey, QMailMessage::New, false);
    }
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


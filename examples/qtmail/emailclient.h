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

#ifndef EMAILCLIENT_H
#define EMAILCLIENT_H

#include "messagelistview.h"
#include <qmailmessageserver.h>
#include <qmailserviceaction.h>
#include <qmailviewer.h>
#include <QList>
#include <QMainWindow>
#include <QStack>
#include <QSystemTrayIcon>
#include <QTime>
#include <QTimer>
#if defined(SERVER_AS_DLL)
#include <QThread>
#endif
#include <QProcess>

class EmailFolderModel;
class EmailFolderView;
class SearchView;
class UILocation;
class ReadMail;
class WriteMail;
class QuickSearchWidget;
class StatusMonitorWidget;

class QMailAccount;
class QMailMessageSet;
class QMailRetrievalAction;
class QMailSearchAction;
class QMailTransmitAction;
class QMailStorageAction;

QT_BEGIN_NAMESPACE

class QAction;
class QStackedWidget;
class QStringList;
class QToolBar;

QT_END_NAMESPACE

class MessageUiBase : public QMainWindow
{
    Q_OBJECT

public:
    MessageUiBase(QWidget *parent = Q_NULLPTR, Qt::WindowFlags f = 0);
    virtual ~MessageUiBase() {}

signals:
    void updateStatus(const QString&);
    void updateProgress(uint, uint);
    void clearStatus();
    void clearProgress();

protected:
    void viewSearchResults(const QMailMessageKey& filter, const QString& title = QString());
    void viewComposer();

protected:
    WriteMail* writeMailWidget() const;
    ReadMail* readMailWidget() const;
    EmailFolderView* folderView() const;
    MessageListView* messageListView() const;
    EmailFolderModel* emailFolderModel() const;
    SearchView* searchView() const;

    void suspendMailCounts();
    void resumeMailCounts();
    virtual void contextStatusUpdate();
    virtual void showFolderStatus(QMailMessageSet* item);
    virtual void setMarkingMode(bool set);
    virtual void setThreaded(bool set);
    virtual void clearStatusText();

    virtual WriteMail* createWriteMailWidget();
    virtual ReadMail* createReadMailWidget();
    virtual EmailFolderView* createFolderView();
    virtual MessageListView* createMessageListView();
    virtual EmailFolderModel* createEmailFolderModel();
    virtual SearchView* createSearchView();

protected slots:
    virtual void messageSelectionChanged();
    virtual void presentMessage(const QMailMessageId &, QMailViewerFactory::PresentationType);

    // Slots to be implemented by subclass
    virtual void folderSelected(QMailMessageSet*) = 0;
    virtual void composeActivated() = 0;
    virtual void emptyTrashFolder() = 0;
    virtual void messageActivated() = 0;
    virtual void messageOpenRequested() = 0;
    virtual void allWindowsClosed() = 0;
    virtual void search() = 0;

private slots:
    void updateWindowTitle();
    void checkUpdateWindowTitle(const QModelIndex& topLeft, const QModelIndex& bottomRight);

protected:
    QString appTitle;
    bool suspendMailCount;
    bool markingMode;
    bool threaded;
    QMailMessageId selectedMessageId;
    int selectionCount;
    bool emailCountSuspended;
};


class EmailClient : public MessageUiBase
{
    Q_OBJECT

public:
    EmailClient(QWidget *parent = Q_NULLPTR, Qt::WindowFlags f = 0);
    ~EmailClient();

    bool cleanExit(bool force);
    bool closeImmediately();

    void setVisible(bool visible);

public slots:
    void sendMessageTo(const QMailAddress &address, QMailMessage::MessageType type);
    void quit();
    void closeEvent(QCloseEvent *e);

protected:
    void setupUi();
    void showEvent(QShowEvent* e);

protected slots:
    void cancelOperation();

    void noSendAccount(QMailMessage::MessageType);
    void beginEnqueueMail(QMailMessage&);
    void finishEnqueueMail(QMailServiceAction::Activity);
    void saveAsDraft(QMailMessage&);
    void discardMail();

    void sendAllQueuedMail(bool userRequest = false);

    void getSingleMail(const QMailMessageMetaData& message);
    void retrieveMessagePortion(const QMailMessageMetaData& message, uint bytes);
    void retrieveMessagePart(const QMailMessagePart::Location& partLocation);
    void retrieveMessagePartPortion(const QMailMessagePart::Location& partLocation, uint bytes);
   
    void rollBackUpdates(QMailAccountId accountId);
    void flagMessage(const QMailMessageId& id, quint64 setMask, quint64 unsetMask, const QString& description = QString("Updating message flags"));

    void messageActivated();
    void messageOpenRequested();
    void showSearchResult(const QMailMessageId& id);
    void emptyTrashFolder();

    void moveSelectedMessages();
    void copySelectedMessages();
    void restoreSelectedMessages();
    void deleteSelectedMessages();

    void moveSelectedMessagesTo(const QMailFolderId &destination);
    void copySelectedMessagesTo(const QMailFolderId &destination);

    void selectAll();

    void detachThread();

    void connectivityChanged(QMailServiceAction::Connectivity connectivity);
    void activityChanged(QMailServiceAction::Activity activity);
    void statusChanged(const QMailServiceAction::Status &status);
    void progressChanged(uint progress, uint total);
    void messagesFailedTransmission();

    void flagRetrievalActivityChanged(QMailServiceAction::Activity activity);

    void transmitCompleted();
    void retrievalCompleted();
    void storageActionCompleted();

    void folderSelected(QMailMessageSet*);

    void composeActivated();

    void getAllNewMail();
    void getAccountMail();
    void getNewMail();

    void synchronizeFolder();

    void updateAccounts();

    void transferFailure(const QMailAccountId& accountId, const QString&, int);
    void storageActionFailure(const QMailAccountId& accountId, const QString&);

    void search();

    void automaticFetch();

    void externalEdit(const QString &);

    void replyClicked();
    void replyAllClicked();
    void forwardClicked();
    void nextMessage();
    void previousMessage();
    void nextUnreadMessage();
    void previousUnreadMessage();
    void scrollReaderDown();
    void scrollReaderUp();
    void readerMarkMessageAsUnread();
    void readerMarkMessageAsImportant();
    void readerMarkMessageAsNotImportant();

    void respond(const QMailMessage& message, QMailMessage::ResponseType type);
    void respond(const QMailMessagePart::Location& partLocation, QMailMessage::ResponseType type);
    void modify(const QMailMessage& message);

    void retrieveMoreMessages();
    void retrieveVisibleMessagesFlags();

    void readReplyRequested(const QMailMessageMetaData&);

    void settings();
    void createStandardFolders();
    void notificationStateChanged();
    void connectionStateChanged();
    void exportPendingChanges();
    void exportPendingChanges(const QMailAccountId &accountId);
    void runNextPendingExport();

    void accountsAdded(const QMailAccountIdList& ids);
    void accountsRemoved(const QMailAccountIdList& ids);
    void accountsUpdated(const QMailAccountIdList& ids);

    void messagesUpdated(const QMailMessageIdList& ids);

    void messageSelectionChanged();

    void allWindowsClosed();

private slots:
    void delayedInit();
    void openFiles();
    void initActions();
    void updateActions();
    void markMessages();
    void threadMessages();
    void resumeInterruptedComposition();
    bool startMessageServer();
    bool waitForMessageServer();
    void messageServerProcessError(QProcess::ProcessError);

    void createFolder();
    void deleteFolder();
    void renameFolder();

private:
    void connectServiceAction(QMailServiceAction* action);

    bool isMessageServerRunning() const;
    virtual EmailFolderView* createFolderView();
    virtual MessageListView* createMessageListView();

    void init();

    void mailResponded();

    void closeAfterTransmissionsFinished();
    bool isTransmitting();
    bool isSending();
    bool isRetrieving();

    bool checkMailConflict(const QString& msg1, const QString& msg2);
    void setNewMessageCount(QMailMessage::MessageType type, uint);

    void readSettings();
    bool saveSettings();

    void displayCachedMail();

    void accessError(const QString &folderName);

    void getNextNewMail();
    bool verifyAccount(const QMailAccountId &accountId, bool outgoing);

    void setSendingInProgress(bool y);
    void setRetrievalInProgress(bool y);

    void transferStatusUpdate(int status);
    void setSuspendPermitted(bool y);

    void updateGetMailButton();
    void updateGetAccountButton();

    QString mailType(QMailMessage::MessageType type);

    void setActionVisible(QAction*, bool);

    void contextStatusUpdate();

    void setMarkingMode(bool set);

    void setThreaded(bool set);

    QMailFolderId containingFolder(const QMailMessageId& id);

    bool applyToSelectedFolder(void (EmailClient::*function)(const QMailFolderId&));

    void sendFailure(const QMailAccountId &accountId);
    void receiveFailure(const QMailAccountId &accountId);

    void closeApplication();

    QMailAccountIdList emailAccounts() const;

    void clearNewMessageStatus(const QMailMessageKey& key);

    QAction* createSeparator();

    QMailStorageAction* storageAction(const QString& description);
    QMailRetrievalAction* retrieveAction(const QString& description);
    QMailTransmitAction* transmitAction(const QString& description);

private:
    // Whether the initial action for the app was to view incoming messages 
    enum InitialAction { None = 0, IncomingMessages, NewMessages, View, Compose, Cleanup };

    bool filesRead;
    QMailMessageId cachedDisplayMailId;

    int transferStatus;
    int primaryActivity;

    uint totalSize;

    QMailAccountId mailAccountId;

    QAction *getMailButton;
    QAction *getAccountButton;
    QAction *composeButton;
    QAction *searchButton;
    QAction *cancelButton;
    QAction *synchronizeAction;
    QAction *createFolderAction;
    QAction *deleteFolderAction;
    QAction *renameFolderAction;
    QAction *settingsAction;
    QAction *standardFoldersAction;
    QAction *emptyTrashAction;
    QAction *deleteMailAction;
    QAction *detachThreadAction;
    QAction *markAction;
    QAction *threadAction;
    QAction *workOfflineAction;
    QAction *notificationAction;
    bool enableMessageActions;

    QMailAccountId selectedAccountId;
    QMailFolderId selectedFolderId;

    QAction *moveAction;
    QAction *copyAction;
    QAction *restoreAction;
    QAction *selectAllAction;
    QAction *replyAction;
    QAction *replyAllAction;
    QAction *forwardAction;
    QAction *nextMessageAction;
    QAction *previousMessageAction;
    QAction *nextUnreadMessageAction;
    QAction *previousUnreadMessageAction;
    QAction *scrollReaderDownAction;
    QAction *scrollReaderUpAction;
    QAction *readerMarkMessageAsUnreadAction;
    QAction *readerMarkMessageAsImportantAction;
    QAction *readerMarkMessageAsNotImportantAction;

    bool closeAfterTransmissions;
    bool closeAfterWrite;
    bool transmissionFailure;

    QTimer fetchTimer;
    bool autoGetMail;

    QMailMessageId repliedFromMailId;
    quint64 repliedFlags;

    QList<int> queuedAccountIds;

    InitialAction initialAction;

    QMap<QAction*, bool> actionVisibility;

    int preSearchWidgetId;

    QSet<QMailFolderId> locationSet;

    QMailAccountIdList transmitAccountIds;
    QMailAccountIdList retrievalAccountIds;
    QMailAccountIdList availableAccounts;

    QMailMessageId lastDraftId;

#if defined(SERVER_AS_DLL)
    QThread* m_messageServerThread;
#else
    QProcess* m_messageServerProcess;
#endif
    QSet<QMailMessageId> flagMessageIds;
    QMenu* m_contextMenu;
    QToolBar* m_toolBar;
    StatusMonitorWidget* m_statusMonitorWidget;

    QMailTransmitAction* m_transmitAction;
    QMailRetrievalAction* m_retrievalAction;
    QMailRetrievalAction* m_flagRetrievalAction;
    QMailRetrievalAction* m_exportAction;
    QList<QMailStorageAction*> m_outboxActions; 
    QMailAccountIdList m_queuedExports;
    QList<QMailMessage> m_outboxingMessages;
};

#if defined(SERVER_AS_DLL)
class MessageServerThread : public QThread
{
    Q_OBJECT

public:
    MessageServerThread();
    ~MessageServerThread();

    void run();

signals:
    void messageServerStarted();
};
#endif

#ifndef QT_NO_SYSTEMTRAYICON
class NotificationTray : public QSystemTrayIcon {
    Q_OBJECT
public:
    NotificationTray(QObject *parent = Q_NULLPTR);

private slots:
    void messagesAdded(const QMailMessageIdList &ids);
};
#endif // QT_NO_SYSTEMTRAYICON

#endif

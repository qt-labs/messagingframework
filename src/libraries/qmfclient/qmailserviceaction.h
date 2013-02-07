/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QMAILSERVICEACTION_H
#define QMAILSERVICEACTION_H

#include "qprivateimplementation.h"
#include "qmailglobal.h"
#include "qmailid.h"
#include "qmailmessagekey.h"
#include "qmailmessagesortkey.h"
#include "qmailmessage.h"
#include "qmailfolder.h"
#include "qmailaction.h"
#include <QString>
#include <QStringList>

class QMailServiceActionPrivate;

class QMF_EXPORT QMailServiceAction
    : public QObject,
      public QPrivatelyNoncopyable<QMailServiceActionPrivate>
{
    Q_OBJECT

    friend class QMailServiceActionPrivate;
    friend class QMailRetrievalActionPrivate;
    friend class QMailTransmitActionPrivate;
    friend class QMailSearchActionPrivate;

public:
    typedef QMailServiceActionPrivate ImplementationType;

    enum Connectivity {
        Offline = 0,
        Connecting,
        Connected,
        Disconnected
    };

    enum Activity {
        Pending = 0,
        InProgress,
        Successful,
        Failed
    };

    class QMF_EXPORT Status
    {
    public: 
        enum ErrorCode {
            ErrNoError = 0,
            ErrorCodeMinimum = 1024,
            ErrNotImplemented = ErrorCodeMinimum,
            ErrFrameworkFault,
            ErrSystemError,
            ErrInternalServer,
            ErrUnknownResponse,
            ErrLoginFailed,
            ErrCancel,
            ErrFileSystemFull,
            ErrNonexistentMessage,
            ErrEnqueueFailed,
            ErrNoConnection,
            ErrConnectionInUse,
            ErrConnectionNotReady,
            ErrConfiguration,
            ErrInvalidAddress,
            ErrInvalidData,
            ErrTimeout,
            ErrInternalStateReset,
            ErrorCodeMaximum = ErrInternalStateReset
        };

        Status();
        Status(ErrorCode code, 
               const QString &text,
               const QMailAccountId &accountId, 
               const QMailFolderId &folderId,
               const QMailMessageId &messageId);

        Status(const Status &other);

        template <typename Stream> void serialize(Stream &stream) const;
        template <typename Stream> void deserialize(Stream &stream);

        ErrorCode errorCode;
        QString text;

        QMailAccountId accountId;
        QMailFolderId folderId;
        QMailMessageId messageId;
    };

    ~QMailServiceAction();

    Connectivity connectivity() const;
    Activity activity() const;
    const Status status() const;
    QPair<uint, uint> progress() const;

    bool isRunning() const;

public slots:
    virtual void cancelOperation();

signals:
    void connectivityChanged(QMailServiceAction::Connectivity c);
    void activityChanged(QMailServiceAction::Activity a);
    void statusChanged(const QMailServiceAction::Status &s);
    void progressChanged(uint value, uint total);

protected:
    // Only allow creation by sub-types
    template<typename Subclass>
    QMailServiceAction(Subclass *p, QObject *parent);

protected:
    void setStatus(Status::ErrorCode code, const QString &text = QString());
    void setStatus(Status::ErrorCode code, const QString &text, const QMailAccountId &accountId,const QMailFolderId &folderId = QMailFolderId(), const QMailMessageId &messageId = QMailMessageId());
};

class QMailRetrievalActionPrivate;

class QMF_EXPORT QMailRetrievalAction : public QMailServiceAction
{
    Q_OBJECT

public:
    typedef QMailRetrievalActionPrivate ImplementationType;

    enum RetrievalSpecification {
        Flags,
        MetaData,
        Content,
        Auto
    };

    QMailRetrievalAction(QObject *parent = 0);
    ~QMailRetrievalAction();

public slots:
    void retrieveFolderList(const QMailAccountId &accountId, const QMailFolderId &folderId, bool descending = true);
    void retrieveMessageList(const QMailAccountId &accountId, const QMailFolderId &folderId, uint minimum = 0, const QMailMessageSortKey &sort = QMailMessageSortKey());
    void retrieveMessageLists(const QMailAccountId &accountId, const QMailFolderIdList &folderIds, uint minimum = 0, const QMailMessageSortKey &sort = QMailMessageSortKey());
    static uint defaultMinimum() { return 20; }
    void retrieveNewMessages(const QMailAccountId &accountId, const QMailFolderIdList &folderIds);

    void createStandardFolders(const QMailAccountId &accountId);

    void retrieveMessages(const QMailMessageIdList &messageIds, RetrievalSpecification spec = MetaData);
    void retrieveMessagePart(const QMailMessagePart::Location &partLocation);

    void retrieveMessageRange(const QMailMessageId &messageId, uint minimum);
    void retrieveMessagePartRange(const QMailMessagePart::Location &partLocation, uint minimum);

    void exportUpdates(const QMailAccountId &accountId);
    void synchronize(const QMailAccountId &accountId, uint minimum);

    void retrieveAll(const QMailAccountId &accountId); // deprecated
    void synchronizeAll(const QMailAccountId &accountId); // deprecated
};


class QMailTransmitActionPrivate;

class QMF_EXPORT QMailTransmitAction : public QMailServiceAction
{
    Q_OBJECT

public:
    typedef QMailTransmitActionPrivate ImplementationType;

    QMailTransmitAction(QObject *parent = 0);
    ~QMailTransmitAction();

signals:
    void messagesTransmitted(const QMailMessageIdList &ids);
    void messagesFailedTransmission(const QMailMessageIdList &ids, QMailServiceAction::Status::ErrorCode);

public slots:
    void transmitMessages(const QMailAccountId &accountId);
    void transmitMessage(const QMailMessageId &messageId);
};


class QMailStorageActionPrivate;

class QMF_EXPORT QMailStorageAction : public QMailServiceAction
{
    Q_OBJECT

public:
    typedef QMailStorageActionPrivate ImplementationType;

    QMailStorageAction(QObject *parent = 0);
    ~QMailStorageAction();

    QMailMessageIdList messagesAdded() const;
    QMailMessageIdList messagesUpdated() const;

public slots:
    void onlineDeleteMessages(const QMailMessageIdList &ids);
    void discardMessages(const QMailMessageIdList &ids);

    void onlineCopyMessages(const QMailMessageIdList &ids, const QMailFolderId &destinationId);
    void onlineMoveMessages(const QMailMessageIdList &ids, const QMailFolderId &destinationId);

    void onlineFlagMessagesAndMoveToStandardFolder(const QMailMessageIdList &ids, quint64 setMask, quint64 unsetMask);
    void addMessages(const QMailMessageList &list);
    void updateMessages(const QMailMessageList &list);
    void updateMessages(const QMailMessageMetaDataList &list);

    void onlineCreateFolder(const QString &name, const QMailAccountId &accountId, const QMailFolderId &parentId);
    void onlineRenameFolder(const QMailFolderId &folderId, const QString &name);
    void onlineDeleteFolder(const QMailFolderId &folderId);

    void deleteMessages(const QMailMessageIdList &ids);
    void rollBackUpdates(const QMailAccountId &mailAccountId);
    void moveToStandardFolder(const QMailMessageIdList& ids, QMailFolder::StandardFolder standardFolder);
    void moveToFolder(const QMailMessageIdList& ids, const QMailFolderId& folderId);
    void flagMessages(const QMailMessageIdList& ids, quint64 setMask, quint64 unsetMask);
    void restoreToPreviousFolder(const QMailMessageKey& key);
};


class QMailSearchActionPrivate;

class QMF_EXPORT QMailSearchAction : public QMailServiceAction
{
    Q_OBJECT

public:
    typedef QMailSearchActionPrivate ImplementationType;

    enum SearchSpecification {
        Local,
        Remote
    };

    QMailSearchAction(QObject *parent = 0);
    ~QMailSearchAction();

    QMailMessageIdList matchingMessageIds() const;
    uint remainingMessagesCount() const;
    uint messagesCount() const;
    static QMailMessageKey temporaryKey();

signals:
    void messageIdsMatched(const QMailMessageIdList &ids);
    void remainingMessagesCount(uint);
    void messagesCount(uint);

public slots:
    void searchMessages(const QMailMessageKey &filter, const QString& bodyText, SearchSpecification spec, const QMailMessageSortKey &sort = QMailMessageSortKey());
    void searchMessages(const QMailMessageKey &filter, const QString& bodyText, SearchSpecification spec, quint64 limit, const QMailMessageSortKey &sort = QMailMessageSortKey());
    void countMessages(const QMailMessageKey &filter, const QString& bodyText);
    void cancelOperation();
};

class QMailActionInfoPrivate;

class QMF_EXPORT  QMailActionInfo : public QMailServiceAction {
    Q_OBJECT
public:
    typedef Status::ErrorCode StatusErrorCode; // moc hack
    typedef QMailActionInfoPrivate ImplementationType;

    QMailActionId id() const;
    QMailServerRequestType requestType() const;
    float totalProgress() const;
    StatusErrorCode statusErrorCode() const;
    QString statusText() const;
    QMailAccountId statusAccountId() const;
    QMailFolderId statusFolderId() const;
    QMailMessageId statusMessageId() const;

signals:
    void statusErrorCodeChanged(QMailActionInfo::StatusErrorCode newError);
    void statusTextChanged(const QString &newText);
    void statusAccountIdChanged(const QMailAccountId &newAccountId);
    void statusFolderIdChanged(const QMailFolderId &newFolderId);
    void statusMessageIdChanged(const QMailMessageId &newMessageId);
    void totalProgressChanged(float progress);
protected:
    friend class QMailActionObserverPrivate;
    QMailActionInfo(const QMailActionData &data);
};

class QMailActionObserverPrivate;

class QMF_EXPORT QMailActionObserver : public QMailServiceAction
{
    Q_OBJECT
public:   
    typedef QMailActionObserverPrivate ImplementationType;

    QMailActionObserver(QObject *parent = 0);
    virtual ~QMailActionObserver();

    QList< QSharedPointer<QMailActionInfo> > actions() const;
    void listActionsRequest();
signals:
    void actionsChanged(const QList< QSharedPointer<QMailActionInfo> > &newActions);
};

class QMailProtocolActionPrivate;

class QMF_EXPORT QMailProtocolAction : public QMailServiceAction
{
    Q_OBJECT

public:
    typedef QMailProtocolActionPrivate ImplementationType;

    QMailProtocolAction(QObject *parent = 0);
    ~QMailProtocolAction();

signals:
    void protocolResponse(const QString &response, const QVariant &data);

public slots:
    void protocolRequest(const QMailAccountId &accountId, const QString &request, const QVariant &data);
};


Q_DECLARE_USER_METATYPE(QMailServiceAction::Status)

Q_DECLARE_USER_METATYPE_ENUM(QMailServiceAction::Connectivity)
Q_DECLARE_USER_METATYPE_ENUM(QMailServiceAction::Activity)
Q_DECLARE_USER_METATYPE_ENUM(QMailServiceAction::Status::ErrorCode)

Q_DECLARE_USER_METATYPE_ENUM(QMailRetrievalAction::RetrievalSpecification)

Q_DECLARE_USER_METATYPE_ENUM(QMailSearchAction::SearchSpecification)

#endif

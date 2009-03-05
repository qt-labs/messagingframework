/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
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
#include <QString>
#include <QStringList>

class QMailServiceActionPrivate;

class QTOPIAMAIL_EXPORT QMailServiceAction
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

    class QTOPIAMAIL_EXPORT Status 
    {
    public: 
        enum ErrorCode {
            ErrNoError = 0,
            ErrorCodeMinimum = 1024,
            ErrNotImplemented = ErrorCodeMinimum,
            ErrFrameworkFault,
            ErrSystemError,
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
            ErrorCodeMaximum = ErrTimeout
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
    void setStatus(Status::ErrorCode code, const QString &text, const QMailAccountId &accountId, const QMailFolderId &folderId = QMailFolderId(), const QMailMessageId &messageId = QMailMessageId());
};


class QMailRetrievalActionPrivate;

class QTOPIAMAIL_EXPORT QMailRetrievalAction : public QMailServiceAction
{
    Q_OBJECT

public:
    typedef QMailRetrievalActionPrivate ImplementationType;

    enum RetrievalSpecification {
        Flags,
        MetaData,
        Content
    };

    QMailRetrievalAction(QObject *parent = 0);
    ~QMailRetrievalAction();

public slots:
    void retrieveFolderList(const QMailAccountId &accountId, const QMailFolderId &folderId, bool descending = true);
    void retrieveMessageList(const QMailAccountId &accountId, const QMailFolderId &folderId, uint minimum = 0, const QMailMessageSortKey &sort = QMailMessageSortKey());

    void retrieveMessages(const QMailMessageIdList &messageIds, RetrievalSpecification spec = MetaData);
    void retrieveMessagePart(const QMailMessagePart::Location &partLocation);

    void retrieveMessageRange(const QMailMessageId &messageId, uint minimum);
    void retrieveMessagePartRange(const QMailMessagePart::Location &partLocation, uint minimum);

    void retrieveAll(const QMailAccountId &accountId);
    void exportUpdates(const QMailAccountId &accountId);

    void synchronize(const QMailAccountId &accountId);
};


class QMailTransmitActionPrivate;

class QTOPIAMAIL_EXPORT QMailTransmitAction : public QMailServiceAction
{
    Q_OBJECT

public:
    typedef QMailTransmitActionPrivate ImplementationType;

    QMailTransmitAction(QObject *parent = 0);
    ~QMailTransmitAction();

public slots:
    void transmitMessages(const QMailAccountId &accountId);
};


class QMailStorageActionPrivate;

class QTOPIAMAIL_EXPORT QMailStorageAction : public QMailServiceAction
{
    Q_OBJECT

public:
    typedef QMailStorageActionPrivate ImplementationType;

    QMailStorageAction(QObject *parent = 0);
    ~QMailStorageAction();

public slots:
    void deleteMessages(const QMailMessageIdList &ids);
    void discardMessages(const QMailMessageIdList &ids);

    void copyMessages(const QMailMessageIdList &ids, const QMailFolderId &destinationId);
    void moveMessages(const QMailMessageIdList &ids, const QMailFolderId &destinationId);
};


class QMailSearchActionPrivate;

class QTOPIAMAIL_EXPORT QMailSearchAction : public QMailServiceAction
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

signals:
    void messageIdsMatched(const QMailMessageIdList &ids);

public slots:
    void searchMessages(const QMailMessageKey &filter, const QString& bodyText, SearchSpecification spec, const QMailMessageSortKey &sort = QMailMessageSortKey());
    void cancelOperation();
};


class QMailProtocolActionPrivate;

class QTOPIAMAIL_EXPORT QMailProtocolAction : public QMailServiceAction
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

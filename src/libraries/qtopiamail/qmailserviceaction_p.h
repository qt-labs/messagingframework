/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef QMAILSERVICEACTION_P_H
#define QMAILSERVICEACTION_P_H

#include "qmailserviceaction.h"
#include "qmailmessageserver.h"


// These classes are implemented via qmailmessage.cpp and qmailinstantiations.cpp

class QMailServiceActionPrivate : public QObject, public QPrivateNoncopyableBase
{
    Q_OBJECT

public:
    template<typename Subclass>
    QMailServiceActionPrivate(Subclass *p, QMailServiceAction *i);

    ~QMailServiceActionPrivate();

    void cancelOperation();

protected slots:
    void activityChanged(quint64, QMailServiceAction::Activity activity);
    void connectivityChanged(quint64, QMailServiceAction::Connectivity connectivity);
    void statusChanged(quint64, const QMailServiceAction::Status status);
    void progressChanged(quint64, uint progress, uint total);

protected:
    friend class QMailServiceAction;

    virtual void init();

    quint64 newAction();
    bool validAction(quint64 action);

    void setActivity(QMailServiceAction::Activity newActivity);
    void setConnectivity(QMailServiceAction::Connectivity newConnectivity);

    void setStatus(const QMailServiceAction::Status &status);
    void setStatus(QMailServiceAction::Status::ErrorCode code, const QString &text);
    void setStatus(QMailServiceAction::Status::ErrorCode code, const QString &text, const QMailAccountId &accountId, const QMailFolderId &folderId, const QMailMessageId &messageId);

    void setProgress(uint newProgress, uint newTotal);

    void emitChanges();

    QMailServiceAction *_interface;
    QMailMessageServer *_server;

    QMailServiceAction::Connectivity _connectivity;
    QMailServiceAction::Activity _activity;
    QMailServiceAction::Status _status;

    uint _total;
    uint _progress;

    quint64 _action;

    bool _connectivityChanged;
    bool _activityChanged;
    bool _progressChanged;
    bool _statusChanged;
};


class QMailRetrievalActionPrivate : public QMailServiceActionPrivate
{
    Q_OBJECT

public:
    QMailRetrievalActionPrivate(QMailRetrievalAction *);

    void retrieveFolderList(const QMailAccountId &accountId, const QMailFolderId &folderId, bool descending);
    void retrieveMessageList(const QMailAccountId &accountId, const QMailFolderId &folderId, uint minimum, const QMailMessageSortKey &sort);

    void retrieveMessages(const QMailMessageIdList &messageIds, QMailRetrievalAction::RetrievalSpecification spec);
    void retrieveMessagePart(const QMailMessagePart::Location &partLocation);

    void retrieveMessageRange(const QMailMessageId &messageId, uint minimum);
    void retrieveMessagePartRange(const QMailMessagePart::Location &partLocation, uint minimum);

    void retrieveAll(const QMailAccountId &accountId);
    void exportUpdates(const QMailAccountId &accountId);

    void synchronize(const QMailAccountId &accountId);

protected slots:
    void retrievalCompleted(quint64);

private:
    friend class QMailRetrievalAction;
};


class QMailTransmitActionPrivate : public QMailServiceActionPrivate
{
    Q_OBJECT

public:
    QMailTransmitActionPrivate(QMailTransmitAction *i);

    void transmitMessages(const QMailAccountId &accountId);

protected:
    virtual void init();

protected slots:
    void messagesTransmitted(quint64, const QMailMessageIdList &id);
    void transmissionCompleted(quint64);

private:
    friend class QMailTransmitAction;

    QMailMessageIdList _ids;
};


class QMailStorageActionPrivate : public QMailServiceActionPrivate
{
    Q_OBJECT

public:
    QMailStorageActionPrivate(QMailStorageAction *i);

    void deleteMessages(const QMailMessageIdList &ids);
    void discardMessages(const QMailMessageIdList &ids);

    void copyMessages(const QMailMessageIdList &ids, const QMailFolderId &destination);
    void moveMessages(const QMailMessageIdList &ids, const QMailFolderId &destination);

protected:
    virtual void init();

protected slots:
    void messagesEffected(quint64, const QMailMessageIdList &id);
    void actionCompleted(quint64);

private:
    friend class QMailStorageAction;

    QMailMessageIdList _ids;
};


class QMailSearchActionPrivate : public QMailServiceActionPrivate
{
    Q_OBJECT

public:
    QMailSearchActionPrivate(QMailSearchAction *i);

    void searchMessages(const QMailMessageKey &filter, const QString &bodyText, QMailSearchAction::SearchSpecification spec, const QMailMessageSortKey &sort);
    void cancelOperation();

protected:
    virtual void init();

signals:
    void messageIdsMatched(const QMailMessageIdList &ids);

private slots:
    void matchingMessageIds(quint64 action, const QMailMessageIdList &ids);
    void searchCompleted(quint64 action);

    void finaliseSearch();

private:
    friend class QMailSearchAction;

    QMailMessageIdList _matchingIds;
};


class QMailProtocolActionPrivate : public QMailServiceActionPrivate
{
    Q_OBJECT

public:
    QMailProtocolActionPrivate(QMailProtocolAction *i);

    void protocolRequest(const QMailAccountId &accountId, const QString &request, const QVariant &data);

signals:
    void protocolResponse(const QString &response, const QVariant &data);

private slots:
    void protocolResponse(quint64 action, const QString &response, const QVariant &data);
    void protocolRequestCompleted(quint64 action);

private:
    friend class QMailProtocolAction;
};

#endif

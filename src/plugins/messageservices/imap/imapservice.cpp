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

#include "imapservice.h"
#ifndef QMF_NO_WIDGETS
#include "imapsettings.h"
#endif
#include "imapconfiguration.h"
#include "imapstrategy.h"
#include "serviceactionqueue.h"
#include <QtPlugin>
#include <QTimer>
#include <qmaillog.h>
#include <qmailmessage.h>
#include <qmaildisconnected.h>
#include <QCoreApplication>
#include <typeinfo>

namespace { 

const QString serviceKey("imap4");

}

class ImapService::Source : public QMailMessageSource
{
    Q_OBJECT

public:
    Source(ImapService *service)
        : QMailMessageSource(service),
          _service(service),
          _queuedMailCheckInProgress(false),
          _mailCheckPhase(RetrieveFolders),
          _unavailable(false),
          _synchronizing(false),
          _setMask(0),
          _unsetMask(0)
    {
        connect(&_intervalTimer, SIGNAL(timeout()), this, SLOT(intervalCheck()));
        connect(&_pushIntervalTimer, SIGNAL(timeout()), this, SLOT(pushIntervalCheck()));
        connect(&_strategyExpiryTimer, SIGNAL(timeout()), this, SLOT(expireStrategy()));
    }

    void initClientConnections() {
        connect(_service->_client, SIGNAL(allMessagesReceived()), this, SIGNAL(newMessagesAvailable()));
        connect(_service->_client, SIGNAL(messageCopyCompleted(QMailMessage&, QMailMessage)), this, SLOT(messageCopyCompleted(QMailMessage&, QMailMessage)));
        connect(_service->_client, SIGNAL(messageActionCompleted(QString)), this, SLOT(messageActionCompleted(QString)));
        connect(_service->_client, SIGNAL(retrievalCompleted()), this, SLOT(retrievalCompleted()));
        connect(_service->_client, SIGNAL(idleNewMailNotification(QMailFolderId)), this, SLOT(queueMailCheck(QMailFolderId)));
        connect(_service->_client, SIGNAL(idleFlagsChangedNotification(QMailFolderId)), this, SLOT(queueFlagsChangedCheck(QMailFolderId)));
        connect(_service->_client, SIGNAL(matchingMessageIds(QMailMessageIdList)), this, SIGNAL(matchingMessageIds(QMailMessageIdList)));
        connect(_service->_client, SIGNAL(remainingMessagesCount(uint)), this, SIGNAL(remainingMessagesCount(uint)));
        connect(_service->_client, SIGNAL(messagesCount(uint)), this, SIGNAL(messagesCount(uint)));
    }
    
    void setIntervalTimer(int interval)
    {
        _intervalTimer.stop();
        if (interval > 0 && _service->_client) {
            _intervalTimer.start(interval*1000*60); // interval minutes
        }
    }

    void setPushIntervalTimer(int pushInterval)
    {
        _pushIntervalTimer.stop();
        if (pushInterval > 0 && _service->_client) {
            _pushIntervalTimer.start(pushInterval*1000*60); // interval minutes
        }
    }

    QMailStore::MessageRemovalOption messageRemovalOption() const override
    {
        return QMailStore::CreateRemovalRecord;
    }

signals:
    void messageActionCompleted(const QMailMessageIdList &ids);

public slots:
    bool retrieveFolderList(const QMailAccountId &accountId, const QMailFolderId &folderId, bool descending) override;
    bool retrieveMessageLists(const QMailAccountId &accountId, const QMailFolderIdList &folderIds,
                              uint minimum, const QMailMessageSortKey &sort) override;
    bool retrieveMessageList(const QMailAccountId &accountId, const QMailFolderId &folderId,
                             uint minimum, const QMailMessageSortKey &sort) override;
    bool retrieveNewMessages(const QMailAccountId &accountId,
                             const QMailFolderIdList &_folderIds) override;
    virtual bool retrieveMessageLists(const QMailAccountId &accountId,
                                      const QMailFolderIdList &_folderIds,
                                      uint minimum,
                                      const QMailMessageSortKey &sort,
                                      bool retrieveAll);

    bool retrieveMessages(const QMailMessageIdList &messageIds,
                          QMailRetrievalAction::RetrievalSpecification spec) override;
    bool retrieveMessagePart(const QMailMessagePart::Location &partLocation) override;

    bool retrieveMessageRange(const QMailMessageId &messageId, uint minimum) override;
    bool retrieveMessagePartRange(const QMailMessagePart::Location &partLocation, uint minimum) override;

    bool retrieveAll(const QMailAccountId &accountId) override;
    bool exportUpdates(const QMailAccountId &accountId) override;

    bool synchronize(const QMailAccountId &accountId) override;

    bool deleteMessages(const QMailMessageIdList &ids) override;

    bool copyMessages(const QMailMessageIdList &ids, const QMailFolderId &destinationId) override;
    bool moveMessages(const QMailMessageIdList &ids, const QMailFolderId &destinationId) override;
    bool flagMessages(const QMailMessageIdList &ids, quint64 setMask, quint64 unsetMask) override;

    bool createFolder(const QString &name, const QMailAccountId &accountId, const QMailFolderId &parentId) override;
    bool createStandardFolders(const QMailAccountId &accountId) override;
    bool deleteFolder(const QMailFolderId &folderId) override;
    bool renameFolder(const QMailFolderId &folderId, const QString &name) override;
    bool moveFolder(const QMailFolderId &folderId, const QMailFolderId &newParentId) override;

    bool searchMessages(const QMailMessageKey &searchCriteria, const QString &bodyText,
                        const QMailMessageSortKey &sort) override;
    bool searchMessages(const QMailMessageKey &searchCriteria, const QString &bodyText,
                        quint64 limit, const QMailMessageSortKey &sort) override;
    virtual bool searchMessages(const QMailMessageKey &searchCriteria, const QString &bodyText,
                                quint64 limit, const QMailMessageSortKey &sort, bool count);
    bool countMessages(const QMailMessageKey &searchCriteria, const QString &bodyText) override;
    bool cancelSearch() override;

    bool prepareMessages(const QList<QPair<QMailMessagePart::Location, QMailMessagePart::Location> > &ids) override;

    void messageCopyCompleted(QMailMessage &message, const QMailMessage &original);
    void messageActionCompleted(const QString &uid);
    void retrievalCompleted();
    void retrievalTerminated();
    void intervalCheck();
    void pushIntervalCheck();
    void queueMailCheck(QMailFolderId folderId);
    void queueFlagsChangedCheck(QMailFolderId folderId);
    void resetExpiryTimer();
    void expireStrategy();
    void emitActionSuccessfullyCompleted();

private:
    bool doDelete(const QMailMessageIdList & ids);

    virtual bool setStrategy(ImapStrategy *strategy, const char *signal = Q_NULLPTR);

    virtual void appendStrategy(ImapStrategy *strategy, const char *signal = Q_NULLPTR);
    virtual bool initiateStrategy();
    void queueDisconnectedOperations(const QMailAccountId &accountId);

    enum MailCheckPhase { RetrieveFolders = 0, RetrieveMessages, CheckFlags };

    ImapService *_service;
    bool _queuedMailCheckInProgress;
    MailCheckPhase _mailCheckPhase;
    QMailFolderId _mailCheckFolderId;
    bool _unavailable;
    bool _synchronizing;
    QTimer _intervalTimer;
    QTimer _pushIntervalTimer;
    QList<QMailFolderId> _queuedFolders; // require new mail check
    QList<QMailFolderId> _queuedFoldersFullCheck; // require flags check also
    quint64 _setMask;
    quint64 _unsetMask;
    QList<QPair<ImapStrategy*, QLatin1String> > _pendingStrategies;
    QTimer _strategyExpiryTimer; // Required to expire interval mail check triggered by push email
    ServiceActionQueue _actionQueue;
};

bool ImapService::Source::retrieveFolderList(const QMailAccountId &accountId, const QMailFolderId &folderId, bool descending)
{
    Q_ASSERT(!_unavailable);
    if (!_service->_client) {
        _service->errorOccurred(QMailServiceAction::Status::ErrFrameworkFault, tr("Account disabled"));
        return false;
    }

    if (!accountId.isValid()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("No account specified"));
        return false;
    }

    _service->_client->strategyContext()->foldersOnlyStrategy.clearSelection();
    _service->_client->strategyContext()->foldersOnlyStrategy.setBase(folderId);
    _service->_client->strategyContext()->foldersOnlyStrategy.setQuickList(!folderId.isValid());
    _service->_client->strategyContext()->foldersOnlyStrategy.setDescending(descending);
    _service->_client->strategyContext()->foldersOnlyStrategy.setIgnoreSyncFlag(true);
    appendStrategy(&_service->_client->strategyContext()->foldersOnlyStrategy);
    if(!_unavailable)
        return initiateStrategy();
    return true;
}

bool ImapService::Source::retrieveMessageLists(const QMailAccountId &accountId, const QMailFolderIdList &folderIds, uint minimum, const QMailMessageSortKey &sort)
{
    Q_ASSERT(!_unavailable);
    QMailFolderIdList ids;

    foreach (const QMailFolderId &id, folderIds) {
        if (QMailFolder(id).status() & QMailFolder::MessagesPermitted)
            ids.append(id);
    }

    if (ids.isEmpty()) {
        QTimer::singleShot(0, this, SLOT(retrievalCompleted()));
        return true;
    }

    return retrieveMessageLists(accountId, ids, minimum, sort, true /* accountCheck */);
}

bool ImapService::Source::retrieveMessageList(const QMailAccountId &accountId, const QMailFolderId &folderId, uint minimum, const QMailMessageSortKey &sort)
{
    Q_ASSERT(!_unavailable);
    if (folderId.isValid()) {
        return retrieveMessageLists(accountId, QMailFolderIdList() << folderId, minimum, sort, true /* Full check */);
    }
    
    return retrieveMessageLists(accountId, QMailFolderIdList(), minimum, sort, true /* Full check */);
}

bool ImapService::Source::retrieveNewMessages(const QMailAccountId &accountId, const QMailFolderIdList &folderIds)
{
    Q_ASSERT(!_unavailable);
    QMailFolderIdList ids;

    foreach (const QMailFolderId &id, folderIds) {
        if (QMailFolder(id).status() & QMailFolder::MessagesPermitted)
            ids.append(id);
    }

    if (ids.isEmpty()) {
        QTimer::singleShot(0, this, SLOT(retrievalCompleted()));
        return true;
    }

    // Use defaultMinimum so that for a freshly created push enabled account
    // defaultMinimum messages are retrieved. But this means when new push
    // emails arrive, flags will be checked, but for only defaultMinimum messages.
    return retrieveMessageLists(accountId, ids, QMailRetrievalAction::defaultMinimum(), QMailMessageSortKey(), false /* not accountCheck, don't detect flag changes and removed messages */);
}

bool ImapService::Source::retrieveMessageLists(const QMailAccountId &accountId, const QMailFolderIdList &_folderIds, uint minimum, const QMailMessageSortKey &sort, bool accountCheck)
{
    Q_ASSERT(!_unavailable);
    if (!_service->_client) {
        _service->errorOccurred(QMailServiceAction::Status::ErrFrameworkFault, tr("Account disabled"));
        return false;
    }

    if (!accountId.isValid()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("No account specified"));
        return false;
    }

    if (!sort.isEmpty()) {
        qWarning() << "IMAP Search sorting not yet implemented!";
    }
    
    QMailFolderIdList folderIds;
    uint adjustedMinimum = minimum ? minimum : INT_MAX; // zero means retrieve all mail
    _service->_client->strategyContext()->retrieveMessageListStrategy.clearSelection();
    _service->_client->strategyContext()->retrieveMessageListStrategy.setMinimum(adjustedMinimum);
    if (!_folderIds.isEmpty()) {
        folderIds = _folderIds;
    } else {
        // Retrieve messages for all folders in the account that have undiscovered messages
        QMailFolderKey accountKey(QMailFolderKey::parentAccountId(accountId));
        QMailFolderKey canSelectKey(QMailFolderKey::status(QMailFolder::MessagesPermitted));
        QMailFolderKey filterKey(accountKey & canSelectKey);
        folderIds = QMailStore::instance()->queryFolders(filterKey, QMailFolderSortKey::id(Qt::AscendingOrder));
    }
    // accountCheck false, just retrieve new mail or minimum mails whichever is more
    // accountCheck is true, also update status of messages on device, and detect removed messages
    _service->_client->strategyContext()->retrieveMessageListStrategy.setAccountCheck(accountCheck);

    _service->_client->strategyContext()->retrieveMessageListStrategy.setOperation(_service->_client->strategyContext(), QMailRetrievalAction::Auto);
    _service->_client->strategyContext()->retrieveMessageListStrategy.selectedFoldersAppend(folderIds);
    _service->_client->strategyContext()->retrieveMessageListStrategy.setIgnoreSyncFlag(!_folderIds.isEmpty());
    appendStrategy(&_service->_client->strategyContext()->retrieveMessageListStrategy);
    if(!_unavailable)
        return initiateStrategy();
    return true;
}

bool ImapService::Source::retrieveMessages(const QMailMessageIdList &messageIds, QMailRetrievalAction::RetrievalSpecification spec)
{
    Q_ASSERT(!_unavailable);
    if (!_service->_client) {
        _service->errorOccurred(QMailServiceAction::Status::ErrFrameworkFault, tr("Account disabled"));
        return false;
    }

    if (messageIds.isEmpty()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("No messages to retrieve"));
        return false;
    }

    if (spec == QMailRetrievalAction::Flags) {
        _service->_client->strategyContext()->updateMessagesFlagsStrategy.clearSelection();
        _service->_client->strategyContext()->updateMessagesFlagsStrategy.selectedMailsAppend(messageIds);
        appendStrategy(&_service->_client->strategyContext()->updateMessagesFlagsStrategy);
        if(!_unavailable)
            return initiateStrategy();
        return true;
    }

    _service->_client->strategyContext()->selectedStrategy.clearSelection();

    // Select the parts to be downloaded according to "spec".
    _service->_client->strategyContext()->selectedStrategy.setOperation(
            _service->_client->strategyContext(), spec);
    QMailMessageIdList completionList;
    QList<QPair<QMailMessagePart::Location, int> > completionSectionList;
    foreach (const QMailMessageId &id, messageIds) {
        QMailMessage message(id);
        _service->_client->strategyContext()->selectedStrategy.prepareCompletionList(
                _service->_client->strategyContext(), message,
                completionList, completionSectionList);
    }
    _service->_client->strategyContext()->selectedStrategy.selectedMailsAppend(completionList);
    typedef QPair<QMailMessagePart::Location, int > SectionDescription;
    foreach (const SectionDescription &section, completionSectionList) {
        _service->_client->strategyContext()->selectedStrategy.selectedSectionsAppend(section.first, section.second);
    }

    appendStrategy(&_service->_client->strategyContext()->selectedStrategy);

    if(!_unavailable)
        return initiateStrategy();
    return true;
}

bool ImapService::Source::retrieveMessagePart(const QMailMessagePart::Location &partLocation)
{
    Q_ASSERT(!_unavailable);
    if (!_service->_client) {
        _service->errorOccurred(QMailServiceAction::Status::ErrFrameworkFault, tr("Account disabled"));
        return false;
    }

    if (!partLocation.containingMessageId().isValid()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("No message to retrieve"));
        return false;
    }
    if (!partLocation.isValid(false)) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("No part specified"));
        return false;
    }
    if (!QMailMessage(partLocation.containingMessageId()).id().isValid()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("Invalid message specified"));
        return false;
    }

    QMailMessage msg(partLocation.containingMessageId());
    if (!msg.contains(partLocation) || msg.partAt(partLocation).contentAvailable()) {
        // Already retrieved (or invalid)
        QTimer::singleShot(0, this, SLOT(retrievalCompleted()));
        return true;
    }
    
    _service->_client->strategyContext()->selectedStrategy.clearSelection();
    _service->_client->strategyContext()->selectedStrategy.setOperation(_service->_client->strategyContext(), QMailRetrievalAction::Content);
    _service->_client->strategyContext()->selectedStrategy.selectedSectionsAppend(partLocation);
    appendStrategy(&_service->_client->strategyContext()->selectedStrategy);
    if(!_unavailable)
        return initiateStrategy();
    return true;
}

bool ImapService::Source::retrieveMessageRange(const QMailMessageId &messageId, uint minimum)
{
    Q_ASSERT(!_unavailable);
    if (!_service->_client) {
        _service->errorOccurred(QMailServiceAction::Status::ErrFrameworkFault, tr("Account disabled"));
        return false;
    }

    if (!messageId.isValid()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("No message to retrieve"));
        return false;
    }
    if (!QMailMessage(messageId).id().isValid()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("Invalid message specified"));
        return false;
    }
    // Not tested yet
    if (!minimum) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("No minimum specified"));
        return false;
    }
    
    QMailMessage msg(messageId);
    if (msg.contentAvailable()) {
        // Already retrieved
        QTimer::singleShot(0, this, SLOT(retrievalCompleted()));
        return true;
    }
    
    QMailMessagePart::Location location;
    location.setContainingMessageId(messageId);

    _service->_client->strategyContext()->selectedStrategy.clearSelection();
    _service->_client->strategyContext()->selectedStrategy.setOperation(_service->_client->strategyContext(), QMailRetrievalAction::Content);
    _service->_client->strategyContext()->selectedStrategy.selectedSectionsAppend(location, minimum);
    appendStrategy(&_service->_client->strategyContext()->selectedStrategy);
    if(!_unavailable)
        return initiateStrategy();
    return true;
}

bool ImapService::Source::retrieveMessagePartRange(const QMailMessagePart::Location &partLocation, uint minimum)
{
    Q_ASSERT(!_unavailable);
    if (!_service->_client) {
        _service->errorOccurred(QMailServiceAction::Status::ErrFrameworkFault, tr("Account disabled"));
        return false;
    }

    if (!partLocation.containingMessageId().isValid()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("No message to retrieve"));
        return false;
    }
    if (!partLocation.isValid(false)) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("No part specified"));
        return false;
    }
    if (!QMailMessage(partLocation.containingMessageId()).id().isValid()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("Invalid message specified"));
        return false;
    }
    if (!minimum) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("No minimum specified"));
        return false;
    }

    QMailMessage msg(partLocation.containingMessageId());
    if (!msg.contains(partLocation) || msg.partAt(partLocation).contentAvailable()) {
        // Already retrieved (or invalid)
        QTimer::singleShot(0, this, SLOT(retrievalCompleted()));
        return true;
    }
    
    _service->_client->strategyContext()->selectedStrategy.clearSelection();
    _service->_client->strategyContext()->selectedStrategy.setOperation(_service->_client->strategyContext(), QMailRetrievalAction::Content);
    _service->_client->strategyContext()->selectedStrategy.selectedSectionsAppend(partLocation, minimum);
    appendStrategy(&_service->_client->strategyContext()->selectedStrategy);

    if(!_unavailable)
        return initiateStrategy();
    return true;
}

bool ImapService::Source::retrieveAll(const QMailAccountId &accountId)
{
    Q_ASSERT(!_unavailable);
    if (!_service->_client) {
        _service->errorOccurred(QMailServiceAction::Status::ErrFrameworkFault, tr("Account disabled"));
        return false;
    }

    if (!accountId.isValid()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("No account specified"));
        return false;
    }

    _service->_client->strategyContext()->retrieveAllStrategy.clearSelection();
    _service->_client->strategyContext()->retrieveAllStrategy.setBase(QMailFolderId());
    _service->_client->strategyContext()->retrieveAllStrategy.setQuickList(false);
    _service->_client->strategyContext()->retrieveAllStrategy.setDescending(true);
    _service->_client->strategyContext()->retrieveAllStrategy.setOperation(_service->_client->strategyContext(), QMailRetrievalAction::Auto);
    _service->_client->strategyContext()->retrieveAllStrategy.setIgnoreSyncFlag(false);
    appendStrategy(&_service->_client->strategyContext()->retrieveAllStrategy);
    if(!_unavailable)
        return initiateStrategy();
    return true;
}

void ImapService::Source::queueDisconnectedOperations(const QMailAccountId &accountId)
{
    Q_ASSERT(!_unavailable);
    //sync disconnected move and copy operations for account

    QMailFolderIdList folderList = QMailStore::instance()->queryFolders(QMailFolderKey::parentAccountId(accountId));
    bool pendingDisconnectedOperations = false;
    _service->_client->strategyContext()->moveMessagesStrategy.clearSelection();

    foreach(const QMailFolderId& folderId, folderList) {
        if (!folderId.isValid())
            continue;

        QMailMessageKey movedIntoFolderKey(QMailDisconnected::destinationKey(folderId));
        QMailMessageIdList movedMessages = QMailStore::instance()->queryMessages(movedIntoFolderKey);

        if (movedMessages.isEmpty())
            continue;

        pendingDisconnectedOperations = true;
        _service->_client->strategyContext()->moveMessagesStrategy.appendMessageSet(movedMessages, folderId);
    }

    if (pendingDisconnectedOperations)
        appendStrategy(&_service->_client->strategyContext()->moveMessagesStrategy, SIGNAL(messagesMoved(QMailMessageIdList)));
}

bool ImapService::Source::exportUpdates(const QMailAccountId &accountId)
{
    Q_ASSERT(!_unavailable);
    if (!_service->_client) {
        _service->errorOccurred(QMailServiceAction::Status::ErrFrameworkFault, tr("Account disabled"));
        return false;
    }

    if (!accountId.isValid()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("No account specified"));
        return false;
    }
    
    queueDisconnectedOperations(accountId);

    _service->_client->strategyContext()->exportUpdatesStrategy.clearSelection();
    appendStrategy(&_service->_client->strategyContext()->exportUpdatesStrategy);
    if(!_unavailable)
        return initiateStrategy();
    return true;
}

bool ImapService::Source::synchronize(const QMailAccountId &accountId)
{
    Q_ASSERT(!_unavailable);
    if (!_service->_client) {
        _service->errorOccurred(QMailServiceAction::Status::ErrFrameworkFault, tr("Account disabled"));
        return false;
    }

    if (!accountId.isValid()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("No account specified"));
        return false;
    }

    queueDisconnectedOperations(accountId);

    _service->_client->strategyContext()->synchronizeAccountStrategy.clearSelection();
    _service->_client->strategyContext()->synchronizeAccountStrategy.setBase(QMailFolderId());
    _service->_client->strategyContext()->synchronizeAccountStrategy.setQuickList(false);
    _service->_client->strategyContext()->synchronizeAccountStrategy.setDescending(true);
    _service->_client->strategyContext()->synchronizeAccountStrategy.setOperation(_service->_client->strategyContext(), QMailRetrievalAction::Auto);
    _service->_client->strategyContext()->synchronizeAccountStrategy.setIgnoreSyncFlag(false);
    appendStrategy(&_service->_client->strategyContext()->synchronizeAccountStrategy);
    if(!_unavailable)
        return initiateStrategy();
    return true;
}

bool ImapService::Source::deleteMessages(const QMailMessageIdList &allIds)
{
    Q_ASSERT(!_unavailable);
    // If a server crash has occurred duplicate messages may exist in the store.
    // A duplicate message is one that refers to the same serverUid as another message in the same account & folder.
    // Ensure that when a duplicate message is deleted no message is deleted from the server.

    QMailMessageKey::Properties props(QMailMessageKey::ServerUid | QMailMessageKey::Id);
    QStringList serverUids;
    QMailMessageIdList ids;
    QMailMessageIdList localIds;

    for (const QMailMessageMetaData &metaData : QMailStore::instance()->messagesMetaData(QMailMessageKey::id(allIds), props)) {
        if (!metaData.serverUid().isEmpty()) {
            serverUids.push_back(metaData.serverUid());
            ids.push_back(metaData.id());
        } else {
            localIds.append(metaData.id());
        }
    }
    if (!localIds.isEmpty()) {
        if (!QMailMessageSource::deleteMessages(localIds)) {
            _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("Could not delete messages"));
            return false;
        }
        if (ids.isEmpty())
            return true;
    }

    Q_ASSERT(serverUids.size() == ids.size());
    QMailMessageKey accountKey(QMailMessageKey::parentAccountId(_service->accountId()));

    int matching(QMailStore::instance()->countMessages(QMailMessageKey::serverUid(serverUids, QMailDataComparator::Includes) & accountKey));
    Q_ASSERT(matching >= ids.size());

    if (matching == ids.size()) { // no dupes, lets go
        return doDelete(ids);
    } else {
        QMailMessageIdList duplicateIds;
        QMailMessageIdList singularIds;

        for (int i(0) ; i < ids.size() ; ++i) {
            if (QMailStore::instance()->countMessages(QMailMessageKey::serverUid(serverUids[i]) & accountKey) > 1) {
                duplicateIds.push_back(ids[i]);
            } else {
                singularIds.push_back(ids[i]);
            }
        }
        Q_ASSERT(!duplicateIds.empty());

        if (!QMailMessageSource::deleteMessages(duplicateIds)) {
            _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("Could not delete messages"));
            return false;
        }

        return doDelete(singularIds);
    }
}

bool ImapService::Source::doDelete(const QMailMessageIdList &ids)
{
    Q_ASSERT(!_unavailable);
    if (!_service->_client) {
        _service->errorOccurred(QMailServiceAction::Status::ErrFrameworkFault, tr("Account disabled"));
        return false;
    }

    QMailAccountConfiguration accountCfg(_service->accountId());
    ImapConfiguration imapCfg(accountCfg);
    if (imapCfg.canDeleteMail()) {
        // Delete the messages from the server
        _service->_client->strategyContext()->deleteMessagesStrategy.clearSelection();
        _service->_client->strategyContext()->deleteMessagesStrategy.setLocalMessageRemoval(true);
        _service->_client->strategyContext()->deleteMessagesStrategy.selectedMailsAppend(ids);
        appendStrategy(&_service->_client->strategyContext()->deleteMessagesStrategy, SIGNAL(messagesDeleted(QMailMessageIdList)));
        if(!_unavailable)
            return initiateStrategy();
        return true;
    }

    // Just delete the local copies
    return QMailMessageSource::deleteMessages(ids);
}

bool ImapService::Source::copyMessages(const QMailMessageIdList &messageIds, const QMailFolderId &destinationId)
{
    Q_ASSERT(!_unavailable);
    if (!_service->_client) {
        _service->errorOccurred(QMailServiceAction::Status::ErrFrameworkFault, tr("Account disabled"));
        return false;
    }

    if (messageIds.isEmpty()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("No messages to copy"));
        return false;
    }
    if (!destinationId.isValid()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("Invalid destination folder"));
        return false;
    }

    QMailFolder destination(destinationId);
    if (destination.parentAccountId() == _service->accountId()) {
        _service->_client->strategyContext()->copyMessagesStrategy.clearSelection();
        _service->_client->strategyContext()->copyMessagesStrategy.appendMessageSet(messageIds, destinationId);
        appendStrategy(&_service->_client->strategyContext()->copyMessagesStrategy, SIGNAL(messagesCopied(QMailMessageIdList)));
        if(!_unavailable)
            return initiateStrategy();
        return true;
    }

    // Otherwise create local copies
    return QMailMessageSource::copyMessages(messageIds, destinationId);
}

bool ImapService::Source::moveMessages(const QMailMessageIdList &messageIds, const QMailFolderId &destinationId)
{
    Q_ASSERT(!_unavailable);
    if (!_service->_client) {
        _service->errorOccurred(QMailServiceAction::Status::ErrFrameworkFault, tr("Account disabled"));
        return false;
    }

    if (messageIds.isEmpty()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("No messages to move"));
        return false;
    }
    if (!destinationId.isValid()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("Invalid destination folder"));
        return false;
    }

    QMailFolder destination(destinationId);
    if (destination.parentAccountId() == _service->accountId()) {
        _service->_client->strategyContext()->moveMessagesStrategy.clearSelection();
        _service->_client->strategyContext()->moveMessagesStrategy.appendMessageSet(messageIds, destinationId);
        appendStrategy(&_service->_client->strategyContext()->moveMessagesStrategy, SIGNAL(messagesMoved(QMailMessageIdList)));
        if(!_unavailable)
            return initiateStrategy();
        return true;
    }

    // Otherwise - if any of these messages are in folders on the server, we should remove them
    QMailMessageIdList serverMessages;

    // Do we need to remove these messages from the server?
    QMailAccountConfiguration accountCfg(_service->accountId());
    ImapConfiguration imapCfg(accountCfg);
    if (imapCfg.canDeleteMail()) {
        serverMessages = QMailStore::instance()->queryMessages(QMailMessageKey::id(messageIds) & QMailMessageKey::parentAccountId(_service->accountId()));
        if (!serverMessages.isEmpty()) {
            // Delete the messages from the server
            _service->_client->strategyContext()->deleteMessagesStrategy.clearSelection();
            _service->_client->strategyContext()->deleteMessagesStrategy.setLocalMessageRemoval(false);
            _service->_client->strategyContext()->deleteMessagesStrategy.selectedMailsAppend(serverMessages);
            appendStrategy(&_service->_client->strategyContext()->deleteMessagesStrategy);
            if(!_unavailable)
                initiateStrategy();
        }
    }

    // Move the local copies
    QMailMessageMetaData metaData;
    metaData.setParentFolderId(destinationId);

    // Clear the server UID, because it won't refer to anything useful...
    metaData.setServerUid(QString());

    QMailMessageKey idsKey(QMailMessageKey::id(messageIds));
    if (!QMailStore::instance()->updateMessagesMetaData(idsKey, QMailMessageKey::ParentFolderId | QMailMessageKey::ServerUid, metaData)) {
        qWarning() << "Unable to update message metadata for move to folder:" << destinationId;
    } else {
        emit messagesMoved(messageIds);
    }

    if (serverMessages.isEmpty()) {
        QTimer::singleShot(0, this, SLOT(retrievalCompleted()));
    }
    return true;
}

bool ImapService::Source::flagMessages(const QMailMessageIdList &messageIds, quint64 setMask, quint64 unsetMask)
{
    Q_ASSERT(!_unavailable);
    if (!_service->_client) {
        _service->errorOccurred(QMailServiceAction::Status::ErrFrameworkFault, tr("Account disabled"));
        return false;
    }

    if (messageIds.isEmpty()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("No messages to flag"));
        return false;
    }
    if (!setMask && !unsetMask) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("No flags to be applied"));
        return false;
    }

    // Update the local copy status immediately
    QMailMessageSource::modifyMessageFlags(messageIds, setMask, unsetMask);


    // See if there are any further actions to be taken
    QMailAccountConfiguration accountCfg(_service->accountId());
    ImapConfiguration imapCfg(accountCfg);

    // Note: we can't do everything all at once - just perform the first change that we
    // identify, as a client can always perform the changes incrementally.

    if ((setMask & QMailMessage::Trash) || (unsetMask & QMailMessage::Trash)) {

        QMailFolderId trashId(QMailAccount(_service->accountId()).standardFolder(QMailFolder::TrashFolder));

        if (trashId.isValid()) {

            _setMask = setMask;
            _unsetMask = unsetMask;

            if (setMask & QMailMessage::Trash) {
                _service->_client->strategyContext()->moveMessagesStrategy.clearSelection();
                _service->_client->strategyContext()->moveMessagesStrategy.appendMessageSet(messageIds, trashId);

                appendStrategy(&_service->_client->strategyContext()->moveMessagesStrategy, SIGNAL(messagesFlagged(QMailMessageIdList)));

                if(!_unavailable)
                    return initiateStrategy();
                return true;

            } else if (_unsetMask & QMailMessage::Trash) {

                QMap<QMailFolderId, QMailMessageIdList> destinationList;
                // These messages need to be restored to their previous locations
                destinationList = QMailDisconnected::restoreMap(messageIds);

                _service->_client->strategyContext()->moveMessagesStrategy.clearSelection();
                QMap<QMailFolderId, QMailMessageIdList>::const_iterator it = destinationList.begin(), end = destinationList.end();
                for ( ; it != end; ++it) {
                    _service->_client->strategyContext()->moveMessagesStrategy.appendMessageSet(it.value(), it.key());
                }

                appendStrategy(&_service->_client->strategyContext()->moveMessagesStrategy, SIGNAL(messagesFlagged(QMailMessageIdList)));
                if(!_unavailable)
                    return initiateStrategy();
                return true;
            }
        }
    }

    if (setMask & QMailMessage::Sent) {
        QMailFolderId sentId(QMailAccount(_service->accountId()).standardFolder(QMailFolder::SentFolder));
        if (sentId.isValid()) {
            _setMask = setMask;
            _unsetMask = unsetMask;

            QMailMessageIdList moveIds;
            QMailMessageIdList flagIds;

            QMailMessageKey key(QMailMessageKey::id(messageIds));
            QMailMessageKey::Properties props(QMailMessageKey::Id | QMailMessageKey::ParentFolderId);

            for (const QMailMessageMetaData &metaData : QMailStore::instance()->messagesMetaData(key, props)) {
                // If the message is already in the correct location just update the flags to remove \Draft
                if (metaData.parentFolderId() == sentId) {
                    flagIds.append(metaData.id());
                } else {
                    moveIds.append(metaData.id());
                }
            }

            if (!flagIds.isEmpty()) {
                _service->_client->strategyContext()->flagMessagesStrategy.clearSelection();
                _service->_client->strategyContext()->flagMessagesStrategy.setMessageFlags(MFlag_Draft, false);
                _service->_client->strategyContext()->flagMessagesStrategy.selectedMailsAppend(flagIds);
                appendStrategy(&_service->_client->strategyContext()->flagMessagesStrategy, SIGNAL(messagesFlagged(QMailMessageIdList)));
            }
            if (!moveIds.isEmpty()) {
                _service->_client->strategyContext()->moveMessagesStrategy.clearSelection();
                _service->_client->strategyContext()->moveMessagesStrategy.appendMessageSet(moveIds, sentId);
                appendStrategy(&_service->_client->strategyContext()->moveMessagesStrategy, SIGNAL(messagesFlagged(QMailMessageIdList)));
            }
            if(!_unavailable)
                return initiateStrategy();
            else return true;
        }
    }

    if (setMask & QMailMessage::Draft) {
        QMailFolderId draftId(QMailAccount(_service->accountId()).standardFolder(QMailFolder::DraftsFolder));
        if (draftId.isValid()) {
            _setMask = setMask;
            _unsetMask = unsetMask;

            // Move these messages to the predefined location - if they're already in the drafts
            // folder, we still want to overwrite them with the current content in case it has been updated
            _service->_client->strategyContext()->moveMessagesStrategy.clearSelection();
            _service->_client->strategyContext()->moveMessagesStrategy.appendMessageSet(messageIds, draftId);
            appendStrategy(&_service->_client->strategyContext()->moveMessagesStrategy, SIGNAL(messagesFlagged(QMailMessageIdList)));
            if(!_unavailable)
                return initiateStrategy();
            return true;
        }
    }

    quint64 updatableFlags(QMailMessage::Replied | QMailMessage::RepliedAll | QMailMessage::Forwarded | QMailMessage::Read | QMailMessage::Important);
    if ((setMask & updatableFlags) || (unsetMask & updatableFlags)) {
        // We could hold on to these changes until exportUpdates instead...
        MessageFlags setFlags(0);
        MessageFlags unsetFlags(0);

        if (setMask & (QMailMessage::Replied | QMailMessage::RepliedAll)) {
            setFlags |= MFlag_Answered;
        }
        if (unsetMask & (QMailMessage::Replied | QMailMessage::RepliedAll)) {
            unsetFlags |= MFlag_Answered;
        }
        if (setMask & QMailMessage::Read) {
            setFlags |= MFlag_Seen;
        }
        if (unsetMask & QMailMessage::Read) {
            unsetFlags |= MFlag_Seen;
        }
        if (setMask & QMailMessage::Important) {
            setFlags |= MFlag_Flagged;
        }
        if (unsetMask & QMailMessage::Important) {
            unsetFlags |= MFlag_Flagged;
        }

        if ((setMask | unsetMask) & QMailMessage::Forwarded) {
            // We can only modify this flag if the folders support $Forwarded
            bool supportsForwarded(true);

            QMailMessageKey key(QMailMessageKey::id(messageIds));
            QMailMessageKey::Properties props(QMailMessageKey::Id | QMailMessageKey::ParentFolderId);

            for (const QMailMessageMetaData &metaData : QMailStore::instance()->messagesMetaData(key, props, QMailStore::ReturnDistinct)) {
                QMailFolder folder(metaData.parentFolderId());
                if (folder.customField("qmf-supports-forwarded").isEmpty()) {
                    supportsForwarded = false;
                    break;
                }
            }

            if (supportsForwarded) {
                if (setMask & QMailMessage::Forwarded) {
                    setFlags |= MFlag_Forwarded;
                }
                if (unsetMask & QMailMessage::Forwarded) {
                    unsetFlags |= MFlag_Forwarded;
                }
            }
        }

        if (setFlags || unsetFlags) {
            _service->_client->strategyContext()->flagMessagesStrategy.clearSelection();
            if (setFlags) {
                _service->_client->strategyContext()->flagMessagesStrategy.setMessageFlags(setFlags, true);
            }
            if (unsetFlags) {
                _service->_client->strategyContext()->flagMessagesStrategy.setMessageFlags(unsetFlags, false);
            }
            _service->_client->strategyContext()->flagMessagesStrategy.selectedMailsAppend(messageIds);
            appendStrategy(&_service->_client->strategyContext()->flagMessagesStrategy, SIGNAL(messagesFlagged(QMailMessageIdList)));
            if(!_unavailable)
                return initiateStrategy();
            return true;
        }
    }

    //ensure retrievalCompleted gets called when a strategy has not been used (i.e. local read flag change)
    //otherwise actionCompleted does not get signaled to messageserver and service becomes permanently unavailable
    QTimer::singleShot(0, this, SLOT(retrievalCompleted()));

    return true;
}

bool ImapService::Source::createFolder(const QString &name, const QMailAccountId &accountId, const QMailFolderId &parentId)
{
    Q_ASSERT(!_unavailable);
    if (!_service->_client) {
        _service->errorOccurred(QMailServiceAction::Status::ErrFrameworkFault, tr("Account disabled"));
        return false;
    }

    if (!accountId.isValid()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("No account specified"));
        return false;
    }
    //here we'll create a QMailFolder and give it to the strategy
    //if it is successful, we'll let it register it as a real folder in the QMailStore
    if(name.isEmpty()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("Cannot create empty named folder"));
        return false;
    }
    bool matchFolderRequired = false;
    _service->_client->strategyContext()->createFolderStrategy.createFolder(parentId, name, matchFolderRequired);

    appendStrategy(&_service->_client->strategyContext()->createFolderStrategy);
    if(!_unavailable)
        return initiateStrategy();
    return true;
}

bool ImapService::Source::createStandardFolders(const QMailAccountId &accountId)
{
    Q_ASSERT(!_unavailable);
    if (!_service->_client) {
        _service->errorOccurred(QMailServiceAction::Status::ErrFrameworkFault, tr("Account disabled"));
        return false;
    }

    if (!accountId.isValid()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("No account specified"));
        return false;
    }

    QMailAccount account = QMailAccount(accountId);
    QStringList folderNames;
    QList<QMailFolder::StandardFolder> defaultFolders;
    defaultFolders << QMailFolder::DraftsFolder << QMailFolder::SentFolder <<
                     QMailFolder::TrashFolder << QMailFolder::JunkFolder;

    //fix me create the names from the translations
    foreach (QMailFolder::StandardFolder folder, defaultFolders) {
        QMailFolderId standardFolderId = account.standardFolder(folder);

        if (!standardFolderId.isValid()) {
            switch (folder) {
            case QMailFolder::DraftsFolder:
                folderNames << tr("Drafts");
                break;
            case QMailFolder::SentFolder:
                folderNames << tr("Sent");
                break;
            case QMailFolder::JunkFolder:
                folderNames << tr("Junk");
                break;
            case QMailFolder::TrashFolder:
                folderNames << tr("Trash");
                break;
            default:
                return false;
                break;
            }
        }
    }

    //Create the folder in the root
    QMailFolder dummyParent;
    for (int i = 0; i < folderNames.size(); ++i) {
        qMailLog(Messaging) << "Creating folder: " << folderNames.at(i);
        bool matchFolderRequired = true;
        _service->_client->strategyContext()->createFolderStrategy.createFolder(dummyParent.id(), folderNames.at(i), matchFolderRequired);
    }

    appendStrategy(&_service->_client->strategyContext()->createFolderStrategy);


    if(!_unavailable)
        return initiateStrategy();
    return true;
}

bool ImapService::Source::deleteFolder(const QMailFolderId &folderId)
{
    Q_ASSERT(!_unavailable);
    if (!_service->_client) {
        _service->errorOccurred(QMailServiceAction::Status::ErrFrameworkFault, tr("Account disabled"));
        return false;
    }

    if(!folderId.isValid()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("Deleting invalid folder"));
        return false;
    }

    // Don't delete messages that the user has moved out of the folder
    QMailFolder folder(folderId);
    queueDisconnectedOperations(folder.parentAccountId());
    
    //remove remote copy
    _service->_client->strategyContext()->deleteFolderStrategy.deleteFolder(folderId);
    appendStrategy(&_service->_client->strategyContext()->deleteFolderStrategy);
    if(!_unavailable)
        return initiateStrategy();
    return true;
}

bool ImapService::Source::renameFolder(const QMailFolderId &folderId, const QString &name)
{
    Q_ASSERT(!_unavailable);
    if (!_service->_client) {
        _service->errorOccurred(QMailServiceAction::Status::ErrFrameworkFault, tr("Account disabled"));
        return false;
    }

    if(name.isEmpty()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("Cannot rename to an empty folder"));
        return false;
    }
    if(!folderId.isValid()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("Cannot rename an invalid folder"));
        return false;
    }

    _service->_client->strategyContext()->renameFolderStrategy.renameFolder(folderId, name);

    appendStrategy(&_service->_client->strategyContext()->renameFolderStrategy);
    if(!_unavailable)
        return initiateStrategy();
    return true;
}

bool ImapService::Source::moveFolder(const QMailFolderId &folderId, const QMailFolderId &newParentId)
{
    Q_ASSERT(!_unavailable);
    if (!_service)
        return false;

    if (!_service->_client) {
        _service->errorOccurred(QMailServiceAction::Status::ErrFrameworkFault, tr("Account disabled"));
        return false;
    }

    if (!folderId.isValid()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("Cannot move an invalid folder"));
        return false;
    }

    _service->_client->strategyContext()->moveFolderStrategy.moveFolder(folderId, newParentId);

    appendStrategy(&_service->_client->strategyContext()->moveFolderStrategy);

    if (!_unavailable)
        return initiateStrategy();

    return true;
}

bool ImapService::Source::searchMessages(const QMailMessageKey &searchCriteria, const QString &bodyText, const QMailMessageSortKey &sort)
{
    QMailAccountConfiguration accountCfg(_service->accountId());
    ImapConfiguration imapCfg(accountCfg);
    return searchMessages(searchCriteria, bodyText, imapCfg.searchLimit(), sort, false);
}

bool ImapService::Source::searchMessages(const QMailMessageKey &searchCriteria, const QString &bodyText, quint64 limit, const QMailMessageSortKey &sort)
{
    QMailAccountConfiguration accountCfg(_service->accountId());
    ImapConfiguration imapCfg(accountCfg);
    return searchMessages(searchCriteria, bodyText, limit, sort, false);
}

bool ImapService::Source::countMessages(const QMailMessageKey &searchCriteria, const QString &bodyText)
{
    QMailAccountConfiguration accountCfg(_service->accountId());
    ImapConfiguration imapCfg(accountCfg);
    return searchMessages(searchCriteria, bodyText, 0, QMailMessageSortKey(), true);
}

bool ImapService::Source::searchMessages(const QMailMessageKey &searchCriteria, const QString &bodyText, quint64 limit, const QMailMessageSortKey &sort, bool count)
{
    Q_ASSERT(!_unavailable);
    if (!_service->_client) {
        _service->errorOccurred(QMailServiceAction::Status::ErrFrameworkFault, tr("Account disabled"));
        return false;
    }

    if(searchCriteria.isEmpty() && bodyText.isEmpty()) {
        //we're not going to do an empty search (which returns all emails..)
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("Empty search provided"));
        return false;
    }

    _service->_client->strategyContext()->searchMessageStrategy.searchArguments(searchCriteria, bodyText, limit, sort, count);
    appendStrategy(&_service->_client->strategyContext()->searchMessageStrategy);
    if(!_unavailable)
        initiateStrategy();
    return true;
}

bool ImapService::Source::cancelSearch()
{
    if (!_service->_client) {
        _service->errorOccurred(QMailServiceAction::Status::ErrFrameworkFault, tr("Account disabled"));
        return false;
    }

    _service->_client->strategyContext()->searchMessageStrategy.cancelSearch();
    appendStrategy(&_service->_client->strategyContext()->searchMessageStrategy);
    if(!_unavailable)
        initiateStrategy();
   return true;
}

bool ImapService::Source::prepareMessages(const QList<QPair<QMailMessagePart::Location, QMailMessagePart::Location> > &messageIds)
{
    Q_ASSERT(!_unavailable);
    if (!_service->_client) {
        _service->errorOccurred(QMailServiceAction::Status::ErrFrameworkFault, tr("Account disabled"));
        return false;
    }

    if (messageIds.isEmpty()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("No messages to prepare"));
        return false;
    }

    QList<QPair<QMailMessagePart::Location, QMailMessagePart::Location> > unresolved;
    QSet<QMailMessageId> referringIds;
    QMailMessageIdList externaliseIds;

    QList<QPair<QMailMessagePart::Location, QMailMessagePart::Location> >::const_iterator it = messageIds.begin(), end = messageIds.end();
    for ( ; it != end; ++it) {
        if (!(*it).second.isValid()) {
            // This message just needs to be externalised
            externaliseIds.append((*it).first.containingMessageId());
        } else {
            // This message needs to be made available for another message's reference
            unresolved.append(*it);
            referringIds.insert((*it).second.containingMessageId());
        }
    }

    if (!unresolved.isEmpty()) {
        bool external(false);

        // Are these messages being resolved for internal or external references?
        QMailMessageKey key(QMailMessageKey::id(referringIds.values()));
        QMailMessageKey::Properties props(QMailMessageKey::Id | QMailMessageKey::ParentAccountId | QMailMessageKey::Status);

        for (const QMailMessageMetaData &metaData : QMailStore::instance()->messagesMetaData(key, props)) {
            if ((metaData.parentAccountId() != _service->accountId()) ||
                !(metaData.status() & QMailMessage::TransmitFromExternal)) {
                // This message won't be transmitted by reference from the IMAP server - supply an external reference
                external = true;
                break;
            }
        }

        _service->_client->strategyContext()->prepareMessagesStrategy.setUnresolved(unresolved, external);
        appendStrategy(&_service->_client->strategyContext()->prepareMessagesStrategy, SIGNAL(messagesPrepared(QMailMessageIdList)));
    }

    if (!externaliseIds.isEmpty()) {
        QMailAccountConfiguration accountCfg(_service->accountId());
        ImapConfiguration imapCfg(accountCfg);

        QMailFolderId sentId(QMailAccount(_service->accountId()).standardFolder(QMailFolder::SentFolder));
        // Prepare these messages by copying to the sent folder
        _service->_client->strategyContext()->externalizeMessagesStrategy.clearSelection();
        _service->_client->strategyContext()->externalizeMessagesStrategy.appendMessageSet(externaliseIds, sentId);
        appendStrategy(&_service->_client->strategyContext()->externalizeMessagesStrategy, SIGNAL(messagesPrepared(QMailMessageIdList)));
    }
    if(!_unavailable)
        return initiateStrategy();
    return true;
}

bool ImapService::Source::setStrategy(ImapStrategy *strategy, const char *signal)
{
    qMailLog(Messaging) << "Setting imap strategy" << typeid(*strategy).name();

    disconnect(this, SIGNAL(messageActionCompleted(QMailMessageIdList)));
    if (signal) {
        connect(this, SIGNAL(messageActionCompleted(QMailMessageIdList)), this, signal);
    }

    resetExpiryTimer();
    _unavailable = true;
    _service->_client->setStrategy(strategy);
    _service->_client->newConnection();
    return true;
}

void ImapService::Source::appendStrategy(ImapStrategy *strategy, const char *signal)
{
    _pendingStrategies.append(qMakePair(strategy, QLatin1String(signal)));
}

bool ImapService::Source::initiateStrategy()
{
    if (_pendingStrategies.isEmpty())
        return false;

    QPair<ImapStrategy*, QLatin1String> data(_pendingStrategies.takeFirst());
    return setStrategy(data.first, data.second.latin1());
}

// Copy or Move Completed
void ImapService::Source::messageCopyCompleted(QMailMessage &message, const QMailMessage &original)
{
    if (_service->_client->strategy()->error()) {
        _service->errorOccurred(QMailServiceAction::Status::ErrInvalidData, tr("Destination message failed to match source message"));
        return;
    }
    if (_setMask || _unsetMask) {
        if (_setMask) {
            message.setStatus(_setMask, true);
        }
        if (_unsetMask) {
            message.setStatus(_unsetMask, false);
        }
    }
    Q_UNUSED(original);
}

void ImapService::Source::messageActionCompleted(const QString &uid)
{
    if (uid.startsWith("id:")) {
        emit messageActionCompleted(QMailMessageIdList() << QMailMessageId(uid.mid(3).toULongLong()));
    } else if (!uid.isEmpty()) {
        QMailMessageMetaData metaData(uid, _service->accountId());
        if (metaData.id().isValid()) {
            emit messageActionCompleted(QMailMessageIdList() << metaData.id());
        }
    }
}

void ImapService::Source::retrievalCompleted()
{
    _strategyExpiryTimer.stop();
    _unavailable = false;
    _setMask = 0;
    _unsetMask = 0;

    // See if there are any other actions pending
    if (initiateStrategy()) {
        return;
    }

    if (_queuedMailCheckInProgress) {
        if (_mailCheckPhase == RetrieveFolders) {
            QMailFolderIdList folders;
            _mailCheckPhase = RetrieveMessages;
            if (!_mailCheckFolderId.isValid()) {
                // Full check all folders
                _actionQueue.append(new RetrieveMessageListCommand(_service->accountId(), QMailFolderId(), QMailRetrievalAction::defaultMinimum()));
            } else if (_queuedFoldersFullCheck.contains(_mailCheckFolderId)) {
                // Full check only _mailCheckFolderId
                folders.append(_mailCheckFolderId);
                _actionQueue.append(new RetrieveMessageListsCommand(_service->accountId(), folders, QMailRetrievalAction::defaultMinimum()));
            } else {
                // Retrieve only new mail in _mailCheckFolderId
                folders.append(_mailCheckFolderId);
                _actionQueue.append(new RetrieveNewMessagesCommand(_service->accountId(), folders));
            }
            _queuedFoldersFullCheck.removeAll(_mailCheckFolderId);
            emit _service->actionCompleted(true);
            return;
        } else if (_service->_establishingPushEmail) {
            // Push email is successfully established for all push folders
            _service->_establishingPushEmail = false;
            _service->_pushRetry = ThirtySeconds;
            qMailLog(Messaging) << "Push email established for account" << _service->_accountId 
                                << QMailAccount(_service->_accountId).name();
        }
        _queuedMailCheckInProgress = false;
    }
    emit _service->actionCompleted(true);

    if (_synchronizing) {
        _synchronizing = false;

        // Mark this account as synchronized
        QMailAccount account(_service->accountId());
        if (!(account.status() & QMailAccount::Synchronized)) {
            account.setStatus(QMailAccount::Synchronized, true);
            QMailStore::instance()->updateAccount(&account);
        }
    }

    if (!_queuedFolders.isEmpty()) {
        queueMailCheck(_queuedFolders.first());
    }
}

// Interval mail checking timer has expired, perform mail check on all folders
void ImapService::Source::intervalCheck()
{
    _service->_client->requestRapidClose();
    _actionQueue.append(new ExportUpdatesCommand(_service->accountId())); // Convenient for user to export pending changes also
    queueMailCheck(QMailFolderId()); // Full check including flags
}

// Push interval mail checking timer has expired, perform mail check on all push enabled folders
void ImapService::Source::pushIntervalCheck()
{
    _service->_client->requestRapidClose();
    _actionQueue.append(new ExportUpdatesCommand(_service->accountId())); // Convenient for user to export pending changes also
    QMailFolderIdList ids(_service->_client->configurationIdleFolderIds());
    if (ids.count()) {
        foreach(QMailFolderId id, ids) {
            // Check for flag changes and new mail
            _service->_source->queueFlagsChangedCheck(id);
        }
    }
}

void ImapService::Source::queueMailCheck(QMailFolderId folderId)
{
    if (_unavailable) {
        if (!_queuedFolders.contains(folderId)) {
            _queuedFolders.append(folderId);
        }
        return;
    }

    _queuedFolders.removeAll(folderId);
    _queuedMailCheckInProgress = true;
    _mailCheckPhase = RetrieveFolders;
    _mailCheckFolderId = folderId;

    _service->_client->requestRapidClose();
    if (folderId.isValid()) {
        retrievalCompleted(); // move onto retrieveMessageList stage
    } else {
        _actionQueue.append(new RetrieveFolderListCommand(_service->accountId(), folderId, true)); // Convenient for user to export pending changes also
    }
}

void ImapService::Source::queueFlagsChangedCheck(QMailFolderId folderId)
{
    if (!_queuedFoldersFullCheck.contains(folderId)) {
        _queuedFoldersFullCheck.append(folderId);
    }
    queueMailCheck(folderId);
}

void ImapService::Source::retrievalTerminated()
{
    _strategyExpiryTimer.stop();
    _unavailable = false;
    _synchronizing = false;
    if (_queuedMailCheckInProgress) {
        _queuedMailCheckInProgress = false;
    }
    
    // Just give up if an error occurs
    _queuedFolders.clear();
    _queuedFoldersFullCheck.clear();
    _actionQueue.clear();
}

void ImapService::Source::resetExpiryTimer()
{
    static const int ExpirySeconds = 3600; // Should be larger than imapservice.h value
    _strategyExpiryTimer.start(ExpirySeconds * 1000);
}

void ImapService::Source::expireStrategy()
{
    qMailLog(Messaging) << "IMAP Strategy is not progressing. Internally reseting IMAP service for account" << _service->_accountId;
    _service->disable();
    _service->enable();
}

void ImapService::Source::emitActionSuccessfullyCompleted()
{
    _service->actionCompleted(true);
}

ImapService::ImapService(const QMailAccountId &accountId)
    : QMailMessageService(),
      _accountId(accountId),
      _client(0),
      _source(new Source(this)),
      _restartPushEmailTimer(new QTimer(this)),
      _establishingPushEmail(false),
      _idling(false),
      _accountWasPushEnabled(false),
      _networkSession(0),
      _networkSessionTimer(new QTimer(this))
{
    QMailAccount account(accountId);
    if (account.status() & QMailAccount::Enabled) {
        enable();
    }
    connect(_restartPushEmailTimer, SIGNAL(timeout()), this, SLOT(restartPushEmail()));
    connect(QMailStore::instance(), SIGNAL(accountsUpdated(const QMailAccountIdList&)), 
            this, SLOT(accountsUpdated(const QMailAccountIdList&)));
}

void ImapService::enable()
{
    qMailLog(IMAP) << "account enabled, initiating a new client.";
    _client = new ImapClient(_accountId, this);
    _source->initClientConnections();
    _establishingPushEmail = false;
    connect(_client, SIGNAL(progressChanged(uint, uint)), this, SIGNAL(progressChanged(uint, uint)));
    connect(_client, SIGNAL(progressChanged(uint, uint)), _source, SLOT(resetExpiryTimer()));
    connect(_client, SIGNAL(errorOccurred(int, QString)), this, SLOT(errorOccurred(int, QString)));
    connect(_client, SIGNAL(errorOccurred(QMailServiceAction::Status::ErrorCode, QString)), this, SLOT(errorOccurred(QMailServiceAction::Status::ErrorCode, QString)));
    connect(_client, SIGNAL(updateStatus(QString)), this, SLOT(updateStatus(QString)));
    connect(_client, &ImapClient::pushEmailError, this, &ImapService::retryPushEmail);

    QMailAccountConfiguration accountCfg(_accountId);
    ImapConfiguration imapCfg(accountCfg);
    _source->setIntervalTimer(imapCfg.checkInterval());

    if (imapCfg.pushEnabled()) {
        enablePushEmail();
    }
}

void ImapService::enablePushEmail()
{
    QMailAccountConfiguration accountCfg(_accountId);
    ImapConfiguration imapCfg(accountCfg);

    int allowedConnections = reservePushConnections(imapCfg.pushFolders().count());
    if (allowedConnections > 0) {
        _client->setPushConnectionsReserved(allowedConnections);
        _accountWasPushEnabled = true;
        _previousPushFolders = imapCfg.pushFolders();
        _pushRetry = ThirtySeconds;

        initiatePushEmail();
    }
}

void ImapService::disable()
{
    disablePushEmail();
    _source->setIntervalTimer(0);
    _source->setPushIntervalTimer(0);
    _source->retrievalTerminated();
    qMailLog(IMAP) << "account disabled, terminating the client.";
    delete _client;
    _client = 0;
}

void ImapService::disablePushEmail()
{
    QMailAccountConfiguration accountCfg(_accountId);
    ImapConfiguration imapCfg(accountCfg);
    _restartPushEmailTimer->stop();
    setPersistentConnectionStatus(false);
    _accountWasPushEnabled = false;
    if (_client && _client->pushConnectionsReserved() > 0) {
        releasePushConnections(_client->pushConnectionsReserved());
        _client->setPushConnectionsReserved(0);
        _client->stopIdleConnections();
    }
    closeIdleSession();
}

void ImapService::accountsUpdated(const QMailAccountIdList &ids)
{
    if (!ids.contains(_accountId))
        return;

    QMailAccountConfiguration accountCfg(_accountId);
    ImapConfiguration imapCfg(accountCfg);

    QMailAccount account(_accountId);
    bool isEnabled(account.status() & QMailAccount::Enabled);
    if (isEnabled && _client) {
        bool isPushEnabled(imapCfg.pushEnabled());
        if (isPushEnabled && _accountWasPushEnabled) {
            const QStringList pushFolders(imapCfg.pushFolders());
            if (_previousPushFolders != pushFolders) {
                _previousPushFolders = pushFolders;
                restartPushEmail();
            }
        } else if (isPushEnabled && !_accountWasPushEnabled) {
            enablePushEmail();
        } else if (!isPushEnabled && _accountWasPushEnabled) {
            disablePushEmail();
        }
    } else if (isEnabled && !_client) {
        enable();
    } else if (!isEnabled && _client) {
        cancelOperation(QMailServiceAction::Status::ErrConfiguration,
                        tr("Account disabled"));
        disable();
    }
    
    _source->setIntervalTimer(imapCfg.checkInterval());
}

ImapService::~ImapService()
{
    disable();
    closeIdleSession();
    delete _source;
}

QString ImapService::service() const
{
    return serviceKey;
}

QMailAccountId ImapService::accountId() const
{
    return _accountId;
}

bool ImapService::hasSource() const
{
    return true;
}

QMailMessageSource &ImapService::source() const
{
    return *_source;
}

bool ImapService::available() const
{
    return true;
}

bool ImapService::cancelOperation(QMailServiceAction::Status::ErrorCode code, const QString &text)
{
    if (!_client) {
        errorOccurred(QMailServiceAction::Status::ErrFrameworkFault, tr("Account disabled"));
        return false;
    }

    _client->cancelTransfer(code, text);
    _client->closeConnection();
    _client->stopIdleConnections();
    _source->retrievalTerminated();
    return true;
}

void ImapService::restartPushEmail()
{
    if (!_establishingPushEmail) {
        qMailLog(Messaging) << "Attempting to restart push email for account" << _accountId
                            << QMailAccount(_accountId).name();
        cancelOperation(QMailServiceAction::Status::ErrInternalStateReset, tr("Initiating push email"));
        initiatePushEmail();
    }
}
    
void ImapService::initiatePushEmail()
{
    _restartPushEmailTimer->stop();
    setPersistentConnectionStatus(false);

    if (!_networkSession || _networkSession->state() != IdleNetworkSession::Connected) {
        createIdleSession();
        return;
    }

    qMailLog(Messaging) << "Attempting to establish push email for account" << _accountId
                        << QMailAccount(_accountId).name();
    QMailFolderIdList ids(_client->configurationIdleFolderIds());
    if (ids.count()) {
        _establishingPushEmail = true;
        setPersistentConnectionStatus(true);

        foreach(QMailFolderId id, ids) {
            // Check for flag changes and new mail
            _source->queueFlagsChangedCheck(id);
        }
    }
    // Interval check to update flags on servers that do not push flag changes when idling e.g. gmail
    _source->setPushIntervalTimer(60); // minutes    
}

void ImapService::retryPushEmail()
{
    if (!_restartPushEmailTimer->isActive()) {
        const int oneHour = 60*60;
        qMailLog(Messaging) << "Push email connection could not be established. Reattempting to establish in" << _pushRetry << "seconds";
        _restartPushEmailTimer->start(_pushRetry*1000);
        _pushRetry = qMin(oneHour, _pushRetry * 2);
    }
    _establishingPushEmail = false;
}

void ImapService::errorOccurred(int code, const QString &text)
{
    if (_establishingPushEmail
        && (!_client || !_client->isPushEmailEstablished())) {
        retryPushEmail();
    }
    _source->retrievalTerminated();
    updateStatus(code, text, _accountId);
    emit actionCompleted(false);
}

void ImapService::errorOccurred(QMailServiceAction::Status::ErrorCode code, const QString &text)
{
    if (_establishingPushEmail
        && (!_client || !_client->isPushEmailEstablished())) {
        retryPushEmail();
    }
    _source->retrievalTerminated();
    updateStatus(code, text, _accountId);
    emit actionCompleted(false);
}

void ImapService::updateStatus(const QString &text)
{
    updateStatus(QMailServiceAction::Status::ErrNoError, text, _accountId);
}

void ImapService::createIdleSession()
{
    // Fail after 10 sec if no network reply is received
    _networkSessionTimer->setSingleShot(true);
    _networkSessionTimer->setInterval(10000);
    connect(_networkSessionTimer,SIGNAL(timeout()),
            this,SLOT(onSessionConnectionTimeout()));
    openIdleSession();
}

void ImapService::openIdleSession()
{
    closeIdleSession();

    qMailLog(Messaging) << "IDLE Session: Opening...";
    _networkSession = new IdleNetworkSession(this);

    connect(_networkSession, &IdleNetworkSession::errorChanged,
            this, &ImapService::onSessionError);
    connect(_networkSession, &IdleNetworkSession::opened,
            this, &ImapService::onSessionOpened);

    _networkSession->open();

    // This timer will cancel the IDLE session if no network
    // connection can be established in a given amount of time.
    _networkSessionTimer->start();
}

void ImapService::closeIdleSession()
{
    if (_networkSession) {
        qMailLog(Messaging) << "IDLE Session: Closing...";
        _networkSession->disconnect();
        _networkSession->close();
        delete _networkSession;
        _networkSession = 0;
    }
    _networkSessionTimer->stop();
    _networkSessionTimer->disconnect();
}

void ImapService::onOnlineStateChanged(bool isOnline)
{
    qMailLog(Messaging) << "IDLE Session: Network state changed: " << isOnline;
    if (accountPushEnabled() && isOnline
        && (!_networkSession
            || _networkSession->state() != IdleNetworkSession::Connected)) {
        openIdleSession();
    } else if (!isOnline) {
        onSessionError(IdleNetworkSession::InvalidConfigurationError);
        closeIdleSession();
    }
}

void ImapService::onSessionOpened()
{
    if (!_networkSession || sender() != _networkSession) return;

    // stop timer
    _networkSessionTimer->stop();
    _networkSessionTimer->disconnect();

    qMailLog(Messaging) << "IDLE session opened, state" << _networkSession->state();
    connect(_networkSession, &IdleNetworkSession::stateChanged,
            this, &ImapService::onSessionStateChanged);

    if (accountPushEnabled() && !_idling) {
        restartPushEmail();
    }
}

void ImapService::onSessionStateChanged(IdleNetworkSession::State status)
{

    qMailLog(Messaging) << "IDLE session state now: " << status;
    if (status != IdleNetworkSession::Connecting
            && status != IdleNetworkSession::Connected
            && status != IdleNetworkSession::Closing
            && status != IdleNetworkSession::Disconnected) {
        onSessionError(IdleNetworkSession::InvalidConfigurationError);
    }
}

void ImapService::onSessionError(IdleNetworkSession::Error error)
{
    switch (error) {
    case IdleNetworkSession::UnknownError:
        qMailLog(Messaging) << "IDLE session error: IdleNetworkSession::UnknownError";
        break;
    case IdleNetworkSession::AbortedError:
        qMailLog(Messaging) << "IDLE session error: IdleNetworkSession::AbortedError";
        break;
    case IdleNetworkSession::RoamingError:
        qMailLog(Messaging) << "IDLE session error: IdleNetworkSession::RoamingError";
        break;
    case IdleNetworkSession::OperationNotSupportedError:
        qMailLog(Messaging) << "IDLE session error: IdleNetworkSession::OperationNotSupportedError";
        break;
    case IdleNetworkSession::InvalidConfigurationError:
        qMailLog(Messaging) << "IDLE session error: IdleNetworkSession::InvalidConfigurationError";
        break;
    default:
        qMailLog(Messaging) << "IDLE session error: IdleNetworkSession:: Invalid error code";
        break;
    }

    if (_client) {
        emit _client->sessionError();
    }
    disablePushEmail();
}

void ImapService::onSessionConnectionTimeout()
{
    if (_networkSession) {
        if (!_networkSession->isOpen()) {
            qWarning() << "IDLE session error: No network reply received after 10 seconds";
            onSessionError(_networkSession->error());
        }
    }
}

bool ImapService::accountPushEnabled()
{
    QMailAccountConfiguration accountCfg(_accountId);
    ImapConfiguration imapCfg(accountCfg);
    return imapCfg.pushEnabled();
}

void ImapService::setPersistentConnectionStatus(bool status)
{
    QMailAccount account(_accountId);
    if (static_cast<bool>(account.status() & QMailAccount::HasPersistentConnection) != status) {
        account.setStatus(QMailAccount::HasPersistentConnection, status);
        if (!status) {
            account.setLastSynchronized(QMailTimeStamp::currentDateTime());
        }
        if (!QMailStore::instance()->updateAccount(&account)) {
            qWarning() << "Unable to update account" << account.id() << "to HasPersistentConnection" << status;
        } else {
            qMailLog(Messaging) << "HasPersistentConnection for" << account.id() << "changed to" << status;
        }
    }
    _idling = status;
}

class ImapConfigurator : public QMailMessageServiceConfigurator
{
public:
    ImapConfigurator();
    ~ImapConfigurator();

    QString service() const override;
    QString displayName() const override;

#ifndef QMF_NO_WIDGETS
    QMailMessageServiceEditor *createEditor(QMailMessageServiceFactory::ServiceType type) override;
#endif
};

ImapConfigurator::ImapConfigurator()
{
}

ImapConfigurator::~ImapConfigurator()
{
}

QString ImapConfigurator::service() const
{
    return serviceKey;
}

QString ImapConfigurator::displayName() const
{
    return QCoreApplication::instance()->translate("QMailMessageService", "IMAP");
}

#ifndef QMF_NO_WIDGETS
QMailMessageServiceEditor *ImapConfigurator::createEditor(QMailMessageServiceFactory::ServiceType type)
{
    if (type == QMailMessageServiceFactory::Source)
        return new ImapSettings;

    return 0;
}
#endif

ImapServicePlugin::ImapServicePlugin()
    : QMailMessageServicePlugin()
{
}

QString ImapServicePlugin::key() const
{
    return serviceKey;
}

bool ImapServicePlugin::supports(QMailMessageServiceFactory::ServiceType type) const
{
    return (type == QMailMessageServiceFactory::Source);
}

bool ImapServicePlugin::supports(QMailMessage::MessageType type) const
{
    return (type == QMailMessage::Email);
}

QMailMessageService *ImapServicePlugin::createService(const QMailAccountId &id)
{
    return new ImapService(id);
}

QMailMessageServiceConfigurator *ImapServicePlugin::createServiceConfigurator()
{
    return new ImapConfigurator();
}

#include "imapservice.moc"

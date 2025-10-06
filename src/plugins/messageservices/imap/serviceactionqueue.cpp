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

#include "serviceactionqueue.h"

// Auto executing queue of service actions

ServiceActionQueue::ServiceActionQueue()
    : _running(false)
{
    QObject::connect(&_timer, SIGNAL(timeout()), this, SLOT(executeNextCommand()));
}

// Takes ownership of *command.
void ServiceActionQueue::append(ServiceActionCommand *command)
{
    _commands.append(command);
    if (!_running) {
        _timer.start(0);
    }
}

void ServiceActionQueue::executeNextCommand()
{
    _timer.stop();
    if (_running) {
        return;
    }
    if (_commands.isEmpty()) {
        return;
    }
    _running = true;
    ServiceActionCommand *command(_commands.first());
    QObject::connect(command->action(), SIGNAL(activityChanged(QMailServiceAction::Activity)),
                     this, SLOT(activityChanged(QMailServiceAction::Activity)));
    command->execute();
}

void ServiceActionQueue::activityChanged(QMailServiceAction::Activity activity)
{
    if ((activity == QMailServiceAction::Successful)
        || (activity == QMailServiceAction::Failed)) {
        delete _commands.takeFirst();
        _running = false;
        _timer.start(0);
    }
}

void ServiceActionQueue::clear()
{
    qDeleteAll(_commands);
    _commands.clear();
    _timer.stop();
    _running = false;
}

// Command pattern definitions for various service action classes

ExportUpdatesCommand::ExportUpdatesCommand(const QMailAccountId &accountId)
{
    _action = QPointer<QMailServiceAction>(new QMailRetrievalAction()); _accountId = accountId;
}

void ExportUpdatesCommand::execute()
{
    static_cast<QMailRetrievalAction*>(_action.data())->exportUpdates(_accountId);
}

RetrieveFolderListCommand::RetrieveFolderListCommand(const QMailAccountId &accountId, const QMailFolderId &folderId, bool descending)
{
    _action = QPointer<QMailServiceAction>(new QMailRetrievalAction());
    _accountId = accountId;
    _folderId = folderId;
    _descending = descending;
}

void RetrieveFolderListCommand::execute()
{
    static_cast<QMailRetrievalAction*>(_action.data())->retrieveFolderList(_accountId, QMailFolderId());
}

RetrieveMessageListCommand::RetrieveMessageListCommand(const QMailAccountId &accountId, const QMailFolderId &folderId, uint minimum, const QMailMessageSortKey &sort)
{
    _action = QPointer<QMailServiceAction>(new QMailRetrievalAction());
    _accountId = accountId;
    _folderId = folderId;
    _minimum = minimum;
    _sort = sort;
}

void RetrieveMessageListCommand::execute()
{
    static_cast<QMailRetrievalAction*>(_action.data())->retrieveMessageList(_accountId, _folderId, _minimum, _sort);
}

RetrieveMessageListsCommand::RetrieveMessageListsCommand(const QMailAccountId &accountId, const QMailFolderIdList &folderIds, uint minimum, const QMailMessageSortKey &sort)
{
    _action = QPointer<QMailServiceAction>(new QMailRetrievalAction());
    _accountId = accountId;
    _folderIds = folderIds;
    _minimum = minimum;
    _sort = sort;
}

void RetrieveMessageListsCommand::execute()
{
    static_cast<QMailRetrievalAction*>(_action.data())->retrieveMessageLists(_accountId, _folderIds, _minimum, _sort);
}

RetrieveNewMessagesCommand::RetrieveNewMessagesCommand(const QMailAccountId &accountId, const QMailFolderIdList &folderIds)
{
    _action = QPointer<QMailServiceAction>(new QMailRetrievalAction());
    _accountId = accountId;
    _folderIds = folderIds;
}

void RetrieveNewMessagesCommand::execute()
{
    static_cast<QMailRetrievalAction*>(_action.data())->retrieveNewMessages(_accountId, _folderIds);
}

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

#ifndef SERVICEACTIONQUEUE_H
#define SERVICEACTIONQUEUE_H

#include <qmailserviceaction.h>
#include <qpointer.h>
#include <qtimer.h>

class ServiceActionCommand
{
public:
    virtual ~ServiceActionCommand() { if (!_action.isNull()) _action->deleteLater(); }
    virtual void execute() = 0;
    QPointer<QMailServiceAction> action() { return _action; }

protected:
    QPointer<QMailServiceAction> _action;
};

class ServiceActionQueue : public QObject
{
    Q_OBJECT
    
public:
    ServiceActionQueue();
    void append(ServiceActionCommand *command);
    void clear();

private slots:
    void executeNextCommand();
    void activityChanged(QMailServiceAction::Activity activity);

private:
    bool _running;
    QTimer _timer;
    QList<ServiceActionCommand*> _commands;
};

class ExportUpdatesCommand : public ServiceActionCommand
{
public:
    ExportUpdatesCommand(const QMailAccountId &accountId);
    void execute();

private:
    QMailAccountId _accountId;
};

class RetrieveFolderListCommand : public ServiceActionCommand
{
public:
    RetrieveFolderListCommand(const QMailAccountId &accountId, const QMailFolderId &folderId, bool descending = true);
    void execute();

private:
    QMailAccountId _accountId;
    QMailFolderId _folderId;
    bool _descending;
};

class RetrieveMessageListCommand : public ServiceActionCommand
{
public:
    RetrieveMessageListCommand(const QMailAccountId &accountId, const QMailFolderId &folderId, uint minimum = 0, const QMailMessageSortKey &sort = QMailMessageSortKey());
    void execute();

private:
    QMailAccountId _accountId;
    QMailFolderId _folderId;
    uint _minimum;
    QMailMessageSortKey _sort;
};

class RetrieveMessageListsCommand : public ServiceActionCommand
{
public:
    RetrieveMessageListsCommand(const QMailAccountId &accountId, const QMailFolderIdList &folderIds, uint minimum = 0, const QMailMessageSortKey &sort = QMailMessageSortKey());
    void execute();

private:
    QMailAccountId _accountId;
    QMailFolderIdList _folderIds;
    uint _minimum;
    QMailMessageSortKey _sort;
};

class RetrieveNewMessagesCommand : public ServiceActionCommand
{
public:
    RetrieveNewMessagesCommand(const QMailAccountId &accountId, const QMailFolderIdList &folderIds);
    void execute();

private:
    QMailAccountId _accountId;
    QMailFolderIdList _folderIds;
};
#endif // SERVICEACTIONQUEUE_H

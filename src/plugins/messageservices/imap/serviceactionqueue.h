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

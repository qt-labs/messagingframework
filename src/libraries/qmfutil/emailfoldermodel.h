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

#ifndef EMAILFOLDERMODEL_H
#define EMAILFOLDERMODEL_H

#include "foldermodel.h"
#include <qmailfolder.h>

// A message set that returns only the messages matching a specific status field

class QMFUTIL_EXPORT EmailStandardFolderMessageSet : public QMailFilterMessageSet
{
    Q_OBJECT

public:
    EmailStandardFolderMessageSet(QMailMessageSetContainer *container, QMailFolder::StandardFolder folderType, const QString &name);

    virtual QMailFolder::StandardFolder standardFolderType() const;

    static QMailMessageKey contentKey(QMailFolder::StandardFolder folderType);

protected:
    QMailFolder::StandardFolder _type;
};


// A message set that returns only the email messages within a folder:

class QMFUTIL_EXPORT EmailFolderMessageSet : public QMailFolderMessageSet
{
    Q_OBJECT

public:
    EmailFolderMessageSet(QMailMessageSetContainer *container, const QMailFolderId &folderId, bool hierarchical);

    virtual QMailMessageKey messageKey() const;
    virtual QMailMessageKey descendantsMessageKey() const;

    static QMailMessageKey contentKey(const QMailFolderId &id, bool descendants);

protected:
    virtual void createChild(const QMailFolderId &childId);
};


// An account folder element which returns only email messages

class QMFUTIL_EXPORT EmailAccountMessageSet : public QMailAccountMessageSet
{
    Q_OBJECT

public:
    EmailAccountMessageSet(QMailMessageSetContainer *container, const QMailAccountId &accountId);

    virtual QMailMessageKey messageKey() const;
    virtual QMailMessageKey descendantsMessageKey() const;

    static QMailMessageKey contentKey(const QMailAccountId &id);

protected:
    virtual void createChild(const QMailFolderId &childId);
};


// A folder element which includes all email accounts as sub-folders:

class QMFUTIL_EXPORT InboxMessageSet : public EmailStandardFolderMessageSet
{
    Q_OBJECT

public:
    InboxMessageSet(QMailMessageSetContainer *container);

    virtual QMailMessageKey messageKey() const;

    static QMailMessageKey contentKey();

protected slots:
    virtual void accountsAdded(const QMailAccountIdList &ids);
    virtual void accountsUpdated(const QMailAccountIdList &ids);
    virtual void accountsRemoved(const QMailAccountIdList &ids);
    virtual void accountContentsModified(const QMailAccountIdList &ids);

protected:
    virtual void init();
    virtual void synchronizeAccountChildren();
    virtual void resyncState();

    static QMailAccountKey emailAccountKey();

protected:
    QMailAccountIdList _accountIds;
};


class QMFUTIL_EXPORT EmailFolderModel : public FolderModel
{
    Q_OBJECT

public:
    using FolderModel::data;

    enum Roles
    {
        FolderIconRole = FolderModel::FolderIconRole,
        FolderStatusRole = FolderModel::FolderStatusRole,
        FolderStatusDetailRole = FolderModel::FolderStatusDetailRole,
        FolderIdRole = FolderModel::FolderIdRole,
        FolderSynchronizationEnabledRole,
        ContextualAccountIdRole,
    };

    EmailFolderModel(QObject *parent = 0);
    ~EmailFolderModel();

    virtual void init();

    virtual QVariant data(QMailMessageSet *item, int role, int column) const;
    virtual QVariant headerData(int section, Qt::Orientation, int role) const;

protected:
    virtual QIcon itemIcon(QMailMessageSet *item) const;
    virtual QString itemStatusDetail(QMailMessageSet *item) const;
    virtual FolderModel::StatusText itemStatusText(QMailMessageSet *item) const;

    virtual QIcon standardFolderIcon(EmailStandardFolderMessageSet *item) const;
    virtual QIcon emailFolderIcon(EmailFolderMessageSet *item) const;

    virtual FolderModel::StatusText standardFolderStatusText(EmailStandardFolderMessageSet *item) const;

    virtual bool itemSynchronizationEnabled(QMailMessageSet *item) const;
    virtual QMailAccountId itemContextualAccountId(QMailMessageSet *item) const;
};

class QMFUTIL_EXPORT AccountFolderModel : public EmailFolderModel
{
    Q_OBJECT

public:
    AccountFolderModel(const QMailAccountId &id, QObject *parent = 0);

    virtual void init();

protected:
    QMailAccountId accountId;
};

#endif


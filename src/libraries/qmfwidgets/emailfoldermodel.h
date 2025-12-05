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

    QMailMessageKey messageKey() const override;
    QMailMessageKey descendantsMessageKey() const override;

    static QMailMessageKey contentKey(const QMailFolderId &id, bool descendants);

protected:
    void createChild(const QMailFolderId &childId) override;
};


// An account folder element which returns only email messages

class QMFUTIL_EXPORT EmailAccountMessageSet : public QMailAccountMessageSet
{
    Q_OBJECT

public:
    EmailAccountMessageSet(QMailMessageSetContainer *container, const QMailAccountId &accountId);

    QMailMessageKey messageKey() const override;
    QMailMessageKey descendantsMessageKey() const override;

    static QMailMessageKey contentKey(const QMailAccountId &id);

protected:
    void createChild(const QMailFolderId &childId) override;
};


// A folder element which includes all email accounts as sub-folders:

class QMFUTIL_EXPORT InboxMessageSet : public EmailStandardFolderMessageSet
{
    Q_OBJECT

public:
    InboxMessageSet(QMailMessageSetContainer *container);

    QMailMessageKey messageKey() const override;

    static QMailMessageKey contentKey();

protected Q_SLOTS:
    virtual void accountsAdded(const QMailAccountIdList &ids);
    virtual void accountsUpdated(const QMailAccountIdList &ids);
    virtual void accountsRemoved(const QMailAccountIdList &ids);
    virtual void accountContentsModified(const QMailAccountIdList &ids);

protected:
    void init() override;
    virtual void synchronizeAccountChildren();
    void resyncState() override;

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
        FolderDeletionPermittedRole,
        FolderChildCreationPermittedRole,
        FolderRenamePermittedRole
    };

    EmailFolderModel(QObject *parent = nullptr);
    ~EmailFolderModel();

    virtual void init();

    QVariant data(QMailMessageSet *item, int role, int column) const override;
    QVariant headerData(int section, Qt::Orientation, int role) const override;

protected:
    QIcon itemIcon(QMailMessageSet *item) const override;
    QString itemStatusDetail(QMailMessageSet *item) const override;
    FolderModel::StatusText itemStatusText(QMailMessageSet *item) const override;

    virtual QIcon standardFolderIcon(EmailStandardFolderMessageSet *item) const;
    virtual QIcon emailFolderIcon(EmailFolderMessageSet *item) const;

    virtual FolderModel::StatusText standardFolderStatusText(EmailStandardFolderMessageSet *item) const;

    virtual bool itemSynchronizationEnabled(QMailMessageSet *item) const;
    virtual bool itemPermitted(QMailMessageSet *item, Roles role) const;
    virtual QMailAccountId itemContextualAccountId(QMailMessageSet *item) const;
};

class QMFUTIL_EXPORT AccountFolderModel : public EmailFolderModel
{
    Q_OBJECT

public:
    explicit AccountFolderModel(const QMailAccountId &id, QObject *parent = nullptr);

    void init() override;

protected:
    QMailAccountId accountId;
};

#endif


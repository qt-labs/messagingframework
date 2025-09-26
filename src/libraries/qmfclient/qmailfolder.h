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

#ifndef QMAILFOLDER_H
#define QMAILFOLDER_H

#include "qmailglobal.h"
#include "qmailid.h"

#include <QString>
#include <QList>
#include <QSharedData>

class QMailFolderPrivate;

class QMF_EXPORT QMailFolder
{
public:
    enum StandardFolder {
        InboxFolder = 1,
        OutboxFolder,
        DraftsFolder,
        SentFolder,
        TrashFolder,
        JunkFolder
    };

    static const quint64 &SynchronizationEnabled;
    static const quint64 &Synchronized;
    static const quint64 &PartialContent;
    static const quint64 &Removed;
    static const quint64 &Incoming;
    static const quint64 &Outgoing;
    static const quint64 &Sent;
    static const quint64 &Trash;
    static const quint64 &Drafts;
    static const quint64 &Junk;
    static const quint64 &ChildCreationPermitted;
    static const quint64 &RenamePermitted;
    static const quint64 &DeletionPermitted;
    static const quint64 &NonMail;
    static const quint64 &MessagesPermitted;
    static const quint64 &ReadOnly;
    static const quint64 &Favourite;

    QMailFolder();

    QMailFolder(const QString& name, const QMailFolderId& parentFolderId = QMailFolderId(), const QMailAccountId& parentAccountId = QMailAccountId());
    explicit QMailFolder(const QMailFolderId& id);
    QMailFolder(const QMailFolder& other);

    virtual ~QMailFolder();

    QMailFolder& operator=(const QMailFolder& other);

    QMailFolderId id() const;
    void setId(const QMailFolderId& id);

    QString path() const;
    void setPath(const QString& path);

    QString displayName() const;
    void setDisplayName(const QString& name);

    QMailFolderId parentFolderId() const;
    void setParentFolderId(const QMailFolderId& id);

    QMailAccountId parentAccountId() const;
    void setParentAccountId(const QMailAccountId& id);

    quint64 status() const;
    void setStatus(quint64 newStatus);
    void setStatus(quint64 mask, bool set);

    static quint64 statusMask(const QString &flagName);

    uint serverCount() const;
    void setServerCount(uint count);

    uint serverUnreadCount() const;
    void setServerUnreadCount(uint count);

    uint serverUndiscoveredCount() const;
    void setServerUndiscoveredCount(uint count);

    QString customField(const QString &name) const;
    void setCustomField(const QString &name, const QString &value);
    void setCustomFields(const QMap<QString, QString> &fields);

    void removeCustomField(const QString &name);

    const QMap<QString, QString> &customFields() const;

private:
    friend class QMailStore;
    friend class QMailStorePrivate;
    friend class QMailStoreSql;

    bool customFieldsModified() const;
    void setCustomFieldsModified(bool set);

    QSharedDataPointer<QMailFolderPrivate> d;
};

typedef QList<QMailFolder> QMailFolderList;

#endif

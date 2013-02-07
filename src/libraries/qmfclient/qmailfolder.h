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

#ifndef QMAILFOLDER_H
#define QMAILFOLDER_H

#include "qmailglobal.h"
#include "qmailid.h"
#include "qmailfolderfwd.h"
#include <QString>
#include <QList>
#include <QSharedData>

class QMailFolderPrivate;

class QMF_EXPORT QMailFolder : public QMailFolderFwd
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

    bool customFieldsModified() const;
    void setCustomFieldsModified(bool set);

    QSharedDataPointer<QMailFolderPrivate> d;
};

typedef QList<QMailFolder> QMailFolderList;

#endif

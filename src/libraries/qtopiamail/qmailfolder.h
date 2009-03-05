/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef QMAILFOLDER_H
#define QMAILFOLDER_H

#include "qmailid.h"
#include <QString>
#include <QList>
#include <QSharedData>
#include "qmailglobal.h"

class QMailFolderPrivate;

class QTOPIAMAIL_EXPORT QMailFolder
{
public:
    enum StandardFolder
    {
        InboxFolder = 1,
        OutboxFolder = 2,
        DraftsFolder = 3,
        SentFolder = 4,
        TrashFolder = 5
    };

    static const quint64 &SynchronizationEnabled;
    static const quint64 &Synchronized;
    static const quint64 &PartialContent;

    QMailFolder();

    QMailFolder(const QString& name, const QMailFolderId& parentFolderId = QMailFolderId(), const QMailAccountId& parentAccountId = QMailAccountId());
    explicit QMailFolder(const StandardFolder& sf);
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

    static void initStore();

    QSharedDataPointer<QMailFolderPrivate> d;
};

typedef QList<QMailFolder> QMailFolderList;

#endif

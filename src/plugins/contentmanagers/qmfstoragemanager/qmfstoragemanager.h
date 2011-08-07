/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QMFSTORAGEMANAGER_H
#define QMFSTORAGEMANAGER_H

#include <qmailid.h>
#include <qmailcontentmanager.h>
#include <QList>
#include <QMap>
#include <QSharedPointer>

class QMailMessagePart;
class QMailMessagePartContainer;

QT_BEGIN_NAMESPACE

#ifdef SYMBIAN_USE_DATA_CAGED_FILES
class SymbianFile;
#else
class QFile;
#endif

QT_END_NAMESPACE

class QmfStorageManager : public QObject, public QMailContentManager
{
    Q_OBJECT

public:
    QmfStorageManager(QObject *parent = 0);
    ~QmfStorageManager();

    QMailStore::ErrorCode add(QMailMessage *message, QMailContentManager::DurabilityRequirement durability);
    QMailStore::ErrorCode update(QMailMessage *message, QMailContentManager::DurabilityRequirement durability);

    QMailStore::ErrorCode ensureDurability();
    QMailStore::ErrorCode ensureDurability(const QList<QString> &identifiers);

    QMailStore::ErrorCode remove(const QString &identifier);
    QMailStore::ErrorCode load(const QString &identifier, QMailMessage *message);

    bool init();
    void clearContent();

    static const QString &messagesBodyPath(const QMailAccountId &accountId);
    static QString messageFilePath(const QString &fileName, const QMailAccountId &accountId);
    static QString messagePartDirectory(const QString &fileName);
    static QString messagePartFilePath(const QMailMessagePart &part, const QString &fileName);

    virtual ManagerRole role() const { return StorageRole; }
protected slots:
    void clearAccountPath(const QMailAccountIdList&);

private:
    QMailStore::ErrorCode addOrRename(QMailMessage *message, const QString &existingIdentifier, QMailContentManager::DurabilityRequirement durability);

    bool addOrRenameParts(QMailMessage *message, const QString &fileName, const QString &existing, QMailContentManager::DurabilityRequirement durability);
    bool removeParts(const QString &fileName);
#ifdef SYMBIAN_USE_DATA_CAGED_FILES
    void syncLater(QSharedPointer<SymbianFile> file);
#else
    void syncLater(QSharedPointer<QFile> file);
#endif

#ifdef SYMBIAN_USE_DATA_CAGED_FILES
    QList< QSharedPointer<SymbianFile> > _openFiles;
#else
    QList< QSharedPointer<QFile> > _openFiles;
#endif
    bool _useFullSync;
};


class QmfStorageManagerPlugin : public QMailContentManagerPlugin
{
    Q_OBJECT

public:
    QmfStorageManagerPlugin();

    virtual QString key() const;
    QMailContentManager *create();
};

#endif


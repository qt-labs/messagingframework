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

class QFile;

QT_END_NAMESPACE

class QmfStorageManager : public QObject, public QMailContentManager
{
    Q_OBJECT

public:
    QmfStorageManager(QObject *parent = Q_NULLPTR);
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
    static QString messagePartUndecodedFilePath(const QMailMessagePart &part, const QString &fileName);

    virtual ManagerRole role() const { return StorageRole; }
protected slots:
    void clearAccountPath(const QMailAccountIdList&);

private:
    QMailStore::ErrorCode addOrRename(QMailMessage *message, const QString &existingIdentifier, QMailContentManager::DurabilityRequirement durability);

    bool addOrRenameParts(QMailMessage *message, const QString &fileName, const QString &existing, QMailContentManager::DurabilityRequirement durability);
    bool removeParts(const QString &fileName);
    void syncLater(QSharedPointer<QFile> file);

    QList< QSharedPointer<QFile> > _openFiles;
    bool _useFullSync;
};


class QmfStorageManagerPlugin : public QMailContentManagerPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QmfStorageManagerPluginHandlerFactoryInterface")

public:
    QmfStorageManagerPlugin();

    virtual QString key() const;
    QMailContentManager *create();
};

#endif


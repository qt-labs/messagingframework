/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QTOPIAMAILFILEMANAGER_H
#define QTOPIAMAILFILEMANAGER_H

#include <qmailid.h>
#include <qmailcontentmanager.h>
#include <QList>
#include <QMap>
#include <QSharedPointer>

class QMailMessagePart;
class QMailMessagePartContainer;

QT_BEGIN_NAMESPACE

class QFile;

QT_END_NAMESPACE;

class QtopiamailfileManager : public QObject, public QMailContentManager
{
    Q_OBJECT

public:
    QtopiamailfileManager();
    ~QtopiamailfileManager();

    QMailStore::ErrorCode add(QMailMessage *message, QMailContentManager::DurabilityRequirement durability);
    QMailStore::ErrorCode update(QMailMessage *message, QMailContentManager::DurabilityRequirement durability);

    QMailStore::ErrorCode ensureDurability();

    QMailStore::ErrorCode remove(const QString &identifier);
    QMailStore::ErrorCode load(const QString &identifier, QMailMessage *message);

    bool init();
    void clearContent();

    static const QString &messagesBodyPath(const QMailAccountId &accountId);
    static QString messageFilePath(const QString &fileName, const QMailAccountId &accountId);
    static QString messagePartDirectory(const QString &fileName);
    static QString messagePartFilePath(const QMailMessagePart &part, const QString &fileName);

protected slots:
    void clearAccountPath(const QMailAccountIdList&);

private:
    QMailStore::ErrorCode addOrRename(QMailMessage *message, const QString &existingIdentifier, bool durable);

    bool addOrRenameParts(QMailMessage *message, const QString &fileName, const QString &existing, bool durable);
    bool removeParts(const QString &fileName);
    void syncLater(QSharedPointer<QFile> file);

    QList< QSharedPointer<QFile> > _openFiles;
    bool _useFullSync;
};


class QtopiamailfileManagerPlugin : public QMailContentManagerPlugin
{
    Q_OBJECT

public:
    QtopiamailfileManagerPlugin();

    virtual QString key() const;
    QMailContentManager *create();
};

#endif


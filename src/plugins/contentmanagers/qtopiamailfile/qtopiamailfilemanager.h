/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef QTOPIAMAILFILEMANAGER_H
#define QTOPIAMAILFILEMANAGER_H

#include <qmailid.h>
#include <qmailcontentmanager.h>
#include <QList>
#include <QMap>


class QMailMessagePart;
class QMailMessagePartContainer;
class QFile;

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

    bool addOrRenameParts(QMailMessage *message, const QMailMessagePartContainer &container, const QString &fileName, const QString &existing, bool durable);
    bool loadParts(QMailMessage *message, QMailMessagePartContainer *container, const QString &fileName);
    bool removeParts(const QString &fileName);

    QList<QFile*> _openFiles;
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


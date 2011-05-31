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

#ifndef QMAILCONTENTMANAGER_H
#define QMAILCONTENTMANAGER_H

#include "qmailglobal.h"
#include "qmailstore.h"
#include <qfactoryinterface.h>
#include <QMap>
#include <QString>

class QMailMessage;
class QMailContentManager;


class QMF_EXPORT QMailContentManagerFactory
{
public:
    static QStringList schemes();
    static QString defaultFilterScheme();
    static QString defaultScheme();
    static QString defaultIndexerScheme();

    static QMailContentManager *create(const QString &scheme);

    static bool init();
    static void clearContent();
};


struct QMF_EXPORT QMailContentManagerPluginInterface : public QFactoryInterface
{
    virtual QString key() const = 0;
    virtual QMailContentManager *create() = 0;
};


QT_BEGIN_NAMESPACE

#define QMailContentManagerPluginInterface_iid "com.nokia.QMailContentManagerPluginInterface"
Q_DECLARE_INTERFACE(QMailContentManagerPluginInterface, QMailContentManagerPluginInterface_iid)

QT_END_NAMESPACE;


class QMF_EXPORT QMailContentManagerPlugin : public QObject, public QMailContentManagerPluginInterface
{
    Q_OBJECT
    Q_INTERFACES(QMailContentManagerPluginInterface:QFactoryInterface)

public:
    QMailContentManagerPlugin();
    ~QMailContentManagerPlugin();

    virtual QStringList keys() const;
};


class QMF_EXPORT QMailContentManager
{
    friend class QMailManagerContentFactory;

protected:
    QMailContentManager();
    virtual ~QMailContentManager();

public:
    enum ManagerRole {
        FilterRole,
        StorageRole,
        IndexRole
    };

    enum DurabilityRequirement { 
        EnsureDurability = 0,
        DeferDurability,
        NoDurability
    };

    virtual QMailStore::ErrorCode add(QMailMessage *message, DurabilityRequirement durability) = 0;
    virtual QMailStore::ErrorCode update(QMailMessage *message, DurabilityRequirement durability) = 0;

    virtual QMailStore::ErrorCode ensureDurability() = 0;
    virtual QMailStore::ErrorCode ensureDurability(const QList<QString> &identifiers) = 0;

    virtual QMailStore::ErrorCode remove(const QString &identifier) = 0;
    virtual QMailStore::ErrorCode remove(const QList<QString> &identifiers);
    virtual QMailStore::ErrorCode load(const QString &identifier, QMailMessage *message) = 0;

    virtual bool init();
    virtual void clearContent();
    virtual ManagerRole role() const;
};

#endif


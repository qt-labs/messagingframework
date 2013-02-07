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


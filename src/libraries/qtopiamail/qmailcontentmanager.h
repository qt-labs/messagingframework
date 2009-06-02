/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
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


class QTOPIAMAIL_EXPORT QMailContentManagerFactory
{
public:
    static QStringList schemes();
    static QString defaultScheme();

    static QMailContentManager *create(const QString &scheme);

    static bool init();
    static void clearContent();
};


struct QTOPIAMAIL_EXPORT QMailContentManagerPluginInterface : public QFactoryInterface
{
    virtual QString key() const = 0;
    virtual QMailContentManager *create() = 0;
};


#define QMailContentManagerPluginInterface_iid "com.trolltech.Qtopia.Qtopiamail.QMailContentManagerPluginInterface"
Q_DECLARE_INTERFACE(QMailContentManagerPluginInterface, QMailContentManagerPluginInterface_iid)


class QTOPIAMAIL_EXPORT QMailContentManagerPlugin : public QObject, public QMailContentManagerPluginInterface
{
    Q_OBJECT
    Q_INTERFACES(QMailContentManagerPluginInterface:QFactoryInterface)

public:
    QMailContentManagerPlugin();
    ~QMailContentManagerPlugin();

    virtual QStringList keys() const;
};


class QTOPIAMAIL_EXPORT QMailContentManager
{
    friend class QMailManagerContentFactory;

protected:
    QMailContentManager();
    virtual ~QMailContentManager();

public:
    enum DurabilityRequirement { 
        EnsureDurability = 0,
        DeferDurability
    };

    virtual QMailStore::ErrorCode add(QMailMessage *message, DurabilityRequirement durability) = 0;
    virtual QMailStore::ErrorCode update(QMailMessage *message, DurabilityRequirement durability) = 0;

    virtual QMailStore::ErrorCode ensureDurability() = 0;

    virtual QMailStore::ErrorCode remove(const QString &identifier) = 0;
    virtual QMailStore::ErrorCode load(const QString &identifier, QMailMessage *message) = 0;

    virtual bool init();
    virtual void clearContent();
};

#endif


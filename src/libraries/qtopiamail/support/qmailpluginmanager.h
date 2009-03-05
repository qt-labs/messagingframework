/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef QMAILPLUGINMANAGER_H
#define QMAILPLUGINMANAGER_H

#include "qmailglobal.h"
#include <QObject>
#include <QStringList>

class QMailPluginManagerPrivate;

class QTOPIAMAIL_EXPORT QMailPluginManager : public QObject
{
    Q_OBJECT

public:
    explicit QMailPluginManager(const QString &identifier, QObject *parent=0);
    ~QMailPluginManager();

    QStringList list() const;
    QObject* instance(const QString &name);

private:
    QMailPluginManagerPrivate* d;
};

#endif

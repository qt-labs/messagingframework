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

#include "qmailpluginmanager.h"
#include "qmaillog.h"
#include <qmailnamespace.h>

#include <QMap>
#include <QPluginLoader>
#include <QDir>
#include <QCoreApplication>

/*!
  \class QMailPluginManager

  \brief The QMailPluginManager class is a helper class that simplifies plug-in loading
  for the Messaging framework.

  The list() function returns a list of available plugins in the subdirectory specified in the constructor.

  In order to load a plugin, call the instance() function with the
  name of the plugin to load.  qobject_cast() may then be used to query
  for the desired interface.
*/

/*!
  \fn QMailPluginManager::QMailPluginManager(const QString &subdir, QObject *parent)

  Creates a QMailPluginManager for plugins located in the plugin subdirectory \a subdir with the given
  \a parent.
*/

/*!
  \fn QMailPluginManager::~QMailPluginManager()

  Destroys the QMailPluginManager and releases any resources allocated by
  the PluginManager.
*/

/*!
  \fn QStringList QMailPluginManager::list() const

  Returns the list of plugins that are available in the plugin subdirectory.
*/

/*!
  \fn QObject* QMailPluginManager::instance(const QString& name)

  Load the plug-in specified by \a name.

  Returns the plugin interface if found, otherwise 0.

  \code
    QObject *instance = pluginManager->instance("name");
    if (instance) {
        EffectsInterface *iface = 0;
        iface = qobject_cast<EffectsInterface*>(instance);
        if (iface) {
            // We have an instance of the desired type.
        }
    }
  \endcode
*/

namespace {

QStringList pluginFilePatterns()
{
#if defined(Q_OS_WIN)
    return QStringList() << QString::fromLatin1("*.dll") << QString::fromLatin1("*.DLL");
#elif defined(Q_OS_MAC)
    return QStringList() << QLatin1String("*.dylib");
#else
    return QStringList() << QString::fromLatin1("*.so*");
#endif
}

}

class QMailPluginManagerPrivate
{
public:
    QMailPluginManagerPrivate(const QString &subdir);
    ~QMailPluginManagerPrivate();

public:
    QMap<QString, QPluginLoader*> pluginMap;
};

QMailPluginManagerPrivate::QMailPluginManagerPrivate(const QString &subdir)
{
    QStringList libraryPaths;
    for (const QString &path : QCoreApplication::libraryPaths()) {
        libraryPaths.append(path + "/messagingframework");
    }

    foreach (QString libraryPath, libraryPaths) {
        QDir dir(libraryPath);
        // Change into the sub directory, and make sure it's readable
        if (!dir.cd(subdir) || !dir.isReadable())
            continue;

        foreach (const QString &libname, dir.entryList(pluginFilePatterns(), QDir::Files)) {
            QString libfile = dir.absoluteFilePath(libname);
            if (pluginMap.contains(libname))
                pluginMap[libname]->setFileName(libfile);
            else
                pluginMap[libname] = new QPluginLoader(libfile);
        }
    }
}

QMailPluginManagerPrivate::~QMailPluginManagerPrivate()
{
    qDeleteAll(pluginMap.values());
}

QMailPluginManager::QMailPluginManager(const QString &subdir, QObject* parent)
    : QObject(parent)
    , d(new QMailPluginManagerPrivate(subdir))
{
}

QMailPluginManager::~QMailPluginManager()
{
    delete d;
    d = nullptr;
}

QStringList QMailPluginManager::list() const
{
    return d->pluginMap.keys();
}

QObject* QMailPluginManager::instance(const QString& name)
{
    if (d->pluginMap.contains(name)) {
        if (d->pluginMap[name]->load()) {
            return d->pluginMap[name]->instance();
        } else {
            qCWarning(lcMessaging) << "Error loading" << name << "with errorString()" << d->pluginMap[name]->errorString();
            return nullptr;
        }
    } else {
        qCWarning(lcMessaging) << "Could not find" << name << "to load";
        return nullptr;
    }
}

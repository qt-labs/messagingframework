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

#include "qmailpluginmanager.h"
#include <QMap>
#include <QPluginLoader>
#include <QDir>
#include <QtDebug>
#include <QCoreApplication>
#include <qmailnamespace.h>


/*!
  \class QMailPluginManager

  \brief The QMailPluginManager class is a helper class that simplifies plug-in loading
  for the Messaging framework.

  The list() function returns a list of available plugins in the subdirectory specified in the constructor.

  Plugin subdirectories are searched for from the directory specified by QMail::pluginPath().

  In order to load a plugin, call the instance() function with the
  name of the plugin to load.  qobject_cast() may then be used to query
  for the desired interface.
*/


/*!
  \fn QMailPluginManager::QMailPluginManager(const QString& dir, QObject* parent)

  Creates a QMailPluginManager for plugins located in the plugin subdirectory \a dir with the given
  \a parent.

  The plugins must be installed in the QMail::pluginPath()/\i{dir}  directory.
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
#ifdef LOAD_DEBUG_VERSION
    QString debugSuffix("d");
#else
    QString debugSuffix;
#endif

#ifdef Q_OS_SYMBIAN
    return QStringList() << "*.qtplugin";
#elif defined(Q_OS_WIN)
	return QStringList() << QString("*%1.dll").arg(debugSuffix) << QString("*%1.DLL").arg(debugSuffix);
#elif defined(Q_OS_MAC)
	return QStringList() << "*.dylib";
#else
	return QStringList() << QString("*%1.so*").arg(debugSuffix);
#endif
}

}

class QMailPluginManagerPrivate
{
public:
    QMailPluginManagerPrivate(const QString& ident);
    ~QMailPluginManagerPrivate();

public:
    QMap<QString,QPluginLoader*> pluginMap;
};

QMailPluginManagerPrivate::QMailPluginManagerPrivate(const QString& path)
{
    QStringList libraryPaths;
    QString mailPluginsPath = QMail::pluginsPath();
    QStringList coreLibraryPaths = QCoreApplication::libraryPaths();
    libraryPaths.append(mailPluginsPath);
    libraryPaths.append(coreLibraryPaths);

    foreach(QString libraryPath, libraryPaths) {
        QDir dir(libraryPath);
        //Change into the sub directory, and make sure it's readable
        if(!dir.cd(path) || !dir.isReadable())
            continue;

        foreach(const QString &libname, dir.entryList(pluginFilePatterns(), QDir::Files)) {
            QString libfile = dir.absoluteFilePath(libname);
            if(pluginMap.contains(libname))
                pluginMap[libname]->setFileName(libfile);
            else
                pluginMap[libname] = new QPluginLoader(libfile);
        }
    }
}

QMailPluginManagerPrivate::~QMailPluginManagerPrivate()
{
    foreach (QPluginLoader *lib, pluginMap.values()) {
        delete lib;
    }
}

QMailPluginManager::QMailPluginManager(const QString& dir, QObject* parent)
:
    QObject(parent),
    d(new QMailPluginManagerPrivate(dir))
{
}

QMailPluginManager::~QMailPluginManager()
{
    delete d; d = 0;
}

QStringList QMailPluginManager::list() const
{
    return d->pluginMap.keys();
}

QObject* QMailPluginManager::instance(const QString& name)
{
    if (d->pluginMap.contains(name)) {
        if(d->pluginMap[name]->load()) {
            return d->pluginMap[name]->instance();
        } else {
            qWarning() << "Error loading" << name << "with errorString()" << d->pluginMap[name]->errorString();
            return 0;
        }
    } else {
        qWarning() << "Could not find" << name << "to load";
        return 0;
    }
}

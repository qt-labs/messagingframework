/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#include "qmailpluginmanager.h"
#include <QMap>
#include <QPluginLoader>
#include <QDir>
#include <QtDebug>
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
#ifdef Q_OS_WIN
	return QStringList() << "*.dll" << "*.DLL";
#else
	return QStringList() << "*.so*";
#endif
}

}

class QMailPluginManagerPrivate
{
public:
    QMailPluginManagerPrivate(const QString& ident);

public:
    QMap<QString,QPluginLoader*> pluginMap;
    QString pluginPath;
};

QMailPluginManagerPrivate::QMailPluginManagerPrivate(const QString& path)
:
    pluginPath(QMail::pluginsPath() + path)
{
    //initialize the plugin map
    QDir dir(pluginPath.toLatin1());
    QStringList libs = dir.entryList(pluginFilePatterns(), QDir::Files);

    if(libs.isEmpty())
    {
        qWarning() << "Could not find any plugins in path " << pluginPath << "!";
        return;
    }

    foreach(const QString& libname,libs)
        pluginMap[libname] = 0;
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
    QString libfile = d->pluginPath + "/" + name;
    if (!QFile::exists(libfile))
        return 0;

    QPluginLoader *lib = 0;
    QMap<QString,QPluginLoader*>::const_iterator it = d->pluginMap.find(name);
    if (it != d->pluginMap.end())
        lib = *it;
    if ( !lib ) {
        lib = new QPluginLoader(libfile);
        lib->load();
        if ( !lib->isLoaded() ) {
            qWarning() << "Could not load" << libfile << "errorString()" << lib->errorString();
            delete lib;
            return 0;
        }
    }
    d->pluginMap[name] = lib;
    return lib->instance();
}

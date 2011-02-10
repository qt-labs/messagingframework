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

#include "qmaillog.h"
#include "qlogsystem.h"
#include "qloggers.h"

#include <QString>
#include <QSettings>
#include <QHash>
#include <QStringList>

#include <sys/types.h>
#if (!defined(Q_OS_WIN) && !defined(Q_OS_SYMBIAN))
#include <sys/socket.h>
#endif

QMF_EXPORT QDebug QLogBase::log(const char* category)
{
    QDebug r(QtDebugMsg);
    if ( category )
        r << category << ": ";
    return r;
}

/// This hash stores the logging categories which are explicitly enabled or disabled in config file.
/// It is being updated when configuration is re-read (QMailLoggersReCreate is called)
static QHash<QString, bool> LogCatsMode;

namespace
{
    void addLoggerIfReady(BaseLoggerFoundation* logger)
    {
        Q_ASSERT(logger);
        LogSystem& loggers = LogSystem::getInstance();

        QString err;
        const bool isReady = logger->isReady(err);
        if(!isReady) {
            // Need to print to stderr in case no loggers are acting now
            fprintf(stderr, "%s: Can't initialize logger, error: '%s'\n", Q_FUNC_INFO, qPrintable(err));
            // Printing through the log subsystem
            qWarning() << Q_FUNC_INFO << "Can't initialize logger, error: " << err;
            delete logger;
        } else {
            logger->setMinLogLvl(LlDbg);
            loggers.addLogger(logger);
        };
    };
}

#if (!defined(Q_OS_WIN) && !defined(Q_OS_SYMBIAN))
QMF_EXPORT
void qMailLoggersRecreate(const QString& organization, const QString& application, const char* ident)
{
    QSettings settings(organization, application);

    bool defaultStdError(
#ifdef QMF_ENABLE_LOGGING
     true
#else
     false
#endif
     );

    const bool syslogEnabled = settings.value("Syslog/Enabled", false).toBool();
    const bool stderrEnabled = settings.value("StdStreamLog/StdErrEnabled", defaultStdError).toBool();
    const QString filePath = settings.value("FileLog/Path").toString();

    LogSystem& loggers = LogSystem::getInstance();
    loggers.clear();

    if(syslogEnabled) {
        SysLogger<LvlLogPrefix>* sl = new SysLogger<LvlLogPrefix>(ident, LOG_PID, LOG_LOCAL7);
        addLoggerIfReady(sl);
    };

    if(!filePath.isEmpty()) {
        FileLogger<LvlTimePidLogPrefix>* fl = new FileLogger<LvlTimePidLogPrefix>(filePath);
        addLoggerIfReady(fl);
    };

    if(stderrEnabled) {
        FileLogger<LvlTimePidLogPrefix>* el = new FileLogger<LvlTimePidLogPrefix>(stderr);
        addLoggerIfReady(el);
    };

    // Filling the LogCatsEnabled list
    settings.beginGroup("Log categories");
    LogCatsMode.clear();
    foreach(const QString& key, settings.allKeys()) {
        LogCatsMode[key] = settings.value(key).toBool();
    };

    qmf_resetLoggingFlags();
};

static QList<char*> LogFlagsCache;

// Register the flag variable so it can be reset later
QMF_EXPORT void qmf_registerLoggingFlag(char *flag)
{
    if (!LogFlagsCache.contains(flag))
        LogFlagsCache.append(flag);
}

// Reset the logging flags
QMF_EXPORT void qmf_resetLoggingFlags()
{
    // force everyone to re-check their logging status
    foreach (char *flag, LogFlagsCache)
            (*flag) = 0;
}

// Check if a given category is enabled.
// This looks for <category>=[1|0] in the [Logging] section of the .conf file.
// eg.
//
// [Logging]
// Foo=1
//
// qMailLog(Foo) << "this will work";
// qMailLog(Bar) << "not seen";
//
// Note that qMailLog(Foo) will cause a compile error unless one of the
// QLOG_ENABLE(Foo), QLOG_DISABLE(Foo) or QLOG_RUNTIME(Foo) macros have
// been used.

// Note that in debug mode (CONFIG+=debug), runtime categories default
// to on instead of off due to the defines here.
QMF_EXPORT bool qmf_checkLoggingEnabled(const char *category, const bool defValue)
{
    const bool r = LogCatsMode.value(QLatin1String(category), defValue);

    return r;
}

#else

QMF_EXPORT void qMailLoggersRecreate(const QString& organization, const QString& application, const char* ident)
{
    Q_UNUSED(organization);
    Q_UNUSED(application);
    Q_UNUSED(ident);
}

QMF_EXPORT void qmf_registerLoggingFlag(char *flag)
{
    Q_UNUSED(flag);
}
QMF_EXPORT void qmf_resetLoggingFlags() { }

QMF_EXPORT bool qmf_checkLoggingEnabled(const char *category, const bool defValue)
{
    Q_UNUSED(category);
    Q_UNUSED(defValue);
    return true;
}
#endif

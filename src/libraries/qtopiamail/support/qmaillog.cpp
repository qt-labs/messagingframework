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
#include <QString>
#include <QSocketNotifier>
#include <QSettings>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>

#ifdef QMAIL_SYSLOG

#include <syslog.h>

static const char* SysLogId = "QMF";

SysLog::SysLog(){}

SysLog::~SysLog()
{
    write(buffer);
}

void SysLog::write(const QString& message)
{
    static bool open = false;
    if(!open)
    {
        openlog(SysLogId,(LOG_CONS | LOG_PID),LOG_LOCAL1);
        open = true;
    }
    syslog(LOG_INFO,message.toLocal8Bit().data());
}

QTOPIAMAIL_EXPORT SysLog QLogBase::log(const char* category)
{
    SysLog r;
    if ( category )
        r << category << ": ";
    return r;
}

#else

QTOPIAMAIL_EXPORT QDebug QLogBase::log(const char* category)
{
    QDebug r(QtDebugMsg);
    if ( category )
        r << category << ": ";
    return r;
}

#endif //QMAIL_SYSLOG

// Singleton that manages the runtime logging housekeeping
class RuntimeLoggingManager : public QObject
{
    Q_OBJECT

public:
    RuntimeLoggingManager(QObject *parent = 0);
    ~RuntimeLoggingManager();

    static void hupSignalHandler(int unused);

public slots:
    void handleSigHup();

private:
    static int sighupFd[2];
    QSocketNotifier *snHup;

public:
    QSettings settings;
    QList<char*> cache;
};

int RuntimeLoggingManager::sighupFd[2];

RuntimeLoggingManager::RuntimeLoggingManager(QObject *parent)
    : QObject(parent)
    , settings("Nokia", "QMF") // This is ~/.config/Nokia/QMF.conf
{
    settings.beginGroup("Logging");
    // Use a socket and notifier because signal handlers can't call Qt code
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sighupFd))
        qFatal("Couldn't create HUP socketpair");
    snHup = new QSocketNotifier(sighupFd[1], QSocketNotifier::Read, this);
    connect(snHup, SIGNAL(activated(int)), this, SLOT(handleSigHup()));

    struct sigaction hup;
    hup.sa_handler = RuntimeLoggingManager::hupSignalHandler;
    sigemptyset(&hup.sa_mask);
    hup.sa_flags = 0;
    hup.sa_flags |= SA_RESTART;
    if (sigaction(SIGHUP, &hup, 0) > 0)
        qFatal("Couldn't register HUP handler");
}

RuntimeLoggingManager::~RuntimeLoggingManager()
{
}

void RuntimeLoggingManager::hupSignalHandler(int)
{
    // Can't call Qt code. Write to the socket and the notifier will fire from the Qt event loop
    char a = 1;
    ::write(sighupFd[0], &a, sizeof(a));
}

void RuntimeLoggingManager::handleSigHup()
{
    snHup->setEnabled(false);
    char tmp;
    ::read(sighupFd[1], &tmp, sizeof(tmp));

    qmf_resetLoggingFlags();

    snHup->setEnabled(true);
}

Q_GLOBAL_STATIC(RuntimeLoggingManager, runtimeLoggingManager)

// Register the flag variable so it can be reset later
QTOPIAMAIL_EXPORT void qmf_registerLoggingFlag(char *flag)
{
    RuntimeLoggingManager *rlm = runtimeLoggingManager();
    if (!rlm->cache.contains(flag))
        rlm->cache.append(flag);
}

// Reset the logging flags
QTOPIAMAIL_EXPORT void qmf_resetLoggingFlags()
{
    RuntimeLoggingManager *rlm = runtimeLoggingManager();

    // re-read the .conf file
    rlm->settings.sync();

    // force everyone to re-check their logging status
    foreach (char *flag, rlm->cache)
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
// QLOG_ENABLED(Foo), QLOG_DISABLED(Foo) or QLOG_RUNTIME(Foo) macros have
// been used.
QTOPIAMAIL_EXPORT bool qmf_checkLoggingEnabled(const char *category)
{
    RuntimeLoggingManager *rlm = runtimeLoggingManager();
    return rlm->settings.value(QLatin1String(category),0).toBool();
}

#include "qmaillog.moc"

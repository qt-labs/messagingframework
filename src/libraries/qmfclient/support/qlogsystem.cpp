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

#include "qlogsystem.h"

#include <QDateTime>

#ifndef Q_OS_WIN
extern "C"
{
#include <unistd.h>
}
#else
#include <QCoreApplication>
#endif

/// singleton access
LogSystem& LogSystem::getInstance()
{
    static LogSystem instance;
    return instance;
}

//LogSystem implementation
LogSystem::LogSystem()
{
    qInstallMessageHandler(debugMsgFwd);
}

LogSystem::~LogSystem()
{
    qInstallMessageHandler(NULL);
    clear();
}

void LogSystem::log(LogLevel lvl, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    foreach(ILogger* logger, loggers)
    {
        logger->log(lvl, fmt, args);
    }
    va_end(args);
}

void LogSystem::addLogger(ILogger* logger)
{
    Q_ASSERT(logger);
    if(!loggers.contains(logger))
        loggers.append(logger);
}

void LogSystem::clear()
{
    foreach(ILogger* logger, loggers)
    {
        Q_ASSERT(logger);
        delete logger;
    }
    loggers.clear();
}

void LogSystem::debugMsgFwd(QtMsgType type, const QMessageLogContext &ctxt, const QString &msg)
{
    Q_UNUSED(ctxt);
    QByteArray ba = msg.toLatin1();

    switch (type)
    {
    case QtDebugMsg:
        LogSystem::getInstance().log(LlDbg, "%s", ba.data());
        break;
    case QtWarningMsg:
        LogSystem::getInstance().log(LlWarning, "%s", ba.data());
        break;
    case QtFatalMsg:
        LogSystem::getInstance().log(LlCritical, "%s", ba.data());
        abort();
    case QtCriticalMsg:
        LogSystem::getInstance().log(LlError, "%s", ba.data());
        break;
    default:
        Q_ASSERT(false);
        break;
    }
}

//Aux classes implementation

const QString& NoLogPrefix::operator()(const LogLevel& lvl)
{
    Q_UNUSED(lvl);
    return empty;
}

LvlLogPrefix::LvlLogPrefix()
{
    levels_str[LlDbg]      =  QLatin1String("[Debug] ");
    levels_str[LlInfo]     =  QLatin1String("[Info] ");
    levels_str[LlWarning]  =  QLatin1String("[Warning] ");
    levels_str[LlError]    =  QLatin1String("[Error] ");
    levels_str[LlCritical] =  QLatin1String("[Critical] ");
}

const QString& LvlLogPrefix::operator()(const LogLevel&  lvl)
{
    out = levels_str[lvl];
    return out;
}

const QString& LvlTimeLogPrefix::operator()(const LogLevel& lvl)
{
    out = QDateTime::currentDateTime().toString(QLatin1String("MMM dd hh:mm:ss ")) + LvlLogPrefix::operator()(lvl);
    return out;
}

LvlTimePidLogPrefix::LvlTimePidLogPrefix()
{
#ifndef Q_OS_WIN
    stPid = QString::fromLatin1("[%1] ").arg(getpid());
#else
    stPid = QString("[%1] ").arg(qApp->applicationPid());
#endif
}

const QString& LvlTimePidLogPrefix::operator ()(const LogLevel& lvl)
{
    out = stPid + LvlTimeLogPrefix::operator ()(lvl);
    return out;
}

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
    qInstallMsgHandler(debugMsgFwd);
}

LogSystem::~LogSystem()
{
    qInstallMsgHandler(NULL);
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

void LogSystem::debugMsgFwd(QtMsgType type, const char *msg)
{
    switch (type)
    {
    case QtDebugMsg:
        LogSystem::getInstance().log(LlDbg, "%s", msg);
        break;
    case QtWarningMsg:
        LogSystem::getInstance().log(LlWarning, "%s", msg);
        break;
    case QtFatalMsg:
        LogSystem::getInstance().log(LlCritical, "%s", msg);
        abort();
    case QtCriticalMsg:
        LogSystem::getInstance().log(LlError, "%s", msg);
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
    levels_str[LlDbg]  	   =  QString("[Debug] ");
    levels_str[LlInfo] 	   =  QString("[Info] ");
    levels_str[LlWarning]  =  QString("[Warning] ");
    levels_str[LlError]    =  QString("[Error] ");
    levels_str[LlCritical] =  QString("[Critical] ");
}

const QString& LvlLogPrefix::operator()(const LogLevel&  lvl)
{
    out = levels_str[lvl];
    return out;
}

const QString& LvlTimeLogPrefix::operator()(const LogLevel& lvl)
{
    out = QDateTime::currentDateTime().toString ("MMM dd hh:mm:ss ") + LvlLogPrefix::operator()(lvl);
    return out;
}

LvlTimePidLogPrefix::LvlTimePidLogPrefix()
{
#ifndef Q_OS_WIN
    stPid = QString("[%1] ").arg(getpid());
#else
    stPid = QString("[%1] ").arg(qApp->applicationPid());
#endif
}

const QString& LvlTimePidLogPrefix::operator ()(const LogLevel& lvl)
{
    out = stPid + LvlTimeLogPrefix::operator ()(lvl);
    return out;
}

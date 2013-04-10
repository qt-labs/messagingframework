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
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
    qInstallMsgHandler(debugMsgFwd);
#else
    qInstallMessageHandler(debugMsgFwd);
#endif
}

LogSystem::~LogSystem()
{
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
    qInstallMsgHandler(NULL);
#else
    qInstallMessageHandler(NULL);
#endif
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
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
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
#else
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
#endif

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

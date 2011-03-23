/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the either Technology Preview License Agreement or the
** Beta Release License Agreement.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QLOGSYSTEM_H
#define QLOGSYSTEM_H

#include "qmailglobal.h"
#include <QtDebug>

#include <QTextStream>
#include <QString>
#include <iostream>
#include <list>

extern "C"
{
#include <stdarg.h>
#include <errno.h>
}

/// This enumeration represents different widely-used log levels
typedef enum {
    /// Debug message
    LlDbg = 0,
    /// Informational message
    LlInfo,
    /// Warning message
    LlWarning,
    /// Error message
    LlError,
    /// Critical error message
    LlCritical
} LogLevel;

/*!
 * \brief This is the logger interface for all Logsystem loggers.
 *
 *  Never delete loggers added to the LogSystem via LogSystem::addLogger function!!!
 *  They will be deleted by LogSystem
*/
class QMF_EXPORT ILogger
{
public:
    /// This method is invoked by the LogSystem instance
    virtual void log(LogLevel lvl, const char* fmt, va_list args)  = 0;
    virtual ~ILogger()  { }
};

/*! \brief This is a logging system class.

     The LogSystem class is a singletone storage of loggers.
     The core logging function is called log and it appeals to all contained loggers.

     The LogSystem class should be initialized at the beginning of the application, before
     any log output is issued.
     This class forwards standard Qt log functions e.g. qWarning, qDebug, ect.

     Never delete loggers added to the LogSystem via LogSystem::addLogger function!!!
     They will be deleted by LogSystem

     The LogSystem is not thread safe!!!
*/
class QMF_EXPORT LogSystem
{
public:
    /// singleton access
    static LogSystem& getInstance();
    /*!
      \brief Simple log function. It looks (almost) like printf.

      \param _lvl Log level

      \param _fmt Log string format. Its specifications depends on real logger used but usually it equals to
             printf format specification.
    */
    void log(LogLevel lvl, const char* fmt, ...);

    /// Adds new logger to the system
    void addLogger(ILogger* logger);

    /// Removes all loggers from the system
    void clear();

protected:
    static void debugMsgFwd(QtMsgType type, const char *msg);


private:
    /// Do not allow to create new instance of that class
    LogSystem();
    /// Do not allow to delete object of this class from outside
    ~LogSystem();

    Q_DISABLE_COPY(LogSystem)

    QList<ILogger*> loggers;

};

/// Log prefix policy - no prefix (log messages are not prefixed by anything)
class QMF_EXPORT NoLogPrefix
{
public:
    /// Returns empty string (empty prefix)
    const QString& operator()(const LogLevel& lvl);

private:
    /// Empty strings
    QString empty;
};

/// Log prefix policy - log level prefix (log messages are prefixed by log level)
class QMF_EXPORT LvlLogPrefix
{
public:
    /// Initialization
    LvlLogPrefix();
    /// Returns textual representation of the log level
    const QString& operator()(const LogLevel& lvl);

private:
    /// Buffer to store the textual representation of the log level
    QString out;
    /// Maps log levels to their textual representation
    QMap<LogLevel, QString> levels_str;
};

/// Log prefix policy - log level & time prefix (log messages are prefixed by log level and time of the message)
class QMF_EXPORT LvlTimeLogPrefix : public LvlLogPrefix
{
public:
    /// Returns textual representation of the log level and current date/time
    const QString& operator()(const LogLevel& lvl);

private:
    QString out;
};

/// Log prefix policy - log level & time prefix & PID
class QMF_EXPORT LvlTimePidLogPrefix: public LvlTimeLogPrefix
{
public:
    /// Initialization
    LvlTimePidLogPrefix();
    /// returns textural reprezentation of PID
    const QString& operator()(const LogLevel& lvl);

private:
    QString stPid;
    QString out;

};

#endif // QLOGSYSTEM_H

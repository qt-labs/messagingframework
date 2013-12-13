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

#ifndef QMAILLOG_H
#define QMAILLOG_H

#include "qmailglobal.h"
#include "qlogsystem.h"

#include <QtDebug>

class QMF_EXPORT QLogBase {
public:
    static QDebug log(const char*);
};

#define QLOG_DISABLE(dbgcat) \
    class dbgcat##_QLog : public QLogBase { \
    public: \
        static inline bool enabled() { return 0; }\
    };
#define QLOG_ENABLE(dbgcat) \
    class dbgcat##_QLog : public QLogBase { \
    public: \
        static inline bool enabled() { return 1; }\
    };

/*!
 * \brief Reads settings and creates loggers
 *
 * Reads the settings from QSettings(organization, application), constructs the loggers and adds them
 * to LogSystem::getInstance(), if the settings configuration file does not exist, a default with logging disabled is created.
 * Before doing this, remove all the registered loggers from LogSystem::getInstance().
 *
 * This function could be used during application initialization phase or when the configuration file shoudd be re-read.
 *
 * \param organization Organization (as per QSettings API)
 * \param application  Application (as per QSettings API)
 * \param ident        Ident value for syslog (will be used if syslog is enabled in settings)
*/
QMF_EXPORT void qMailLoggersRecreate(const QString& organization, const QString& application, const char* ident);

QMF_EXPORT void qmf_registerLoggingFlag(char *flag);
QMF_EXPORT void qmf_resetLoggingFlags();
QMF_EXPORT bool qmf_checkLoggingEnabled(const char *category, const bool defValue);

#define QLOG_RUNTIME(dbgcat, deflvl) \
    class dbgcat##_QLog : public QLogBase { \
    public: \
        static inline bool enabled() { static char mem=0; if (!mem) { qmf_registerLoggingFlag(&mem); mem=(qmf_checkLoggingEnabled(#dbgcat, deflvl))?3:2; } return mem&1; }\
    };

#define qMailLog(dbgcat) if (!dbgcat##_QLog::enabled()) qt_noop(); else dbgcat##_QLog::log(#dbgcat)

// By default, these categories are completely disabled.
// Any logging statements for these categories will be compiled out of the executable.
QLOG_DISABLE(ImapData)
QLOG_DISABLE(MessagingState)

// Use macros QLOG_RUNTIME to let the user turn on/off the logging for specific category in runtime
// The second parameter of the macros means the default value. "true" means the logging is
// turned ON by default, "false" means the logging is turned OFF by default.
// To prevent logging statements using these categories from being compiled into the executable
// these statements shall be changed to QLOG_DISABLE calls.

#ifdef QMF_ENABLE_LOGGING
QLOG_RUNTIME(Messaging, true)
QLOG_RUNTIME(IMAP, true)
QLOG_RUNTIME(SMTP, true)
QLOG_RUNTIME(POP, true)
#else
QLOG_DISABLE(Messaging)
QLOG_DISABLE(IMAP)
QLOG_DISABLE(SMTP)
QLOG_DISABLE(POP)
#endif // QMF_ENABLE_LOGGING

#endif //QMAILLOG_H

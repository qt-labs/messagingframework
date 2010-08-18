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

#ifndef QMAILLOG_H
#define QMAILLOG_H

#include "qmailglobal.h"
#include <QtDebug>

#ifdef QMAIL_SYSLOG

#include <QTextStream>

class QTOPIAMAIL_EXPORT SysLog
{
public:
    SysLog();
    virtual ~SysLog();
    template<typename T> SysLog& operator<<(const T&);
    template<typename T> SysLog& operator<<(const QList<T> &list);
    template<typename T> SysLog& operator<<(const QSet<T> &set);

private:
    void write(const QString& message);

private:
    QString buffer;
};

template <class T>
inline SysLog& SysLog::operator<<(const QList<T> &list)
{
    operator<<("(");
    for (Q_TYPENAME QList<T>::size_type i = 0; i < list.count(); ++i) {
        if (i)
            operator<<(", ");
        operator<<(list.at(i));
    }
    operator<<(")");
    return *this;
}

template <class T>
inline SysLog& SysLog::operator<<(const QSet<T> &set)
{
    operator<<("QSET");
    operator<<(set.toList());
    return *this;
}

template<typename T>
SysLog& SysLog::operator<<(const T& item)
{
    QTextStream stream(&buffer);
    stream << item;
    return *this;
}
#endif //QMAIL_SYSLOG

class QTOPIAMAIL_EXPORT QLogBase {
public:
#ifdef QMAIL_SYSLOG
    static SysLog log(const char*);
#else
    static QDebug log(const char*);
#endif
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

QTOPIAMAIL_EXPORT void qmf_registerLoggingFlag(char *flag);
QTOPIAMAIL_EXPORT void qmf_resetLoggingFlags();
QTOPIAMAIL_EXPORT bool qmf_checkLoggingEnabled(const char *category);
#define QLOG_RUNTIME(dbgcat) \
    class dbgcat##_QLog : public QLogBase { \
    public: \
        static inline bool enabled() { static char mem=0; if (!mem) { qmf_registerLoggingFlag(&mem); mem=(qmf_checkLoggingEnabled(#dbgcat))?3:2; } return mem&1; }\
    };

#define qMailLog(dbgcat) if(!dbgcat##_QLog::enabled()); else dbgcat##_QLog::log(#dbgcat)

// By default, these categories are completely disabled.
// Any logging statements for these categories will be compiled out of the executable.
QLOG_DISABLE(ImapData)
QLOG_DISABLE(MessagingState)

// By default, these categories are disabled but can be enabled at runtime.
// Note that a debug build (CONFIG+=debug) will cause these categories to default to on rather than off.
// To prevent logging statements using these categories from being compiled into the executable
// these statements must be changed to QLOG_DISABLE calls.
QLOG_RUNTIME(Messaging)
QLOG_RUNTIME(IMAP)
QLOG_RUNTIME(SMTP)
QLOG_RUNTIME(POP)

#endif //QMAILLOG_H

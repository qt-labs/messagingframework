/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
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

#define qMailLog(dbgcat) if(!dbgcat##_QLog::enabled()); else dbgcat##_QLog::log(#dbgcat)

QLOG_DISABLE(Messaging)
QLOG_DISABLE(IMAP)
QLOG_DISABLE(SMTP)
QLOG_DISABLE(POP)
QLOG_DISABLE(ImapData)
QLOG_DISABLE(MessagingState)

#endif //QMAILLOG_H

/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#include "qmaillog.h"
#include <QString>

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


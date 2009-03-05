/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef QMAILGLOBAL_H
#define QMAILGLOBAL_H
#ifdef QMAIL_QTOPIA
# include "../qtopiabase/qtopiaglobal.h"
#else
#define QTOPIAMAIL_EXPORT __attribute__((visibility("default")))
#endif
#endif

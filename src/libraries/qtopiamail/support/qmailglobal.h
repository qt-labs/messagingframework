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

#include <qglobal.h>

#if defined(Q_OS_WIN) || defined(Q_OS_SYMBIAN)
#define QTOPIAMAIL_DECL_EXPORT Q_DECL_EXPORT
#define QTOPIAMAIL_DECL_IMPORT Q_DECL_IMPORT
#define QTOPIAMAIL_VISIBILITY
#elif defined(QT_VISIBILITY_AVAILABLE)
#define QTOPIAMAIL_DECL_EXPORT Q_DECL_EXPORT
#define QTOPIAMAIL_DECL_IMPORT Q_DECL_IMPORT
#define QTOPIAMAIL_VISIBILITY __attribute__((visibility("default")))
#else
#define QTOPIAMAIL_DECL_EXPORT
#define QTOPIAMAIL_DECL_IMPORT
#define QTOPIAMAIL_VISIBILITY
#endif

#ifdef QTOPIAMAIL_INTERNAL
#define QTOPIAMAIL_EXPORT QTOPIAMAIL_DECL_EXPORT
#else
#define QTOPIAMAIL_EXPORT QTOPIAMAIL_DECL_IMPORT
#endif

#ifdef QMFUTIL_INTERNAL
#define QMFUTIL_EXPORT QTOPIAMAIL_DECL_EXPORT
#else
#define QMFUTIL_EXPORT QTOPIAMAIL_DECL_IMPORT
#endif

#ifdef MESSAGESERVER_INTERNAL
#define MESSAGESERVER_EXPORT QTOPIAMAIL_DECL_EXPORT
#else
#define MESSAGESERVER_EXPORT QTOPIAMAIL_DECL_IMPORT
#endif

#endif

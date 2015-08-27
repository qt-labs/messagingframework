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

#ifndef QMAILGLOBAL_H
#define QMAILGLOBAL_H

#include <qglobal.h>

#ifdef SINGLE_MODULE_QTOPIAMAIL
#define QMF_DECL_EXPORT
#define QMF_DECL_IMPORT
#define QMF_VISIBILITY
#else
#if defined(Q_OS_WIN)
#define QMF_DECL_EXPORT Q_DECL_EXPORT
#define QMF_DECL_IMPORT Q_DECL_IMPORT
#define QMF_VISIBILITY
#elif defined(QT_VISIBILITY_AVAILABLE)
#define QMF_DECL_EXPORT Q_DECL_EXPORT
#define QMF_DECL_IMPORT Q_DECL_IMPORT
#define QMF_VISIBILITY __attribute__((visibility("default")))
#else
#define QMF_DECL_EXPORT
#define QMF_DECL_IMPORT
#define QMF_VISIBILITY
#endif
#endif

#ifdef QMF_INTERNAL
#define QMF_EXPORT QMF_DECL_EXPORT
#else
#define QMF_EXPORT QMF_DECL_IMPORT
#endif

#ifdef QMFUTIL_INTERNAL
#define QMFUTIL_EXPORT QMF_DECL_EXPORT
#else
#define QMFUTIL_EXPORT QMF_DECL_IMPORT
#endif

#ifdef MESSAGESERVER_INTERNAL
#define MESSAGESERVER_EXPORT QMF_DECL_EXPORT
#else
#define MESSAGESERVER_EXPORT QMF_DECL_IMPORT
#endif

#ifndef ENFORCE
#ifdef QT_NO_DEBUG
#define ENFORCE(expr) expr
#else
#define ENFORCE(expr) { bool res = (expr); Q_ASSERT(res); }
#endif
#endif

#endif

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

#ifndef QMAILTHREAD_P_H
#define QMAILTHREAD_P_H

#include "qmailthread.h"


class QMF_EXPORT QMailThreadPrivate : public QPrivateImplementationBase
{
public:
    QMailThreadPrivate()
        : QPrivateImplementationBase(this),
          messageCount(0),
          unreadCount(0),
          status(0)
    {
    }

    QMailThreadId id;
    uint messageCount;
    uint unreadCount;
    QString serverUid;
    QMailAccountId parentAccountId;
    QString subject;
    QString preview;
    QString senders;
    QMailTimeStamp lastDate;
    QMailTimeStamp startedDate;
    quint64 status;
};


#endif // QMAILTHREAD_P_H

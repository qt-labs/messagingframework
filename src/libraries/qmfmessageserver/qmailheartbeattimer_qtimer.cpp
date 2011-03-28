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

#include "qmailheartbeattimer.h"
#include <QPair>

struct QMailHeartbeatTimerPrivate
{
    QMailHeartbeatTimerPrivate()
        : interval(0, 0)
    {}

    QTimer timer;
    QPair<int, int> interval;
};

QMailHeartbeatTimer::QMailHeartbeatTimer(QObject *parent)
    : QObject(parent), d(new QMailHeartbeatTimerPrivate)
{
    connect(&d->timer, SIGNAL(timeout()), this, SIGNAL(timeout()));
}

QMailHeartbeatTimer::~QMailHeartbeatTimer()
{
}

bool QMailHeartbeatTimer::isActive() const
{
    return d->timer.isActive();
}

int QMailHeartbeatTimer::timerId() const
{
    return d->timer.timerId();
}

void QMailHeartbeatTimer::setInterval(int interval)
{
    setInterval(interval, interval);
}

void QMailHeartbeatTimer::setInterval(int minimum, int maximum)
{
    Q_ASSERT(minimum <= maximum);
    d->timer.setInterval((minimum + maximum) / 2);
    d->interval = qMakePair(minimum, maximum);
}

QPair<int, int> QMailHeartbeatTimer::interval() const
{
    return d->interval;
}

void QMailHeartbeatTimer::setSingleShot(bool singleShot)
{
    d->timer.setSingleShot(singleShot);
}

bool QMailHeartbeatTimer::isSingleShot() const
{
    return d->timer.isSingleShot();
}

void QMailHeartbeatTimer::singleShot(int minimum, int maximum, QObject *receiver, const char *member)
{
    Q_ASSERT(minimum <= maximum);
    QTimer::singleShot((minimum + maximum) / 2, receiver, member);
}

void QMailHeartbeatTimer::start(int interval)
{
    start(interval, interval);
}

void QMailHeartbeatTimer::start(int minimum, int maximum)
{
    Q_ASSERT(minimum <= maximum);
    d->timer.start((minimum + maximum) / 2);
}

void QMailHeartbeatTimer::start()
{
    d->timer.start();
}

void QMailHeartbeatTimer::stop()
{
    d->timer.stop();
}

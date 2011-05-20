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
#include <QSystemAlignedTimer>

QTM_USE_NAMESPACE

struct QMailHeartbeatTimerPrivate
{
    QMailHeartbeatTimerPrivate()
        : timer(new QSystemAlignedTimer)
    {}

    ~QMailHeartbeatTimerPrivate()
    {
        delete timer;
    }

    QSystemAlignedTimer* timer;
};

QMailHeartbeatTimer::QMailHeartbeatTimer(QObject *parent)
    : QObject(parent), d_ptr(new QMailHeartbeatTimerPrivate)
{
    Q_D(QMailHeartbeatTimer);
    connect(d->timer, SIGNAL(timeout()), this, SIGNAL(timeout()));
}

static QPair<int, int> calculatePeriod(int interval)
{
    const int seconds = interval / 1000;
    const int dev = qMin(qMax(seconds / 8, 3), 30);  // deviation is not greater than 1/8 of interval, but 3-30 sec

    // not earlier than in 1 sec
    return qMakePair(qMax(seconds - dev, 1), seconds + dev);
}

QMailHeartbeatTimer::~QMailHeartbeatTimer()
{
    delete d_ptr;
}

bool QMailHeartbeatTimer::isActive() const
{
    const Q_D(QMailHeartbeatTimer);
    return d->timer->isActive();
}

void QMailHeartbeatTimer::setInterval(int interval)
{
    QPair<int, int> period = calculatePeriod(interval);
    setInterval(period.first, period.second);
}

void QMailHeartbeatTimer::setInterval(int minimum, int maximum)
{
    Q_ASSERT(minimum <= maximum);
    Q_D(QMailHeartbeatTimer);
    d->timer->setMinimumInterval(minimum);
    d->timer->setMaximumInterval(maximum);
}

QPair<int, int> QMailHeartbeatTimer::interval() const
{
    const Q_D(QMailHeartbeatTimer);
    return qMakePair(d->timer->minimumInterval(), d->timer->maximumInterval());
}

void QMailHeartbeatTimer::setSingleShot(bool singleShot)
{
    Q_D(QMailHeartbeatTimer);
    d->timer->setSingleShot(singleShot);
}

bool QMailHeartbeatTimer::isSingleShot() const
{
    const Q_D(QMailHeartbeatTimer);
    return d->timer->isSingleShot();
}

void QMailHeartbeatTimer::singleShot(int minimum, int maximum, QObject *receiver, const char *member)
{
    Q_ASSERT(minimum <= maximum);
    QSystemAlignedTimer::singleShot(minimum, maximum, receiver, member);
}

void QMailHeartbeatTimer::singleShot(int interval, QObject *receiver, const char *member)
{
    QPair<int, int> period = calculatePeriod(interval);
    QSystemAlignedTimer::singleShot(period.first, period.second, receiver, member);
}

void QMailHeartbeatTimer::start(int interval)
{
    QPair<int, int> period = calculatePeriod(interval);
    start(period.first, period.second);
}

void QMailHeartbeatTimer::start(int minimum, int maximum)
{
    Q_ASSERT(minimum <= maximum);
    Q_D(QMailHeartbeatTimer);
    d->timer->start(minimum, maximum);
}

void QMailHeartbeatTimer::start()
{
    Q_D(QMailHeartbeatTimer);
    d->timer->start();
}

void QMailHeartbeatTimer::stop()
{
    Q_D(QMailHeartbeatTimer);
    d->timer->stop();
}

void QMailHeartbeatTimer::wokeUp()
{
    Q_D(QMailHeartbeatTimer);
    d->timer->wokeUp();
}

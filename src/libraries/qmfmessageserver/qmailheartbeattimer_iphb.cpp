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
#include <errno.h>
#include <glib.h>
#include <QtDebug>


/**
 * @file qmailheartbeattimer_iphb.cpp
 * Contains declaration of QMailHeartbeatTimerPrivate, object like QTimer, but using IPHB.
 */

#include <QObject>
#include <glib.h>
#include <qmailglobal.h>

/**
 * Timer based on IP heartbeat.
 * This class provides the functionality similar to QTimer, but it based on
 * IP heartbeat framework, so the intervals are correspoded with network interfaces
 * activity, to save battery live.
 * @author Michael Limanskii <mikhael.limanskii@teleca.com>
 */
class QMailHeartbeatTimerPrivate : public QObject
{
    Q_OBJECT

public:
    /**
     * Default constructor.
     * @param parent parent object.
     */
    explicit QMailHeartbeatTimerPrivate(QObject* parent = 0);

    /**
     * Destructor.
     */
    virtual ~QMailHeartbeatTimerPrivate();

    /**
     * Starts or restarts the timer for provided interval.
     * @param msecs interval in miliseconds.
     * @return true if timer is started, false otherwise.
     */
    bool start(int msecs);

    /**
     * Starts or restarts timer.
     * @return true if timer is started, false otherwise.
     */
    bool start();

    /**
     * Provedes the timer status.
     * @retval true timer is started.
     * @retval false timer is not started.
     */
    bool isActive() const;

    /**
     * Stops the timer.
     */
    void stop();

    /**
     * Return single shot setting value.
     * @retval true timer is singleshot.
     * @retval false timer is repeatable.
     */
    bool isSingleShot() const { return mSingleShot; }

    /**
     * Sets the single shot setting.
     * @param singleShot a new setting value. Default value is false.
     */
    void setSingleShot(bool singleShot) { mSingleShot = singleShot; }

    /**
     * Provides current interval value.
     * @return interval value in miliseconds.
     */
    QPair<int, int> interval() const { return qMakePair(mMinInterval, mMaxInterval); }

    /**
     * Sets interval value.
     * @param interval new value in miliseconds.
     */
    void setInterval(int interval);

    void setInterval(int minimum, int maximum);

signals:
    /**
     * This signal is emitted when the timer is timed out.
     */
    void timeout();

private:
    /**
     * Initializes Heartbeat GSource.
     */
    void initSource();

    // glib callbacks

    /**
     * Called before descriptor is polled.
     * @param source not used.
     * @param timeout sets to -1, by this method, because timeout is not needed.
     * @return FALSE.
     * @see GSourceFuncs documentation for more info.
     */
    static gboolean prepareSource(GSource *source, gint *timeout);

    /**
     * Called after descriptor is polled.
     * @param source pointer to mSource.
     * @return TRUE if all works fine.
     * @see GSourceFuncs documentation for more info.
     */
    static gboolean checkSource(GSource *source);

    /**
     * Called to dispatch event source.
     * @param source pointer to mSource.
     * @param callback pointer to hbCallback
     * @param userData pointer to QMailHeartbeatTimerPrivate instance.
     * @see GSourceFuncs documentation for more info.
     */
    static gboolean dispatchSource(GSource *source, GSourceFunc callback, gpointer userData);

    /**
     * Called when the source is finalized.
     * @param source pointer to mSource.
     * @see GSourceFuncs documentation for more info.
     */
    static void finalizeSource(GSource* source);

    /**
     * GSourceFunc for mSource.
     * @param aData pointer to QMailHeartbeatTimerPrivate instance.
     * @retval FALSE if timer is single shot.
     * @retval TRUE if time is repeatable.
     * @see GSourceFunc documentation for more info.
     */
    static gboolean hbCallback(gpointer data);

private:
    struct HBSource;

    HBSource* mSource;
    static GSourceFuncs SourceFuncs;
    bool mSingleShot;
    int mMinInterval;
    int mMaxInterval;
    guint mSourceId;
    bool mIsActive;
};

extern "C" {
#include <iphbd/libiphb.h>
}

struct QMailHeartbeatTimerPrivate::HBSource
{
    GSource source;
    iphb_t iphb;
    GPollFD poll;
};

GSourceFuncs QMailHeartbeatTimerPrivate::SourceFuncs = {
    QMailHeartbeatTimerPrivate::prepareSource,
    QMailHeartbeatTimerPrivate::checkSource,
    QMailHeartbeatTimerPrivate::dispatchSource,
    QMailHeartbeatTimerPrivate::finalizeSource,
    0,
    0
};

QMailHeartbeatTimerPrivate::QMailHeartbeatTimerPrivate(QObject* parent /* = 0 */)
    : QObject(parent)
    , mSource(0)
    , mSingleShot(false)
    , mIsActive(false)
{
    initSource();
}

void QMailHeartbeatTimerPrivate::initSource()
{
    qDebug() << "QMailHeartbeatTimerPrivate::initSource start";
    int hbeatInterval = 0;
    iphb_t iphb = iphb_open(&hbeatInterval);

    if (iphb != 0)
    {
        mSource = (HBSource*)g_source_new(&SourceFuncs, sizeof(QMailHeartbeatTimerPrivate::HBSource));
        mSource->iphb = iphb;
        g_source_set_priority((GSource*)mSource, G_PRIORITY_DEFAULT);
        g_source_set_callback((GSource*)mSource, hbCallback, this, 0);
        mSourceId = g_source_attach((GSource*)mSource, 0);
    }
    else
    {
        qCritical() << "QMailHeartbeatTimerPrivate::init unable to open heartbeat ('" << strerror(errno) << "')";
    }
    qDebug() << "QMailHeartbeatTimerPrivate::initSource end";
}

QMailHeartbeatTimerPrivate::~QMailHeartbeatTimerPrivate()
{
    stop();
    g_source_destroy((GSource*)mSource);
    g_source_unref((GSource*)mSource);
}

gboolean QMailHeartbeatTimerPrivate::hbCallback(gpointer data)
{
    qDebug() << "QMailHeartbeatTimerPrivate::hbCallback";
    Q_ASSERT(data != 0);

    QMailHeartbeatTimerPrivate* self = static_cast<QMailHeartbeatTimerPrivate*>(data);
    self->emit timeout();

    // return FALSE if we need to be removed from polling.
    return self->mSingleShot ? FALSE : TRUE;
}

void QMailHeartbeatTimerPrivate::setInterval(int interval)
{
    const int seconds = interval / 1000;
    const int dev = qMin(qMax(seconds / 8, 3), 30);  // deviation is not greater than 1/8 of interval, but 3-30 sec
    mMinInterval = qMax(seconds - dev, 1); // not earlier than in 1 sec
    mMaxInterval = seconds + dev;
}

void QMailHeartbeatTimerPrivate::setInterval(int minimum, int maximum)
{
    mMinInterval = qMax(minimum / 1000, 1);
    mMaxInterval = qMax(maximum / 1000, mMinInterval + 3);
}

bool QMailHeartbeatTimerPrivate::start()
{
    qDebug() << "QMailHeartbeatTimerPrivate::start";

    if (mSource && mSource->iphb)
    {
        stop();

        mIsActive = true;
        iphb_wait(mSource->iphb, mMinInterval, mMaxInterval, 0);

        mSource->poll.fd = iphb_get_fd(mSource->iphb);
        mSource->poll.events = G_IO_IN;
        g_source_add_poll((GSource*)mSource, &mSource->poll);

        qDebug() << "QMailHeartbeatTimerPrivate::start end";
        return true;
    }
    else
    {
        qCritical() << "QMailHeartbeatTimerPrivate::start: mSource or iphb isn't initialized.";
        return false;
    }
}

bool QMailHeartbeatTimerPrivate::start(int msecs)
{
    qDebug() << "QMailHeartbeatTimerPrivate::start start msecs=" << msecs;

    setInterval(msecs);
    return start();
}

bool QMailHeartbeatTimerPrivate::isActive() const
{
    return mIsActive;
}

void QMailHeartbeatTimerPrivate::stop()
{
    qDebug() << "QMailHeartbeatTimerPrivate::stop";
    if (mIsActive)
    {
        g_source_remove_poll((GSource*)mSource, &mSource->poll);
        mIsActive = false;
    }
}

gboolean QMailHeartbeatTimerPrivate::prepareSource(GSource* /*source*/, gint *timeout)
{
    *timeout = -1;
    return FALSE;
}

gboolean QMailHeartbeatTimerPrivate::checkSource(GSource* source)
{
    return reinterpret_cast<HBSource*>(source)->poll.revents != 0;
}

gboolean QMailHeartbeatTimerPrivate::dispatchSource(GSource *source, GSourceFunc callback, gpointer userData)
{
    QMailHeartbeatTimerPrivate* self = static_cast<QMailHeartbeatTimerPrivate*>(userData);

    if (callback(userData))
    {
        HBSource* hbSource = reinterpret_cast<HBSource*>(source);
        g_source_remove_poll(source, &hbSource->poll);

        iphb_wait(self->mSource->iphb, self->mMinInterval, self->mMaxInterval, 0);

        hbSource->poll.fd = iphb_get_fd(hbSource->iphb);
        hbSource->poll.events = G_IO_IN;
        hbSource->poll.revents = 0;

        g_source_add_poll(source, &hbSource->poll);

        return TRUE;
    }
    else
    {
        self->mIsActive = false;
        return FALSE;
    }
}

void QMailHeartbeatTimerPrivate::finalizeSource(GSource* source)
{
    qDebug() << "QMailHeartbeatTimerPrivate::finalizeSource";
    HBSource* hbSource = reinterpret_cast<HBSource*>(source);
    hbSource->iphb = iphb_close(hbSource->iphb);
}

QMailHeartbeatTimer::QMailHeartbeatTimer(QObject *parent)
    : QObject(parent), d(new QMailHeartbeatTimerPrivate())
{
    connect(d.data(), SIGNAL(timeout()), this, SIGNAL(timeout()));
}

QMailHeartbeatTimer::~QMailHeartbeatTimer()
{
}

bool QMailHeartbeatTimer::isActive() const
{
    return d->isActive();
}

int QMailHeartbeatTimer::timerId() const
{
    return 0;
}

void QMailHeartbeatTimer::setInterval(int interval)
{
    d->setInterval(interval);
}

void QMailHeartbeatTimer::setInterval(int minimum, int maximum)
{
    d->setInterval(minimum, maximum);
}

QPair<int, int> QMailHeartbeatTimer::interval() const
{
    return d->interval();
}

void QMailHeartbeatTimer::setSingleShot(bool singleShot)
{
    d->setSingleShot(singleShot);
}

bool QMailHeartbeatTimer::isSingleShot() const
{
    return d->isSingleShot();
}

void QMailHeartbeatTimer::singleShot(int minimum, int maximum, QObject *receiver, const char *member)
{
    QMailHeartbeatTimerPrivate* timer = new QMailHeartbeatTimerPrivate;
    timer->setSingleShot(true);
    timer->setInterval(minimum, maximum);
    connect(timer, SIGNAL(timeout()), receiver, member);
    connect(timer, SIGNAL(timeout()), timer, SLOT(deleteLater()));
    timer->start();
}

void QMailHeartbeatTimer::start(int interval)
{
    d->setInterval(interval);
    d->start();
}

void QMailHeartbeatTimer::start(int minimum, int maximum)
{
    d->setInterval(minimum, maximum);
    d->start();
}

void QMailHeartbeatTimer::start()
{
    d->start();
}

void QMailHeartbeatTimer::stop()
{
    d->stop();
}



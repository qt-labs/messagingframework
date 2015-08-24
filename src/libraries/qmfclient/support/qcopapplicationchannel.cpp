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

#include "qcopapplicationchannel_p.h"
#include "qcopchannel_p_p.h"
#include <QtCore/qcoreapplication.h>

/* ! - documentation comments in this file are disabled:
    \class QCopApplicationChannel
    \inpublicgroup QtBaseModule
    \ingroup qws
    \brief The QCopApplicationChannel class provides access to QCop messages that were specifically sent to this application.

    \sa QCopChannel, QCopServer
*/

// Get the name of the application-specific channel, based on the pid.
static QString applicationChannelName()
{
    return QLatin1String("QPE/Pid/") +
           QString::number(QCoreApplication::applicationPid());
}

/* !
    Constructs a new QCop channel for the application's private channel with
    parent object \a parent
*/
QCopApplicationChannel::QCopApplicationChannel(QObject *parent)
    : QCopChannel(applicationChannelName(), parent)
{
    d = 0;

    QCopThreadData *td = QCopThreadData::instance();
    connect(td->clientConnection(), SIGNAL(startupComplete()),
            this, SIGNAL(startupComplete()));
}

/* !
    Destroys this QCopApplicationChannel object.
*/
QCopApplicationChannel::~QCopApplicationChannel()
{
}

/* !
    Returns true if application channel startup has completed and the
    startupComplete() signal has already been issued.

    \sa startupComplete()
*/
bool QCopApplicationChannel::isStartupComplete() const
{
    QCopThreadData *td = QCopThreadData::instance();
    return td->clientConnection()->isStartupComplete;
}

/* !
    \fn void QCopApplicationChannel::startupComplete()

    This signal is emitted once the QCop server has forwarded all queued
    messages for the application channel at startup time.

    If the application is running in background mode and does not have a
    user interface, it may elect to exit once all of the queued messages
    have been received and processed.

    \sa isStartupComplete()
*/

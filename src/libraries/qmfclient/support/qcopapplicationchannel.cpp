/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qcopapplicationchannel.h"
#include "qcopchannel_p.h"
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

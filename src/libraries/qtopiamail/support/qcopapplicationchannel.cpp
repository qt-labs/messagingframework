/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
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

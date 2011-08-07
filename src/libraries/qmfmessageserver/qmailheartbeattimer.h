/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QMAILHEARTBEATTIMER_H
#define QMAILHEARTBEATTIMER_H

#include <QObject>
#include <QTimer>
#include <QPair>
#include <qmailglobal.h>

class MESSAGESERVER_EXPORT QMailHeartbeatTimer : public QObject
{
    Q_OBJECT
public:
    explicit QMailHeartbeatTimer(QObject *parent = 0);
    ~QMailHeartbeatTimer();

    bool isActive() const;

    void setInterval(int interval);
    void setInterval(int minimum, int maximum);
    QPair<int, int> interval() const;

    void setSingleShot(bool singleShot);
    bool isSingleShot() const;
    void wokeUp();

    static void singleShot(int interval, QObject *receiver, const char *member);
    static void singleShot(int minimum, int maximum, QObject *receiver, const char *member);

public slots:
    void start(int interval);
    void start(int minimum, int maximum);

    void start();
    void stop();

signals:
    void timeout();

private:
    Q_DISABLE_COPY(QMailHeartbeatTimer)

    class QMailHeartbeatTimerPrivate* const d_ptr;
    Q_DECLARE_PRIVATE(QMailHeartbeatTimer)
};

#endif

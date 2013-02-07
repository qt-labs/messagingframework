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

#ifndef QMAILTHREAD_H
#define QMAILTHREAD_H

#include "qmailglobal.h"
#include "qmailid.h"
#include "qmailaddress.h"
#include "qmailtimestamp.h"
#include "qprivateimplementation.h"
#include <QString>
#include <QList>
#include <QSharedData>

class QMailThreadPrivate;

class QMF_EXPORT QMailThread : public QPrivatelyImplemented<QMailThreadPrivate>
{
public:
    typedef QMailThreadPrivate ImplementationType;

    QMailThread();
    explicit QMailThread(const QMailThreadId &id);

    QMailThreadId id() const;
    void setId(const QMailThreadId& id);

    QMailAccountId parentAccountId() const;
    void setParentAccountId(const QMailAccountId& id);

    QString serverUid() const;
    void setServerUid(const QString& serverUid);

    uint unreadCount() const;
    void setUnreadCount(uint value);

    uint messageCount() const;
    void setMessageCount(uint value);

    QString subject() const;
    void setSubject(const QString &value);

    QMailAddressList senders() const;
    void setSenders(const QMailAddressList &value);
    void addSender(const QMailAddress &value);

    QString preview() const;
    void setPreview(const QString &value);

    QMailTimeStamp lastDate() const;
    void setLastDate(const QMailTimeStamp &value);

    QMailTimeStamp startedDate() const;
    void setStartedDate(const QMailTimeStamp &value);

    quint64 status() const;
    void setStatus(quint64 value);

private:
};

typedef QList<QMailThread> QMailThreadList;

#endif

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

#ifndef STATUSMONITOR_H
#define STATUSMONITOR_H

#include <QObject>
#include <qmailserviceaction.h>

class StatusMonitor;
class QMailServiceAction;

class StatusItem : public QObject
{
    Q_OBJECT

signals:
    void finished();
    void progressChanged(uint,uint);
    void statusChanged(const QString& message);

public:
    virtual QWidget* widget() = 0;
    virtual QPair<uint,uint> progress() const = 0;
    virtual QString status() const = 0;
};

class ServiceActionStatusItem : public StatusItem
{
    Q_OBJECT

public:
    ServiceActionStatusItem(QMailServiceAction* sa, const QString& description);
    QWidget* widget();
    QPair<uint,uint> progress() const;
    QString status() const;

private slots:
    void serviceActivityChanged(QMailServiceAction::Activity a);
    void serviceStatusChanged(const QMailServiceAction::Status& s);

private:
    QMailServiceAction* m_serviceAction;
    QString m_description;
};

#ifdef DEFINE_STATUS_MONITOR_INSTANCE
static StatusMonitor* StatusMonitorInstance();
#endif

class StatusMonitor : public QObject
{
    Q_OBJECT

public:
    static StatusMonitor* instance();
    void add(StatusItem* newItem);
    int itemCount() const;

signals:
    void added(StatusItem* s);
    void removed(StatusItem* s);
    void progressChanged(uint,uint);
    void statusChanged(const QString& msg);

private slots:
    void statusItemFinished();
    void statusItemProgressChanged();
    void statusItemDestroyed(QObject*);

private:
    StatusMonitor();
    void updateProgress();
#ifdef DEFINE_STATUS_MONITOR_INSTANCE
    friend StatusMonitor* StatusMonitorInstance();
#endif

private:
    QList<StatusItem*> m_statusItems;
};

#endif

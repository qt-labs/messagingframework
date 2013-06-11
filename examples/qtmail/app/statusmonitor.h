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

class StatusMonitor : public QObject
{
    Q_OBJECT

public:
    StatusMonitor();
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
    void updateProgress();

private:
    QList<StatusItem*> m_statusItems;
};

#endif

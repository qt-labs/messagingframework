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

#include "statusmonitorwidget.h"
#include "statusmonitor.h"
#include <qmailserviceaction.h>
#include <QLayout>
#include <QScrollArea>
#include <QResizeEvent>

StatusMonitorWidget::StatusMonitorWidget(QWidget* parent, uint bottomMargin, uint rightMargin)
:
QFrame(parent),
m_layout(0),
m_bottomMargin(bottomMargin),
m_rightMargin(rightMargin)
{
    init();
    if(parent)
        parent->installEventFilter(this);
    connect(StatusMonitor::instance(),SIGNAL(added(StatusItem*)),this,
            SLOT(statusAdded(StatusItem*)));
    connect(StatusMonitor::instance(),SIGNAL(removed(StatusItem*)),this,
            SLOT(statusRemoved(StatusItem*)));
}

QSize StatusMonitorWidget::sizeHint() const
{
    int totalWidth = 0;
    int totalHeight = 0;
    foreach(QWidget* asw, m_statusWidgets)
    {
        totalHeight += asw->sizeHint().height();
        totalWidth = asw->sizeHint().width();
    }
    return QSize(totalWidth,totalHeight);
}

bool StatusMonitorWidget::eventFilter(QObject* source, QEvent* event)
{
    if(source == parent() && isVisible() && event->type() == QEvent::Resize)
        repositionToParent();
    return QWidget::eventFilter(source,event);
}

void StatusMonitorWidget::resizeEvent(QResizeEvent*)
{
    repositionToParent();
}

void StatusMonitorWidget::statusAdded(StatusItem* s)
{
    QWidget* statusWidget = s->widget();
    m_statusWidgets.insert(s,statusWidget);
    m_layout->addWidget(statusWidget);
    adjustSize();
}

void StatusMonitorWidget::statusRemoved(StatusItem* s)
{
    if(QWidget* w = m_statusWidgets.value(s))
    {
        m_statusWidgets.remove(s);
        w->deleteLater();
    }
    adjustSize();
}

void StatusMonitorWidget::init()
{
    setAutoFillBackground(true);
    setFrameStyle(QFrame::Sunken | QFrame::Panel);

    m_layout = new QVBoxLayout(this);
    m_layout->setSpacing(0);
    m_layout->setContentsMargins(0,0,0,0);
}

void StatusMonitorWidget::repositionToParent()
{
    QWidget* parentWidget = qobject_cast<QWidget*>(parent());
    if(!parentWidget)
        return;
    int x = parentWidget->width() - m_rightMargin - width();
    int y = parentWidget->height() - m_bottomMargin - height();
    move(x,y);
}


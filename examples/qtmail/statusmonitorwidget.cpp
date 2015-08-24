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


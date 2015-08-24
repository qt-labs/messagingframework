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

#include "folderdelegate.h"
#include <QAbstractItemView>
#include <QKeyEvent>
#include <QPainter>
#include <QScrollBar>

FolderDelegate::FolderDelegate(QAbstractItemView *parent)
    : QItemDelegate(parent),
      _parent(parent),
      _scrollBar(parent ? parent->verticalScrollBar() : 0),
      m_showStatus(true)
{
}

FolderDelegate::FolderDelegate(QWidget *parent)
    : QItemDelegate(parent),
      _parent(parent),
      _scrollBar(0),
      m_showStatus(true)
{
}

void FolderDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    const_cast<FolderDelegate*>(this)->init(option, index);
    QItemDelegate::paint(painter, option, index);
}

void FolderDelegate::drawDisplay(QPainter *painter, const QStyleOptionViewItem &option, const QRect &originalRect, const QString &text) const
{
    static const int smoothListScrollBarWidth = 6;

    // Reduce the available width by the scrollbar size, if necessary
    QRect rect(originalRect);
    if (_scrollBar && _scrollBar->isVisible())
        rect.setWidth(rect.width() - _parent->style()->pixelMetric(QStyle::PM_ScrollBarExtent));
    else if (!_scrollBar)
        rect.setWidth(rect.width() - smoothListScrollBarWidth);

    int tw = 0;
    if (!_statusText.isEmpty()) {
        QFontMetrics fontMetrics(option.font);
        tw = fontMetrics.width(_statusText);
    }

    QRect textRect(rect);
    textRect.setWidth(rect.width() - tw);
    QItemDelegate::drawDisplay(painter, option, textRect, text);

    if (tw) {
        static const int margin = 5;

        QRect statusRect = option.direction == Qt::RightToLeft
            ? QRect(0, rect.top(), tw + margin, rect.height())
            : QRect(rect.left()+rect.width()-tw-margin, rect.top(), tw, rect.height());
        if(m_showStatus)
            painter->drawText(statusRect, Qt::AlignCenter, _statusText);
    }
}

QSize FolderDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    // Ensure that we use the full width for our item
    QSize base(QItemDelegate::sizeHint(option,index));
    return QSize(qMax(base.width(), option.rect.width()), base.height());
}

bool FolderDelegate::showStatus() const
{
    return m_showStatus;
}

void FolderDelegate::setShowStatus(bool val)
{
    m_showStatus = val;
}

void FolderDelegate::init(const QStyleOptionViewItem &option, const QModelIndex &index)
{
    _statusText = index.data(FolderModel::FolderStatusRole).value<QString>();

    Q_UNUSED(option)
}



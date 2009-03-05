/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
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
      _scrollBar(parent ? parent->verticalScrollBar() : 0)
{
}

FolderDelegate::FolderDelegate(QWidget *parent)
    : QItemDelegate(parent),
      _parent(parent),
      _scrollBar(0)
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

        painter->drawText(statusRect, Qt::AlignCenter, _statusText);
    }
}

void FolderDelegate::drawDecoration(QPainter *painter, const QStyleOptionViewItem &option, const QRect &rect, const QVariant &decoration) const
{
    if (!rect.isValid())
        return;

    // If we have an icon, we ignore the pixmap
    if (decoration.type() == QVariant::Icon) {
        QIcon icon = qvariant_cast<QIcon>(decoration);

        QIcon::Mode mode(QIcon::Normal);
        if (!(option.state & QStyle::State_Enabled))
            mode = QIcon::Disabled;

        QIcon::State state(option.state & QStyle::State_Open ? QIcon::On : QIcon::Off);
        icon.paint(painter, rect, option.decorationAlignment, mode, state);
    }
}

QSize FolderDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    // Ensure that we use the full width for our item
    QSize base(QItemDelegate::sizeHint(option,index));
    return QSize(qMax(base.width(), option.rect.width()), base.height());
}

void FolderDelegate::init(const QStyleOptionViewItem &option, const QModelIndex &index)
{
    _statusText = index.data(FolderModel::FolderStatusRole).value<QString>();

    Q_UNUSED(option)
}



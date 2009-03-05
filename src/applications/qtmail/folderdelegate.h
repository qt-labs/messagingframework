/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef FOLDERDELEGATE_H
#define FOLDERDELEGATE_H

#include "foldermodel.h"
#ifdef QMAIL_QTOPIA
#include <QtopiaItemDelegate>
#else
#include <QItemDelegate>
#endif

class QAbstractItemView;
class QScrollBar;

#ifdef QMAIL_QTOPIA
class FolderDelegate : public QtopiaItemDelegate
#else
class FolderDelegate : public QItemDelegate
#endif
{
    Q_OBJECT

public:
#ifdef QMAIL_QTOPIA
    using QtopiaItemDelegate::drawDecoration;
#endif

    FolderDelegate(QAbstractItemView *parent = 0);
    FolderDelegate(QWidget *parent = 0);

    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    virtual void drawDisplay(QPainter *painter, const QStyleOptionViewItem &option, const QRect &rect, const QString &text) const;
    virtual void drawDecoration(QPainter *painter, const QStyleOptionViewItem &option, const QRect &rect, const QVariant &decoration) const;

    virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;

protected:
    virtual void init(const QStyleOptionViewItem &option, const QModelIndex &index);

    QWidget *_parent;
    QScrollBar *_scrollBar;
    QString _statusText;
};

#endif

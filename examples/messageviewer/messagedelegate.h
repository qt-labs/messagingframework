/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef MESSAGEDELEGATE_H
#define MESSAGEDELEGATE_H

#include <QAbstractItemDelegate>
#include <QSize>

class QModelIndex;
class QPainter;
class QStyleOptionViewItem;

class MessageDelegate : public QAbstractItemDelegate
{
    Q_OBJECT

public:
    explicit MessageDelegate(QObject* parent = 0);
    virtual ~MessageDelegate();

    enum Role
    {
        SubLabelRole = Qt::UserRole,
        SecondaryDecorationRole
    };

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const;
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
};

#endif

/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef EMAILFOLDERVIEW_H
#define EMAILFOLDERVIEW_H

#include "emailfoldermodel.h"
#include "folderdelegate.h"
#include "folderview.h"

class EmailFolderView : public FolderView
{
    Q_OBJECT

public:
    EmailFolderView(QWidget *parent);

    virtual EmailFolderModel *model() const;
    void setModel(EmailFolderModel *model);

private:
    virtual void setModel(QAbstractItemModel *model);

    EmailFolderModel *mModel;
};


class EmailFolderDelegate : public FolderDelegate
{
    Q_OBJECT

public:
#ifdef QMAIL_QTOPIA
    using QtopiaItemDelegate::drawDecoration;
#endif

    EmailFolderDelegate(EmailFolderView *parent = 0);

    virtual void drawDisplay(QPainter *painter, const QStyleOptionViewItem &option, const QRect &rect, const QString &text) const;
    virtual void drawDecoration(QPainter *painter, const QStyleOptionViewItem &option, const QRect &rect, const QVariant &decoration) const;

private:
    virtual void init(const QStyleOptionViewItem &option, const QModelIndex &index);

    bool _unsynchronized;
};

#endif

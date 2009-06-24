/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Qt Software Information (qt-info@nokia.com)
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the either Technology Preview License Agreement or the
** Beta Release License Agreement.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/

#include "emailfolderview.h"
#include <QPainter>

EmailFolderView::EmailFolderView(QWidget *parent)
    : FolderView(parent),
      mModel(0),
      mDelegate(new EmailFolderDelegate(this))
{
    setItemDelegate(mDelegate);
}

EmailFolderModel *EmailFolderView::model() const
{
    return mModel;
}

void EmailFolderView::setModel(EmailFolderModel *model)
{
    mModel = model;
    FolderView::setModel(model);

    if (!mModel->isEmpty()) {
        setCurrentIndex(mModel->index(0, 0, QModelIndex()));

        // Expand the inbox to show accounts
        //QModelIndex inboxIndex(mModel->indexFromFolderId(QMailFolderId(QMailFolder::InboxFolder)));
        QModelIndex inboxIndex(mModel->index(0, 0, QModelIndex()));
        expand(inboxIndex);
    }
}

void EmailFolderView::setModel(QAbstractItemModel *)
{
    qWarning() << "EmailFolderView requires a model of type: EmailFolderModel!";
}


EmailFolderDelegate::EmailFolderDelegate(EmailFolderView *parent)
    : FolderDelegate(parent),
      _unsynchronized(false)
{
}

void EmailFolderDelegate::drawDisplay(QPainter *painter, const QStyleOptionViewItem &option, const QRect &originalRect, const QString &text) const
{
    if (_unsynchronized) {
        painter->save();
        painter->setOpacity(0.5);
    }

    FolderDelegate::drawDisplay(painter, option, originalRect, text);

    if (_unsynchronized)
        painter->restore();
}

void EmailFolderDelegate::drawDecoration(QPainter *painter, const QStyleOptionViewItem &option, const QRect &rect, const QVariant &decoration) const
{
    if (_unsynchronized) {
        painter->save();
        painter->setOpacity(0.5);
    }

    FolderDelegate::drawDecoration(painter, option, rect, decoration);

    if (_unsynchronized)
        painter->restore();
}

void EmailFolderDelegate::init(const QStyleOptionViewItem &option, const QModelIndex &index)
{
    FolderDelegate::init(option, index);

    // Don't show the excess indicators if this item is expanded
    if (static_cast<EmailFolderView*>(_parent)->isExpanded(index)) {
        // Don't show the indicators for hidden counts when they're not hidden
        _statusText.remove(FolderModel::excessIndicator());

        // Don't show an empty unread count
        if (_statusText.startsWith("0/"))
            _statusText.remove(0, 2);

        // Don't show a zero count
        if (_statusText == "0")
            _statusText = QString();
    }

    _unsynchronized = !index.data(EmailFolderModel::FolderSynchronizationEnabledRole).value<bool>();
}


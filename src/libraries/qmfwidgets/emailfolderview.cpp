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

#include "emailfolderview.h"
#include <QPainter>

EmailFolderView::EmailFolderView(QWidget *parent)
    : FolderView(parent),
      mModel(0),
      mDelegate(new EmailFolderDelegate(this))
{
    setItemDelegate(mDelegate);
    setUniformRowHeights(true);
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

void EmailFolderDelegate::init(const QStyleOptionViewItem &option, const QModelIndex &index)
{
    FolderDelegate::init(option, index);

    // Don't show the excess indicators if this item is expanded
    if (static_cast<EmailFolderView*>(_parent)->isExpanded(index)) {
        // Don't show the indicators for hidden counts when they're not hidden
        _statusText.remove(FolderModel::excessIndicator());

        // Don't show an empty unread count
        if (_statusText.startsWith(QLatin1String("0/")))
            _statusText.remove(0, 2);

        // Don't show a zero count
        if (_statusText == QLatin1String("0"))
            _statusText.clear();
    }

    _unsynchronized = !index.data(EmailFolderModel::FolderSynchronizationEnabledRole).value<bool>();
}


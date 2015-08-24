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

#ifndef FOLDERVIEW_H
#define FOLDERVIEW_H

#include "foldermodel.h"
#include "folderdelegate.h"
#include <QSet>
#include <QTreeView>
#include <QPointer>
class QMFUTIL_EXPORT FolderView : public QTreeView
{
    Q_OBJECT

public:
    FolderView(QWidget *parent);
    virtual ~FolderView();

    virtual FolderModel *model() const = 0;

    QMailMessageSet *currentItem() const;
    bool setCurrentItem(QMailMessageSet *item);

    QMailAccountId currentAccountId() const;
    bool setCurrentAccountId(const QMailAccountId& id);

    QMailFolderId currentFolderId() const;
    bool setCurrentFolderId(const QMailFolderId& id);

    bool ignoreMailStoreUpdates() const;
    void setIgnoreMailStoreUpdates(bool ignore);

    virtual void setModel(QAbstractItemModel *model);

Q_SIGNALS:
    void selected(QMailMessageSet *);
    void activated(QMailMessageSet *);
    void selectionUpdated();
    void backPressed();

protected Q_SLOTS:
    virtual void itemSelected(const QModelIndex &index);
    virtual void itemActivated(const QModelIndex &index);
    virtual void itemExpanded(const QModelIndex &index);
    virtual void itemCollapsed(const QModelIndex &index);
    virtual void currentChanged(const QModelIndex &currentIndex, const QModelIndex &previousIndex);
    virtual void dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int> &roles = QVector<int>());
    virtual void modelReset();

protected:
    virtual void keyPressEvent(QKeyEvent *e);
    virtual void showEvent(QShowEvent *e);
    virtual void hideEvent(QHideEvent *e);

private:
    template<typename SetType>
    bool expandSet(SetType &ids, FolderModel *model);

    template<typename SetType>
    void removeNonexistent(SetType &ids, FolderModel *model);

    bool expandKeys(QSet<QByteArray> &keys, FolderModel *model);
    bool expandAccounts(QSet<QMailAccountId> &accountIds, FolderModel *model);
    bool expandFolders(QSet<QMailFolderId> &folderIds, FolderModel *model);

    QSet<QMailAccountId> expandedAccounts;
    QSet<QMailFolderId> expandedFolders;
    QSet<QByteArray> expandedKeys;
    QPointer<QMailMessageSet> lastItem;
    QPointer<QAbstractItemModel> oldModel;
};

#endif

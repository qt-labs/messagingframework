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

signals:
    void selected(QMailMessageSet *);
    void activated(QMailMessageSet *);
    void selectionUpdated();
    void backPressed();

protected slots:
    virtual void itemSelected(const QModelIndex &index);
    virtual void itemActivated(const QModelIndex &index);
    virtual void itemExpanded(const QModelIndex &index);
    virtual void itemCollapsed(const QModelIndex &index);
    virtual void currentChanged(const QModelIndex &currentIndex, const QModelIndex &previousIndex);
    virtual void dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);
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

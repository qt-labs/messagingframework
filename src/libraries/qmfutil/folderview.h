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

#ifndef FOLDERVIEW_H
#define FOLDERVIEW_H

#include "foldermodel.h"
#include "folderdelegate.h"
#include <QSet>
#include <QTreeView>

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

signals:
    void selected(QMailMessageSet *);
    void activated(QMailMessageSet *);
    void backPressed();

protected slots:
    virtual void itemSelected(const QModelIndex &index);
    virtual void itemActivated(const QModelIndex &index);
    virtual void itemExpanded(const QModelIndex &index);
    virtual void itemCollapsed(const QModelIndex &index);
    virtual void currentChanged(const QModelIndex &currentIndex, const QModelIndex &previousIndex);

protected:
    virtual void keyPressEvent(QKeyEvent *e);
    virtual void showEvent(QShowEvent *e);
    virtual void hideEvent(QHideEvent *e);

    virtual void modelReset();

private:
    template<typename SetType>
    bool expandSet(SetType &ids, FolderModel *model);

    template<typename SetType>
    void removeNonexistent(SetType &ids, FolderModel *model);

    bool expandAccounts(QSet<QMailAccountId> &accountIds, FolderModel *model);
    bool expandFolders(QSet<QMailFolderId> &folderIds, FolderModel *model);

    QSet<QMailAccountId> expandedAccounts;
    QSet<QMailFolderId> expandedFolders;
};

#endif

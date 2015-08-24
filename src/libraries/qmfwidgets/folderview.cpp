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

#include "folderview.h"
#include <QKeyEvent>
#include <qmailmessagekey.h>
#include <qdatastream.h>

FolderView::FolderView(QWidget *parent)
    : QTreeView(parent),
      lastItem(0),
      oldModel(0)
{
    connect(this, SIGNAL(activated(QModelIndex)), this, SLOT(itemActivated(QModelIndex)));
    connect(this, SIGNAL(expanded(QModelIndex)), this, SLOT(itemExpanded(QModelIndex)));
    connect(this, SIGNAL(collapsed(QModelIndex)), this, SLOT(itemCollapsed(QModelIndex)));
}

FolderView::~FolderView()
{
}

QMailMessageSet *FolderView::currentItem() const
{
    if (FolderModel *folderModel = model())
        return folderModel->itemFromIndex(currentIndex());

    return 0;
}

bool FolderView::setCurrentItem(QMailMessageSet *item)
{
    if (FolderModel *folderModel = model()) {
        QModelIndex index(folderModel->indexFromItem(item));
        if (index.isValid()) {
            setCurrentIndex(index);
            return true;
        }
    }

    return false;
}

QMailAccountId FolderView::currentAccountId() const
{
    if (FolderModel *folderModel = model())
        return folderModel->accountIdFromIndex(currentIndex());

    return QMailAccountId();
}

bool FolderView::setCurrentAccountId(const QMailAccountId& id)
{
    if (FolderModel *folderModel = model()) {
        QModelIndex index(folderModel->indexFromAccountId(id));
        if (index.isValid()) {
            setCurrentIndex(index);
            return true;
        }
    }

    return false;
}

QMailFolderId FolderView::currentFolderId() const
{
    if (FolderModel *folderModel = model())
        return folderModel->folderIdFromIndex(currentIndex());

    return QMailFolderId();
}

bool FolderView::setCurrentFolderId(const QMailFolderId& id)
{
    if (FolderModel *folderModel = model()) {
        QModelIndex index(folderModel->indexFromFolderId(id));
        if (index.isValid()) {
            setCurrentIndex(index);
            return true;
        }
    }

    return false;
}

bool FolderView::ignoreMailStoreUpdates() const
{
    if (FolderModel *folderModel = model())
        return folderModel->ignoreMailStoreUpdates();

    return false;
}

void FolderView::setIgnoreMailStoreUpdates(bool ignore)
{
    if (FolderModel *folderModel = model())
        folderModel->setIgnoreMailStoreUpdates(ignore);
}

void FolderView::itemActivated(const QModelIndex &index)
{
    if (FolderModel *folderModel = model())
        if (QMailMessageSet *item = folderModel->itemFromIndex(index))
            emit activated(item);
}

void FolderView::itemSelected(const QModelIndex &index)
{
    if (FolderModel *folderModel = model())
        if (QMailMessageSet *item = folderModel->itemFromIndex(index))
            emit selected(item);
}

static QByteArray arrayFromKey(const QMailMessageKey &key)
{
    QByteArray array;
    QDataStream stream(&array, QIODevice::WriteOnly);
    key.serialize<QDataStream>(stream);
    return array;
}

void FolderView::itemExpanded(const QModelIndex &index)
{
    if (FolderModel *folderModel = model()) {

        QMailFolderId folderId = folderModel->folderIdFromIndex(index);
        if (folderId.isValid()) {
            expandedFolders.insert(folderId);
            return;
        }
        
        QMailAccountId accountId = folderModel->accountIdFromIndex(index);
        if (accountId.isValid()) {
            expandedAccounts.insert(accountId);
            return;
        }

        QMailMessageSet *item = folderModel->itemFromIndex(index);
        if (item) {
            QMailMessageKey key = item->messageKey();
            expandedKeys.insert(arrayFromKey(key));
            return;
        }
    }
}

void FolderView::itemCollapsed(const QModelIndex &index)
{
    if (FolderModel *folderModel = model()) {

        QMailFolderId folderId = folderModel->folderIdFromIndex(index);
        if (folderId.isValid()) {
            expandedFolders.remove(folderId);
            return;
        }

        QMailAccountId accountId = folderModel->accountIdFromIndex(index);
        if (accountId.isValid()) {
            expandedAccounts.remove(accountId);
            return;
        }

        QMailMessageSet *item = folderModel->itemFromIndex(index);
        if (item) {
            QMailMessageKey key = item->messageKey();
            expandedKeys.remove(arrayFromKey(key));
            return;
        }
    }
}

void FolderView::currentChanged(const QModelIndex &currentIndex, const QModelIndex &previousIndex)
{
    itemSelected(currentIndex);

    Q_UNUSED(previousIndex)
}

void FolderView::dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int> &roles)
{
    Q_UNUSED(roles)

    if(topLeft == currentIndex() || bottomRight == currentIndex())
        emit selectionUpdated();
    QTreeView::dataChanged(topLeft,bottomRight);
}

namespace {

template<typename IdType>
QModelIndex itemIndex(const IdType &id, FolderModel *folderModel);

template<>
QModelIndex itemIndex<QMailAccountId>(const QMailAccountId &id, FolderModel *folderModel)
{
    return folderModel->indexFromAccountId(id);
}

template<>
QModelIndex itemIndex<QMailFolderId>(const QMailFolderId &id, FolderModel *folderModel)
{
    return folderModel->indexFromFolderId(id);
}

template<>
QModelIndex itemIndex<QByteArray>(const QByteArray &key, FolderModel *folderModel)
{
    for (int i = 0; i < folderModel->count(); ++i) {
        if (arrayFromKey(folderModel->at(i)->messageKey()) == key) {
            return folderModel->indexFromItem(folderModel->at(i));
        }
    }
    return QModelIndex();
}
}

template<typename SetType>
bool FolderView::expandSet(SetType &ids, FolderModel *model)
{
    int originalCount = ids.count();
    int count = originalCount;
    int oldCount = count + 1;

    while (count && (count < oldCount)) {
        oldCount = count;

        typename SetType::iterator it = ids.begin();
        while (it != ids.end()) {
            QModelIndex index(itemIndex(*it, model));
            if (index.isValid()) {
                if (!isExpanded(index))
                    expand(index);

                if (isExpanded(index)) {
                    // We no longer need to expand this folder
                    it = ids.erase(it);
                    --count;
                }
                else
                {
                    ++it;
                }
            } else {
                ++it;
            }
        }
    }

    return (count != originalCount);
}

template<typename SetType>
void FolderView::removeNonexistent(SetType &ids, FolderModel *model)
{
    typename SetType::iterator it = ids.begin();
    while (it != ids.end()) {
        QModelIndex index(itemIndex(*it, model));
        if (!index.isValid()) {
            it = ids.erase(it);
        } else {
            ++it;
        }
    }
}

bool FolderView::expandKeys(QSet<QByteArray> &keys, FolderModel *model)
{
    return expandSet(keys, model);
}

bool FolderView::expandFolders(QSet<QMailFolderId> &folderIds, FolderModel *model)
{
    return expandSet(folderIds, model);
}

bool FolderView::expandAccounts(QSet<QMailAccountId> &accountIds, FolderModel *model)
{
    return expandSet(accountIds, model);
}

void FolderView::modelReset()
{
    if (FolderModel *folderModel = model()) {
        // Remove any items that are no longer in the model
        removeNonexistent(expandedKeys, folderModel);
        removeNonexistent(expandedAccounts, folderModel);
        removeNonexistent(expandedFolders, folderModel);

        // Ensure all the expanded items are re-expanded
        QSet<QByteArray> keys(expandedKeys);
        QSet<QMailAccountId> accountIds(expandedAccounts);
        QSet<QMailFolderId> folderIds(expandedFolders);

        bool itemsExpanded(false);
        do {
            itemsExpanded = false;

            // We need to repeat this process, because many items cannot be expanded until
            // their parent item is expanded...
            itemsExpanded |= expandKeys(keys, folderModel);
            itemsExpanded |= expandAccounts(accountIds, folderModel);
            itemsExpanded |= expandFolders(folderIds, folderModel);
        } while (itemsExpanded);

        // Any remainining IDs must not be accessible in the model any longer
        foreach (const QByteArray &key, keys)
            expandedKeys.remove(key);

        foreach (const QMailAccountId &accountId, accountIds)
            expandedAccounts.remove(accountId);

        foreach (const QMailFolderId &folderId, folderIds)
            expandedFolders.remove(folderId);
    }
}

void FolderView::keyPressEvent(QKeyEvent *e)
{
    switch (e->key())
    {
    case Qt::Key_Select:
    case Qt::Key_Space:
    case Qt::Key_Return:
    case Qt::Key_Enter:

        e->accept();
        itemActivated(currentIndex());
        break;

    case Qt::Key_Back:
        e->accept();
        emit backPressed();
        break;

    default:
        QTreeView::keyPressEvent(e);
    }
}

void FolderView::showEvent(QShowEvent *e)
{
    setIgnoreMailStoreUpdates(false);
    QTreeView::showEvent(e);
    if (lastItem) {
        setCurrentItem(lastItem);
    }
    lastItem = 0;
}

void FolderView::hideEvent(QHideEvent *e)
{
    lastItem = currentItem();
    setIgnoreMailStoreUpdates(true);
    QTreeView::hideEvent(e);
}

void FolderView::setModel(QAbstractItemModel *model)
{
    if (oldModel) {
        disconnect(oldModel, SIGNAL(reset()), this, SLOT(modelReset()));
    }
    QTreeView::setModel(model);
    oldModel = model;
    connect(model, SIGNAL(modelReset()), this, SLOT(modelReset()));
}

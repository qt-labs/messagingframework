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

#ifndef MESSAGELISTVIEW_H
#define MESSAGELISTVIEW_H

#include <QModelIndex>
#include <QTimer>
#include <QWidget>
#include <QCache>
#include <qmailmessage.h>
#include <qmailmessagekey.h>
#include <qmailmessagesortkey.h>

class MessageList;
class QMailMessageModelBase;
class QuickSearchWidget;

QT_BEGIN_NAMESPACE

class QFrame;
class QLineEdit;
class QPushButton;
class QSortFilterProxyModel;
class QTabBar;
class QToolButton;
class QItemDelegate;

QT_END_NAMESPACE

class MessageListView : public QWidget
{
    Q_OBJECT

public:
    MessageListView(QWidget* parent = Q_NULLPTR);
    virtual ~MessageListView();

    QMailMessageKey key() const;
    void setKey(const QMailMessageKey& key);

    QMailMessageSortKey sortKey() const;
    void setSortKey(const QMailMessageSortKey& sortKey);

    QMailFolderId folderId() const;
    void setFolderId(const QMailFolderId &id);

    QMailMessageId current() const;
    void setCurrent(const QMailMessageId& id);
    void setNextCurrent();
    void setPreviousCurrent();

    bool hasNext() const;
    bool hasPrevious() const;
    bool hasParent() const;
    bool hasChildren() const;

    int rowCount(const QModelIndex &parentIndex = QModelIndex()) const;

    QMailMessageIdList selected() const;
    void setSelected(const QMailMessageIdList& idList);
    void setSelected(const QMailMessageId& id);
    void selectAll();
    void clearSelection();

    bool markingMode() const;
    void setMarkingMode(bool set);

    bool moreButtonVisible() const;
    void setMoreButtonVisible(bool set);

    bool threaded() const;
    void setThreaded(bool set);

    bool ignoreUpdatesWhenHidden() const;
    void setIgnoreUpdatesWhenHidden(bool ignore);

    QMailMessageIdList visibleMessagesIds(bool buffer = true) const;

    bool showingQuickSearch() const;
    void showQuickSearch(bool val);

signals:
    void clicked(const QMailMessageId& id);
    void activated(const QMailMessageId& id);
    void currentChanged(const QMailMessageId& oldId, const QMailMessageId& newId);
    void selectionChanged();
    void rowCountChanged();
    void backPressed();
    void responseRequested(const QMailMessage&, QMailMessage::ResponseType);
    void moreClicked();
    void visibleMessagesChanged();
    void fullSearchRequested();
    void doubleClicked(const QMailMessageId& id);

public slots:
    void reset();
    void updateActions();

protected slots:
    void indexClicked(const QModelIndex& index);
    void indexActivated(const QModelIndex& index);
    void currentIndexChanged(const QModelIndex& currentIndex, const QModelIndex& previousIndex);
    void modelChanged();
    void rowsAboutToBeRemoved(const QModelIndex&, int, int);
    void layoutChanged();
    void reviewVisibleMessages();
    void scrollTimeout();
    void quickSearch(const QMailMessageKey& key);
    void quickSearchReset();
    void expandAll();
    void scrollTo(const QMailMessageId& id);
    void indexDoubleClicked(const QModelIndex& index);

protected:
    void showEvent(QShowEvent* e);
    void hideEvent(QHideEvent* e);

private:
    void init();
    bool eventFilter(QObject*, QEvent*);
    void selectedChildren(const QModelIndex &index, QMailMessageIdList *selectedIds) const;
    void selectChildren(const QModelIndex &index, bool selected, bool *modified);

private:
    MessageList* mMessageList;
    QMailMessageModelBase* mModel;
    bool mMarkingMode;
    bool mIgnoreWhenHidden;
    bool mSelectedRowsRemoved;
    QMailFolderId mFolderId;
    QMailAccountId mAccountId;
    QTimer mScrollTimer;
    QMailMessageIdList mPreviousVisibleItems;
    QuickSearchWidget* mQuickSearchWidget;
    QMailMessageKey mKey;
    bool mShowMoreButton;
    bool mThreaded;
    QTimer mExpandAllTimer;
    QCache<QByteArray, QMailMessageId> mPreviousCurrentCache;
    QMailMessageId mPreviousCurrent;
    bool mQuickSearchIsEmpty;
};

#endif

/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef MESSAGELISTVIEW_H
#define MESSAGELISTVIEW_H

#include <QWidget>
#include <qmailmessage.h>
#include <qmailmessagekey.h>
#include <qmailmessagesortkey.h>

class MessageList;
class QFrame;
class QLineEdit;
class QMailMessageListModel;
class QModelIndex;
class QPushButton;
class QSortFilterProxyModel;
class QTabBar;
class QToolButton;
class QItemDelegate;
class QMailMessageDelegate;
class QtopiaHomeMailMessageDelegate;
class MessageListModel;

class MessageListView : public QWidget
{
    Q_OBJECT

public:
    enum DisplayMode
    {
        DisplayMessages = 0,
        DisplayReceived = 1,
        DisplaySent = 2,
        DisplayDrafts = 3,
        DisplayTrash = 4,
        DisplayFilter = 5
    };

    MessageListView(QWidget* parent = 0);
    virtual ~MessageListView();

    QMailMessageKey key() const;
    void setKey(const QMailMessageKey& key);

    QMailMessageSortKey sortKey() const;
    void setSortKey(const QMailMessageSortKey& sortKey);

    DisplayMode displayMode() const;
    void setDisplayMode(const DisplayMode& m);

    QMailFolderId folderId() const;
    void setFolderId(const QMailFolderId &id);

    QMailMessageListModel* model() const;

    QMailMessageId current() const;
    void setCurrent(const QMailMessageId& id);
    void setNextCurrent();
    void setPreviousCurrent();

    bool hasNext() const;
    bool hasPrevious() const;

    int rowCount() const;

    QMailMessageIdList selected() const;
    void setSelected(const QMailMessageIdList& idList);
    void setSelected(const QMailMessageId& id);
    void selectAll();
    void clearSelection();

    bool markingMode() const;
    void setMarkingMode(bool set);

    bool ignoreUpdatesWhenHidden() const;
    void setIgnoreUpdatesWhenHidden(bool ignore);

signals:
    void clicked(const QMailMessageId& id);
    void currentChanged(const QMailMessageId& oldId, const QMailMessageId& newId);
    void selectionChanged();
    void displayModeChanged(MessageListView::DisplayMode);
    void backPressed();
    void resendRequested(const QMailMessage&, int);
    void moreClicked();

protected slots:
    void indexClicked(const QModelIndex& index);
    void currentIndexChanged(const QModelIndex& currentIndex, const QModelIndex& previousIndex);
    void filterTextChanged(const QString& text);
    void closeFilterButtonClicked();
    void tabSelected(int index);
    void modelChanged();
    void rowsAboutToBeRemoved(const QModelIndex&, int, int);
    void layoutChanged();

protected:
    void showEvent(QShowEvent* e);
    void hideEvent(QHideEvent* e);

private:
    void init();
    bool eventFilter(QObject*, QEvent*);

private:
    MessageList* mMessageList;
    QFrame* mFilterFrame;
    QLineEdit* mFilterEdit;
    QToolButton* mCloseFilterButton;
    QTabBar* mTabs;
    QPushButton* mMoreButton;
    MessageListModel* mModel;
    QSortFilterProxyModel* mFilterModel;
    DisplayMode mDisplayMode;
    bool mMarkingMode;
    bool mIgnoreWhenHidden;
    bool mSelectedRowsRemoved;
    QMailFolderId mFolderId;
};

#endif

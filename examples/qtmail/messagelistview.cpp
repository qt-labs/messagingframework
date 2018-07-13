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

#include "messagelistview.h"
#include <qmaillog.h>
#include <qmailaccount.h>
#include <qmailfolder.h>
#include <qmailfolderkey.h>
#include <qmailmessagethreadedmodel.h>
#include <qmailmessagelistmodel.h>
#include <QTreeView>
#include <QKeyEvent>
#include <QLayout>
#include <QLabel>
#include <QMap>
#include <QModelIndex>
#include <QPushButton>
#include <QTabBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QItemDelegate>
#include <QPainter>
#include <QComboBox>
#include <QLineEdit>
#include <QApplication>
#include <qmaildatacomparator.h>
#include <qtmailnamespace.h>

static QStringList headers(QStringList() << QObject::tr("Subject") << QObject::tr("Addressee") << QObject::tr("Date") << QObject::tr("Size"));
static const QColor newMessageColor(Qt::blue);

class QuickSearchWidget : public QWidget
{
    Q_OBJECT
public:
    QuickSearchWidget(QWidget* parent = Q_NULLPTR);

    QMailMessageKey searchKey() const;

public slots:
    void reset(bool emitReset = true);

signals:
    void quickSearchRequested(const QMailMessageKey& key);
    void quickSearchReset();
    void fullSearchRequested();

private slots:
    void searchTermsChanged();

private:
    QMailMessageKey buildSearchKey() const;

private:
    QLineEdit* m_searchTerms;
    QComboBox* m_statusCombo;
    QToolButton* m_clearButton;
    QMailMessageKey m_key;
};

QuickSearchWidget::QuickSearchWidget(QWidget* parent)
:
QWidget(parent)
{
    QHBoxLayout* layout = new QHBoxLayout(this);

    m_clearButton = new QToolButton(this);
    m_clearButton->setIcon(Qtmail::icon("clear"));
    connect(m_clearButton,SIGNAL(clicked()),this,SLOT(reset()));
    layout->addWidget(m_clearButton);

    layout->addWidget(new QLabel("Search:"));
    m_searchTerms = new QLineEdit(this);
    connect(m_searchTerms,SIGNAL(textChanged(QString)),this,SLOT(searchTermsChanged()));
    layout->addWidget(m_searchTerms);

    layout->addWidget(new QLabel("Status:"));

    m_statusCombo = new QComboBox(this);
    QMailMessageKey removed(QMailMessageKey::status(QMailMessage::Removed));
    m_statusCombo->addItem(QIcon(":icon/exec"),tr("Any Status"),~removed);
    m_statusCombo->addItem(QIcon(":icon/mail_generic"),tr("Unread"),QMailMessageKey::status(QMailMessage::Read,QMailDataComparator::Excludes)&~removed);
    m_statusCombo->addItem(QIcon(":icon/mail_generic"),tr("Important"),QMailMessageKey::status(QMailMessage::Important)&~removed);
    m_statusCombo->addItem(QIcon(":icon/new"),tr("New"),QMailMessageKey::status(QMailMessage::New)&~removed);
    m_statusCombo->addItem(QIcon(":icon/mail_reply"),tr("Replied"),QMailMessageKey::status(QMailMessage::Replied)&~removed);
    m_statusCombo->addItem(QIcon(":icon/mail_forward"),tr("Forwarded"),QMailMessageKey::status(QMailMessage::Forwarded)&~removed);
    m_statusCombo->addItem(QIcon(":/icon/attach"),tr("Has Attachment"),QMailMessageKey::status(QMailMessage::HasAttachments)&~removed);
    m_statusCombo->addItem(QIcon(":icon/exec"),tr("Removed"), removed);
    connect(m_statusCombo,SIGNAL(currentIndexChanged(int)),this,SLOT(searchTermsChanged()));
    layout->addWidget(m_statusCombo);

    m_key = buildSearchKey();
}

QMailMessageKey QuickSearchWidget::searchKey() const
{
    return m_key;
}

void QuickSearchWidget::reset(bool emitReset)
{
    blockSignals(!emitReset);
    m_statusCombo->setCurrentIndex(0);
    m_searchTerms->clear();
    blockSignals(false);
}

void QuickSearchWidget::searchTermsChanged()
{
    m_key = buildSearchKey();
    emit quickSearchRequested(m_key);
    if (m_searchTerms->text().isEmpty() && m_statusCombo->currentIndex() == 0)
        emit quickSearchReset();
}

QMailMessageKey QuickSearchWidget::buildSearchKey() const
{
    QMailMessageKey statusKey = qvariant_cast<QMailMessageKey>(m_statusCombo->itemData(m_statusCombo->currentIndex()));
    if(m_searchTerms->text().isEmpty() && m_statusCombo->currentIndex() == 0)
        return statusKey;

    QMailMessageKey subjectKey = QMailMessageKey::subject(m_searchTerms->text(),QMailDataComparator::Includes);
    QMailMessageKey senderKey = QMailMessageKey::sender(m_searchTerms->text(),QMailDataComparator::Includes);
    QMailMessageKey recipientsKey = QMailMessageKey::recipients(m_searchTerms->text(),QMailDataComparator::Includes);
    return ((subjectKey | senderKey | recipientsKey) & statusKey);
}


template <typename BaseModel>
class MessageListModel : public BaseModel
{
    // Doesn't work in template classes...
    //Q_OBJECT

    typedef BaseModel SuperType;

public:
    MessageListModel(MessageListView* parent);

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    int columnCount(const QModelIndex & parent = QModelIndex()) const;
    int rowCount(const QModelIndex& parent = QModelIndex()) const;

    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;

private:
    MessageListView *m_parent;
};

template <typename BaseModel>
MessageListModel<BaseModel>::MessageListModel(MessageListView *parent)
    : BaseModel(parent),
      m_parent(parent)
{
}

template <typename BaseModel>
QVariant MessageListModel<BaseModel>::headerData(int section, Qt::Orientation o, int role) const
{
    if (role == Qt::DisplayRole) {
        if (section < headers.count())
            return headers.at(section);
    }

    return SuperType::headerData(section, o, role);
}

template <typename BaseModel>
int MessageListModel<BaseModel>::columnCount(const QModelIndex & parent) const
{
    Q_UNUSED(parent);

    return headers.count();
}

template <typename BaseModel>
int MessageListModel<BaseModel>::rowCount(const QModelIndex &parentIndex) const
{
    int actualRows = SuperType::rowCount(parentIndex);

    if (!parentIndex.isValid() && m_parent->moreButtonVisible())
        actualRows++;

    return actualRows;
}

static QString dateString(const QDateTime& dt)
{
    QDateTime current = QDateTime::currentDateTime();
    //today
    if(dt.date() == current.date())
        return QString("Today %1").arg(dt.toString("h:mm:ss ap"));
    //yesterday
    else if(dt.daysTo(current) <= 1)
        return QString("Yesterday %1").arg(dt.toString("h:mm:ss ap"));
    //within 7 days
    else if(dt.daysTo(current) < 7)
        return dt.toString("dddd h:mm:ss ap");
    else return dt.toString("dd/MM/yy h:mm:ss ap");
}

template <typename BaseModel>
QVariant MessageListModel<BaseModel>::data(const QModelIndex & index, int role) const
{
    if (index.isValid()) {
        if (role == Qt::DisplayRole && index.isValid()) {
            switch(index.column())
            {
            case 0:
                return SuperType::data(index, QMailMessageModelBase::MessageSubjectTextRole);
            case 1:
                return SuperType::data(index, QMailMessageModelBase::MessageAddressTextRole);
            //case 2:
            //    return SuperType::data(index, QMailMessageModelBase::MessageTimeStampTextRole);
            case 3:
                return SuperType::data(index, QMailMessageModelBase::MessageSizeTextRole);
            default:
            break;
            }

            if (index.column() == 2) {
                // Customize the date display
                QMailMessageMetaData message(SuperType::idFromIndex(index));
                if (message.id().isValid()) {
                    return dateString(message.date().toLocalTime());
                }
            }
        } else if ((role == Qt::DecorationRole || role == Qt::CheckStateRole) && (index.column() > 0)) {
            return QVariant();
        } else if (role == Qt::CheckStateRole && !m_parent->markingMode()) {
            Qt::CheckState state = static_cast<Qt::CheckState>(SuperType::data(index,role).toInt());
            if (state == Qt::Unchecked)
                return QVariant();
        } else if (role == Qt::DecorationRole) {
            return QIcon(SuperType::data(index, QMailMessageModelBase::MessageStatusIconRole).toString());
        } else if (role == Qt::ForegroundRole) {
            QMailMessageMetaData message(SuperType::idFromIndex(index));
            if (message.id().isValid()) {
                quint64 status = message.status();
                bool unread = !(status & QMailMessage::Read);
                if (status & QMailMessage::PartialContentAvailable && unread)
                    return newMessageColor;
            }
        }
    }

    return SuperType::data(index, role);
}


class MessageList : public QTreeView
{
    Q_OBJECT

public:
    MessageList(MessageListView *parent);
    virtual ~MessageList();

    QPoint clickPos() const { return pos; }

signals:
    void backPressed();
    void scrolled();
    void moreButtonClicked();

protected:
    void keyPressEvent(QKeyEvent* e);
    void mouseReleaseEvent(QMouseEvent* e);
    void scrollContentsBy(int dx, int dy);
    void mouseMoveEvent(QMouseEvent* e);
    void mousePressEvent(QMouseEvent* e);
    void mouseDoubleClickEvent(QMouseEvent* e);
    bool mouseOverMoreLink(QMouseEvent* e);

    void drawRow(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

protected slots:
    void rowsInserted(const QModelIndex &parent, int start, int end);

private:
    MessageListView *m_parent;
    QPoint pos;
};

MessageList::MessageList(MessageListView *parent)
    : QTreeView(parent),
      m_parent(parent)
{
    setMouseTracking(true);
}

MessageList::~MessageList()
{
}

void MessageList::keyPressEvent(QKeyEvent* e)
{
    switch( e->key() ) {
        case Qt::Key_Space:
        case Qt::Key_Return:
        case Qt::Key_Select:
        case Qt::Key_Enter:
        {
            if(currentIndex().isValid())
                emit clicked(currentIndex());
        }
        break;
        case Qt::Key_No:
        case Qt::Key_Back:
        case Qt::Key_Backspace:
            emit backPressed();
        break;
        case Qt::Key_Plus:
        case Qt::Key_Minus:
        {
            QModelIndex index(currentIndex());
            if (!index.isValid())
                break;
            if (e->key() == Qt::Key_Plus) {
                index = indexBelow(index);
            } else if (e->key() == Qt::Key_Minus) {
                index = indexAbove(index);
            }
            while (index.isValid()) {
                QMailMessageId id(index.data(QMailMessageModelBase::MessageIdRole).value<QMailMessageId>());
                if (!id.isValid())
                    break;
                QMailMessageMetaData message(id);
                quint64 status = message.status();
                bool unread = !(status & QMailMessage::Read);
                if (unread) {
                    setCurrentIndex(index);
                    scrollTo(index);
                    QApplication::postEvent(this, new QKeyEvent(QEvent::KeyPress, Qt::Key_Enter, 0));
                    QApplication::postEvent(this, new QKeyEvent(QEvent::KeyRelease, Qt::Key_Enter, 0));
                    break;
                }
                if (e->key() == Qt::Key_Plus) {
                    index = indexBelow(index);
                } else if (e->key() == Qt::Key_Minus) {
                    index = indexAbove(index);
                }
            }
        }
        break;
        default: QTreeView::keyPressEvent(e);
    }
}

void MessageList::mouseReleaseEvent(QMouseEvent* e)
{
    pos = e->pos();
    QTreeView::mouseReleaseEvent(e);
}

void MessageList::scrollContentsBy(int dx, int dy)
{
    QTreeView::scrollContentsBy(dx, dy);
    emit scrolled();
}

void MessageList::mouseMoveEvent(QMouseEvent* e)
{
    if (mouseOverMoreLink(e)) {
        QCursor handCursor(Qt::PointingHandCursor);
        setCursor(handCursor);
    } else if (cursor().shape() == Qt::PointingHandCursor) {
        setCursor(QCursor());
    }

    QTreeView::mouseMoveEvent(e);
}

void MessageList::mousePressEvent(QMouseEvent* e)
{
    // TODO: Should be in release event?
    if (mouseOverMoreLink(e)) {
        emit moreButtonClicked();
    }

    QTreeView::mousePressEvent(e);
}

void MessageList::mouseDoubleClickEvent(QMouseEvent* e)
{
    QTreeView::mouseDoubleClickEvent(e);
}

bool MessageList::mouseOverMoreLink(QMouseEvent* e)
{
    if (!m_parent->moreButtonVisible())
        return false;

    QModelIndex index = indexAt(e->pos());
    if (index.isValid() && ((!index.parent().isValid()) && (index.row() == model()->rowCount(QModelIndex()) - 1))) {
        QFont font;
        font.setUnderline(true);
        QFontMetrics fm(font);
        QItemSelection fakeSelection(index.sibling(index.row(),0), index.sibling(index.row(),3));
        QRegion r = visualRegionForSelection(fakeSelection);
        QRect brect = r.boundingRect();
        QRect textRect = fm.boundingRect(brect, Qt::AlignHCenter | Qt::AlignVCenter, tr("Get more messages"));
        return textRect.contains(e->pos());
    }

    return false;
}

void MessageList::drawRow(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    bool isMoreEntry = (!index.parent().isValid() &&
                        m_parent->moreButtonVisible() &&
                        (index.row() == (model()->rowCount(QModelIndex()) - 1)));
    if (isMoreEntry) {
        QLinearGradient lg(option.rect.topLeft(), option.rect.topRight());
        lg.setColorAt(0,option.palette.color(QPalette::Base));
        lg.setColorAt(0.5,option.palette.color(QPalette::Button));
        lg.setColorAt(1,option.palette.color(QPalette::Base));

        QFont font = painter->font();
        font.setUnderline(true);

        painter->save();
        painter->setFont(font);
        painter->fillRect(option.rect, QBrush(lg));
        painter->drawText(option.rect, Qt::AlignHCenter | Qt::AlignVCenter, tr("Get more messages"));
        painter->restore();
    } else {
        QTreeView::drawRow(painter, option, index);
    }
}

void MessageList::rowsInserted(const QModelIndex &parent, int start, int end)
{
    QTreeView::rowsInserted(parent, start, end);
    setExpanded(parent, true);
}


MessageListView::MessageListView(QWidget* parent)
:
    QWidget(parent),
    mMessageList(new MessageList(this)),
    mModel(0),
    mMarkingMode(false),
    mIgnoreWhenHidden(true),
    mSelectedRowsRemoved(false),
    mQuickSearchWidget(0),
    mShowMoreButton(false),
    mThreaded(true),
    mQuickSearchIsEmpty(true)
{
    init();
    showQuickSearch(true);
}

MessageListView::~MessageListView()
{
}

QMailMessageKey MessageListView::key() const
{
    return mModel->key();
}

void MessageListView::setKey(const QMailMessageKey& newKey)
{
    // Save the current index for the old key
    QByteArray keyArray;
    QDataStream keyStream(&keyArray, QIODevice::WriteOnly);
    mKey.serialize<QDataStream>(keyStream);
    mPreviousCurrentCache.insert(keyArray, new QMailMessageId(current()));

    // Find the previous current index for the new key
    QByteArray newKeyArray;
    QDataStream newKeyStream(&newKeyArray, QIODevice::WriteOnly);
    newKey.serialize<QDataStream>(newKeyStream);
    if (mPreviousCurrentCache[newKeyArray]) {
        mPreviousCurrent = *mPreviousCurrentCache[newKeyArray];
    }
    mModel->setKey(newKey & mQuickSearchWidget->searchKey());
    mExpandAllTimer.start(0);
    mScrollTimer.stop();
    mKey = newKey;
}

QMailMessageSortKey MessageListView::sortKey() const
{
    return mModel->sortKey();
}

void MessageListView::setSortKey(const QMailMessageSortKey& sortKey)
{
    mModel->setSortKey(sortKey);
}

QMailFolderId MessageListView::folderId() const
{
    return mFolderId;
}

void MessageListView::setFolderId(const QMailFolderId& folderId)
{
    mFolderId = folderId;
}

void MessageListView::init()
{
    mPreviousCurrentCache.setMaxCost(10000); // Cache current item for 1,000 folders
    connect(&mExpandAllTimer, SIGNAL(timeout()),
            this, SLOT(expandAll()));

    mQuickSearchWidget = new QuickSearchWidget(this);
    connect(mQuickSearchWidget,SIGNAL(quickSearchRequested(QMailMessageKey)),this,SLOT(quickSearch(QMailMessageKey)));
    connect(mQuickSearchWidget,SIGNAL(quickSearchReset()),this,SLOT(quickSearchReset()));
    connect(mQuickSearchWidget,SIGNAL(fullSearchRequested()),this,SIGNAL(fullSearchRequested()));

    reset();

    mMessageList->setObjectName("messagelistview");
    mMessageList->setUniformRowHeights(true);
    mMessageList->setAlternatingRowColors(true);
    mMessageList->header()->setDefaultSectionSize(180);

    connect(mMessageList, SIGNAL(clicked(QModelIndex)),
            this, SLOT(indexClicked(QModelIndex)));
    connect(mMessageList, SIGNAL(activated(QModelIndex)),
            this, SLOT(indexActivated(QModelIndex)));
    connect(mMessageList, SIGNAL(backPressed()),
            this, SIGNAL(backPressed()));
    connect(mMessageList, SIGNAL(doubleClicked(QModelIndex)),
            this, SLOT(indexDoubleClicked(QModelIndex)));

    connect(mMessageList, SIGNAL(moreButtonClicked()),
            this, SIGNAL(moreClicked()));

    connect(mMessageList, SIGNAL(scrolled()),
            this, SLOT(reviewVisibleMessages()));

    connect(mMessageList, SIGNAL(collapsed(QModelIndex)),
            this, SIGNAL(rowCountChanged()));
    connect(mMessageList, SIGNAL(expanded(QModelIndex)),
            this, SIGNAL(rowCountChanged()));

    mScrollTimer.setSingleShot(true);
    connect(&mScrollTimer, SIGNAL(timeout()),
            this, SLOT(scrollTimeout()));

    QVBoxLayout* vLayout = new QVBoxLayout(this);
    vLayout->setContentsMargins(0,0,0,0);
    vLayout->setSpacing(0);
    vLayout->addWidget(mQuickSearchWidget);
    vLayout->addWidget(mMessageList);

    setFocusProxy(mMessageList);
}

void MessageListView::updateActions()
{
    // Don't show get more message button when quick search bar is being used
    if (!mQuickSearchIsEmpty) {
        setMoreButtonVisible(false);
        return;
    }
    
    // We need to see if the folder status has changed
    bool partialContent(false);
    if (mFolderId.isValid()) {
        QMailFolder folder;
        folder = QMailFolder(mFolderId);

        // Are there more messages to be retrieved for this folder?
        if (folder.status() & QMailFolder::PartialContent) {
            // More messages to retrieve
            partialContent = true;
        }
    }
    
    setMoreButtonVisible(partialContent);
}

QMailMessageId MessageListView::current() const
{
    return mMessageList->currentIndex().data(QMailMessageModelBase::MessageIdRole).value<QMailMessageId>();
}

void MessageListView::setCurrent(const QMailMessageId& id)
{
    QModelIndex index = mModel->indexFromId(id);
    if (index.isValid()) {
        mMessageList->setCurrentIndex(index);
    }
}

void MessageListView::setNextCurrent()
{
    QModelIndex index = mMessageList->indexBelow(mMessageList->currentIndex());
    if (index.isValid()) {
        mMessageList->setCurrentIndex(index);
    }
}

void MessageListView::setPreviousCurrent()
{
    QModelIndex index = mMessageList->indexAbove(mMessageList->currentIndex());
    if (index.isValid()) {
        mMessageList->setCurrentIndex(index);
    }
}

bool MessageListView::hasNext() const
{
    return (mMessageList->indexBelow(mMessageList->currentIndex()).isValid());
}

bool MessageListView::hasPrevious() const
{
    return (mMessageList->indexAbove(mMessageList->currentIndex()).isValid());
}

bool MessageListView::hasParent() const
{
    return (mMessageList->currentIndex().parent().isValid());
}

bool MessageListView::hasChildren() const
{
    return (mModel->rowCount(mMessageList->currentIndex()) > 0);
}

int MessageListView::rowCount(const QModelIndex &parentIndex) const
{
    int total = 0;

    int count = mModel->rowCount(parentIndex);
    total += count;

    for (int i = 0; i < count; ++i) {
        // Count the children of any expanded node
        QModelIndex idx(mModel->index(i, 0, parentIndex));
        if (mMessageList->isExpanded(idx)) {
            total += rowCount(idx);
        }
    }

    return total;
}

void MessageListView::selectedChildren(const QModelIndex &parentIndex, QMailMessageIdList *selectedIds) const
{
    for (int i = 0, count = mModel->rowCount(parentIndex); i < count; ++i) {
        QModelIndex idx(mModel->index(i, 0, parentIndex));
        if (static_cast<Qt::CheckState>(idx.data(Qt::CheckStateRole).toInt()) == Qt::Checked) {
            selectedIds->append(idx.data(QMailMessageModelBase::MessageIdRole).value<QMailMessageId>());
        }

        selectedChildren(idx, selectedIds);
    }
}

QMailMessageIdList MessageListView::selected() const
{
    QMailMessageIdList selectedIds;

    if (mMarkingMode) {
        selectedChildren(mMessageList->rootIndex(), &selectedIds);
    } else {
        if (current().isValid())
            selectedIds.append(current());
    }

    return selectedIds;
}

void MessageListView::setSelected(const QMailMessageIdList& ids)
{
    if (mMarkingMode) {
        foreach (const QMailMessageId& id, ids)
            setSelected(id);
    }
}

void MessageListView::setSelected(const QMailMessageId& id)
{
    QModelIndex index = mModel->indexFromId(id);
    if (index.isValid()) {
        if (mMarkingMode) {
            mModel->setData(index, static_cast<int>(Qt::Checked), Qt::CheckStateRole);
        } else {
            mMessageList->setCurrentIndex(index);
        }
    }
}

void MessageListView::selectChildren(const QModelIndex &parentIndex, bool selected, bool *modified)
{
    for (int i = 0, count = mModel->rowCount(parentIndex); i < count; ++i) {
        QModelIndex idx(mModel->index(i, 0, parentIndex));
        Qt::CheckState state(static_cast<Qt::CheckState>(idx.data(Qt::CheckStateRole).toInt()));

        if (selected && (state == Qt::Unchecked)) {
            mModel->setData(idx, static_cast<int>(Qt::Checked), Qt::CheckStateRole);
            *modified = true;
        } else if (!selected && (state == Qt::Checked)) {
            mModel->setData(idx, static_cast<int>(Qt::Unchecked), Qt::CheckStateRole);
            *modified = true;
        }

        selectChildren(idx, selected, modified);
    }
}

void MessageListView::selectAll()
{
    bool modified(false);

    selectChildren(mMessageList->rootIndex(), true, &modified);

    if (modified)
        emit selectionChanged();
}

void MessageListView::clearSelection()
{
    bool modified(false);

    selectChildren(mMessageList->rootIndex(), false, &modified);

    if (modified)
        emit selectionChanged();
}

bool MessageListView::markingMode() const
{
    return mMarkingMode;
}

void MessageListView::setMarkingMode(bool set)
{
    if (mMarkingMode != set) {
        mMarkingMode = set;
        emit selectionChanged();
    }
}

bool MessageListView::moreButtonVisible() const
{
    return mShowMoreButton;
}

void MessageListView::setMoreButtonVisible(bool set)
{
    if (mShowMoreButton != set) {
        mShowMoreButton = set;
        mMessageList->reset();
    }
}

bool MessageListView::threaded() const
{
    return mThreaded;
}

void MessageListView::setThreaded(bool set)
{
    if (mThreaded != set) {
        mThreaded = set;
        reset();
    }
}

bool MessageListView::ignoreUpdatesWhenHidden() const
{
    return mIgnoreWhenHidden;
}

void MessageListView::setIgnoreUpdatesWhenHidden(bool ignore)
{
    if (ignore != mIgnoreWhenHidden) {
        mIgnoreWhenHidden = ignore;

        if (!ignore && mModel->ignoreMailStoreUpdates())
            mModel->setIgnoreMailStoreUpdates(false);
    }
}

void MessageListView::showEvent(QShowEvent* e)
{
    mModel->setIgnoreMailStoreUpdates(false);

    QWidget::showEvent(e);
}

void MessageListView::hideEvent(QHideEvent* e)
{
    if (mIgnoreWhenHidden)
        mModel->setIgnoreMailStoreUpdates(true);

    QWidget::hideEvent(e);
}

void MessageListView::modelChanged()
{
    mMarkingMode = false;
}

void MessageListView::indexClicked(const QModelIndex& index)
{
    if (mMarkingMode) {
        bool checked(static_cast<Qt::CheckState>(index.data(Qt::CheckStateRole).toInt()) == Qt::Checked);
        mModel->setData(index, static_cast<int>(checked ? Qt::Unchecked : Qt::Checked), Qt::CheckStateRole);
        emit selectionChanged();
    } else {
        QMailMessageId id(index.data(QMailMessageModelBase::MessageIdRole).value<QMailMessageId>());
        if (id.isValid()) {
            emit clicked(id);
        }
    }
}

void MessageListView::indexDoubleClicked(const QModelIndex& index)
{
    if (!mMarkingMode) {
        QMailMessageId id(index.data(QMailMessageModelBase::MessageIdRole).value<QMailMessageId>());
        if (id.isValid()) {
            emit doubleClicked(id);
        }
    }
}

void MessageListView::indexActivated(const QModelIndex& index)
{
    if (mMarkingMode) {
        return;
    } else {
        QMailMessageId id(index.data(QMailMessageListModel::MessageIdRole).value<QMailMessageId>());
        if (id.isValid()) {
            emit activated(id);
        }
    }
}

void MessageListView::currentIndexChanged(const QModelIndex& currentIndex,
                                          const QModelIndex& previousIndex)
{
    QMailMessageId currentId(currentIndex.data(QMailMessageModelBase::MessageIdRole).value<QMailMessageId>());
    QMailMessageId previousId(previousIndex.data(QMailMessageModelBase::MessageIdRole).value<QMailMessageId>());

    emit currentChanged(currentId, previousId);

    if (!mMarkingMode)
        emit selectionChanged();
}

void MessageListView::rowsAboutToBeRemoved(const QModelIndex &parentIndex, int start, int end)
{
    if (mMarkingMode && !mSelectedRowsRemoved) {
        // See if any of the removed rows are currently selected
        for (int row = start; row <= end; ++row) {
            QModelIndex idx(mModel->index(row, 0, parentIndex));
            if (static_cast<Qt::CheckState>(idx.data(Qt::CheckStateRole).toInt()) == Qt::Checked) {
                mSelectedRowsRemoved = true;
                break;
            }
        }
    }
}

void MessageListView::layoutChanged()
{
    if (mSelectedRowsRemoved) {
        mSelectedRowsRemoved = false;
        emit selectionChanged();
    }
}

void MessageListView::reviewVisibleMessages()
{
    const int maximumRefreshRate = 7; //seconds

    if (mScrollTimer.isActive())
        return;

    mPreviousVisibleItems = visibleMessagesIds();
    emit visibleMessagesChanged();

    mScrollTimer.start(maximumRefreshRate*1000);
}

void MessageListView::scrollTimeout()
{
    QMailMessageIdList strictlyVisibleIds(visibleMessagesIds(false));
    bool contained = true;
    foreach(QMailMessageId id, strictlyVisibleIds) {
        if (!mPreviousVisibleItems.contains(id)) {
            contained = false;
            break;
        }
    }
    if (!contained) {
        reviewVisibleMessages();
    } else {
        mPreviousVisibleItems.clear();
    }
}

void MessageListView::quickSearch(const QMailMessageKey& key)
{
    mQuickSearchIsEmpty = false;
    if (current().isValid())
        mPreviousCurrent = current();

    if (key.isEmpty()) {
        mModel->setKey(mKey);
        mKey = QMailMessageKey();
    } else {
        if (mKey.isEmpty()) {
            mKey = mModel->key();
        }
        mModel->setKey(key & mKey);
        setMoreButtonVisible(false);
    }
    mExpandAllTimer.start(0);
}

void MessageListView::quickSearchReset()
{
    mQuickSearchIsEmpty = true;
    updateActions();
}

QMailMessageIdList MessageListView::visibleMessagesIds(bool buffer) const
{
    QMailMessageIdList visibleIds;
    const int bufferWidth = 50;

    QRect viewRect(mMessageList->viewport()->rect());
    QModelIndex start(mMessageList->indexAt(QPoint(0,0)));
    QModelIndex index = start;
    while (index.isValid()) { // Add visible items
        QRect indexRect(mMessageList->visualRect(index));
        if (!indexRect.isValid() || (!viewRect.intersects(indexRect)))
            break;
        visibleIds.append(index.data(QMailMessageModelBase::MessageIdRole).value<QMailMessageId>());
        index = mMessageList->indexBelow(index);
    }
    if (!buffer)
        return visibleIds;

    int i = bufferWidth;
    while (index.isValid() && (i > 0)) { // Add succeeding items
        visibleIds.append(index.data(QMailMessageModelBase::MessageIdRole).value<QMailMessageId>());
        index = mMessageList->indexBelow(index);
        --i;
    }
    i = bufferWidth;
    index = start;
    if (index.isValid())
        index = mMessageList->indexAbove(index);
    while (index.isValid() && (i > 0)) { // Add preceding items
        visibleIds.append(index.data(QMailMessageModelBase::MessageIdRole).value<QMailMessageId>());
        index = mMessageList->indexAbove(index);
        --i;
    }
    return visibleIds;
}

bool MessageListView::showingQuickSearch() const
{
    return mQuickSearchWidget->isVisible();
}

void MessageListView::showQuickSearch(bool val)
{
    mQuickSearchWidget->setVisible(val);
}

bool MessageListView::eventFilter(QObject *, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        if (QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event)) {
            if (keyEvent->key() == Qt::Key_Back) {
                emit backPressed();
                return true;
            }
        }
    }

    return false;
}

void MessageListView::reset()
{
    if (mQuickSearchWidget && mQuickSearchWidget->isVisible()) {
        mQuickSearchWidget->reset();
    }

    QMailMessageKey key;
    QMailMessageSortKey sortKey;
    QMailMessageId currentId;

    if (mModel) {
        key = this->key();
        sortKey = this->sortKey();
        currentId = this->current();
        delete mModel;
    }

    if (mThreaded) {
        mModel = new MessageListModel<QMailMessageThreadedModel>(this);
    } else {
        mModel = new MessageListModel<QMailMessageListModel>(this);
    }

    connect(mModel, SIGNAL(modelChanged()),
            this, SLOT(modelChanged()));
    connect(mModel, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
            this, SLOT(rowsAboutToBeRemoved(QModelIndex,int,int)));
    connect(mModel, SIGNAL(layoutChanged()),
            this, SLOT(layoutChanged()));
    connect(mModel, SIGNAL(rowsRemoved(QModelIndex,int,int)),
            this, SLOT(reviewVisibleMessages()));
    connect(mModel, SIGNAL(rowsInserted(QModelIndex,int,int)),
            this, SLOT(reviewVisibleMessages()));

    mMessageList->setModel(mModel);
    mMessageList->setRootIsDecorated(false);
    mExpandAllTimer.start(0);
    connect(mMessageList->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this, SLOT(currentIndexChanged(QModelIndex,QModelIndex)));

    if (!key.isEmpty() || !mQuickSearchWidget->searchKey().isEmpty()) {
        setKey(key);
    }
    if (!sortKey.isEmpty()) {
        setSortKey(sortKey);
    }
    if (currentId.isValid()) {
        setCurrent(currentId);
    }
}

void MessageListView::expandAll()
{
    mExpandAllTimer.stop();
    mMessageList->expandAll();
    scrollTo(mPreviousCurrent);
    QTimer::singleShot(0, this, SLOT(reviewVisibleMessages()));
}

void MessageListView::scrollTo(const QMailMessageId &id)
{
    if (!id.isValid())
        return;

    QModelIndex index(mModel->indexFromId(id));
    if (index.isValid()) {
        mMessageList->scrollTo(index);
        mMessageList->setCurrentIndex(index);
    }
}

#include "messagelistview.moc"


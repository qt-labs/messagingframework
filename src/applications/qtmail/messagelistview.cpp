/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#include "messagelistview.h"
#include <qmaillog.h>
#include <qmailfolder.h>
#include <qmailfolderkey.h>
#ifndef USE_NONTHREADED_MODEL
#include <qmailmessagethreadedmodel.h>
#else
#include <qmailmessagelistmodel.h>
#endif
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
#include <qmaildatacomparator.h>

static QStringList headers(QStringList() << "Subject" << "Sender" << "Date" << "Size");
static const QColor newMessageColor(Qt::blue);

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

// Also in plugins/viewers/generic/attachment_options:
static QString sizeString(uint size)
{
    if(size < 1024)
        return QObject::tr("%n byte(s)", "", size);
    else if(size < (1024 * 1024))
        return QObject::tr("%1 KB").arg(((float)size)/1024.0, 0, 'f', 1);
    else if(size < (1024 * 1024 * 1024))
        return QObject::tr("%1 MB").arg(((float)size)/(1024.0 * 1024.0), 0, 'f', 1);
    else
        return QObject::tr("%1 GB").arg(((float)size)/(1024.0 * 1024.0 * 1024.0), 0, 'f', 1);
}

class QuickSearchWidget : public QWidget
{
    Q_OBJECT
public:
    QuickSearchWidget(QWidget* parent = 0);

public slots:
    void reset();

signals:
    void quickSearch(const QMailMessageKey& key);

private slots:
    void searchTermsChanged();

private:
    QMailMessageKey buildSearchKey() const;

private:
    QLineEdit* m_searchTerms;
    QComboBox* m_statusCombo;
    QToolButton* m_fullSearchButton;
    QToolButton* m_clearButton;
};

QuickSearchWidget::QuickSearchWidget(QWidget* parent)
:
QWidget(parent)
{
    QHBoxLayout* layout = new QHBoxLayout(this);

    m_clearButton = new QToolButton(this);
    m_clearButton->setIcon(QIcon(":icon/clear_right"));
    connect(m_clearButton,SIGNAL(clicked()),this,SLOT(reset()));
    layout->addWidget(m_clearButton);

    layout->addWidget(new QLabel("Search:"));
    m_searchTerms = new QLineEdit(this);
    connect(m_searchTerms,SIGNAL(textChanged(QString)),this,SLOT(searchTermsChanged()));
    layout->addWidget(m_searchTerms);

    layout->addWidget(new QLabel("Status:"));

    m_statusCombo = new QComboBox(this);
    m_statusCombo->addItem(QIcon(":icon/exec"),"Any Status",QMailMessageKey());
    m_statusCombo->addItem(QIcon(":icon/mail_generic"),"Unread",QMailMessageKey::status(QMailMessage::Read,QMailDataComparator::Excludes));
    m_statusCombo->addItem(QIcon(":icon/new"),"New",QMailMessageKey::status(QMailMessage::New));
    m_statusCombo->addItem(QIcon(":icon/mail_reply"),"Replied",QMailMessageKey::status(QMailMessage::Replied));
    m_statusCombo->addItem(QIcon(":icon/mail_forward"),"Forwarded",QMailMessageKey::status(QMailMessage::Forwarded));
    m_statusCombo->addItem(QIcon(":/icon/attach"),"Has Attachment",QMailMessageKey::status(QMailMessage::HasAttachments));
    connect(m_statusCombo,SIGNAL(currentIndexChanged(int)),this,SLOT(searchTermsChanged()));
    layout->addWidget(m_statusCombo);

    m_fullSearchButton = new QToolButton(this);
    m_fullSearchButton->setIcon(QIcon(":icon/find"));
    layout->addWidget(m_fullSearchButton);
}

void QuickSearchWidget::reset()
{
    m_statusCombo->setCurrentIndex(0);
    m_searchTerms->clear();
}

void QuickSearchWidget::searchTermsChanged()
{
    QMailMessageKey key = buildSearchKey();
    emit quickSearch(key);
}

QMailMessageKey QuickSearchWidget::buildSearchKey() const
{
    if(m_searchTerms->text().isEmpty() && m_statusCombo->currentIndex() == 0)
        return QMailMessageKey();
    QMailMessageKey subjectKey = QMailMessageKey::subject(m_searchTerms->text(),QMailDataComparator::Includes);
    QMailMessageKey senderKey = QMailMessageKey::sender(m_searchTerms->text(),QMailDataComparator::Includes);

    QMailMessageKey statusKey = qvariant_cast<QMailMessageKey>(m_statusCombo->itemData(m_statusCombo->currentIndex()));
    if(!statusKey.isEmpty())
        return ((subjectKey | senderKey) & statusKey);
    return subjectKey | senderKey;
}

class MessageListModel : public MessageListView::ModelType
{
    Q_OBJECT

public:
    MessageListModel(QWidget* parent );
    QVariant headerData(int section,Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
    int columnCount(const QModelIndex & parent = QModelIndex()) const;
    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
    bool markingMode() const;
    void setMarkingMode(bool val);

    bool moreButtonVisible() const;
    void setMoreButtonVisible(bool val);

private:
    bool m_markingMode;
    bool m_moreButtonVisible;
};

MessageListModel::MessageListModel(QWidget* parent)
:
MessageListView::ModelType(parent),
m_markingMode(false),
m_moreButtonVisible(false)
{
}

QVariant MessageListModel::headerData(int section, Qt::Orientation o, int role) const
{
    if (role == Qt::DisplayRole)
    {
        if(section < headers.count())
            return headers.at(section);
    }

   return MessageListView::ModelType::headerData(section,o,role);
}

int MessageListModel::columnCount(const QModelIndex & parent ) const
{
    Q_UNUSED(parent);
    return headers.count();
}

int MessageListModel::rowCount(const QModelIndex& parent) const
{
    int actualRows = MessageListView::ModelType::rowCount(parent);

    if (!parent.isValid() && (actualRows > 0) && m_moreButtonVisible)
        actualRows++;

    return actualRows;
}

QVariant MessageListModel::data( const QModelIndex & index, int role) const
{
    if (index.isValid()) {
        if (role == Qt::DisplayRole && index.isValid()) {
            QMailMessageMetaData message(idFromIndex(index));
            if (!message.id().isValid())
                return QVariant();

            switch(index.column())
            {
            case 0:
                return message.subject();
            case 1:
                return message.from().name();
            case 2:
                return dateString(message.date().toLocalTime());
            case 3:
                return sizeString(message.size());
            break;
            }
        } else if ((role == Qt::DecorationRole  || role == Qt::CheckStateRole )&& index.column() > 0) {
            return QVariant();
        } else if (role == Qt::CheckStateRole && !m_markingMode) {
            Qt::CheckState state = static_cast<Qt::CheckState>(MessageListView::ModelType::data(index,role).toInt());
            if (state == Qt::Unchecked)
                return QVariant();
        } else if (role == Qt::DecorationRole) {
            static QIcon readIcon(":icon/flag_normal");
            static QIcon unreadIcon(":icon/flag_unread");
            static QIcon toGetIcon(":icon/flag_toget");
            static QIcon toSendIcon(":icon/flag_tosend");
            static QIcon unfinishedIcon(":icon/flag_unfinished");
            static QIcon removedIcon(":icon/flag_removed");

            QMailMessageMetaData message(idFromIndex(index));
            if(!message.id().isValid())
                return QVariant();

            bool sent(message.status() & QMailMessage::Sent);
            bool incoming(message.status() & QMailMessage::Incoming);

            if (incoming) {
                quint64 status = message.status();
                if ( status & QMailMessage::Removed ) {
                    return removedIcon;
                } else if ( status & QMailMessage::PartialContentAvailable ) {
                    if ( status & QMailMessage::Read || status & QMailMessage::ReadElsewhere ) {
                        return readIcon;
                    } else {
                        return unreadIcon;
                    }
                } else {
                    return toGetIcon;
                }
            } else {
                if (sent) {
                    return readIcon;
                } else if ( message.to().isEmpty() ) {
                    // Not strictly true - there could be CC or BCC addressees
                    return unfinishedIcon;
                } else {
                    return toSendIcon;
                }
            }
        } else if(role == Qt::ForegroundRole) {
            QMailMessageMetaData message(idFromIndex(index));
            if (!message.id().isValid())
                return QVariant();

            quint64 status = message.status();
            bool unread = !(status & QMailMessage::Read || status & QMailMessage::ReadElsewhere);
            if (status & QMailMessage::PartialContentAvailable && unread)
                return newMessageColor;
        }
    }
    return MessageListView::ModelType::data(index,role);
}

void MessageListModel::setMarkingMode(bool val)
{
    m_markingMode = val;
}

bool MessageListModel::markingMode() const
{
    return m_markingMode;
}

bool MessageListModel::moreButtonVisible() const
{
    return m_moreButtonVisible;
}

void MessageListModel::setMoreButtonVisible(bool val)
{
    m_moreButtonVisible = val;
    reset();
}

class MessageList : public QTreeView
{
    Q_OBJECT

public:
    MessageList(QWidget* parent = 0);
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
    bool mouseOverMoreLink(QMouseEvent* e);

    void drawRow(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

private:
    MessageListModel* sourceModel() const;

private:
    QPoint pos;
};

MessageList::MessageList(QWidget* parent)
:
    QTreeView(parent)
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

void MessageList::drawRow(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    bool isMoreEntry = ((!index.parent().isValid()) && 
                        (sourceModel()->moreButtonVisible()) &&
                        (index.row() == (sourceModel()->rowCount(QModelIndex()) - 1)));
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

bool MessageList::mouseOverMoreLink(QMouseEvent* e)
{
    if (!sourceModel()->moreButtonVisible())
        return false;

    QModelIndex index = indexAt(e->pos());
    if (index.isValid() && ((!index.parent().isValid()) && (index.row() == sourceModel()->rowCount(QModelIndex()) - 1))) {
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

MessageListModel* MessageList::sourceModel() const
{
    MessageListModel* sm = qobject_cast<MessageListModel*>(model());
    Q_ASSERT(sm);
    return sm;
}

MessageListView::MessageListView(QWidget* parent)
:
    QWidget(parent),
    mMessageList(new MessageList(this)),
    mModel(new MessageListModel(this)),
    mMarkingMode(false),
    mIgnoreWhenHidden(true),
    mSelectedRowsRemoved(false),
    mShowMoreButton(false)
{
    init();
}

MessageListView::~MessageListView()
{
}

QMailMessageKey MessageListView::key() const
{
    return mModel->key();
}

void MessageListView::setKey(const QMailMessageKey& key)
{
    mModel->setKey(key);
    mScrollTimer.stop();
    QTimer::singleShot(0, this, SLOT(reviewVisibleMessages()));
    mQuickSearchWidget->blockSignals(true);
    mQuickSearchWidget->reset();
    mQuickSearchWidget->blockSignals(false);
    mKey = key;
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
    mModel->setMoreButtonVisible(mFolderId.isValid());
    mShowMoreButton = mModel->moreButtonVisible();
}

MessageListModel* MessageListView::model() const
{
    return mModel;
}

void MessageListView::init()
{
    mMessageList->setUniformRowHeights(true);
#ifndef USE_NONTHREADED_MODEL
    mMessageList->setRootIsDecorated(true);
#else
    mMessageList->setRootIsDecorated(false);
#endif
    mMessageList->setAlternatingRowColors(true);
    mMessageList->header()->setDefaultSectionSize(180);
    mMessageList->setModel(mModel);
    mMessageList->setAlternatingRowColors(true);

    connect(mMessageList, SIGNAL(clicked(QModelIndex)),
            this, SLOT(indexClicked(QModelIndex)));
    connect(mMessageList->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this, SLOT(currentIndexChanged(QModelIndex,QModelIndex)));
    connect(mMessageList, SIGNAL(backPressed()),
            this, SIGNAL(backPressed()));

    connect(mMessageList, SIGNAL(moreButtonClicked()),
            this, SIGNAL(moreClicked()));

    connect(mModel, SIGNAL(modelChanged()),
            this, SLOT(modelChanged()));

    connect(mModel, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
            this, SLOT(rowsAboutToBeRemoved(QModelIndex,int,int)));
    connect(mModel, SIGNAL(layoutChanged()),
            this, SLOT(layoutChanged()));

    mScrollTimer.setSingleShot(true);
    connect(mMessageList, SIGNAL(scrolled()),
            this, SLOT(reviewVisibleMessages()));
    connect(&mScrollTimer, SIGNAL(timeout()),
            this, SLOT(scrollTimeout()));
    connect(mModel, SIGNAL(rowsRemoved(QModelIndex,int,int)),
            this, SLOT(reviewVisibleMessages()));
    connect(mModel, SIGNAL(rowsInserted(QModelIndex,int,int)),
            this, SLOT(reviewVisibleMessages()));

    QVBoxLayout* vLayout = new QVBoxLayout(this);
    vLayout->setContentsMargins(0,0,0,0);
    vLayout->setSpacing(0);
    mQuickSearchWidget = new QuickSearchWidget(this);
    connect(mQuickSearchWidget,SIGNAL(quickSearch(QMailMessageKey)),this,SLOT(quickSearch(QMailMessageKey)));
    vLayout->addWidget(mQuickSearchWidget);
    vLayout->addWidget(mMessageList);

    this->setLayout(vLayout);
    setFocusProxy(mMessageList);
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

int MessageListView::rowCount() const
{
    return mModel->rowCount();
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
        mModel->setMarkingMode(set);
        emit selectionChanged();
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
    const int maximumRefreshRate = 15; //seconds

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
    if(key.isEmpty())
    {
        mModel->setKey(mKey);
        mModel->setMoreButtonVisible(mShowMoreButton);
        mKey = QMailMessageKey();
    }
    else
    {
        if(mKey.isEmpty())
        {
            mKey = mModel->key();
            mShowMoreButton = mModel->moreButtonVisible();
        }
        mModel->setKey(key & mKey);
        mModel->setMoreButtonVisible(false);
    }
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

#include "messagelistview.moc"


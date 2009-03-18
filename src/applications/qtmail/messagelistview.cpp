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
#include <qmailmessagelistmodel.h>
#include <QTreeView>
#include <QKeyEvent>
#include <QLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QModelIndex>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QTabBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QHeaderView>

static QStringList headers(QStringList() << "Subject" << "Sender" << "Date" << "Size");
static const bool alternatingBackground = true;
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

class MessageListModel : public QMailMessageListModel
{
public:
    MessageListModel(QWidget* parent );
    QVariant headerData(int section,Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
    int columnCount(const QModelIndex & parent = QModelIndex()) const;
    QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
    bool markingMode() const;
    void setMarkingMode(bool val);
private:
    bool m_markingMode;
};

MessageListModel::MessageListModel(QWidget* parent)
:
QMailMessageListModel(parent),
m_markingMode(false)
{
}

QVariant MessageListModel::headerData(int section, Qt::Orientation o, int role) const
{
    if (role == Qt::DisplayRole)
    {
        if(section < headers.count())
            return headers.at(section);
    }

   return QMailMessageListModel::headerData(section,o,role);
}

int MessageListModel::columnCount(const QModelIndex & parent ) const
{
    Q_UNUSED(parent);
    return headers.count();
}

QVariant MessageListModel::data( const QModelIndex & index, int role) const
{
    if(index.isValid())
    {
        if(role == Qt::DisplayRole && index.isValid())
        {
            QMailMessageMetaData message(idFromIndex(index));
            if(!message.id().isValid())
                return QVariant();

            switch(index.column())
            {
            case 0:
                return message.subject();
            case 1:
                return message.from().toString();
            case 2:
                return dateString(message.date().toLocalTime());
            case 3:
            {
                static const float kilobyte = 1024.0;
                return QString::number(message.size() / kilobyte,'f',1) + " KB";
            }
            break;
            }
        }
        else if((role == Qt::DecorationRole  || role == Qt::CheckStateRole )&& index.column() > 0)
            return QVariant();
        else if(role == Qt::CheckStateRole && !m_markingMode)
        {
            Qt::CheckState state = static_cast<Qt::CheckState>(QMailMessageListModel::data(index,role).toInt());
            if(state == Qt::Unchecked)
                return QVariant();
        }
        else if(role == Qt::DecorationRole)
        {
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

            if (incoming){
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
        }
        else if(role == Qt::ForegroundRole)
        {
            QMailMessageMetaData message(idFromIndex(index));
            if(!message.id().isValid())
                return QVariant();
            quint64 status = message.status();
            bool unread = !(status & QMailMessage::Read || status & QMailMessage::ReadElsewhere);
            if(status & QMailMessage::PartialContentAvailable && unread)
                return newMessageColor;
        }
        else if(role == Qt::BackgroundRole && alternatingBackground)
        {
            bool oddrow = index.row() % 2;
            if(oddrow)
            {
                QWidget* p = static_cast<QWidget*>(QObject::parent());
                return p->palette().color(QPalette::AlternateBase);
            }
        }
    }
    return QMailMessageListModel::data(index,role);
}

void MessageListModel::setMarkingMode(bool val)
{
    m_markingMode = val;
}

bool MessageListModel::markingMode() const
{
    return m_markingMode;
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

protected:
    void keyPressEvent(QKeyEvent* e);
    void mouseReleaseEvent(QMouseEvent* e);

    QPoint pos;
};

MessageList::MessageList(QWidget* parent)
:
    QTreeView(parent)
{
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


MessageListView::MessageListView(QWidget* parent)
:
    QWidget(parent),
    mMessageList(new MessageList(this)),
    mFilterFrame(new QFrame(this)),
    mFilterEdit(new QLineEdit(this)),
    mCloseFilterButton(0),
    mTabs(new QTabBar(this)),
    mMoreButton(new QPushButton(this)),
    mModel(new MessageListModel(this)),
    mFilterModel(new QSortFilterProxyModel(this)),
    mDisplayMode(DisplayMessages),
    mMarkingMode(false),
    mIgnoreWhenHidden(true),
    mSelectedRowsRemoved(false)
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
    mMoreButton->setVisible(mFolderId.isValid());
}

QMailMessageListModel* MessageListView::model() const
{
    return mModel;
}

void MessageListView::init()
{
    mMoreButton->hide();
    mFilterModel->setSourceModel(mModel);
    mFilterModel->setFilterRole(QMailMessageListModel::MessageFilterTextRole);
    mFilterModel->setDynamicSortFilter(true);
    mMessageList->setUniformRowHeights(true);
    mMessageList->setRootIsDecorated(false);
    mMessageList->header()->setDefaultSectionSize(180);
    mMessageList->setModel(mFilterModel);

    mTabs->setFocusPolicy(Qt::NoFocus);

    mCloseFilterButton = new QToolButton(this);
    mCloseFilterButton->setText(tr("Done"));
    mCloseFilterButton->setFocusPolicy(Qt::NoFocus);

    mMoreButton->setText(tr("Get more messages"));
    mMoreButton->installEventFilter(this);

    connect(mMessageList, SIGNAL(clicked(QModelIndex)),
            this, SLOT(indexClicked(QModelIndex)));
    connect(mMessageList->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this, SLOT(currentIndexChanged(QModelIndex,QModelIndex)));
    connect(mMessageList, SIGNAL(backPressed()),
            this, SIGNAL(backPressed()));

    connect(mFilterEdit, SIGNAL(textChanged(QString)),
            this, SLOT(filterTextChanged(QString)));

    connect(mCloseFilterButton, SIGNAL(clicked()),
            this, SLOT(closeFilterButtonClicked()));

    connect(mMoreButton, SIGNAL(clicked()),
            this, SIGNAL(moreClicked()));

    connect(mTabs, SIGNAL(currentChanged(int)),
            this, SLOT(tabSelected(int)));

    connect(mModel, SIGNAL(modelChanged()),
            this, SLOT(modelChanged()));

    connect(mFilterModel, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
            this, SLOT(rowsAboutToBeRemoved(QModelIndex,int,int)));
    connect(mFilterModel, SIGNAL(layoutChanged()),
            this, SLOT(layoutChanged()));

    QHBoxLayout* hLayout = new QHBoxLayout(mFilterFrame);
    hLayout->setContentsMargins(0,0,0,0);
    hLayout->setSpacing(0);
    hLayout->addWidget(new QLabel(tr("Search"),this));
    hLayout->addWidget(mFilterEdit);
    hLayout->addWidget(mCloseFilterButton);
    mFilterFrame->setLayout(hLayout);

    QVBoxLayout* vLayout = new QVBoxLayout(this);
    vLayout->setContentsMargins(0,0,0,0);
    vLayout->setSpacing(0);
    vLayout->addWidget(mTabs);
    vLayout->addWidget(mFilterFrame);
    vLayout->addWidget(mMessageList);
    vLayout->addWidget(mMoreButton);

    this->setLayout(vLayout);
    setFocusProxy(mMessageList);

    setDisplayMode(DisplayMessages);
}

QMailMessageId MessageListView::current() const
{
    return mMessageList->currentIndex().data(QMailMessageListModel::MessageIdRole).value<QMailMessageId>();
}

void MessageListView::setCurrent(const QMailMessageId& id)
{
    QModelIndex index = mModel->indexFromId(id);
    if (index.isValid()) {
        QModelIndex filteredIndex = mFilterModel->mapFromSource(index);
        if (filteredIndex.isValid())
            mMessageList->setCurrentIndex(filteredIndex);
    }
}

void MessageListView::setNextCurrent()
{
    QModelIndex index = mMessageList->currentIndex();
    if ((index.row() + 1) < mFilterModel->rowCount()) {
        QModelIndex nextIndex = mFilterModel->index(index.row()+1,0);
        mMessageList->setCurrentIndex(nextIndex);
    }
}

void MessageListView::setPreviousCurrent()
{
    QModelIndex index = mMessageList->currentIndex();
    if (index.row() > 0) {
        QModelIndex previousIndex = mFilterModel->index(index.row()-1,0);
        mMessageList->setCurrentIndex(previousIndex);
    }
}

bool MessageListView::hasNext() const
{
    QModelIndex index = mMessageList->currentIndex();
    return index.row() < (mFilterModel->rowCount() - 1);
}

bool MessageListView::hasPrevious() const
{
    QModelIndex index = mMessageList->currentIndex();
    return index.row() > 0;
}

int MessageListView::rowCount() const
{
    return mFilterModel->rowCount();
}

QMailMessageIdList MessageListView::selected() const
{
    QMailMessageIdList selectedIds;

    if (mMarkingMode) {
        // get the selected model indexes and convert to ids
        for (int i = 0, count = mFilterModel->rowCount(); i < count; ++i) {
            QModelIndex index(mFilterModel->index(i, 0));
            if (static_cast<Qt::CheckState>(index.data(Qt::CheckStateRole).toInt()) == Qt::Checked)
                selectedIds.append(index.data(QMailMessageListModel::MessageIdRole).value<QMailMessageId>());
        }
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
            QModelIndex filteredIndex(mFilterModel->mapFromSource(index));
            if (filteredIndex.isValid())
                mMessageList->setCurrentIndex(filteredIndex);
        }
    }
}

void MessageListView::selectAll()
{
    bool modified(false);

    for (int i = 0, count = mFilterModel->rowCount(); i < count; ++i) {
        QModelIndex idx(mFilterModel->index(i, 0));
        if (static_cast<Qt::CheckState>(idx.data(Qt::CheckStateRole).toInt()) == Qt::Unchecked) {
            mFilterModel->setData(idx, static_cast<int>(Qt::Checked), Qt::CheckStateRole);
            modified = true;
        }
    }

    if (modified)
        emit selectionChanged();
}

void MessageListView::clearSelection()
{
    bool modified(false);

    for (int i = 0, count = mFilterModel->rowCount(); i < count; ++i) {
        QModelIndex idx(mFilterModel->index(i, 0));
        if (static_cast<Qt::CheckState>(idx.data(Qt::CheckStateRole).toInt()) == Qt::Checked) {
            mFilterModel->setData(idx, static_cast<int>(Qt::Unchecked), Qt::CheckStateRole);
            modified = true;
        }
    }

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

void MessageListView::filterTextChanged(const QString& text)
{
    mFilterModel->setFilterRegExp(QRegExp(QRegExp::escape(text)));
}

MessageListView::DisplayMode MessageListView::displayMode() const
{
    return mDisplayMode;
}

void MessageListView::setDisplayMode(const DisplayMode& m)
{
    if(m == DisplayFilter) {
        mFilterFrame->setVisible(true);
        mTabs->setVisible(false);
        mFilterEdit->setFocus();
    } else {
        mDisplayMode = m;
        mFilterEdit->clear();
        mFilterFrame->setVisible(false);
        mTabs->setVisible(true);
        mTabs->setCurrentIndex(static_cast<int>(mDisplayMode));
    }
}

void MessageListView::closeFilterButtonClicked()
{
    setDisplayMode(DisplayMessages);
}

void MessageListView::tabSelected(int index)
{
    emit displayModeChanged(static_cast<DisplayMode>(index));
}

void MessageListView::modelChanged()
{
    mMarkingMode = false;
}

void MessageListView::indexClicked(const QModelIndex& index)
{
    if (mMarkingMode) {
        bool checked(static_cast<Qt::CheckState>(index.data(Qt::CheckStateRole).toInt()) == Qt::Checked);
        mFilterModel->setData(index, static_cast<int>(checked ? Qt::Unchecked : Qt::Checked), Qt::CheckStateRole);
        emit selectionChanged();
    } else {
        QMailMessageId id(index.data(QMailMessageListModel::MessageIdRole).value<QMailMessageId>());
        if (id.isValid()) {
            emit clicked(id);
        }
    }
}

void MessageListView::currentIndexChanged(const QModelIndex& currentIndex,
                                          const QModelIndex& previousIndex)
{
    QMailMessageId currentId(currentIndex.data(QMailMessageListModel::MessageIdRole).value<QMailMessageId>());
    QMailMessageId previousId(previousIndex.data(QMailMessageListModel::MessageIdRole).value<QMailMessageId>());

    emit currentChanged(currentId, previousId);

    if (!mMarkingMode)
        emit selectionChanged();
}

void MessageListView::rowsAboutToBeRemoved(const QModelIndex&, int start, int end)
{
    if (mMarkingMode && !mSelectedRowsRemoved) {
        // See if any of the removed rows are currently selected
        for (int row = start; row <= end; ++row) {
            QModelIndex idx(mFilterModel->index(row, 0));
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


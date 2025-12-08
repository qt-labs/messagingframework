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

#include "qmailmessageset.h"

#include "qmailaccount.h"
#include "qmailfolder.h"
#include "qmailstore.h"
#include "qmaillog.h"
#include <QTimer>

class QMailMessageSetContainerPrivate
{
public:
    QMailMessageSetContainerPrivate(QMailMessageSetContainer *parent)
        : _container(parent)
    {
    }

    QMailMessageSetContainer *_container;
    QList<QMailMessageSet*> _children;
};

/*!
    \class QMailMessageSetContainer
    \inmodule QmfClient

    \preliminary

    \brief The QMailMessageSetContainer class specifies the interface implemented by container
    nodes in a QMailMessageSet tree hierarchy.

    QMailMessageSetContainer provides the management for a collection of contained nodes
    in a QMailMessageSet tree hierarchy.  It also defines the interface available to contained
    nodes, which must inherit from QMailMessageSet.
*/

QMailMessageSetContainer::QMailMessageSetContainer(QMailMessageSetContainerPrivate *priv)
    : priv(priv)
{
}

/*! \internal */
QMailMessageSetContainer::~QMailMessageSetContainer()
{
    Q_D(QMailMessageSetContainer);

    while (!d->_children.isEmpty()) {
        delete d->_children.takeFirst();
    }
}

/*!
    Returns the parent container for this container object, or NULL if it has no parent container.
*/
QMailMessageSetContainer *QMailMessageSetContainer::parentContainer()
{
    Q_D(QMailMessageSetContainer);
    return d->_container;
}

/*!
    Returns the number of QMailMessageSets contained by this container object.
*/
int QMailMessageSetContainer::count() const
{
    Q_D(const QMailMessageSetContainer);
    return d->_children.count();
}

/*!
    Returns the QMailMessageSet object located at index \a i within this container object.

    \sa indexOf()
*/
QMailMessageSet *QMailMessageSetContainer::at(int i) const
{
    Q_D(const QMailMessageSetContainer);
    return d->_children.at(i);
}

/*!
    Returns the index within this container of the QMailMessageSet \a child, or -1 if it
    is not contained by this container object.

    \sa at()
*/
int QMailMessageSetContainer::indexOf(QMailMessageSet *child) const
{
    Q_D(const QMailMessageSetContainer);
    return d->_children.indexOf(child);
}

/*!
    Appends \a child to the list of QMailMessageSets contained by this object.

    The container assumes responsibility for deleting the child object.
*/
void QMailMessageSetContainer::append(QMailMessageSet *child)
{
    Q_D(QMailMessageSetContainer);
    model()->beginAppend(child);

    d->_children.append(child);
    child->init();

    model()->endAppend(child);
}

/*!
    Informs the container that \a child has been modified, and the container
    may need to be updated.
*/
void QMailMessageSetContainer::update(QMailMessageSet *child)
{
    model()->doUpdate(child);
}

/*!
    Removes \a child from the list of QMailMessageSets contained by the container object.
*/
void QMailMessageSetContainer::remove(QMailMessageSet *child)
{
    Q_D(QMailMessageSetContainer);

    // Any descendants of this child must first be removed
    child->removeDescendants();

    model()->beginRemove(child);
    d->_children.removeAll(child);
    model()->endRemove(child);

    delete child;
}

/*!
    Removes each member of \a obsoleteChildren from the container object.
*/
void QMailMessageSetContainer::remove(const QList<QMailMessageSet*> &obsoleteChildren)
{
    Q_D(QMailMessageSetContainer);

    foreach (QMailMessageSet *child, obsoleteChildren)
        if (d->_children.contains(child))
            remove(child);
}

/*!
    Removes all descendants of the container from the model.
*/
void QMailMessageSetContainer::removeDescendants()
{
    Q_D(QMailMessageSetContainer);

    foreach (QMailMessageSet *child, d->_children)
        remove(child);
}

/*!
    Resets the state of each child within the container object.
*/
void QMailMessageSetContainer::resyncState()
{
    Q_D(QMailMessageSetContainer);

    for (QMailMessageSet *child : d->_children) {
        child->resyncState();
        update(child);
    }
}

/*!
    \fn QMailMessageSetContainer::model()

    Returns the model that owns this container.
*/


/* QMailMessageSet */

class QMailMessageSetPrivate : public QMailMessageSetContainerPrivate
{
public:
    QMailMessageSetPrivate(QMailMessageSetContainer* parent)
        : QMailMessageSetContainerPrivate(parent)
    {
    }

    // currently empty...
};


/*!
    \class QMailMessageSet
    \inmodule QmfClient

    \preliminary

    \brief The QMailMessageSet class represents a subset of the messages in the mail store.

    QMailMessageSet provides a representation for a named subset of messages, specified
    by a QMailMessageKey selection criterion.

    QMailMessageSets are designed to be arranged in hierarchies, and each message set
    is a container for child message sets, implementing the QMailMessageSetContainer
    interface.  Message sets are owned by QMailMessageSetModel instances, and the
    index of a message set within the model can be retrieved using modelIndex().

    The messageKey() function of each QMailMessageSet object can be used to provide
    the message selection filter for a QMailMessageListModel.  The descendantsMessageKey()
    function can be used to provide a message selection filter matching all messages
    beneath this message set in the hierarchy.

    QMailMessageSet objects should not directly respond to events reported by the QMailStore;
    instead, they should react to notifications of mail store events emitted by the
    QMailMessageSetModel to which they are attached.  Because the events they receive
    from the model may be filtered, QMailMessageSet instances must implement the
    resyncState() function, resynchronizing their state with the current state of the
    mail store.

    \sa QMailMessageSetModel
*/

/*!
    Constructs a new QMailMessageSet within the container object \a container.
*/
QMailMessageSet::QMailMessageSet(QMailMessageSetContainer *container)
    : QObject(container ? container->qObject() : nullptr)
    , QMailMessageSetContainer(new QMailMessageSetPrivate(container))
{
}

QMailMessageSet::QMailMessageSet(QMailMessageSetPrivate *priv)
    : QObject(priv->_container ? priv->_container->qObject() : nullptr)
    , QMailMessageSetContainer(priv)
{
}

/*! \internal */
QMailMessageSet::~QMailMessageSet()
{
}

/*!
    \fn QMailMessageSet::messageKey() const

    Returns the QMailMessageKey that defines the messages represented by this message set.
*/

/*!
    Returns the QMailMessageKey that defines the messages found beneath this message set
    in the hierarchy, not including the messages of this message set itself.
*/
QMailMessageKey QMailMessageSet::descendantsMessageKey() const
{
    // Default implementation: Or together the keys yielding the content of all
    // our children.
    // Note: until QMailMessageKey's operators do automatic complexity reduction,
    // this will result in infeasibly complicated and repetitious queries...

    if (count() == 0)
        return QMailMessageKey::nonMatchingKey();

    QMailMessageKey result;

    for (int i = 0; i < count(); ++i) {
        result |= at(i)->messageKey();
        result |= at(i)->descendantsMessageKey();
    }

    return result;
}

/*!
    \fn QMailMessageSet::displayName() const

    Returns the name of this message set, suitable for display purposes.
*/

/*!
    Returns the data element associated with the specified \a role and \a column,
    from the model that owns this message set.
*/
QVariant QMailMessageSet::data(int role, int column)
{
    return model()->data(this, role, column);
}

/*!
    Returns the index of this message set within the model that owns it, having the specified \a column.
*/
QModelIndex QMailMessageSet::modelIndex(int column)
{
    return model()->index(this, column);
}

/*! \internal */
QMailMessageSetModel *QMailMessageSet::model()
{
    return parentContainer()->model();
}

/*!
    Initialises the message set after it has been appended to the parent container object.
*/
void QMailMessageSet::init()
{
}

/*! \internal */
QObject *QMailMessageSet::qObject()
{
    return this;
}


/* QMailFolderMessageSet */

class QMailFolderMessageSetPrivate : public QMailMessageSetPrivate
{
public:
    QMailFolderMessageSetPrivate(QMailMessageSetContainer *container, const QMailFolderId &folderId, bool hierarchical)
        : QMailMessageSetPrivate(container),
          _id(folderId),
          _hierarchical(hierarchical)
    {
    }

    QMailFolderId _id;
    bool _hierarchical;
    mutable QString _name;
    QMailFolderIdList _folderIds;
};


/*!
    \class QMailFolderMessageSet
    \inmodule QmfClient

    \preliminary

    \brief The QMailFolderMessageSet class represents a set of messages corresponding to the content of a QMailFolder.

    QMailFolderMessageSet provides a representation for a named subset of messages, specified
    by their parent QMailFolder.

    If the QMailFolderMessageSet is hierarchical(), then any folders contained by the
    parent QMailFolder will automatically be managed as child QMailFolderMessageSets of
    the parent QMailFolderMessageSet.
*/

/*!
    Constructs a QMailFolderMessageSet within the parent container \a container,
    whose message set is defined by the content of the QMailFolder identified by
    \a folderId.  If \a hierarchical is true, the message set will automatically
    maintain a set of child QMailFolderMessageSets corresponding to \l{QMailFolder}s
    whose \l{QMailFolder::parentFolderId()}{parentFolderId} is \a folderId.
*/
QMailFolderMessageSet::QMailFolderMessageSet(QMailMessageSetContainer *container, const QMailFolderId &folderId, bool hierarchical)
    : QMailMessageSet(new QMailFolderMessageSetPrivate(container, folderId, hierarchical))
{
}

/*!
    Returns the identifier of the QMailFolder associated with this message set.
*/
QMailFolderId QMailFolderMessageSet::folderId() const
{
    Q_D(const QMailFolderMessageSet);
    return d->_id;
}

/*!
    Returns true if this message set automatically maintains a hierarchy of child folder message sets.
*/
bool QMailFolderMessageSet::hierarchical() const
{
    Q_D(const QMailFolderMessageSet);
    return d->_hierarchical;
}

/*!
    Returns the QMailMessageKey that selects the messages contained by the configured folder.
*/
QMailMessageKey QMailFolderMessageSet::messageKey() const
{
    Q_D(const QMailFolderMessageSet);
    return contentKey(d->_id, false);
}

/*!
    Returns the QMailMessageKey that selects the messages beneath the configured folder in the hierarchy.
*/
QMailMessageKey QMailFolderMessageSet::descendantsMessageKey() const
{
    Q_D(const QMailFolderMessageSet);

    if (d->_hierarchical) {
        return contentKey(d->_id, true);
    } else {
        return QMailMessageSet::descendantsMessageKey();
    }
}

/*!
    Returns the display name of the folder that this message set represents.

    \sa QMailFolder::displayName()
*/
QString QMailFolderMessageSet::displayName() const
{
    Q_D(const QMailFolderMessageSet);

    if (d->_name.isNull()) {
        if (d->_id.isValid()) {
            QMailFolder folder(d->_id);
            d->_name = folder.displayName();
        }
    }
    if (d->_name.isNull()) {
        d->_name = QLatin1String("");
    }

    return d->_name;
}

/*!
    Returns the message key that defines the content of a QMailFolderMessageSet for the
    folder identified by \a id. If \a descendants is true, then the result is the key
    that defines the descendantMessageKey() content.
*/
QMailMessageKey QMailFolderMessageSet::contentKey(const QMailFolderId &id, bool descendants)
{
    if (descendants) {
        return QMailMessageKey::ancestorFolderIds(id, QMailDataComparator::Includes);
    } else {
        return QMailMessageKey::parentFolderId(id);
    }
}

/*! \internal */
void QMailFolderMessageSet::foldersAdded(const QMailFolderIdList &)
{
    synchronizeChildren();
}

/*! \internal */
void QMailFolderMessageSet::foldersRemoved(const QMailFolderIdList &)
{
    synchronizeChildren();
}

/*! \internal */
void QMailFolderMessageSet::foldersUpdated(const QMailFolderIdList &ids)
{
    Q_D(QMailFolderMessageSet);

    if (d->_hierarchical)
        synchronizeChildren();


    if (ids.contains(d->_id)) {
        //update our folder name
        d->_name.clear();

        update(this);
    }

}

/*! \internal */
void QMailFolderMessageSet::folderContentsModified(const QMailFolderIdList &ids)
{
    Q_D(QMailFolderMessageSet);

    if (ids.contains(d->_id))
        update(this);
}

/*! \internal */
void QMailFolderMessageSet::resyncState()
{
    Q_D(QMailFolderMessageSet);

    if (d->_hierarchical) {
        synchronizeChildren();
    }

    QMailMessageSet::resyncState();
}

/*! \internal */
void QMailFolderMessageSet::init()
{
    Q_D(QMailFolderMessageSet);

    if (d->_id.isValid()) {
        if (d->_hierarchical) {
            // Add items for any child folders
            synchronizeChildren();

            connect(model(), SIGNAL(foldersAdded(QMailFolderIdList)), this, SLOT(foldersAdded(QMailFolderIdList)));
            connect(model(), SIGNAL(foldersRemoved(QMailFolderIdList)), this, SLOT(foldersRemoved(QMailFolderIdList)));
        }

        connect(model(), SIGNAL(foldersUpdated(QMailFolderIdList)), this, SLOT(foldersUpdated(QMailFolderIdList)));
        connect(model(), SIGNAL(folderContentsModified(QMailFolderIdList)), this, SLOT(folderContentsModified(QMailFolderIdList)));
    }
}

/*! \internal */
void QMailFolderMessageSet::synchronizeChildren()
{
    Q_D(QMailFolderMessageSet);

    QMailFolderIdList newFolderIds(QMailStore::instance()->queryFolders(folderKey()));
    if (newFolderIds != d->_folderIds) {
        // Our subfolder set has changed
        d->_folderIds = newFolderIds;

        // Delete any child folders that are no longer present
        QList<QMailMessageSet*> obsoleteChildren;
        for (int i = 0; i < count(); ++i) {
            QMailFolderId childId = static_cast<QMailFolderMessageSet*>(at(i))->folderId();
            if (newFolderIds.contains(childId)) {
                newFolderIds.removeAll(childId);
            } else {
                obsoleteChildren.append(at(i));
            }
        }
        remove(obsoleteChildren);

        // Add any child folders we don't already contain
        foreach (const QMailFolderId &folderId, newFolderIds) {
            createChild(folderId);
        }

        update(this);
    }
}

/*!
    Creates a message set object for the folder identified by \a childId, and appends it
    to this object.

    Override this function to specialize the type created for child nodes.
*/
void QMailFolderMessageSet::createChild(const QMailFolderId &childId)
{
    Q_D(QMailFolderMessageSet);

    QMailFolderMessageSet *child = new QMailFolderMessageSet(this, childId, d->_hierarchical);
    append(child);
}

/*! \internal */
QMailFolderKey QMailFolderMessageSet::folderKey() const
{
    Q_D(const QMailFolderMessageSet);

    return QMailFolderKey::parentFolderId(d->_id);
}


/* QMailAccountMessageSet */

class QMailAccountMessageSetPrivate : public QMailMessageSetPrivate
{
public:
    QMailAccountMessageSetPrivate(QMailMessageSetContainer *container, const QMailAccountId &accountId, bool hierarchical)
        : QMailMessageSetPrivate(container),
          _id(accountId),
          _hierarchical(hierarchical)
    {
    }

    QMailAccountId _id;
    bool _hierarchical;
    mutable QString _name;
    QMailFolderIdList _folderIds;
};


/*!
    \class QMailAccountMessageSet
    \inmodule QmfClient

    \preliminary

    \brief The QMailAccountMessageSet class represents a set of messages corresponding to the content of a QMailAccount.

    QMailAccountMessageSet provides a representation for a named subset of messages, specified
    by their parent QMailAccount.

    If the QMailAccountMessageSet is hierarchical(), then any folders contained by the
    account will automatically be managed as child \l{QMailFolderMessageSet}s of the
    parent QMailAccountMessageSet.
*/

/*!
    Constructs a QMailAccountMessageSet within the parent container \a container,
    whose message set is defined by the content of the QMailAccount identified by
    \a accountId.  If \a hierarchical is true, the message set will automatically
    maintain a set of child QMailFolderMessageSets corresponding to \l{QMailFolder}s
    whose \l{QMailFolder::parentAccountId()}{parentAccountId} is \a accountId, and
    whose \l{QMailFolder::parentFolderId()}{parentFolderId} is empty.
*/
QMailAccountMessageSet::QMailAccountMessageSet(QMailMessageSetContainer *container, const QMailAccountId &accountId, bool hierarchical)
    : QMailMessageSet(new QMailAccountMessageSetPrivate(container, accountId, hierarchical))
{
}

/*!
    Returns the identifier of the QMailAccount associated with this message set.
*/
QMailAccountId QMailAccountMessageSet::accountId() const
{
    Q_D(const QMailAccountMessageSet);
    return d->_id;
}

/*!
    Returns true if this message set automatically maintains a hierarchy of child folder message sets.
*/
bool QMailAccountMessageSet::hierarchical() const
{
    Q_D(const QMailAccountMessageSet);
    return d->_hierarchical;
}

/*!
    Returns the QMailMessageKey that selects the messages contained by the configured account.
*/
QMailMessageKey QMailAccountMessageSet::messageKey() const
{
    Q_D(const QMailAccountMessageSet);
    return contentKey(d->_id, false);
}

/*!
    Returns the QMailMessageKey that selects the messages beneath the configured account in the hierarchy.
*/
QMailMessageKey QMailAccountMessageSet::descendantsMessageKey() const
{
    Q_D(const QMailAccountMessageSet);

    if (d->_hierarchical) {
        return contentKey(d->_id, true);
    } else {
        return QMailMessageSet::descendantsMessageKey();
    }
}

/*!
    Returns the name of the account that this message set represents.

    \sa QMailAccount::name()
*/
QString QMailAccountMessageSet::displayName() const
{
    Q_D(const QMailAccountMessageSet);

    if (d->_name.isNull()) {
        if (d->_id.isValid()) {
            QMailAccount account(d->_id);
            d->_name = account.name();
        }
    }
    if (d->_name.isNull()) {
        d->_name = QLatin1String("");
    }

    return d->_name;
}

/*!
    Returns the message key that defines the content of a QMailAccountMessageSet for the
    account identified by \a id.  If \a descendants is true, then the result is the key
    that defines the descendantMessageKey() content.
*/
QMailMessageKey QMailAccountMessageSet::contentKey(const QMailAccountId &id, bool descendants)
{
    if (descendants) {
        // Select any messages in folders owned by the account
        QMailFolderKey folderKey(QMailFolderKey::parentAccountId(id));
        return QMailMessageKey::parentFolderId(folderKey, QMailDataComparator::Includes);
    } else {
        return QMailMessageKey::parentAccountId(id);
    }
}

/*! \internal */
void QMailAccountMessageSet::foldersAdded(const QMailFolderIdList &)
{
    synchronizeChildren();
}

/*! \internal */
void QMailAccountMessageSet::foldersRemoved(const QMailFolderIdList &)
{
    synchronizeChildren();
}

/*! \internal */
void QMailAccountMessageSet::foldersUpdated(const QMailFolderIdList &)
{
    synchronizeChildren();
}

/*! \internal */
void QMailAccountMessageSet::accountsUpdated(const QMailAccountIdList &ids)
{
    Q_D(QMailAccountMessageSet);

    if (ids.contains(d->_id)) {
        //update our account name
        d->_name.clear();
        update(this);
    }
}

/*! \internal */
void QMailAccountMessageSet::accountContentsModified(const QMailAccountIdList &ids)
{
    Q_D(QMailAccountMessageSet);

    if (ids.contains(d->_id))
        update(this);
}

/*! \internal */
void QMailAccountMessageSet::resyncState()
{
    Q_D(QMailAccountMessageSet);

    if (d->_hierarchical) {
        synchronizeChildren();
    }

    QMailMessageSet::resyncState();
}

/*! \internal */
void QMailAccountMessageSet::init()
{
    Q_D(QMailAccountMessageSet);

    if (d->_id.isValid()) {
        if (d->_hierarchical) {
            // Add items for any child folders
            synchronizeChildren();

            connect(model(), SIGNAL(foldersAdded(QMailFolderIdList)), this, SLOT(foldersAdded(QMailFolderIdList)));
            connect(model(), SIGNAL(foldersRemoved(QMailFolderIdList)), this, SLOT(foldersRemoved(QMailFolderIdList)));
            connect(model(), SIGNAL(foldersUpdated(QMailFolderIdList)), this, SLOT(foldersUpdated(QMailFolderIdList)));
        }

        connect(model(), SIGNAL(accountsUpdated(QMailAccountIdList)), this, SLOT(accountsUpdated(QMailAccountIdList)));
        connect(model(), SIGNAL(accountContentsModified(QMailAccountIdList)), this, SLOT(accountContentsModified(QMailAccountIdList)));
    }
}

/*! \internal */
void QMailAccountMessageSet::synchronizeChildren()
{
    Q_D(QMailAccountMessageSet);

    QMailFolderIdList newFolderIds(QMailStore::instance()->queryFolders(rootFolderKey()));
    if (newFolderIds != d->_folderIds) {
        // Our subfolder set has changed
        d->_folderIds = newFolderIds;

        // Delete any child folders that are no longer present
        QList<QMailMessageSet*> obsoleteChildren;
        for (int i = 0; i < count(); ++i) {
            QMailFolderId childId = static_cast<QMailFolderMessageSet*>(at(i))->folderId();
            if (newFolderIds.contains(childId)) {
                newFolderIds.removeAll(childId);
            } else {
                obsoleteChildren.append(at(i));
            }
        }
        remove(obsoleteChildren);

        // Add any child folders we don't already contain
        foreach (const QMailFolderId &folderId, newFolderIds) {
            createChild(folderId);
        }

        update(this);
    }
}

/*!
    Creates a message set object for the folder identified by \a childId, and appends it
    to this object.

    Override this function to specialize the type created for child nodes.
*/
void QMailAccountMessageSet::createChild(const QMailFolderId &childId)
{
    Q_D(QMailAccountMessageSet);

    QMailFolderMessageSet *child = new QMailFolderMessageSet(this, childId, d->_hierarchical);
    append(child);
}

/*! \internal */
QMailFolderKey QMailAccountMessageSet::rootFolderKey() const
{
    Q_D(const QMailAccountMessageSet);

    // Select folders belonging to the account, that have no parent folder ID
    return QMailFolderKey::parentAccountId(d->_id) & QMailFolderKey::parentFolderId(QMailFolderId());
}


/* QMailFilterMessageSet */

class QMailFilterMessageSetPrivate : public QMailMessageSetPrivate
{
public:
    QMailFilterMessageSetPrivate(QMailMessageSetContainer *container, const QMailMessageKey &key, const QString &name, bool minimized)
        : QMailMessageSetPrivate(container),
          _key(key),
          _name(name),
          _minimized(minimized)
    {
    }

    QMailMessageKey _key;
    QString _name;
    bool _minimized;
    QSet<QMailMessageId> _messageIds;
};


/*!
    \class QMailFilterMessageSet
    \inmodule QmfClient

    \preliminary

    \brief The QMailFilterMessageSet class represents a set of messages selected by a pre-determined filter criteria.

    QMailFilterMessageSet provides a representation for a named subset of messages, specified
    by a set of criteria encoded into a QMailMessageKey object.  The properties of the
    QMailFilterMessageSet are mutable and can be changed after construction.
*/

/*!
    Constructs a QMailFilterMessageSet within the parent container \a container,
    named \a name, whose message set is specified by the filter \a key, and with
    update minimization set to \a minimalUpdates.

    \sa setUpdatesMinimized()
*/
QMailFilterMessageSet::QMailFilterMessageSet(QMailMessageSetContainer *container, const QMailMessageKey &key, const QString &name, bool minimalUpdates)
    : QMailMessageSet(new QMailFilterMessageSetPrivate(container, key, name, minimalUpdates))
{
}

/*!
    Returns the QMailMessageKey that selects the messages represented by this message set.
*/
QMailMessageKey QMailFilterMessageSet::messageKey() const
{
    Q_D(const QMailFilterMessageSet);

    return d->_key;
}

/*!
    Sets the QMailMessageKey that selects the messages represented by this message set to \a key.
*/
void QMailFilterMessageSet::setMessageKey(const QMailMessageKey &key)
{
    Q_D(QMailFilterMessageSet);

    d->_key = key;
    update(this);
}

/*!
    Returns the name of this message set for display purposes.
*/
QString QMailFilterMessageSet::displayName() const
{
    Q_D(const QMailFilterMessageSet);
    return d->_name;
}

/*!
    Sets the name of this message set for display purposes to \a name.
*/
void QMailFilterMessageSet::setDisplayName(const QString &name)
{
    Q_D(QMailFilterMessageSet);

    d->_name = name;
    update(this);
}

/*!
    Returns true if this message set has update minimization enabled; otherwise returns false;

    \sa setUpdatesMinimized()
*/
bool QMailFilterMessageSet::updatesMinimized() const
{
    Q_D(const QMailFilterMessageSet);
    return d->_minimized;
}

/*!
    Sets update minimization to \a set.

    If update minimization is set to true, the QMailFilterMessageSet will only
    emit the update() signal when the list of messages matching the filter key actually
    changes.  If update minimization is false, the update() signal will also be
    spuriously emitted; depending on the handling of that signal, this strategy
    may consume significantly less resources than are required to ensure minimal
    updates are emitted.

    \sa updatesMinimized()
*/
void QMailFilterMessageSet::setUpdatesMinimized(bool set)
{
    Q_D(QMailFilterMessageSet);

    if (d->_minimized != set) {
        d->_minimized = set;
        reset();
    }
}

/*! \internal */
void QMailFilterMessageSet::messagesAdded(const QMailMessageIdList &ids)
{
    Q_D(QMailFilterMessageSet);

    QMailMessageKey key(messageKey());
    if (!key.isNonMatching()) {
        // See if any of these messages match our filter
        QMailMessageKey idFilter(QMailMessageKey::id(ids));
        QMailMessageIdList matchingIds = QMailStore::instance()->queryMessages(key & idFilter);
        if (!matchingIds.isEmpty()) {
            // Our filtered message set has changed
            d->_messageIds.unite(QSet<QMailMessageId>(matchingIds.constBegin(), matchingIds.constEnd()));
            update(this);
        }
    }
}

/*! \internal */
void QMailFilterMessageSet::messagesRemoved(const QMailMessageIdList &ids)
{
    Q_D(QMailFilterMessageSet);

    QSet<QMailMessageId>& _messageIds = d->_messageIds;
    if (!_messageIds.isEmpty()) {
        QSet<QMailMessageId> removedIds = QSet<QMailMessageId>(ids.constBegin(), ids.constEnd());

        // See if any of these messages are in our set
        removedIds.intersect(_messageIds);
        if (!removedIds.isEmpty()) {
            _messageIds.subtract(removedIds);
            update(this);
        }
    }
}

/*! \internal */
void QMailFilterMessageSet::messagesUpdated(const QMailMessageIdList &ids)
{
    Q_D(QMailFilterMessageSet);

    QMailMessageKey key(messageKey());
    if (!key.isNonMatching()) {
        QSet<QMailMessageId>& _messageIds = d->_messageIds;
        QSet<QMailMessageId> updatedIds = QSet<QMailMessageId>(ids.constBegin(), ids.constEnd());

        // Find which of the updated messages should be in our set
        QMailMessageKey idFilter(QMailMessageKey::id(ids));
        const QMailMessageIdList filteredIds = QMailStore::instance()->queryMessages(key & idFilter);
        QSet<QMailMessageId> matchingIds = QSet<QMailMessageId>(filteredIds.constBegin(), filteredIds.constEnd());

        QSet<QMailMessageId> presentIds = updatedIds;
        QSet<QMailMessageId> absentIds = updatedIds;

        // Find which of these messages we already have, and which are not part of our filter
        presentIds.intersect(_messageIds);
        absentIds.subtract(presentIds);

        bool modified(false);

        if (!presentIds.isEmpty()) {
            // Remove any messages that no longer match the filter
            presentIds.subtract(matchingIds);
            if (!presentIds.isEmpty()) {
                _messageIds.subtract(presentIds);
                modified = true;
            }
        }

        if (!absentIds.isEmpty()) {
            // Add any messages that match our filter but aren't in our set
            absentIds.intersect(matchingIds);
            if (!absentIds.isEmpty()) {
                _messageIds.unite(absentIds);
                modified = true;
            }
        }

        if (modified)
            update(this);
    }
}

/*! \internal */
void QMailFilterMessageSet::folderContentsModified(const QMailFolderIdList &)
{
    if (!messageKey().isNonMatching()) {
        // Whenever any folder changes, we have potentially been modified
        update(this);
    }
}

/*! \internal */
void QMailFilterMessageSet::resyncState()
{
    Q_D(QMailFilterMessageSet);

    if (d->_minimized) {
        const QMailMessageIdList ids = QMailStore::instance()->queryMessages(messageKey());
        d->_messageIds = QSet<QMailMessageId>(ids.constBegin(), ids.constEnd());
    } else {
        d->_messageIds.clear();
    }

    QMailMessageSet::resyncState();
}

/*! \internal */
void QMailFilterMessageSet::init()
{
    reset();
}

/*! \internal */
void QMailFilterMessageSet::reset()
{
    Q_D(QMailFilterMessageSet);

    if (d->_minimized) {
        disconnect(model(), SIGNAL(folderContentsModified(QMailFolderIdList)), this, SLOT(folderContentsModified(QMailFolderIdList)));

        const QMailMessageIdList ids = QMailStore::instance()->queryMessages(messageKey());
        d->_messageIds = QSet<QMailMessageId>(ids.constBegin(), ids.constEnd());

        connect(model(), SIGNAL(messagesAdded(QMailMessageIdList)), this, SLOT(messagesAdded(QMailMessageIdList)));
        connect(model(), SIGNAL(messagesRemoved(QMailMessageIdList)), this, SLOT(messagesRemoved(QMailMessageIdList)));
        connect(model(), SIGNAL(messagesUpdated(QMailMessageIdList)), this, SLOT(messagesUpdated(QMailMessageIdList)));
    } else {
        disconnect(model(), SIGNAL(messagesAdded(QMailMessageIdList)), this, SLOT(messagesAdded(QMailMessageIdList)));
        disconnect(model(), SIGNAL(messagesRemoved(QMailMessageIdList)), this, SLOT(messagesRemoved(QMailMessageIdList)));
        disconnect(model(), SIGNAL(messagesUpdated(QMailMessageIdList)), this, SLOT(messagesUpdated(QMailMessageIdList)));

        d->_messageIds.clear();

        connect(model(), SIGNAL(folderContentsModified(QMailFolderIdList)), this, SLOT(folderContentsModified(QMailFolderIdList)));
    }
}


/* QMailMessageSetModel */

class QMailMessageSetModelPrivate : public QMailMessageSetContainerPrivate
{
public:
    enum UpdateState { Propagate, Detect, Detected, Suppressed };

    QMailMessageSetModelPrivate()
        : QMailMessageSetContainerPrivate(nullptr),
          _updateState(Propagate)
    {
    }

    QMap<QMailAccountId, QModelIndex> _accountMap;
    QMap<QMailFolderId, QModelIndex> _folderMap;

    UpdateState _updateState;
};


/*!
    \class QMailMessageSetModel
    \inmodule QmfClient

    \preliminary

    \brief The QMailMessageSetModel class provides a model for a tree of QMailMessageSets.

    QMailMessageSetModel provides a model containing sets of messages, arranged in a
    tree structure.  Each node in the tree is a named entity that represents a set of
    messages, specified by a QMailMessageKey filter.  QMailMessageSetModel can be used
    to construct a hierarchical tree of message folders, or other, more flexible ways of
    partitioning the set of messages into hierarchical groups.

    QMailMessageSetModel inherits from QAbstractItemModel, so it is suitable for use
    with the Qt View classes such as QTreeView, to visually represent the hierarchical
    structure.

    The model listens for change events emitted from the QMailStore, and automatically
    propagates these changes to attached views, unless the setIgnoreMailStoreUpdates()
    function is used to disable this feature.

    To customize the display of QMailMessageSets, create a delegate that paints the
    object as desired, using data elements accessed via the data() function.
    The data() function should be overridden by subclasses to support additional roles,
    or to customize the data displayed for existing roles.

    To define the content of a QMailMessageSetModel, derive classes from QMailMessageSet
    which select your desired message sets, and add them to the model in the init()
    member function.  The model is informed of the addition, removal and update events
    for message sets anywhere within the model, via the notification functions appended(),
    removed() and updated().  Override these functions to perform any content management
    tasks specific to your model.
*/

/*!
    \enum QMailMessageSetModel::Roles

    This enum type is used to define data elements used in common display roles when presenting message set objects.

    \value DisplayNameRole  The name of the message set for display purposes.
    \value MessageKeyRole   The message selection key associated with a message set.
    \value SubclassUserRole The first value that should be used by subclasses when defining new message set roles.
*/

/*!
    Constructs a QMailMessageSetModel object with the supplied \a parent.

    By default, mail store updates are not ignored.

    \sa setIgnoreMailStoreUpdates()
*/
QMailMessageSetModel::QMailMessageSetModel(QObject *parent)
    : QAbstractItemModel(parent),
      QMailMessageSetContainer(new QMailMessageSetModelPrivate)
{
    QTimer::singleShot(0, this, &QMailMessageSetModel::delayedInit);
}

/*! \internal */
QMailMessageSetModel::~QMailMessageSetModel()
{
}

/*! \internal */
int QMailMessageSetModel::rowCount(const QModelIndex &parentIndex) const
{
    if (QMailMessageSet *item = itemFromIndex(parentIndex))
        return item->count();

    return count();
}

/*! \internal */
int QMailMessageSetModel::columnCount(const QModelIndex &) const
{
    return 1;
}

/*!
    Returns true if the model contains no child message set objects; otherwise returns false.
*/
bool QMailMessageSetModel::isEmpty() const
{
    return (count() == 0);
}

/*!
    Returns an index object representing the object at \a row within the container located
    by \a parentIndex, having the column \a column.
*/
QModelIndex QMailMessageSetModel::index(int row, int column, const QModelIndex &parentIndex) const
{
    if (parentIndex.isValid()) {
        if (QMailMessageSetContainer *parent = itemFromIndex(parentIndex))
            if ((parent->count() > row) && (row >= 0))
                return createIndex(row, column, parent->at(row));
    } else {
        // From the top level
        if ((count() > row) && (row >= 0))
            return createIndex(row, column, at(row));
    }

    return QModelIndex();
}

/*!
    Returns an index object representing the parent container of the message set at \a index.
*/
QModelIndex QMailMessageSetModel::parent(const QModelIndex &index) const
{
    if (QMailMessageSet *item = itemFromIndex(index))
        return parentIndex(item, index.column());

    return QModelIndex();
}

/*!
    Return the index of the message set associated with the account identified by
    \a id, if one exists.

    \sa accountIdFromIndex()
*/
QModelIndex QMailMessageSetModel::indexFromAccountId(const QMailAccountId &id) const
{
    Q_D(const QMailMessageSetModel);

    QMap<QMailAccountId, QModelIndex>::const_iterator it = d->_accountMap.find(id);
    if (it != d->_accountMap.end())
        return *it;

    return QModelIndex();
}

/*!
    Return the index of the message set associated with the folder identified by
    \a id, if one exists.

    \sa folderIdFromIndex()
*/
QModelIndex QMailMessageSetModel::indexFromFolderId(const QMailFolderId &id) const
{
    Q_D(const QMailMessageSetModel);

    QMap<QMailFolderId, QModelIndex>::const_iterator it = d->_folderMap.find(id);
    if (it != d->_folderMap.end())
        return *it;

    return QModelIndex();
}

/*!
    Return the identifier of the account associated with the item at \a index, if that
    item's type conforms to QMailAccountMessageSet.
*/
QMailAccountId QMailMessageSetModel::accountIdFromIndex(const QModelIndex &index) const
{
    return itemAccountId(itemFromIndex(index));
}

/*!
    Return the identifier of the folder associated with the item at \a index, if that
    item's type conforms to QMailFolderMessageSet.
*/
QMailFolderId QMailMessageSetModel::folderIdFromIndex(const QModelIndex &index) const
{
    return itemFolderId(itemFromIndex(index));
}

/*!
    Returns the item located at \a index.
*/
QMailMessageSet *QMailMessageSetModel::itemFromIndex(const QModelIndex &index) const
{
    if (index.isValid())
        return static_cast<QMailMessageSet*>(index.internalPointer());

    return 0;
}

/*!
    Returns the index within the model of \a item.
*/
QModelIndex QMailMessageSetModel::indexFromItem(QMailMessageSet *item) const
{
    return const_cast<QMailMessageSetModel*>(this)->index(item, 0);
}

/*!
    Returns the data element for \a item, specified by \a role and \a column.
*/
QVariant QMailMessageSetModel::data(QMailMessageSet *item, int role, int column) const
{
    if (item) {
        // Defined roles:
        if (role >= DisplayNameRole && role <= MessageKeyRole) {
            if (role == DisplayNameRole)
                return item->displayName();
            else if (role == MessageKeyRole)
                return item->messageKey();
        }

        // Default fallback:
        if (role == Qt::DisplayRole && column == 0)
            return item->displayName();
    }

    return QVariant();
}

/*!
    Returns the data element for the item at \a index, specified by \a role.

    Note: this function is implemented by invoking the alternative overloaded method.
*/
QVariant QMailMessageSetModel::data(const QModelIndex &index, int role) const
{
    if (QMailMessageSet *item = itemFromIndex(index))
        return data(item, role, index.column());

    return QVariant();
}

/*! \internal */
QMailMessageSetModel *QMailMessageSetModel::model()
{
    return this;
}

/*!
    Returns true if the model has been set to ignore updates emitted by
    the mail store; otherwise returns false.
*/
bool QMailMessageSetModel::ignoreMailStoreUpdates() const
{
    Q_D(const QMailMessageSetModel);

    return d->_updateState != QMailMessageSetModelPrivate::Propagate;
}

/*!
    Sets whether or not mail store updates are ignored to \a ignore.

    If ignoring updates is set to true, the model will ignore updates reported
    by the mail store.  If set to false, the model will automatically synchronize
    its content in reaction to updates reported by the mail store.

    If updates are ignored, signals such as rowInserted and dataChanged will not
    be emitted; instead, the modelReset signal will be emitted when the model is
    later changed to stop ignoring mail store updates, and detailed change
    information will not be accessible.
*/
void QMailMessageSetModel::setIgnoreMailStoreUpdates(bool ignore)
{
    Q_D(QMailMessageSetModel);

    if (ignore) {
        if (d->_updateState == QMailMessageSetModelPrivate::Propagate)
            d->_updateState = QMailMessageSetModelPrivate::Detect;
    } else {
        bool resyncRequired(d->_updateState == QMailMessageSetModelPrivate::Detected
                            || d->_updateState == QMailMessageSetModelPrivate::Suppressed);

        d->_updateState = QMailMessageSetModelPrivate::Propagate;
        if (resyncRequired) {
            // We need to resynchronize our descendants
            resyncState();

            // Inform any attached views that we have been reset
            QAbstractItemModel::beginResetModel();
            QAbstractItemModel::endResetModel();
        }
    }
}

/*! \internal */
bool QMailMessageSetModel::propagateUpdates() const
{
    Q_D(const QMailMessageSetModel);
    return (d->_updateState != QMailMessageSetModelPrivate::Suppressed);
}

/*! \internal */
void QMailMessageSetModel::ceasePropagatingUpdates()
{
    Q_D(QMailMessageSetModel);
    d->_updateState = QMailMessageSetModelPrivate::Suppressed;
}

/*! \internal */
void QMailMessageSetModel::delayedInit()
{
    if (QMailStore* store = QMailStore::instance()) {
        connect(store, SIGNAL(accountsAdded(QMailAccountIdList)), this, SLOT(mailStoreAccountsAdded(QMailAccountIdList)));
        connect(store, SIGNAL(accountsRemoved(QMailAccountIdList)), this, SLOT(mailStoreAccountsRemoved(QMailAccountIdList)));
        connect(store, SIGNAL(accountsUpdated(QMailAccountIdList)), this, SLOT(mailStoreAccountsUpdated(QMailAccountIdList)));
        connect(store, SIGNAL(accountContentsModified(QMailAccountIdList)), this, SLOT(mailStoreAccountContentsModified(QMailAccountIdList)));

        connect(store, SIGNAL(foldersAdded(QMailFolderIdList)), this, SLOT(mailStoreFoldersAdded(QMailFolderIdList)));
        connect(store, SIGNAL(foldersRemoved(QMailFolderIdList)), this, SLOT(mailStoreFoldersRemoved(QMailFolderIdList)));
        connect(store, SIGNAL(foldersUpdated(QMailFolderIdList)), this, SLOT(mailStoreFoldersUpdated(QMailFolderIdList)));
        connect(store, SIGNAL(folderContentsModified(QMailFolderIdList)), this, SLOT(mailStoreFolderContentsModified(QMailFolderIdList)));

        connect(store, SIGNAL(messagesAdded(QMailMessageIdList)), this, SLOT(mailStoreMessagesAdded(QMailMessageIdList)));
        connect(store, SIGNAL(messagesRemoved(QMailMessageIdList)), this, SLOT(mailStoreMessagesRemoved(QMailMessageIdList)));
        connect(store, SIGNAL(messagesUpdated(QMailMessageIdList)), this, SLOT(mailStoreMessagesUpdated(QMailMessageIdList)));
    }
}

/*! \internal */
void QMailMessageSetModel::testForResync()
{
    Q_D(QMailMessageSetModel);

    if (d->_updateState == QMailMessageSetModelPrivate::Detect) {
        QTimer::singleShot(0, this, &QMailMessageSetModel::ceasePropagatingUpdates);
        d->_updateState = QMailMessageSetModelPrivate::Detected;
    }
}

/*! \internal */
void QMailMessageSetModel::mailStoreAccountsAdded(const QMailAccountIdList &ids)
{
    if (propagateUpdates())
        emit accountsAdded(ids);
}

/*! \internal */
void QMailMessageSetModel::mailStoreAccountsRemoved(const QMailAccountIdList &ids)
{
    if (propagateUpdates())
        emit accountsRemoved(ids);
}

/*! \internal */
void QMailMessageSetModel::mailStoreAccountsUpdated(const QMailAccountIdList &ids)
{
    if (propagateUpdates())
        emit accountsUpdated(ids);
}

/*! \internal */
void QMailMessageSetModel::mailStoreAccountContentsModified(const QMailAccountIdList &ids)
{
    if (propagateUpdates())
        emit accountContentsModified(ids);
}

/*! \internal */
void QMailMessageSetModel::mailStoreFoldersAdded(const QMailFolderIdList &ids)
{
    if (propagateUpdates())
        emit foldersAdded(ids);
}

/*! \internal */
void QMailMessageSetModel::mailStoreFoldersRemoved(const QMailFolderIdList &ids)
{
    if (propagateUpdates())
        emit foldersRemoved(ids);
}

/*! \internal */
void QMailMessageSetModel::mailStoreFoldersUpdated(const QMailFolderIdList &ids)
{
    if (propagateUpdates())
        emit foldersUpdated(ids);
}

/*! \internal */
void QMailMessageSetModel::mailStoreFolderContentsModified(const QMailFolderIdList &ids)
{
    if (propagateUpdates())
        emit folderContentsModified(ids);
}

/*! \internal */
void QMailMessageSetModel::mailStoreMessagesAdded(const QMailMessageIdList &ids)
{
    if (propagateUpdates())
        emit messagesAdded(ids);
}

/*! \internal */
void QMailMessageSetModel::mailStoreMessagesRemoved(const QMailMessageIdList &ids)
{
    if (propagateUpdates())
        emit messagesRemoved(ids);
}

/*! \internal */
void QMailMessageSetModel::mailStoreMessagesUpdated(const QMailMessageIdList &ids)
{
    if (propagateUpdates())
        emit messagesUpdated(ids);
}

/*! \internal */
QMailAccountId QMailMessageSetModel::itemAccountId(QMailMessageSet *item) const
{
    if (QMailAccountMessageSet *accountItem = qobject_cast<QMailAccountMessageSet*>(item)) {
        return accountItem->accountId();
    }

    return QMailAccountId();
}

/*! \internal */
QMailFolderId QMailMessageSetModel::itemFolderId(QMailMessageSet *item) const
{
    if (QMailFolderMessageSet *folderItem = qobject_cast<QMailFolderMessageSet*>(item)) {
        return folderItem->folderId();
    }

    return QMailFolderId();
}

/*!
    Updates the model's indexing information when \a item is appended to a container within the model.

    Override this function to perform any management tasks specific to a subclass of QMailMessageSetContainer.
*/
void QMailMessageSetModel::appended(QMailMessageSet *item)
{
    Q_D(QMailMessageSetModel);

    QMailFolderId folderId = itemFolderId(item);
    if (folderId.isValid()) {
        d->_folderMap[folderId] = item->modelIndex();
        return;
    }

    QMailAccountId accountId = itemAccountId(item);
    if (accountId.isValid()) {
        d->_accountMap[accountId] = item->modelIndex();
        return;
    }
}

/*!
    Updates the model's indexing information when \a item is removed from a container within the model.

    Override this function to perform any management tasks specific to a subclass of QMailMessageSetContainer.
*/
void QMailMessageSetModel::removed(QMailMessageSet *item)
{
    Q_D(QMailMessageSetModel);

    QMailFolderId folderId = itemFolderId(item);
    if (folderId.isValid()) {
        d->_folderMap.remove(folderId);
        return;
    }

    QMailAccountId accountId = itemAccountId(item);
    if (accountId.isValid()) {
        d->_accountMap.remove(accountId);
        return;
    }
}


/*!
    Updates the model's indexing information when \a item is updated.

    Override this function to perform any management tasks specific to a subclass of QMailMessageSetContainer.
*/
void QMailMessageSetModel::updated(QMailMessageSet *item)
{
    Q_UNUSED(item)
}

/*!
    Called immediately before the message set \a child is appended to any container already present in the model.
    \internal
*/
void QMailMessageSetModel::beginAppend(QMailMessageSet *child)
{
    int row(child->parentContainer()->count());
    beginInsertRows(parentIndex(child, 0), row, row);
}

/*!
    Called immediately after the message set \a child is appended to any container already present in the model.
    \internal
*/
void QMailMessageSetModel::endAppend(QMailMessageSet *child)
{
    appended(child);
    endInsertRows();

    testForResync();
}

/*!
    Called immediately before the message set \a child is removed from any container already present in the model.
    \internal
*/
void QMailMessageSetModel::beginRemove(QMailMessageSet *child)
{
    int row(child->parentContainer()->indexOf(child));
    beginRemoveRows(parentIndex(child, 0), row, row);
}

/*!
    Called immediately after the message set \a child is removed from any container already present in the model.
    \internal
*/
void QMailMessageSetModel::endRemove(QMailMessageSet *child)
{
    removed(child);
    endRemoveRows();

    testForResync();
}

/*!
    Called immediately after the message set \a child is updated while owned by a container already in the model.
    \internal
*/
void QMailMessageSetModel::doUpdate(QMailMessageSet *child)
{
    updated(child);

    QModelIndex childIndex(index(child, 0));
    dataChanged(childIndex, childIndex);

    testForResync();
}

/*! \internal */
QObject *QMailMessageSetModel::qObject()
{
    return this;
}

/*! \internal */
QModelIndex QMailMessageSetModel::index(QMailMessageSet *item, int column) const
{
    if (QMailMessageSetContainer *parent = item->parentContainer())
        return createIndex(parent->indexOf(item), column, item);

    return QModelIndex();
}

/*! \internal */
QModelIndex QMailMessageSetModel::parentIndex(QMailMessageSet *item, int column) const
{
    if (QMailMessageSetContainer *parent = item->parentContainer())
        if (parent->parentContainer() != 0)
            return index(static_cast<QMailMessageSet*>(parent), column);

    return QModelIndex();
}

/*!
    \fn void QMailMessageSetModel::accountsAdded(const QMailAccountIdList& ids)

    Signal that is emitted when the accounts in the list \a ids are
    added to the mail store.

    \sa accountsRemoved(), accountsUpdated()
*/

/*!
    \fn void QMailMessageSetModel::accountsRemoved(const QMailAccountIdList& ids)

    Signal that is emitted when the accounts in the list \a ids are
    removed from the mail store.

    \sa accountsAdded(), accountsUpdated()
*/

/*!
    \fn void QMailMessageSetModel::accountsUpdated(const QMailAccountIdList& ids)

    Signal that is emitted when the accounts in the list \a ids are
    updated within the mail store.

    \sa accountsAdded(), accountsRemoved()
*/

/*!
    \fn void QMailMessageSetModel::accountContentsModified(const QMailAccountIdList& ids)

    Signal that is emitted when changes to messages and folders in the mail store
    affect the content of the accounts in the list \a ids.

    \sa messagesAdded(), messagesUpdated(), messagesRemoved(), foldersAdded(), foldersUpdated(), foldersRemoved()
*/

/*!
    \fn void QMailMessageSetModel::foldersAdded(const QMailFolderIdList& ids)

    Signal that is emitted when the folders in the list \a ids are
    added to the mail store.

    \sa foldersRemoved(), foldersUpdated()
*/

/*!
    \fn void QMailMessageSetModel::foldersRemoved(const QMailFolderIdList& ids)

    Signal that is emitted when the folders in the list \a ids are
    removed from the mail store.

    \sa foldersAdded(), foldersUpdated()
*/

/*!
    \fn void QMailMessageSetModel::foldersUpdated(const QMailFolderIdList& ids)

    Signal that is emitted when the folders in the list \a ids are
    updated within the mail store.

    \sa foldersAdded(), foldersRemoved()
*/

/*!
    \fn void QMailMessageSetModel::folderContentsModified(const QMailFolderIdList& ids)

    Signal that is emitted when changes to messages in the mail store
    affect the content of the folders in the list \a ids.

    \sa messagesAdded(), messagesUpdated(), messagesRemoved()
*/

/*!
    \fn void QMailMessageSetModel::messagesAdded(const QMailMessageIdList& ids)

    Signal that is emitted when the messages in the list \a ids are
    added to the mail store.

    \sa messagesRemoved(), messagesUpdated()
*/

/*!
    \fn void QMailMessageSetModel::messagesRemoved(const QMailMessageIdList& ids)

    Signal that is emitted when the messages in the list \a ids are
    removed from the mail store.

    \sa messagesAdded(), messagesUpdated()
*/

/*!
    \fn void QMailMessageSetModel::messagesUpdated(const QMailMessageIdList& ids)

    Signal that is emitted when the messages in the list \a ids are
    updated within the mail store.

    \sa messagesAdded(), messagesRemoved()
*/

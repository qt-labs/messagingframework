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

#include "qmailstoreaccountfilter.h"
#include <QHash>
#include <QMetaMethod>
#include <QMetaObject>
#include <QSettings>

QT_BEGIN_NAMESPACE

static uint qHash(const QMetaMethod &m)
{
    return qHash(m.methodIndex());
}

QT_END_NAMESPACE

class QMailStoreEvents : public QObject
{
    Q_OBJECT

public:
    QMailStoreEvents();
    ~QMailStoreEvents();

    void registerConnection(const QMetaMethod &signal, const QMailAccountId &id, QMailStoreAccountFilter *filter);
    void deregisterConnection(const QMetaMethod &signal, const QMailAccountId &id, QMailStoreAccountFilter *filter);
    
private slots:
    void accountsUpdated(const QMailAccountIdList& ids);
    void accountContentsModified(const QMailAccountIdList& ids);

    void messagesAdded(const QMailMessageIdList& ids);
    void messagesRemoved(const QMailMessageIdList& ids);
    void messagesUpdated(const QMailMessageIdList& ids);
    void messageContentsModified(const QMailMessageIdList& ids);

    void foldersAdded(const QMailFolderIdList& ids);
    void foldersRemoved(const QMailFolderIdList& ids);
    void foldersUpdated(const QMailFolderIdList& ids);
    void folderContentsModified(const QMailFolderIdList& ids);

    void messageRemovalRecordsAdded(const QMailAccountIdList& ids);
    void messageRemovalRecordsRemoved(const QMailAccountIdList& ids);
    
private:
    typedef QMap<QMailAccountId, QSet<QMailStoreAccountFilter*> > ConnectionType;

    bool initConnections();

    void foreachAccount(const QMailAccountIdList& ids, const ConnectionType &connection, void (QMailStoreAccountFilter::*signal)());
    void foreachFolder(const QMailFolderIdList& ids, const ConnectionType &connection, void (QMailStoreAccountFilter::*signal)(const QMailFolderIdList&));
    void foreachMessage(const QMailMessageIdList& ids, const ConnectionType &connection, void (QMailStoreAccountFilter::*signal)(const QMailMessageIdList&));

    QMap<QMailAccountId, QMailFolderIdList> accountFolders(const QMailFolderIdList& ids, const QList<QMailAccountId> &accounts);
    QMap<QMailAccountId, QMailMessageIdList> accountMessages(const QMailMessageIdList& ids, const QList<QMailAccountId> &accounts);

    QHash<QMetaMethod, ConnectionType> _connections;
};


QMailStoreEvents::QMailStoreEvents()
{
}

QMailStoreEvents::~QMailStoreEvents()
{
}

void QMailStoreEvents::registerConnection(const QMetaMethod &signal, const QMailAccountId &id, QMailStoreAccountFilter *filter)
{
    static const bool initialized = initConnections();
    Q_UNUSED(initialized)

    QHash<QMetaMethod, ConnectionType>::iterator it = _connections.find(signal);
    if (it == _connections.end()) {
        qWarning() << "QMailStoreEvents::registerConnection - No such signal:" << signal.methodSignature();
    } else {
        ConnectionType &connection(it.value());

        ConnectionType::iterator cit = connection.find(id);
        if (cit == connection.end()) {
            cit = connection.insert(id, QSet<QMailStoreAccountFilter*>());
        }

        cit.value().insert(filter);
    }
}

void QMailStoreEvents::deregisterConnection(const QMetaMethod &signal, const QMailAccountId &id, QMailStoreAccountFilter *filter)
{
    QHash<QMetaMethod, ConnectionType>::iterator it = _connections.find(signal);
    if (it == _connections.end()) {
        qWarning() << "QMailStoreEvents::deregisterConnection - No such signal:" << signal.methodSignature();
    } else {
        ConnectionType &connection(it.value());

        ConnectionType::iterator cit = connection.find(id);
        if (cit != connection.end()) {
            cit.value().remove(filter);
        }
    }
}
    
void QMailStoreEvents::accountsUpdated(const QMailAccountIdList& ids)
{
    static const QMetaMethod signal = QMetaMethod::fromSignal(&QMailStoreAccountFilter::accountUpdated);
    static const ConnectionType &connection = _connections[signal];

    foreachAccount(ids, connection, &QMailStoreAccountFilter::accountUpdated);
}

void QMailStoreEvents::accountContentsModified(const QMailAccountIdList& ids)
{
    static const QMetaMethod signal = QMetaMethod::fromSignal(&QMailStoreAccountFilter::accountContentsModified);
    static const ConnectionType &connection = _connections[signal];

    foreachAccount(ids, connection, &QMailStoreAccountFilter::accountContentsModified);
}

void QMailStoreEvents::messagesAdded(const QMailMessageIdList& ids)
{
    static const QMetaMethod signal = QMetaMethod::fromSignal(&QMailStoreAccountFilter::messagesAdded);
    static const ConnectionType &connection = _connections[signal];

    foreachMessage(ids, connection, &QMailStoreAccountFilter::messagesAdded);
}

void QMailStoreEvents::messagesRemoved(const QMailMessageIdList& ids)
{
    static const QMetaMethod signal = QMetaMethod::fromSignal(&QMailStoreAccountFilter::messagesRemoved);
    static const ConnectionType &connection = _connections[signal];

    foreachMessage(ids, connection, &QMailStoreAccountFilter::messagesRemoved);
}

void QMailStoreEvents::messagesUpdated(const QMailMessageIdList& ids)
{
    static const QMetaMethod signal = QMetaMethod::fromSignal(&QMailStoreAccountFilter::messagesUpdated);
    static const ConnectionType &connection = _connections[signal];

    foreachMessage(ids, connection, &QMailStoreAccountFilter::messagesUpdated);
}

void QMailStoreEvents::messageContentsModified(const QMailMessageIdList& ids)
{
    static const QMetaMethod signal = QMetaMethod::fromSignal(&QMailStoreAccountFilter::messageContentsModified);
    static const ConnectionType &connection = _connections[signal];

    foreachMessage(ids, connection, &QMailStoreAccountFilter::messageContentsModified);
}

void QMailStoreEvents::foldersAdded(const QMailFolderIdList& ids)
{
    static const QMetaMethod signal = QMetaMethod::fromSignal(&QMailStoreAccountFilter::foldersAdded);
    static const ConnectionType &connection = _connections[signal];

    foreachFolder(ids, connection, &QMailStoreAccountFilter::foldersAdded);
}

void QMailStoreEvents::foldersRemoved(const QMailFolderIdList& ids)
{
    static const QMetaMethod signal = QMetaMethod::fromSignal(&QMailStoreAccountFilter::foldersRemoved);
    static const ConnectionType &connection = _connections[signal];

    foreachFolder(ids, connection, &QMailStoreAccountFilter::foldersRemoved);
}

void QMailStoreEvents::foldersUpdated(const QMailFolderIdList& ids)
{
    static const QMetaMethod signal = QMetaMethod::fromSignal(&QMailStoreAccountFilter::foldersUpdated);
    static const ConnectionType &connection = _connections[signal];

    foreachFolder(ids, connection, &QMailStoreAccountFilter::foldersUpdated);
}

void QMailStoreEvents::folderContentsModified(const QMailFolderIdList& ids)
{
    static const QMetaMethod signal = QMetaMethod::fromSignal(&QMailStoreAccountFilter::folderContentsModified);
    static const ConnectionType &connection = _connections[signal];

    foreachFolder(ids, connection, &QMailStoreAccountFilter::folderContentsModified);
}

void QMailStoreEvents::messageRemovalRecordsAdded(const QMailAccountIdList& ids)
{
    static const QMetaMethod signal = QMetaMethod::fromSignal(&QMailStoreAccountFilter::messageRemovalRecordsAdded);
    static const ConnectionType &connection = _connections[signal];

    foreachAccount(ids, connection, &QMailStoreAccountFilter::messageRemovalRecordsAdded);
}

void QMailStoreEvents::messageRemovalRecordsRemoved(const QMailAccountIdList& ids)
{
    static const QMetaMethod signal = QMetaMethod::fromSignal(&QMailStoreAccountFilter::messageRemovalRecordsRemoved);
    static const ConnectionType &connection = _connections[signal];

    foreachAccount(ids, connection, &QMailStoreAccountFilter::messageRemovalRecordsRemoved);
}

bool QMailStoreEvents::initConnections()
{
    foreach (const QMetaMethod &signal,
             QList<QMetaMethod>() << QMetaMethod::fromSignal(&QMailStoreAccountFilter::accountUpdated)
                           << QMetaMethod::fromSignal(&QMailStoreAccountFilter::accountContentsModified)
                           << QMetaMethod::fromSignal(&QMailStoreAccountFilter::messagesAdded)
                           << QMetaMethod::fromSignal(&QMailStoreAccountFilter::messagesRemoved)
                           << QMetaMethod::fromSignal(&QMailStoreAccountFilter::messagesUpdated)
                           << QMetaMethod::fromSignal(&QMailStoreAccountFilter::messageContentsModified)
                           << QMetaMethod::fromSignal(&QMailStoreAccountFilter::foldersAdded)
                           << QMetaMethod::fromSignal(&QMailStoreAccountFilter::foldersRemoved)
                           << QMetaMethod::fromSignal(&QMailStoreAccountFilter::foldersUpdated)
                           << QMetaMethod::fromSignal(&QMailStoreAccountFilter::folderContentsModified)
                           << QMetaMethod::fromSignal(&QMailStoreAccountFilter::messageRemovalRecordsAdded)
                           << QMetaMethod::fromSignal(&QMailStoreAccountFilter::messageRemovalRecordsRemoved)) {
        _connections.insert(signal, ConnectionType());
    }

    if (QMailStore *store = QMailStore::instance()) {
        connect(store, SIGNAL(accountsUpdated(QMailAccountIdList)), this, SLOT(accountsUpdated(QMailAccountIdList)));
        connect(store, SIGNAL(accountContentsModified(QMailAccountIdList)), this, SLOT(accountContentsModified(QMailAccountIdList)));

        connect(store, SIGNAL(messagesAdded(QMailMessageIdList)), this, SLOT(messagesAdded(QMailMessageIdList)));
        connect(store, SIGNAL(messagesRemoved(QMailMessageIdList)), this, SLOT(messagesRemoved(QMailMessageIdList)));
        connect(store, SIGNAL(messagesUpdated(QMailMessageIdList)), this, SLOT(messagesUpdated(QMailMessageIdList)));
        connect(store, SIGNAL(messageContentsModified(QMailMessageIdList)), this, SLOT(messageContentsModified(QMailMessageIdList)));

        connect(store, SIGNAL(foldersAdded(QMailFolderIdList)), this, SLOT(foldersAdded(QMailFolderIdList)));
        connect(store, SIGNAL(foldersRemoved(QMailFolderIdList)), this, SLOT(foldersRemoved(QMailFolderIdList)));
        connect(store, SIGNAL(foldersUpdated(QMailFolderIdList)), this, SLOT(foldersUpdated(QMailFolderIdList)));
        connect(store, SIGNAL(folderContentsModified(QMailFolderIdList)), this, SLOT(folderContentsModified(QMailFolderIdList)));

        connect(store, SIGNAL(messageRemovalRecordsAdded(QMailAccountIdList)), this, SLOT(messageRemovalRecordsAdded(QMailAccountIdList)));
        connect(store, SIGNAL(messageRemovalRecordsRemoved(QMailAccountIdList)), this, SLOT(messageRemovalRecordsRemoved(QMailAccountIdList)));
    }

    return true;
}

void QMailStoreEvents::foreachAccount(const QMailAccountIdList& ids, const ConnectionType &connection, void (QMailStoreAccountFilter::*signal)())
{
    foreach (const QMailAccountId &id, ids) {
        ConnectionType::const_iterator it = connection.find(id);
        if (it != connection.end())
            foreach (QMailStoreAccountFilter *filter, it.value())
                emit (filter->*signal)();
    }
}

void QMailStoreEvents::foreachFolder(const QMailFolderIdList& ids, const ConnectionType &connection, void (QMailStoreAccountFilter::*signal)(const QMailFolderIdList&))
{
    if (!connection.isEmpty()) {
        QMap<QMailAccountId, QMailFolderIdList> folders(accountFolders(ids, connection.keys()));

        ConnectionType::const_iterator it = connection.begin(), end = connection.end();
        for ( ; it != end; ++it) {
            QMap<QMailAccountId, QMailFolderIdList>::const_iterator fit = folders.find(it.key());
            if (fit != folders.end())
                if (!fit.value().isEmpty())
                    foreach (QMailStoreAccountFilter *filter, it.value())
                        emit (filter->*signal)(fit.value());
        }
    }
}

void QMailStoreEvents::foreachMessage(const QMailMessageIdList& ids, const ConnectionType &connection, void (QMailStoreAccountFilter::*signal)(const QMailMessageIdList&))
{
    if (!connection.isEmpty()) {
        QMap<QMailAccountId, QMailMessageIdList> messages(accountMessages(ids, connection.keys()));

        ConnectionType::const_iterator it = connection.begin(), end = connection.end();
        for ( ; it != end; ++it) {
            QMap<QMailAccountId, QMailMessageIdList>::const_iterator mit = messages.find(it.key());
            if (mit != messages.end())
                if (!mit.value().isEmpty())
                    foreach (QMailStoreAccountFilter *filter, it.value())
                        emit (filter->*signal)(mit.value());
        }
    }
}

QMap<QMailAccountId, QMailFolderIdList> QMailStoreEvents::accountFolders(const QMailFolderIdList& ids, const QList<QMailAccountId> &accounts)
{
    QMap<QMailAccountId, QMailFolderIdList> map;

    foreach (const QMailAccountId &accountId, accounts) {
        QMap<QMailAccountId, QMailFolderIdList>::iterator it = map.insert(accountId, QMailFolderIdList());
        
        // Find which of these folders belong to this account
        foreach (const QMailFolderId &id, QMailStore::instance()->queryFolders(QMailFolderKey::id(ids) & QMailFolderKey::parentAccountId(accountId)))
            it.value().append(id);
    }

    return map;
}

QMap<QMailAccountId, QMailMessageIdList> QMailStoreEvents::accountMessages(const QMailMessageIdList& ids, const QList<QMailAccountId> &accounts)
{
    QMap<QMailAccountId, QMailMessageIdList> map;
    foreach (const QMailAccountId &id, accounts)
        map.insert(id, QMailMessageIdList());

    // Find which accounts these messages belong to (if the account is in our mapping)
    QMailMessageKey::Properties props(QMailMessageKey::Id | QMailMessageKey::ParentAccountId);
    foreach (const QMailMessageMetaData &metaData, QMailStore::instance()->messagesMetaData(QMailMessageKey::id(ids), props)) {
        QMap<QMailAccountId, QMailMessageIdList>::iterator it = map.find(metaData.parentAccountId());
        if (it != map.end())
            it.value().append(metaData.id());
    }

    return map;
}


class QMailStoreAccountFilterPrivate : public QObject
{
    Q_OBJECT

public:
    QMailStoreAccountFilterPrivate(const QMailAccountId &id, QMailStoreAccountFilter *filter);
    ~QMailStoreAccountFilterPrivate();

    void incrementConnectionCount(const QMetaMethod &signal, int increment);

private:
    QMailAccountId _id;
    QMailStoreAccountFilter *_filter;
    QHash<QMetaMethod, uint> _connectionCount;

    static QMailStoreEvents _events;
};

QMailStoreEvents QMailStoreAccountFilterPrivate::_events;


QMailStoreAccountFilterPrivate::QMailStoreAccountFilterPrivate(const QMailAccountId &id, QMailStoreAccountFilter *filter)
    : _id(id),
      _filter(filter)
{
}

QMailStoreAccountFilterPrivate::~QMailStoreAccountFilterPrivate()
{
    QHash<QMetaMethod, uint>::const_iterator it = _connectionCount.begin(), end = _connectionCount.end();
    for ( ; it != end; ++it) {
        if (it.value() > 0)
            _events.deregisterConnection(it.key(), _id, _filter);
    }
}

void QMailStoreAccountFilterPrivate::incrementConnectionCount(const QMetaMethod &signal, int increment)
{
    uint &count = _connectionCount[signal];
    if (count == 0 && (increment > 0)) {
        _events.registerConnection(signal, _id, _filter);
    } else if ((count + increment) == 0) {
        _events.deregisterConnection(signal, _id, _filter);
    }

    count += increment;
}

/*!
    \class QMailStoreAccountFilter
    \ingroup libmessageserver

    \preliminary
    \brief The QMailStoreAccountFilter class provides a filtered view of QMailStore signals, affecting a single account.

    The QMailStoreAccountFilter class allows a client to respond to only those QMailStore signals that
    affect a particular account, without the need to respond to all signals and test for applicability.

    \sa QMailStore
*/

/*!
    Creates a filter object whose signals report only the events that affect
    the account identified by \a id.
*/
QMailStoreAccountFilter::QMailStoreAccountFilter(const QMailAccountId &id)
    : d(new QMailStoreAccountFilterPrivate(id, this))
{
}

/*! \internal */
QMailStoreAccountFilter::~QMailStoreAccountFilter()
{
    delete d;
}

/* \reimp */
void QMailStoreAccountFilter::connectNotify(const QMetaMethod &signal)
{
    d->incrementConnectionCount(signal, 1);
}

/* \reimp */
void QMailStoreAccountFilter::disconnectNotify(const QMetaMethod &signal)
{
    d->incrementConnectionCount(signal, -1);
}

/*!
    \fn void QMailStoreAccountFilter::accountUpdated()

    Signal that is emitted when the filter account is updated within the mail store.

    \sa QMailStore::accountsUpdated()
*/

/*!
    \fn void QMailStoreAccountFilter::accountContentsModified()

    Signal that is emitted when changes to messages and folders in the mail store
    affect the content of the filter account.

    \sa QMailStore::accountContentsModified()
*/

/*!
    \fn void QMailStoreAccountFilter::messagesAdded(const QMailMessageIdList& ids)

    Signal that is emitted when the messages in the list \a ids are added to the mail store.

    \sa QMailStore::messagesAdded()
*/

/*!
    \fn void QMailStoreAccountFilter::messagesRemoved(const QMailMessageIdList& ids)

    Signal that is emitted when the messages in the list \a ids are removed from the mail store.

    \sa QMailStore::messagesRemoved()
*/

/*!
    \fn void QMailStoreAccountFilter::messagesUpdated(const QMailMessageIdList& ids)

    Signal that is emitted when the messages in the list \a ids are updated within the mail store.

    \sa QMailStore::messagesUpdated()
*/

/*!
    \fn void QMailStoreAccountFilter::messageContentsModified(const QMailMessageIdList& ids)

    Signal that is emitted when the content of the messages in list \a ids is updated.

    \sa QMailStore::messageContentsModified()
*/

/*!
    \fn void QMailStoreAccountFilter::foldersAdded(const QMailFolderIdList& ids)

    Signal that is emitted when the folders in the list \a ids are added to the mail store.

    \sa QMailStore::foldersAdded()
*/

/*!
    \fn void QMailStoreAccountFilter::foldersRemoved(const QMailFolderIdList& ids)

    Signal that is emitted when the folders in the list \a ids are removed from the mail store.

    \sa QMailStore::foldersRemoved()
*/

/*!
    \fn void QMailStoreAccountFilter::foldersUpdated(const QMailFolderIdList& ids)

    Signal that is emitted when the folders in the list \a ids are updated within the mail store.

    \sa QMailStore::foldersUpdated()
*/

/*!
    \fn void QMailStoreAccountFilter::messageRemovalRecordsAdded()

    Signal that is emitted when QMailMessageRemovalRecords are added to the store, affecting the filter account.

    \sa QMailStore::messageRemovalRecordsAdded()
*/

/*!
    \fn void QMailStoreAccountFilter::messageRemovalRecordsRemoved()

    Signal that is emitted when QMailMessageRemovalRecords are removed from the store, affecting the filter account.

    \sa QMailStore::messageRemovalRecordsRemoved()
*/

/*!
    \fn void QMailStoreAccountFilter::folderContentsModified(const QMailFolderIdList& ids)

    Signal that is emitted when changes to messages in the mail store
    affect the content of the folders in the list \a ids.

    \sa QMailStore::folderContentsModified()
*/

#include <qmailstoreaccountfilter.moc>


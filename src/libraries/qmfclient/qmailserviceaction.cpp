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

#include "qmailserviceaction_p.h"
#include "qmailipc.h"
#include "qmailmessagekey.h"
#include "qmailstore.h"
#include "qmaillog.h"
#include "qmaildisconnected.h"
#include "qmailnamespace.h"
#include <qmailcontentmanager.h>

#include <QCoreApplication>
#include <QPair>
#include <QTimer>
#include <QDir>
#include <QTemporaryFile>

namespace {

// NOTE: messageserver ServiceHandler::dispatchRequest() relies on internal format defined below.
uint messageCounter = 0;
const uint pid = static_cast<uint>(QCoreApplication::applicationPid() & 0xffffffff);

quint64 nextMessageAction()
{
    return ((quint64(pid) << 32) | quint64(++messageCounter));
}

QPair<uint, uint> messageActionParts(quint64 action)
{
    return qMakePair(uint(action >> 32), uint(action & 0xffffffff));
}

}

QMailServiceActionPrivate::QMailServiceActionPrivate(QMailServiceAction *i,
                                                     QSharedPointer<QMailMessageServer> server)
    : _interface(i),
      _server(server ? server : QSharedPointer<QMailMessageServer>(new QMailMessageServer)),
      _connectivity(QMailServiceAction::Offline),
      _activity(QMailServiceAction::Pending),
      _status(QMailServiceAction::Status::ErrNoError, QString(), QMailAccountId(), QMailFolderId(), QMailMessageId()),
      _total(0),
      _progress(0),
      _isValid(false),
      _action(0),
      _connectivityChanged(false),
      _activityChanged(false),
      _progressChanged(false),
      _statusChanged(false)
{
    connect(_server.data(), &QMailMessageServer::activityChanged,
            this, &QMailServiceActionPrivate::activityChanged);
    connect(_server.data(), &QMailMessageServer::connectivityChanged,
            this, &QMailServiceActionPrivate::connectivityChanged);
    connect(_server.data(), &QMailMessageServer::statusChanged,
            this, &QMailServiceActionPrivate::statusChanged);
    connect(_server.data(), &QMailMessageServer::progressChanged,
            this, &QMailServiceActionPrivate::progressChanged);
}

QMailServiceActionPrivate::~QMailServiceActionPrivate()
{
}

void QMailServiceActionPrivate::cancelOperation()
{
    Q_ASSERT(_action != 0 && _isValid);
    if (_isValid) {
        if (_pendingActions.count()) {
            _pendingActions.first().action->cancelOperation();
            clearSubActions();
        }
        _server->cancelTransfer(_action);
    }
}

void QMailServiceActionPrivate::setAction(quint64 action)
{
    _isValid = action != 0;
    _action = action;
}

void QMailServiceActionPrivate::activityChanged(quint64 action, QMailServiceAction::Activity activity)
{
    if (validAction(action)) {
        setActivity(activity);

        emitChanges();
    }
}

void QMailServiceActionPrivate::connectivityChanged(quint64 action, QMailServiceAction::Connectivity connectivity)
{
    if (validAction(action)) {
        setConnectivity(connectivity);

        emitChanges();
    }
}

void QMailServiceActionPrivate::statusChanged(quint64 action, const QMailServiceAction::Status status)
{
    if (validAction(action)) {
        setStatus(status);

        emitChanges();
    }
}

void QMailServiceActionPrivate::progressChanged(quint64 action, uint progress, uint total)
{
    if (validAction(action)) {
        setProgress(progress, total);

        emitChanges();
    }
}

void QMailServiceActionPrivate::subActionConnectivityChanged(QMailServiceAction::Connectivity c)
{
    connectivityChanged(_action, c);
}

void QMailServiceActionPrivate::subActionActivityChanged(QMailServiceAction::Activity a)
{
    if (a == QMailServiceAction::Failed) {
        clearSubActions();
    }
    if (a == QMailServiceAction::Successful) {
        if (_pendingActions.count()) {
            disconnectSubAction(_pendingActions.first().action);
            // Don't delete QObject while it's emitting a signal
            _pendingActions.first().action->deleteLater();
            _pendingActions.removeFirst();
        }
        if (_pendingActions.count()) {
            // Composite action does not finish until all sub actions finish
            _activityChanged = false;
            executeNextSubAction();
            return;
        }
    }
    activityChanged(_action, a);
}

void QMailServiceActionPrivate::subActionStatusChanged(const QMailServiceAction::Status &s)
{
    statusChanged(_action, s);
}

void QMailServiceActionPrivate::subActionProgressChanged(uint value, uint total)
{
    progressChanged(_action, value, total);
}

void QMailServiceActionPrivate::connectSubAction(QMailServiceAction *subAction)
{
    connect(subAction, &QMailServiceAction::connectivityChanged,
            this, &QMailServiceActionPrivate::subActionConnectivityChanged);
    connect(subAction, &QMailServiceAction::activityChanged,
            this, &QMailServiceActionPrivate::subActionActivityChanged);
    connect(subAction, &QMailServiceAction::statusChanged,
            this, &QMailServiceActionPrivate::subActionStatusChanged);
    connect(subAction, &QMailServiceAction::progressChanged,
            this, &QMailServiceActionPrivate::subActionProgressChanged);
}

void QMailServiceActionPrivate::disconnectSubAction(QMailServiceAction *subAction)
{
    disconnect(subAction, &QMailServiceAction::connectivityChanged,
               this, &QMailServiceActionPrivate::subActionConnectivityChanged);
    disconnect(subAction, &QMailServiceAction::activityChanged,
               this, &QMailServiceActionPrivate::subActionActivityChanged);
    disconnect(subAction, &QMailServiceAction::statusChanged,
               this, &QMailServiceActionPrivate::subActionStatusChanged);
    disconnect(subAction, &QMailServiceAction::progressChanged,
               this, &QMailServiceActionPrivate::subActionProgressChanged);
}

void QMailServiceActionPrivate::clearSubActions()
{
    if (_pendingActions.count())
        disconnectSubAction(_pendingActions.first().action);
    foreach (ActionCommand a, _pendingActions) {
        // Don't delete QObject while it's emitting a signal
        a.action->deleteLater();
    }
    _pendingActions.clear();
}

void QMailServiceActionPrivate::init()
{
    _connectivity = QMailServiceAction::Offline;
    _activity = QMailServiceAction::Pending;
    _status = QMailServiceAction::Status(QMailServiceAction::Status::ErrNoError, QString(), QMailAccountId(),
                                         QMailFolderId(), QMailMessageId());
    _total = 0;
    _progress = 0;
    _action = 0;
    _connectivityChanged = false;
    _activityChanged = false;
    _progressChanged = false;
    _statusChanged = false;
    _isValid = false;
    _pendingActions.clear();
}

quint64 QMailServiceActionPrivate::newAction()
{
    if (_isValid) {
        qCWarning(lcMessaging) << "Unable to allocate new action - oustanding:" << messageActionParts(_action).second;
        return _action;
    }

    init();

    _action = nextMessageAction();
    _isValid = true;
    _activityChanged = true;
    emitChanges();

    return _action;
}

bool QMailServiceActionPrivate::validAction(quint64 action)
{
    return (action && _action == action);
}

void QMailServiceActionPrivate::appendSubAction(QMailServiceAction *subAction,
                                                QSharedPointer<QMailServiceActionCommand> command)
{
    ActionCommand a;
    a.action = subAction;
    a.command = command;
    _pendingActions.append(a);
}

void QMailServiceActionPrivate::executeNextSubAction()
{
    if (_pendingActions.isEmpty())
        return;

    connectSubAction(_pendingActions.first().action);
    _pendingActions.first().command->execute();
}

void QMailServiceActionPrivate::setConnectivity(QMailServiceAction::Connectivity newConnectivity)
{
    if (_isValid && (_connectivity != newConnectivity)) {
        _connectivity = newConnectivity;
        _connectivityChanged = true;
    }
}

void QMailServiceActionPrivate::setActivity(QMailServiceAction::Activity newActivity)
{
    if (_isValid && (_activity != newActivity)) {
        _activity = newActivity;

        if (_activity == QMailServiceAction::Failed || _activity == QMailServiceAction::Successful) {
            // We're finished
            _isValid = false;
        }

        _activityChanged = true;
    }
}

void QMailServiceActionPrivate::setStatus(const QMailServiceAction::Status &status)
{
    if (_isValid) {
        _status = status;
        _statusChanged = true;
    }
}

void QMailServiceActionPrivate::setStatus(QMailServiceAction::Status::ErrorCode code, const QString &text)
{
    setStatus(code, text, QMailAccountId(), QMailFolderId(), QMailMessageId());
}

void QMailServiceActionPrivate::setStatus(QMailServiceAction::Status::ErrorCode code,
                                          const QString &text,
                                          const QMailAccountId &accountId,
                                          const QMailFolderId &folderId,
                                          const QMailMessageId &messageId)
{
    if (_isValid) {
        _status = QMailServiceAction::Status(code, text, accountId, folderId, messageId);
        _statusChanged = true;
    }
}

void QMailServiceActionPrivate::setProgress(uint newProgress, uint newTotal)
{
    if (_isValid) {
        if (newTotal != _total) {
            _total = newTotal;
            _progressChanged = true;
        }

        newProgress = qMin(newProgress, _total);
        if (newProgress != _progress) {
            _progress = newProgress;
            _progressChanged = true;
        }
    }
}

void QMailServiceActionPrivate::emitChanges()
{
    if (_connectivityChanged) {
        _connectivityChanged = false;
        emit _interface->connectivityChanged(_connectivity);
    }
    if (_activityChanged) {
        _activityChanged = false;
        emit _interface->activityChanged(_activity);
    }
    if (_progressChanged) {
        _progressChanged = false;
        emit _interface->progressChanged(_progress, _total);
    }
    if (_statusChanged) {
        _statusChanged = false;
        emit _interface->statusChanged(_status);
    }
}


/*!
    \class QMailServiceAction::Status

    \preliminary
    \ingroup messaginglibrary

    \brief The Status class encapsulates the instantaneous state of a QMailServiceAction.

    QMailServiceAction::Status contains the pieces of information that describe the state of a
    requested action.  The \l errorCode reflects the overall state, and may be supplemented
    by a description in \l text.

    If \l errorCode is not equal to \l ErrNoError, then each of \l accountId, \l folderId and
    \l messageId may have been set to a valid identifier, if pertinent to the situation.
*/

/*!
    \enum QMailServiceAction::Status::ErrorCode

    This enum type is used to identify common error conditions encountered when implementing service actions.

    \value ErrNoError               No error was encountered.
    \value ErrNotImplemented        The requested operation is not implemented by the relevant service component.
    \value ErrFrameworkFault        A fault in the messaging framework prevented the execution of the request.
    \value ErrSystemError           A system-level error prevented the execution of the request.
    \value ErrInternalServer        A system error on server.
    \value ErrCancel                The operation was cancelled by user intervention.
    \value ErrConfiguration         The configuration needed for the requested action is invalid.
    \value ErrNoConnection          A connection could not be established to the external service.
    \value ErrConnectionInUse       The connection to the external service is exclusively held by another user.
    \value ErrConnectionNotReady    The connection to the external service is not ready for operation.
    \value ErrUnknownResponse       The response from the external service could not be handled.
    \value ErrLoginFailed           The external service did not accept the supplied authentication details.
    \value ErrFileSystemFull        The action could not be performed due to insufficient storage space.
    \value ErrNonexistentMessage    The specified message does not exist.
    \value ErrEnqueueFailed         The specified message could not be enqueued for transmission.
    \value ErrInvalidAddress        The specified address is invalid for the requested action.
    \value ErrInvalidData           The supplied data is inappropriate for the requested action.
    \value ErrTimeout               The service failed to report any activity for an extended period.
    \value ErrInternalStateReset    The service was reset due to state change, such as an external process modifying the owner account.
    \value ErrorCodeMinimum         The lowest value of any error condition code.
    \value ErrorCodeMaximum         The highest value of any error condition code.
*/

/*! \variable QMailServiceAction::Status::errorCode

    Describes the error condition encountered by the action.
*/

/*! \variable QMailServiceAction::Status::text

    Provides a human-readable description of the error condition in \l errorCode.
*/

/*! \variable QMailServiceAction::Status::accountId

    If relevant to the \l errorCode, contains the ID of the associated account.
*/

/*! \variable QMailServiceAction::Status::folderId

    If relevant to the \l errorCode, contains the ID of the associated folder.
*/

/*! \variable QMailServiceAction::Status::messageId

    If relevant to the \l errorCode, contains the ID of the associated message.
*/

/*!
    \fn QMailServiceAction::Status::Status()

    Constructs a status object with \l errorCode set to \l{QMailServiceAction::Status::ErrNoError}{ErrNoError}.
*/
QMailServiceAction::Status::Status()
    : errorCode(ErrNoError)
{
}

/*!
    \fn QMailServiceAction::Status::Status(ErrorCode c, const QString &t, const QMailAccountId &a, const QMailFolderId &f, const QMailMessageId &m)

    Constructs a status object with
    \l errorCode set to \a c,
    \l text set to \a t,
    \l accountId set to \a a,
    \l folderId set to \a f and
    \l messageId set to \a m.
*/
QMailServiceAction::Status::Status(ErrorCode c, const QString &t, const QMailAccountId &a,
                                   const QMailFolderId &f, const QMailMessageId &m)
    : errorCode(c),
      text(t),
      accountId(a),
      folderId(f),
      messageId(m)
{
}

/*!
    \fn QMailServiceAction::Status::Status(const Status&)

    Constructs a status object which is a copy of \a other.
*/
QMailServiceAction::Status::Status(const QMailServiceAction::Status &other)
    : errorCode(other.errorCode),
      text(other.text),
      accountId(other.accountId),
      folderId(other.folderId),
      messageId(other.messageId)
{
}

QMailServiceAction::Status& QMailServiceAction::Status::operator=(const QMailServiceAction::Status &other)
{
    errorCode = other.errorCode;
    text = other.text;
    accountId = other.accountId;
    folderId = other.folderId;
    messageId = other.messageId;
    return *this;
}

/*!
    \fn QMailServiceAction::Status::serialize(Stream&) const
    \internal
*/
template <typename Stream>
void QMailServiceAction::Status::serialize(Stream &stream) const
{
    stream << errorCode;
    stream << text;
    stream << accountId;
    stream << folderId;
    stream << messageId;
}

template void QMailServiceAction::Status::serialize(QDataStream &) const;

/*!
    \fn QMailServiceAction::Status::deserialize(Stream&)
    \internal
*/
template <typename Stream>
void QMailServiceAction::Status::deserialize(Stream &stream)
{
    stream >> errorCode;
    stream >> text;
    stream >> accountId;
    stream >> folderId;
    stream >> messageId;
}

template void QMailServiceAction::Status::deserialize(QDataStream &);

/*!
    \class QMailServiceAction

    \preliminary
    \ingroup messaginglibrary

    \brief The QMailServiceAction class provides the interface for requesting actions from external message services.

    QMailServiceAction provides the mechanisms for messaging clients to request actions from
    external services, and to receive information in response.  The details of requests
    differ for each derived action, and the specific data returned is also variable.  However,
    all actions present the same interface for communicating status, connectivity and
    progress information.

    All actions communicate changes in their operational state by emitting the activityChanged()
    signal.  If the execution of the action requires connectivity to an external service, then
    changes in connectivity state are emitted via the connectivityChanged() signal.  Some actions
    are able to provide progress information as they execute; these changes are reported via the
    progressChanged() signal.  If error conditions are encountered, the statusChanged() signal is
    emitted to report the new status.

    A user may attempt to cancel an operation after it has been initiated.  The cancelOperation()
    slot is provided for this purpose.

    A QMailServiceAction instance supports only a single request at any time. Attempting
    to initiate an operation on a QMailServiceAction instace while another operation is already
    in progress for that action is not supported.

    An application may however use multiple QMailServiceAction instances to send independent
    requests concurrently. Each QMailServiceAction instance will report only the changes
    pertaining to the request that instance delivered.  Whether or not concurrent requests are
    concurrently serviced by the messageserver depends on whether those requests must be
    serviced by the same QMailMessageService instance.

    \sa QMailMessageService
*/

/*!
    \enum QMailServiceAction::Connectivity

    This enum type is used to describe the connectivity state of the service implementing an action.

    \value Offline          The service is offline.
    \value Connecting       The service is currently attempting to establish a connection.
    \value Connected        The service is connected.
    \value Disconnected     The service has been disconnected.
*/

/*!
    \enum QMailServiceAction::Activity

    This enum type is used to describe the activity state of the requested action.

    \value Pending          The action has not yet begun execution.
    \value InProgress       The action is currently executing.
    \value Successful       The action has completed successfully.
    \value Failed           The action could not be completed successfully, and has finished execution.
*/

/*!
    \fn QMailServiceAction::connectivityChanged(QMailServiceAction::Connectivity c)

    This signal is emitted when the connectivity status of the service performing
    the action changes, with the new state described by \a c.

    \sa connectivity()
*/

/*!
    \fn QMailServiceAction::activityChanged(QMailServiceAction::Activity a)

    This signal is emitted when the activity status of the action changes,
    with the new state described by \a a.

    \sa activity()
*/

/*!
    \fn QMailServiceAction::statusChanged(const QMailServiceAction::Status &s)

    This signal is emitted when the error status of the action changes, with
    the new status described by \a s.

    \sa status()
*/

/*!
    \fn QMailServiceAction::progressChanged(uint value, uint total)

    This signal is emitted when the progress of the action changes, with
    the new state described by \a value and \a total.

    \sa progress()
*/

/*!
    \fn QMailServiceAction::QMailServiceAction(QMailServiceActionPrivate&, QObject*)
    \internal
*/
QMailServiceAction::QMailServiceAction(QMailServiceActionPrivate &dd, QObject *parent)
    : QObject(parent)
    , d_ptr(&dd)
{
}

/*! \internal */
QMailServiceAction::~QMailServiceAction()
{
}

/*!
    Returns the current connectivity state of the service implementing this action.

    \sa connectivityChanged()
*/
QMailServiceAction::Connectivity QMailServiceAction::connectivity() const
{
    Q_D(const QMailServiceAction);
    return d->_connectivity;
}

/*!
    Returns the current activity state of the action.

    \sa activityChanged()
*/
QMailServiceAction::Activity QMailServiceAction::activity() const
{
    Q_D(const QMailServiceAction);
    return d->_activity;
}

/*!
    Returns the current status of the service action.

    \sa statusChanged()
*/
QMailServiceAction::Status QMailServiceAction::status() const
{
    Q_D(const QMailServiceAction);
    return d->_status;
}

/*!
    Returns the current progress of the service action.

    \sa progressChanged()
*/
QPair<uint, uint> QMailServiceAction::progress() const
{
    Q_D(const QMailServiceAction);
    return qMakePair(d->_progress, d->_total);
}

/*!
    Returns if the service action is currently running (i.e. if it's doing something, but hasn't yet finished)
*/

bool QMailServiceAction::isRunning() const
{
    Q_D(const QMailServiceAction);
    return d->_isValid;
}

/*!
    Attempts to cancel the last requested operation.
*/
void QMailServiceAction::cancelOperation()
{
    Q_D(QMailServiceAction);
    d->cancelOperation();
}

/*!
    Set the current status so that
    \l{QMailServiceAction::Status::errorCode} errorCode is set to \a c and
    \l{QMailServiceAction::Status::text} text is set to \a t.
*/
void QMailServiceAction::setStatus(QMailServiceAction::Status::ErrorCode c, const QString &t)
{
    Q_D(QMailServiceAction);
    d->setStatus(c, t, QMailAccountId(), QMailFolderId(), QMailMessageId());
}

/*!
    Set the current status so that
    \l{QMailServiceAction::Status::errorCode} errorCode is set to \a c,
    \l{QMailServiceAction::Status::text} text is set to \a t,
    \l{QMailServiceAction::Status::accountId} accountId is set to \a a,
    \l{QMailServiceAction::Status::folderId} folderId is set to \a f and
    \l{QMailServiceAction::Status::messageId} messageId is set to \a m.
*/
void QMailServiceAction::setStatus(QMailServiceAction::Status::ErrorCode c, const QString &t,
                                   const QMailAccountId &a, const QMailFolderId &f, const QMailMessageId &m)
{
    Q_D(QMailServiceAction);
    d->setStatus(c, t, a, f, m);
}


QMailRetrievalActionPrivate::QMailRetrievalActionPrivate(QMailRetrievalAction *i)
    : QMailServiceActionPrivate(i)
{
    connect(_server.data(), &QMailMessageServer::retrievalCompleted,
            this, &QMailRetrievalActionPrivate::retrievalCompleted);

    init();
}

void QMailRetrievalActionPrivate::retrieveFolderListHelper(const QMailAccountId &accountId,
                                                           const QMailFolderId &folderId, bool descending)
{
    _server->retrieveFolderList(newAction(), accountId, folderId, descending);
}

void QMailRetrievalActionPrivate::retrieveFolderList(const QMailAccountId &accountId, const QMailFolderId &folderId,
                                                     bool descending)
{
    Q_ASSERT(!_pendingActions.count());
    retrieveFolderListHelper(accountId, folderId, descending);
}

void QMailRetrievalActionPrivate::retrieveMessageListHelper(const QMailAccountId &accountId,
                                                            const QMailFolderId &folderId, uint minimum,
                                                            const QMailMessageSortKey &sort)
{
    _server->retrieveMessageList(newAction(), accountId, folderId, minimum, sort);
}

void QMailRetrievalActionPrivate::retrieveMessageList(const QMailAccountId &accountId, const QMailFolderId &folderId,
                                                      uint minimum, const QMailMessageSortKey &sort)
{
    Q_ASSERT(!_pendingActions.count());
    retrieveMessageListHelper(accountId, folderId, minimum, sort);
}

void QMailRetrievalActionPrivate::retrieveMessageLists(const QMailAccountId &accountId,
                                                       const QMailFolderIdList &folderIds,
                                                       uint minimum, const QMailMessageSortKey &sort)
{
    Q_ASSERT(!_pendingActions.count());
    _server->retrieveMessageLists(newAction(), accountId, folderIds, minimum, sort);
}

void QMailRetrievalActionPrivate::retrieveNewMessages(const QMailAccountId &accountId, const QMailFolderIdList &folderIds)
{
    Q_ASSERT(!_pendingActions.count());
    _server->retrieveNewMessages(newAction(), accountId, folderIds);
}

void QMailRetrievalActionPrivate::createStandardFolders(const QMailAccountId &accountId)
{
    Q_ASSERT(!_pendingActions.count());

    QMailAccount account(accountId);

    if (!QMail::detectStandardFolders(accountId)) {
        if (!(account.status() & QMailAccount::CanCreateFolders)) {
            qCWarning(lcMessaging) << "Unable to create folders for account: " << accountId;
            if (validAction(newAction())) {
                setActivity(QMailServiceAction::Successful);
                emitChanges();
            }
            return;
        } else {
            _server->createStandardFolders(newAction(), accountId);
        }
    } else {
        qCDebug(lcMessaging) << "Standard folders matched for account: " << accountId;
        if (validAction(newAction())) {
            setActivity(QMailServiceAction::Successful);
            emitChanges();
        }
    }
}

void QMailRetrievalActionPrivate::retrieveMessages(const QMailMessageIdList &messageIds, QMailRetrievalAction::RetrievalSpecification spec)
{
    _server->retrieveMessages(newAction(), messageIds, spec);
}

void QMailRetrievalActionPrivate::retrieveMessagePart(const QMailMessagePart::Location &partLocation)
{
    _server->retrieveMessagePart(newAction(), partLocation);
}

void QMailRetrievalActionPrivate::retrieveMessageRange(const QMailMessageId &messageId, uint minimum)
{
    _server->retrieveMessageRange(newAction(), messageId, minimum);
}

void QMailRetrievalActionPrivate::retrieveMessagePartRange(const QMailMessagePart::Location &partLocation, uint minimum)
{
    _server->retrieveMessagePartRange(newAction(), partLocation, minimum);
}

void QMailRetrievalActionPrivate::exportUpdatesHelper(const QMailAccountId &accountId)
{
    _server->exportUpdates(newAction(), accountId);
}

void QMailRetrievalActionPrivate::exportUpdates(const QMailAccountId &accountId)
{
    Q_ASSERT(!_pendingActions.count());
    exportUpdatesHelper(accountId);
}

void QMailRetrievalActionPrivate::synchronize(const QMailAccountId &accountId, uint minimum)
{
    Q_ASSERT(!_pendingActions.count());
    newAction();

    // TODO: consider if we should just (re)introduce a synchronize IPC method and let server deal with sub-actions
    QMailRetrievalAction *exportAction = new QMailRetrievalAction();
    QMailExportUpdatesCommand *exportCommand = new QMailExportUpdatesCommand(exportAction->d_func(), accountId);
    appendSubAction(exportAction, QSharedPointer<QMailServiceActionCommand>(exportCommand));

    QMailRetrievalAction *retrieveFolderListAction = new QMailRetrievalAction();
    QMailRetrieveFolderListCommand *retrieveFolderListCommand
            = new QMailRetrieveFolderListCommand(retrieveFolderListAction->d_func(), accountId);
    appendSubAction(retrieveFolderListAction, QSharedPointer<QMailServiceActionCommand>(retrieveFolderListCommand));

    QMailRetrievalAction *retrieveMessageListAction = new QMailRetrievalAction();
    QMailRetrieveMessageListCommand *retrieveMessageListCommand
            = new QMailRetrieveMessageListCommand(retrieveMessageListAction->d_func(), accountId, minimum);
    appendSubAction(retrieveMessageListAction, QSharedPointer<QMailServiceActionCommand>(retrieveMessageListCommand));
    executeNextSubAction();
}

void QMailRetrievalActionPrivate::retrievalCompleted(quint64 action)
{
    if (validAction(action)) {
        setActivity(QMailServiceAction::Successful);
        emitChanges();
    }
}

/*!
    \class QMailRetrievalAction

    \preliminary
    \ingroup messaginglibrary

    \brief The QMailRetrievalAction class provides the interface for retrieving messages from external message services.

    QMailRetrievalAction provides the mechanism for messaging clients to request that the message
    server retrieve messages from external services.  The retrieval action object reports on
    the progress and outcome of its activities via the signals inherited from QMailServiceAction.

    A range of functions are available to support varying client operations:

    The retrieveFolderList() function allows a client to retrieve the list of folders available for an account.
    The retrieveMessageList() and retrieveMessageLists() functions allows a client to retrieve a subset of messages available for an account or folder.

    The retrieveMessages() function allows a client to retrieve the flags, meta data or content of a
    specific list of messages.

    The retrieveMessageRange() functions allows a portion of a message's content to be retrieved, rather than
    the entire content data.

    The retrieveMessagePart() function allows a specific part of a multi-part message to be retrieved.
    The retrieveMessagePartRange() function allows a portion of a specific part of a multi-part message to be retrieved.

    The exportUpdates() function allows a client to push local changes such as message-read notifications
    and pending disconnected operations to the external server.

    All of these functions require the device to be online, they may initiate communication
    with external servers.

    A QMailRetrievalAction instance supports only a single request at any time. Attempting
    to initiate an operation on a QMailRetrievalAction instace while another operation is already
    in progress for that action is not supported.

    An application may however use multiple QMailRetrievalAction instances to send independent
    requests concurrently. Each QMailServiceAction instance will report only the changes
    pertaining to the request that instance delivered.  Whether or not concurrent requests are
    concurrently serviced by the messageserver depends on whether those requests must be
    serviced by the same QMailMessageService instance.
*/

/*!
    \enum QMailRetrievalAction::RetrievalSpecification

    This enum type is specify the form of retrieval that the message server should perform.

    \value Flags        Changes to the state of the message should be retrieved.
    \value MetaData     Only the meta data of the message should be retrieved.
    \value Content      The entire content of the message should be retrieved.
    \value Auto         Account specific settings are used during synchronization.
                        For example, this may mean skipping the attachments.
*/

/*!
    Constructs a new retrieval action object with the supplied \a parent.
*/
QMailRetrievalAction::QMailRetrievalAction(QObject *parent)
    : QMailServiceAction(*(new QMailRetrievalActionPrivate(this)), parent)
{
}

/*! \internal */
QMailRetrievalAction::~QMailRetrievalAction()
{
}

/*!
    Requests that the message server retrieve the list of folders available for the
    account \a accountId.  If \a folderId is valid, only the identified folder is
    searched for child folders; otherwise the search begins at the root of the
    account.  If \a descending is true, the search should also recursively search
    for child folders within folders discovered during the search.

    The QMailFolder::serverCount(), QMailFolder::serverUnreadCount() and
    QMailFolder::serverUndiscoveredCount() properties will be updated for each
    folder that is searched for child folders; these properties are not updated
    for folders that are merely discovered by searching.

    This function requires the device to be online, it may initiate communication
    with external servers.

    \sa retrieveMessageList(), retrieveMessageLists()
*/
void QMailRetrievalAction::retrieveFolderList(const QMailAccountId &accountId, const QMailFolderId &folderId,
                                              bool descending)
{
    Q_D(QMailRetrievalAction);
    d->retrieveFolderList(accountId, folderId, descending);
}

/*!
    Requests that the message server retrieve the list of messages available for the account \a accountId.
    If \a folderId is valid, then only messages within that folder should be retrieved; otherwise
    messages within all folders in the account should be retrieved, and the lastSynchronized() time
    of the account updated.  If \a minimum is non-zero, then that value will be used to restrict the
    number of messages to be retrieved from each folder.

    If \a sort is not empty, the external service will report the discovered messages in the
    ordering indicated by the sort criterion, if possible.  Services are not required to support
    this facility.

    If a folder messages are being retrieved from contains at least \a minimum messages then the
    messageserver should ensure that at least \a minimum messages are available from the mail
    store for that folder; otherwise if the folder contains less than \a minimum messages the
    messageserver should ensure all the messages for that folder are available from the mail store.
    If a folder has messages locally available, then all previously undiscovered messages will be
    retrieved for that folder, even if that number exceeds \a minimum.

    The QMailFolder::serverCount(), QMailFolder::serverUnreadCount() and
    QMailFolder::serverUndiscoveredCount() properties will be updated for each folder
    from which messages are retrieved.

    New messages will be added to the mail store as they are discovered, and
    marked with the \l QMailMessage::New status flag. Messages in folders inspected that
    are present in the mail store but found to be no longer available are marked with the
    \l QMailMessage::Removed status flag. The status flags of messages in folders inspected
    that are present in the mail store will be updated including the QMailMessage::Read and
    QMailMessage::Important flags.

    This function requires the device to be online, it may initiate communication
    with external servers.

    \sa QMailAccount::lastSynchronized()
*/
void QMailRetrievalAction::retrieveMessageList(const QMailAccountId &accountId, const QMailFolderId &folderId,
                                               uint minimum, const QMailMessageSortKey &sort)
{
    Q_D(QMailRetrievalAction);
    d->retrieveMessageList(accountId, folderId, minimum, sort);
}

/*!
    Requests that the message server retrieve the list of messages available for the account \a accountId.
    If \a folderIds is not empty, then only messages within those folders should be retrieved and the
    lastSynchronized() time of the account updated; otherwise no messages should be retrieved.
    If \a minimum is non-zero, then that value will be used to restrict the
    number of messages to be retrieved from each folder.

    If \a sort is not empty, the external service will report the discovered messages in the
    ordering indicated by the sort criterion, if possible.  Services are not required to support
    this facility.

    If a folder messages are being retrieved from contains at least \a minimum messages then the
    messageserver should ensure that at least \a minimum messages are available from the mail
    store for that folder; otherwise if the folder contains less than \a minimum messages the
    messageserver should ensure all the messages for that folder are available from the mail store.
    If a folder has messages locally available, then all previously undiscovered messages will be
    retrieved for that folder, even if that number exceeds \a minimum.

    The QMailFolder::serverCount(), QMailFolder::serverUnreadCount() and
    QMailFolder::serverUndiscoveredCount() properties will be updated for each folder
    from which messages are retrieved.

    New messages will be added to the mail store as they are discovered, and
    marked with the \l QMailMessage::New status flag. Messages in folders inspected that
    are present in the mail store but found to be no longer available are marked with the
    \l QMailMessage::Removed status flag. The status flags of messages in folders inspected
    that are present in the mail store will be updated including the QMailMessage::Read and
    QMailMessage::Important flags.

    This function requires the device to be online, it may initiate communication
    with external servers.

    \sa QMailAccount::lastSynchronized()
*/
void QMailRetrievalAction::retrieveMessageLists(const QMailAccountId &accountId, const QMailFolderIdList &folderIds,
                                                uint minimum, const QMailMessageSortKey &sort)
{
    Q_D(QMailRetrievalAction);

    if (folderIds.isEmpty()) {
        // nothing to do
        d->newAction();
        d->setActivity(QMailServiceAction::Successful);
        d->emitChanges();
    } else {
        d->retrieveMessageLists(accountId, folderIds, minimum, sort);
    }
}

/*
    Requests that the message server retrieve new messages for the account \a accountId in the
    folders specified by \a folderIds.

    If a folder inspected has been previously inspected then new mails in that folder will
    be retrieved, otherwise the most recent message in that folder, if any, will be retrieved.

    This function is intended for use by protocol plugins to retrieve new messages when
    a push notification event is received from the remote server.

    Detection of deleted messages, and flag updates for messages in the mail store will
    not necessarily be performed.
*/
void QMailRetrievalAction::retrieveNewMessages(const QMailAccountId &accountId, const QMailFolderIdList &folderIds)
{
    Q_D(QMailRetrievalAction);
    d->retrieveNewMessages(accountId, folderIds);
}

/*!
    Requests that the message server create the standard folders for the
    account \a accountId. If all standard folders are already set in the storage
    the service action will return success immediately, in case some standard folders are
    not set, a matching attempt against a predefined list of translations will be made,
    if the folders can't be matched, messageserver will try to create them in the server side
    and match them if the creation is successful. In case folder creation is not allowed for
    the account \a accountId the service action will still complete successfully, clients
    must use local standard folders only in this case.

    \sa retrieveFolderList
*/
void QMailRetrievalAction::createStandardFolders(const QMailAccountId &accountId)
{
    Q_D(QMailRetrievalAction);
    d->createStandardFolders(accountId);
}

/*!
    Requests that the message server retrieve data regarding the messages identified by \a messageIds.

    If \a spec is \l QMailRetrievalAction::Flags, then the message server should detect if
    the messages identified by \a messageIds have been marked as read or have been removed.
    Messages that have been read will be marked with the \l QMailMessage::ReadElsewhere flag, and
    messages that have been removed will be marked with the \l QMailMessage::Removed status flag.

    If \a spec is \l QMailRetrievalAction::MetaData, then the message server should
    retrieve the meta data of the each message listed in \a messageIds.

    If \a spec is \l QMailRetrievalAction::Content, then the message server should
    retrieve the entirety of each message listed in \a messageIds.

    The QMailFolder::serverCount(), QMailFolder::serverUnreadCount() and
    QMailFolder::serverUndiscoveredCount() properties will be updated for each folder
    from which messages are retrieved.

    This function requires the device to be online, it may initiate communication
    with external servers.
*/
void QMailRetrievalAction::retrieveMessages(const QMailMessageIdList &messageIds, RetrievalSpecification spec)
{
    Q_D(QMailRetrievalAction);
    d->retrieveMessages(messageIds, spec);
}

/*!
    Requests that the message server retrieve the message part that is indicated by the
    location \a partLocation.

    The QMailFolder::serverCount(), QMailFolder::serverUnreadCount() and
    QMailFolder::serverUndiscoveredCount() properties will be updated for the folder
    from which the part is retrieved.

    This function requires the device to be online, it may initiate communication
    with external servers.
*/
void QMailRetrievalAction::retrieveMessagePart(const QMailMessagePart::Location &partLocation)
{
    Q_D(QMailRetrievalAction);
    d->retrieveMessagePart(partLocation);
}

/*!
    Requests that the message server retrieve a subset of the message \a messageId, such that
    at least \a minimum bytes are available from the mail store.

    The QMailFolder::serverCount(), QMailFolder::serverUnreadCount() and
    QMailFolder::serverUndiscoveredCount() properties will be updated for the folder
    from which the message is retrieved.

    This function requires the device to be online, it may initiate communication
    with external servers.
*/
void QMailRetrievalAction::retrieveMessageRange(const QMailMessageId &messageId, uint minimum)
{
    Q_D(QMailRetrievalAction);
    d->retrieveMessageRange(messageId, minimum);
}

/*!
    Requests that the message server retrieve a subset of the message part that is indicated
    by the location \a partLocation.  The messageserver should ensure that at least
    \a minimum bytes are available from the mail store.

    The total size of the part on the server is given by QMailMessagePart::contentDisposition().size(),
    the amount of the part available locally is given by QMailMessagePart::body().length().

    The QMailFolder::serverCount(), QMailFolder::serverUnreadCount() and
    QMailFolder::serverUndiscoveredCount() properties will be updated for the folder
    from which the part is retrieved.

    This function requires the device to be online, it may initiate communication
    with external servers.
*/
void QMailRetrievalAction::retrieveMessagePartRange(const QMailMessagePart::Location &partLocation, uint minimum)
{
    Q_D(QMailRetrievalAction);
    d->retrieveMessagePartRange(partLocation, minimum);
}

/*!
    Requests that the message server update the external server with changes that have
    been effected on the local device for account \a accountId.
    Local changes to \l QMailMessage::Read, and \l QMailMessage::Important message status
    flags should be exported to the external server, messages that have been removed
    using the \l QMailStore::CreateRemovalRecord option should be removed from the
    external server, and any flag, copy or move operations that have been applied
    using \l QMailDisconnected should be applied to the external server.

    This function requires the device to be online, it may initiate communication
    with external servers.
*/
void QMailRetrievalAction::exportUpdates(const QMailAccountId &accountId)
{
    Q_D(QMailRetrievalAction);
    if (!QMailDisconnected::updatesOutstanding(accountId)) {
        // nothing to do
        d->newAction();
        d->setActivity(QMailServiceAction::Successful);
        d->emitChanges();
    } else {
        d->exportUpdates(accountId);
    }
}

/*!
    Essentially performs the same functions as calling exportUpdates(), retrieveFolderList()
    and retrieveMessageList() consecutively using the given \a accountId and \a minimum, but
    may perform optimizations such as retrieving the folder list at the same time as
    retrieving messages, and retrieving messages in multiple folder simultaneously by using
    multiple connections to the server.

    On a successful synchronization the lastSynchronized() value of the account should be updated.

    This function requires the device to be online, it may initiate communication
    with external servers.

    \sa exportUpdates(), retrieveMessageList(), retrieveFolderList(), QMailAccount::lastSynchronized()
*/
void QMailRetrievalAction::synchronize(const QMailAccountId &accountId, uint minimum)
{
    Q_D(QMailRetrievalAction);
    d->synchronize(accountId, minimum);
}

/*!
    \fn QMailRetrievalAction::defaultMinimum()

    Default minimum argument clients and protocol plugins should use when
    retrieving messages. Maybe be used by protocol plugins to determine if
    either new messages are being retrieved or if more messages are being
    retrieved.

    \sa retrieveMessageList()
*/


QMailTransmitActionPrivate::QMailTransmitActionPrivate(QMailTransmitAction *i)
    : QMailServiceActionPrivate(i)
{
    connect(_server.data(), &QMailMessageServer::messagesTransmitted,
            this, &QMailTransmitActionPrivate::handleMessagesTransmitted);
    connect(_server.data(), &QMailMessageServer::messagesFailedTransmission,
            this, &QMailTransmitActionPrivate::handleMessagesFailedTransmission);
    connect(_server.data(), &QMailMessageServer::transmissionCompleted,
            this, &QMailTransmitActionPrivate::handleTransmissionCompleted);

    init();
}

void QMailTransmitActionPrivate::transmitMessages(const QMailAccountId &accountId)
{
    _server->transmitMessages(newAction(), accountId);

    emitChanges();
}

void QMailTransmitActionPrivate::transmitMessage(const QMailMessageId &messageId)
{
    _server->transmitMessage(newAction(), messageId);

    emitChanges();
}

void QMailTransmitActionPrivate::init()
{
    QMailServiceActionPrivate::init();
}

void QMailTransmitActionPrivate::handleMessagesTransmitted(quint64 action, const QMailMessageIdList &ids)
{
    if (validAction(action)) {
        emit messagesTransmitted(ids);
    }
}

void QMailTransmitActionPrivate::handleMessagesFailedTransmission(quint64 action, const QMailMessageIdList &ids,
                                                                  QMailServiceAction::Status::ErrorCode error)
{
    if (validAction(action)) {
        emit messagesFailedTransmission(ids, error);
    }
}

void QMailTransmitActionPrivate::handleTransmissionCompleted(quint64 action)
{
    if (validAction(action)) {
        QMailServiceAction::Activity result(QMailServiceAction::Successful);
        setActivity(result);
        emitChanges();
    }
}


/*!
    \class QMailTransmitAction

    \preliminary
    \ingroup messaginglibrary

    \brief The QMailTransmitAction class provides the interface for transmitting messages to external message services.

    QMailTransmitAction provides the mechanism for messaging clients to request that the message
    server transmit messages to external services.  The transmit action object reports on
    the progress and outcome of its activities via the signals inherited from QMailServiceAction.

    The send() slot requests that the message server transmit each identified message to the
    external service associated with each messages' account.

    A QMailTransmitAction instance supports only a single request at any time. Attempting
    to initiate an operation on a QMailTransmitAction instace while another operation is already
    in progress for that action is not supported.

    An application may however use multiple QMailTransmitAction instances to send independent
    requests concurrently. Each QMailServiceAction instance will report only the changes
    pertaining to the request that instance delivered.  Whether or not concurrent requests are
    concurrently serviced by the messageserver depends on whether those requests must be
    serviced by the same QMailMessageService instance.

    Note: the slots exported by QMailTransmitAction are expected to change in future releases, as
    the message server is extended to provide a finer-grained interface for message transmission.
*/

/*!
    \fn QMailTransmitAction::messagesTransmitted(const QMailMessageIdList &ids)

    This signal is emitted to report the successful transmission of the messages listed in \a ids.
*/

/*!
    \fn QMailTransmitAction::messagesFailedTransmission(const QMailMessageIdList &ids, QMailServiceAction::Status::ErrorCode error)

    This signal is emitted to report the failure of an attempt at transmission of the messages listed in \a ids.

    The failure is of type \a error.
*/


/*!
    Constructs a new transmit action object with the supplied \a parent.
*/
QMailTransmitAction::QMailTransmitAction(QObject *parent)
    : QMailServiceAction(*(new QMailTransmitActionPrivate(this)), parent)
{
    Q_D(QMailTransmitAction);
    connect(d, &QMailTransmitActionPrivate::messagesTransmitted,
            this, &QMailTransmitAction::messagesTransmitted);
    connect(d, &QMailTransmitActionPrivate::messagesFailedTransmission,
            this, &QMailTransmitAction::messagesFailedTransmission);
}

/*! \internal */
QMailTransmitAction::~QMailTransmitAction()
{
}

/*!
    Requests that the message server transmit each message belonging to the account identified
    by \a accountId that is currently scheduled for transmission (located in the Outbox folder).

    Any message successfully transmitted will be moved to the account's Sent folder.

    This function requires the device to be online, it may initiate communication
    with external servers.
*/
void QMailTransmitAction::transmitMessages(const QMailAccountId &accountId)
{
    Q_D(QMailTransmitAction);
    d->transmitMessages(accountId);
}

/*!
    Requests that the message server transmit the message identified by
    by \a messageId that is currently scheduled for transmission (located in the Outbox folder).

    Any message successfully transmitted will be moved to the account's Sent folder.
*/
void QMailTransmitAction::transmitMessage(const QMailMessageId &messageId)
{
    Q_D(QMailTransmitAction);
    d->transmitMessage(messageId);
}

QMailStorageActionPrivate::QMailStorageActionPrivate(QMailStorageAction *i)
    : QMailServiceActionPrivate(i)
{
    connect(_server.data(), &QMailMessageServer::messagesDeleted,
            this, &QMailStorageActionPrivate::messagesEffected);
    connect(_server.data(), &QMailMessageServer::messagesMoved,
            this, &QMailStorageActionPrivate::messagesEffected);
    connect(_server.data(), &QMailMessageServer::messagesCopied,
            this, &QMailStorageActionPrivate::messagesEffected);
    connect(_server.data(), &QMailMessageServer::messagesFlagged,
            this, &QMailStorageActionPrivate::messagesEffected);
    connect(_server.data(), &QMailMessageServer::messagesAdded,
            this, &QMailStorageActionPrivate::messagesAdded);
    connect(_server.data(), &QMailMessageServer::messagesUpdated,
            this, &QMailStorageActionPrivate::messagesUpdated);
    connect(_server.data(), &QMailMessageServer::storageActionCompleted,
            this, &QMailStorageActionPrivate::storageActionCompleted);

    init();
}

void QMailStorageActionPrivate::onlineDeleteMessages(const QMailMessageIdList &ids)
{
    Q_ASSERT(!_pendingActions.count());

    _server->onlineDeleteMessages(newAction(), ids, QMailStore::CreateRemovalRecord);
    // Successful as long as ids have been deleted,
    // this action doesn't have to be the one to have deleted them
    _ids.clear();
    emitChanges();
}

void QMailStorageActionPrivate::discardMessages(const QMailMessageIdList &ids)
{
    _server->onlineDeleteMessages(newAction(), ids, QMailStore::NoRemovalRecord);
    _ids = ids;
    emitChanges();
}

void QMailStorageActionPrivate::onlineCopyMessages(const QMailMessageIdList &ids, const QMailFolderId &destination)
{
    _server->onlineCopyMessages(newAction(), ids, destination);
    _ids = ids;
    emitChanges();
}

void QMailStorageActionPrivate::onlineMoveMessages(const QMailMessageIdList &ids, const QMailFolderId &destination)
{
    _server->onlineMoveMessages(newAction(), ids, destination);
    _ids = ids;
    emitChanges();
}

void QMailStorageActionPrivate::onlineFlagMessagesAndMoveToStandardFolder(const QMailMessageIdList &ids, quint64 setMask, quint64 unsetMask)
{
    // Ensure that nothing is both set and unset
    setMask &= ~unsetMask;
    _server->onlineFlagMessagesAndMoveToStandardFolder(newAction(), ids, setMask, unsetMask);

    _ids = ids;
    emitChanges();
}

void QMailStorageActionPrivate::addMessages(const QMailMessageList &list)
{
    _ids.clear();
    _addedOrUpdatedIds.clear();

    // Check to see if any of the messages has unresolved parts (forward without download)
    // If so must use sync adding
    bool fwod(false);
    for (const QMailMessage &message : list) {
        if (message.status() & (QMailMessage::HasUnresolvedReferences | QMailMessage::TransmitFromExternal)) {
            fwod = true;
            break;
        }
    }
    if (fwod) {
        for (QMailMessage mail : list) {
            if (!mail.id().isValid()) {
                mail.setStatus(QMailMessage::LocalOnly, true);
                QMailStore::instance()->addMessage(&mail);
            } else {
                QMailStore::instance()->updateMessage(&mail);
            }
        }

        if (validAction(newAction())) {
            setActivity(QMailServiceAction::Successful);
            emitChanges();
        }
        return;
    }

    // Make non durable changes to the content manager
    // Existing message data in mail store and content manager should not be
    // changed directly by this function, instead the messageserver should do it.
    QMailMessageMetaDataList metadata;
    for (QMailMessage message : list) {
        if (message.contentScheme().isEmpty()) {
            message.setContentScheme(QMailContentManagerFactory::defaultScheme());
        }
        if (QMailContentManager *contentManager = QMailContentManagerFactory::create(message.contentScheme())) {
            QMailStore::ErrorCode code = contentManager->add(&message, QMailContentManager::NoDurability);
            if (code != QMailStore::NoError) {
                qCWarning(lcMessaging) << "Unable to ensure message content durability for scheme:" << message.contentScheme();
                if (validAction(newAction())) {
                    setActivity(QMailServiceAction::Failed);
                    emitChanges();
                }
                return;
            }
            metadata.append(message);
        }
    }
    _server->addMessages(newAction(), metadata);
    emitChanges();
}

void QMailStorageActionPrivate::updateMessages(const QMailMessageList &list)
{
    _ids.clear();
    _addedOrUpdatedIds.clear();

    // Check to see if any of the messages has unresolved parts (forward without download)
    // If so must use sync updating
    bool fwod(false);
    for (QMailMessage message : list) {
        if (message.status() & (QMailMessage::HasUnresolvedReferences
                                | QMailMessage::TransmitFromExternal
                                | QMailMessage::Outgoing)) {
            fwod = true;
            break;
        }
    }
    if (fwod) {
        for (QMailMessage mail : list) {
            if (!mail.id().isValid()) {
                mail.setStatus(QMailMessage::LocalOnly, true);
                QMailStore::instance()->addMessage(&mail);
            } else {
                QMailStore::instance()->updateMessage(&mail);
            }
        }

        if (validAction(newAction())) {
            setActivity(QMailServiceAction::Successful);
            emitChanges();
        }
        return;
    }

    // Ensure that message non metadata is written and flushed to the filesystem,
    // but not synced
    // Existing message data in mail store and content manager should not be
    // changed directly by this function, instead the messageserver should do it.
    QMailMessageMetaDataList metadata;
    for (QMailMessage message : list) {
        if (message.contentScheme().isEmpty()) {
            message.setContentScheme(QMailContentManagerFactory::defaultScheme());
        }
        message.setCustomField(QLatin1String("qmf-obsolete-contentid"), message.contentIdentifier());
        if (QMailContentManager *contentManager = QMailContentManagerFactory::create(message.contentScheme())) {
            QMailStore::ErrorCode code = contentManager->update(&message, QMailContentManager::EnsureDurability);
            if (code != QMailStore::NoError) {
                qCWarning(lcMessaging) << "Unable to ensure message content durability for scheme:" << message.contentScheme();
                if (validAction(newAction())) {
                    setActivity(QMailServiceAction::Failed);
                    emitChanges();
                }
                return;
            }
            metadata.append(message);
        }
    }
    _server->updateMessages(newAction(), metadata);

    emitChanges();
}

void QMailStorageActionPrivate::updateMessages(const QMailMessageMetaDataList &list)
{
    _ids.clear();
    _addedOrUpdatedIds.clear();

    QMailMessageMetaDataList metadata = list;
    _server->updateMessages(newAction(), metadata);

    emitChanges();
}

void QMailStorageActionPrivate::deleteMessages(const QMailMessageIdList &ids)
{
    _server->deleteMessages(newAction(), ids);

    _ids = ids;
    emitChanges();
}

void QMailStorageActionPrivate::rollBackUpdates(const QMailAccountId &mailAccountId)
{
    _server->rollBackUpdates(newAction(), mailAccountId);
    emitChanges();
}

void QMailStorageActionPrivate::moveToStandardFolder(const QMailMessageIdList& ids, quint64 standardFolder)
{
    _server->moveToStandardFolder(newAction(), ids, standardFolder);

    _ids = ids;
    emitChanges();
}

void QMailStorageActionPrivate::moveToFolder(const QMailMessageIdList& ids, const QMailFolderId& folderId)
{
    _server->moveToFolder(newAction(), ids, folderId);

    _ids = ids;
    emitChanges();
}

void QMailStorageActionPrivate::flagMessages(const QMailMessageIdList &ids, quint64 setMask, quint64 unsetMask)
{
    // Ensure that nothing is both set and unset
    setMask &= ~unsetMask;
    _server->flagMessages(newAction(), ids, setMask, unsetMask);

    _ids = ids;
    emitChanges();
}

void QMailStorageActionPrivate::restoreToPreviousFolder(const QMailMessageKey& key)
{
    _server->restoreToPreviousFolder(newAction(), key);
    emitChanges();
}

void QMailStorageActionPrivate::onlineCreateFolder(const QString &name, const QMailAccountId &accountId, const QMailFolderId &parentId)
{
    _server->onlineCreateFolder(newAction(), name, accountId, parentId);
    emitChanges();
}

void QMailStorageActionPrivate::onlineRenameFolder(const QMailFolderId &folderId, const QString &name)
{
    _server->onlineRenameFolder(newAction(), folderId, name);
    emitChanges();
}

void QMailStorageActionPrivate::onlineDeleteFolder(const QMailFolderId &folderId)
{
    Q_ASSERT(!_pendingActions.count());

    _server->onlineDeleteFolder(newAction(), folderId);
    emitChanges();
}

void QMailStorageActionPrivate::onlineMoveFolder(const QMailFolderId &folderId, const QMailFolderId &newParentId)
{
    _server->onlineMoveFolder(newAction(), folderId, newParentId);
    emitChanges();
}

void QMailStorageActionPrivate::init()
{
    QMailServiceActionPrivate::init();

    _ids.clear();
}

void QMailStorageActionPrivate::messagesEffected(quint64 action, const QMailMessageIdList &ids)
{
    if (validAction(action)) {
        foreach (const QMailMessageId &id, ids)
            _ids.removeAll(id);
    }
}

void QMailStorageActionPrivate::messagesAdded(quint64 action, const QMailMessageIdList &ids)
{
    if (validAction(action)) {
        _addedOrUpdatedIds.append(ids);
    }
}

void QMailStorageActionPrivate::messagesUpdated(quint64 action, const QMailMessageIdList &ids)
{
    if (validAction(action)) {
        _addedOrUpdatedIds.append(ids);
    }
}

void QMailStorageActionPrivate::storageActionCompleted(quint64 action)
{
    if (validAction(action)) {
        QMailServiceAction::Activity result(_ids.isEmpty() ? QMailServiceAction::Successful : QMailServiceAction::Failed);
        setActivity(result);
        emitChanges();
    }
}


/*!
    \class QMailStorageAction

    \preliminary
    \ingroup messaginglibrary

    \brief The QMailStorageAction class provides the interface for requesting operations on the
    storage of messages within external message services.

    QMailStorageAction provides the mechanism for messaging clients to request that the message
    server move or copy messages within an external message service.  The storage action object
    reports on the progress and outcome of its activities via the signals inherited from QMailServiceAction.

    The onlineCopyMessages() slot requests that the message server create a copy of each identified
    message in the nominated destination folder.  The onlineMoveMessages() slot requests that the
    message server move each identified message from its current location to the nominated destination
    folder.  Messages cannot be moved or copied between accounts.

    A QMailStorageAction instance supports only a single request at any time. Attempting
    to initiate an operation on a QMailStorageAction instace while another operation is already
    in progress for that action is not supported.

    An application may however use multiple QMailStorageAction instances to send independent
    requests concurrently. Each QMailServiceAction instance will report only the changes
    pertaining to the request that instance delivered.  Whether or not concurrent requests are
    concurrently serviced by the messageserver depends on whether those requests must be
    serviced by the same QMailMessageService instance.
*/

/*!
    Constructs a new transmit action object with the supplied \a parent.
*/
QMailStorageAction::QMailStorageAction(QObject *parent)
    : QMailServiceAction(*(new QMailStorageActionPrivate(this)), parent)
{
}

/*! \internal */
QMailStorageAction::~QMailStorageAction()
{
}

/*!
    Requests that the message server delete the messages listed in \a ids, both from the local device
    and the external message source.  Whether the external messages resources are actually removed is
    at the discretion of the relevant QMailMessageSource object.

    This function requires the device to be online, it may initiate communication with external servers.
*/
void QMailStorageAction::onlineDeleteMessages(const QMailMessageIdList &ids)
{
    Q_D(QMailStorageAction);
    d->onlineDeleteMessages(ids);
}

/*!
    Requests that the message server delete the messages listed in \a ids from the local device only.
*/
void QMailStorageAction::discardMessages(const QMailMessageIdList &ids)
{
    Q_D(QMailStorageAction);
    d->discardMessages(ids);
}

/*!
    Requests that the message server create a new copy of each message listed in \a ids within the folder
    identified by \a destinationId.

    This function requires the device to be online, it may initiate communication with external servers.
*/
void QMailStorageAction::onlineCopyMessages(const QMailMessageIdList &ids, const QMailFolderId &destinationId)
{
    Q_D(QMailStorageAction);
    d->onlineCopyMessages(ids, destinationId);
}

/*!
    Requests that the message server move each message listed in \a ids from its current location
    to the folder identified by \a destinationId.

    This function requires the device to be online, it may initiate communication with external servers.
*/
void QMailStorageAction::onlineMoveMessages(const QMailMessageIdList &ids, const QMailFolderId &destinationId)
{
    Q_D(QMailStorageAction);
    d->onlineMoveMessages(ids, destinationId);
}

/*!
    Requests that the message server flag each message listed in \a ids, by setting any status flags
    set in the \a setMask, and unsetting any status flags set in the \a unsetMask.  The status
    flag values should correspond to those of QMailMessage::status().

    The service implementing the account may choose to take further actions in response to flag
    changes, such as moving or deleting messages.

    This function requires the device to be online, it may initiate communication with external servers.

    \sa QMailMessage::setStatus(), QMailStore::updateMessagesMetaData()
*/
void QMailStorageAction::onlineFlagMessagesAndMoveToStandardFolder(const QMailMessageIdList &ids, quint64 setMask,
                                                                   quint64 unsetMask)
{
    Q_D(QMailStorageAction);
    d->onlineFlagMessagesAndMoveToStandardFolder(ids, setMask, unsetMask);
}

/*!
    Requests that the message server add \a messages to the mail store.

    The messages will be added asynchronously.

    All messages must use the same content scheme.

    \sa QMailStorageAction::messagesAdded(), QMailMessageMetaData::contentScheme()
*/
void QMailStorageAction::addMessages(const QMailMessageList &messages)
{
    Q_D(QMailStorageAction);
    d->addMessages(messages);
}

/*!
    Returns the ids of the messages added to the mail store.
*/
QMailMessageIdList QMailStorageAction::messagesAdded() const
{
    Q_D(const QMailStorageAction);
    return d->_addedOrUpdatedIds;
}

/*!
    Requests that the message server update \a messages in the mail store.

    The messages will be updated asynchronously.

    All messages must use the same content scheme.

    \sa QMailStorageAction::messagesUpdated(), QMailMessageMetaData::contentScheme()
*/
void QMailStorageAction::updateMessages(const QMailMessageList &messages)
{
    Q_D(QMailStorageAction);
    d->updateMessages(messages);
}

/*!
    Requests that the message server updates the meta data of the existing
    messages in the message store, to match each of the messages listed in
    \a messages.

    The messages will be updated asynchronously.

    All messages must use the same content scheme.

    \sa QMailStorageAction::messagesUpdated(), QMailMessageMetaData::contentScheme()
*/
void QMailStorageAction::updateMessages(const QMailMessageMetaDataList &messages)
{
    Q_D(QMailStorageAction);
    d->updateMessages(messages);
}

/*!
    Returns the ids of the messages updated in the mail store.
*/
QMailMessageIdList QMailStorageAction::messagesUpdated() const
{
    Q_D(const QMailStorageAction);
    return d->_addedOrUpdatedIds;
}

/*!
    Asynchronously deletes the messages in \a mailList, messages
    will be removed locally from the device, and if necessary information needed
    to delete messages from an external server is recorded.

    Deleting messages using this slot does not initiate communication with any external
    server; Deletion from the external server will occur when
    QMailRetrievalAction::exportUpdates is called successfully.

    \sa QMailStore::removeMessage()
*/
void QMailStorageAction::deleteMessages(const QMailMessageIdList& mailList)
{
    Q_D(QMailStorageAction);
    emit d->deleteMessages(mailList);
}

/*!
    Asynchronous version of QMailDisconnected::rollBackUpdates()

    Rolls back all disconnected move and copy operations that have been applied to the
    message store since the most recent synchronization of the message with the account
    specified by \a mailAccountId.

    \sa QMailDisconnected::updatesOutstanding()
*/
void QMailStorageAction::rollBackUpdates(const QMailAccountId &mailAccountId)
{
    Q_D(QMailStorageAction);
    d->rollBackUpdates(mailAccountId);
}

/*!
    Asynchronous version of QMailDisconnected::moveToStandardFolder()

    Disconnected moves the list of messages identified by \a ids into the standard folder \a standardFolder, setting standard
    folder flags as appropriate.

    The move operation will be propagated to the server by a successful call to QMailRetrievalAction::exportUpdates().

    \sa QMailDisconnected::moveToStandardFolder()
*/
void QMailStorageAction::moveToStandardFolder(const QMailMessageIdList& ids, QMailFolder::StandardFolder standardFolder)
{
    Q_D(QMailStorageAction);
    d->moveToStandardFolder(ids, standardFolder);
}

/*!
    Asynchronous version of QMailDisconnected::moveToFolder()

    Disconnected moves the list of messages identified by \a ids into the folder identified by \a folderId, setting standard
    folder flags as appropriate.

    Moving to another account is not supported.

    The move operation will be propagated to the server by a successful call to QMailRetrievalAction::exportUpdates().

    \sa QMailDisconnected::moveToFolder()
*/
void QMailStorageAction::moveToFolder(const QMailMessageIdList& ids, const QMailFolderId& folderId)
{
    Q_D(QMailStorageAction);
    emit d->moveToFolder(ids, folderId);
}

/*!
    Asynchronous version of QMailDisconnected::flagMessages()

    Disconnected flags the list of messages identified by \a ids, setting the flags specified by the bit mask \a setMask
    to on and setting the flags set by the bit mask \a unsetMask to off.

    For example this function may be used to mark messages as important.

    The flagging operation will be propagated to the server by a successful call to QMailRetrievalAction::exportUpdates().

    \sa QMailDisconnected::flagMessages(), QMailStorageAction::moveToFolder(), QMailStorageAction::moveToStandardFolder()
*/
void QMailStorageAction::flagMessages(const QMailMessageIdList& ids, quint64 setMask, quint64 unsetMask)
{
    Q_D(QMailStorageAction);
    emit d->flagMessages(ids, setMask, unsetMask);
}

/*!
    Asynchronous version of QMailDisconnected::restoreToPreviousFolder()

    Updates all QMailMessages identified by the key \a key to move the messages back to the
    previous folder they were contained by.

    \sa QMailDisconnected::restoreToPreviousFolder()
*/
void QMailStorageAction::restoreToPreviousFolder(const QMailMessageKey& key)
{
    Q_D(QMailStorageAction);
    emit d->restoreToPreviousFolder(key);
}

/*!
    Requests that the message server create a new folder named \a name, created in the
    account identified by \a accountId.
    If \a parentId is a valid folder identifier the new folder will be a child of the parent;
    otherwise the folder will be have no parent and will be created at the highest level.

    This function requires the device to be online, it may initiate communication with external servers.

    \sa onlineDeleteFolder()
*/
void QMailStorageAction::onlineCreateFolder(const QString &name, const QMailAccountId &accountId, const QMailFolderId &parentId)
{
    Q_D(QMailStorageAction);
    d->onlineCreateFolder(name, accountId, parentId);
}

/*!
    Requests that the message server rename the folder identified by \a folderId to \a name.

    This function requires the device to be online, it may initiate communication with external servers.

    \sa onlineCreateFolder(), onlineMoveFolder()
*/
void QMailStorageAction::onlineRenameFolder(const QMailFolderId &folderId, const QString &name)
{
    Q_D(QMailStorageAction);
    d->onlineRenameFolder(folderId, name);
}

/*!
    Requests that the message server delete the folder identified by \a folderId.
    Any existing folders or messages contained by the folder will also be deleted.

    This function requires the device to be online, it may initiate communication with external servers.

    \sa onlineCreateFolder(), onlineRenameFolder(), onlineMoveFolder()
*/
void QMailStorageAction::onlineDeleteFolder(const QMailFolderId &folderId)
{
    Q_D(QMailStorageAction);
    d->onlineDeleteFolder(folderId);
}

/*!
    Requests that the message server move the folder identified by \a folderId.
    If \a newParentId is a valid folder identifier the folder will be a child of the parent;
    otherwise the folder will be have no parent and will be moved to the highest level.

    This function requires the device to be online, it may initiate communication with external servers.

    \sa onlineCreateFolder(), onlineRenameFolder()
*/
void QMailStorageAction::onlineMoveFolder(const QMailFolderId &folderId, const QMailFolderId &newParentId)
{
    Q_D(QMailStorageAction);
    d->onlineMoveFolder(folderId, newParentId);
}

QMailSearchActionPrivate::QMailSearchActionPrivate(QMailSearchAction *i)
    : QMailServiceActionPrivate(i)
{
    connect(_server.data(), &QMailMessageServer::matchingMessageIds,
            this, &QMailSearchActionPrivate::matchingMessageIds);
    connect(_server.data(), &QMailMessageServer::remainingMessagesCount,
            this, &QMailSearchActionPrivate::handleRemainingMessagesCount);
    connect(_server.data(), &QMailMessageServer::messagesCount,
            this, &QMailSearchActionPrivate::handleMessagesCount);
    connect(_server.data(), &QMailMessageServer::searchCompleted,
            this, &QMailSearchActionPrivate::searchCompleted);

    init();
}

QMailSearchActionPrivate::~QMailSearchActionPrivate()
{
}

void QMailSearchActionPrivate::searchMessages(const QMailMessageKey &filter, const QString &bodyText,
                                              QMailSearchAction::SearchSpecification spec,
                                              const QMailMessageSortKey &sort)
{
    _server->searchMessages(newAction(), filter, bodyText, spec, sort);
    emitChanges();
}

void QMailSearchActionPrivate::searchMessages(const QMailMessageKey &filter, const QString &bodyText,
                                              QMailSearchAction::SearchSpecification spec, quint64 limit,
                                              const QMailMessageSortKey &sort)
{
    _server->searchMessages(newAction(), filter, bodyText, spec, limit, sort);
    emitChanges();
}

void QMailSearchActionPrivate::countMessages(const QMailMessageKey &filter, const QString &bodyText)
{
    _server->countMessages(newAction(), filter, bodyText);
    emitChanges();
}

void QMailSearchActionPrivate::cancelOperation()
{
    Q_ASSERT(_isValid && _action != 0);
    if (_isValid)
        _server->cancelSearch(_action);
}

void QMailSearchActionPrivate::init()
{
    QMailServiceActionPrivate::init();

    _matchingIds.clear();
    _remainingMessagesCount = 0;
    _messagesCount = 0;
}

void QMailSearchActionPrivate::matchingMessageIds(quint64 action, const QMailMessageIdList &ids)
{
    if (validAction(action)) {
        _matchingIds += ids;

        emit messageIdsMatched(ids);
    }
}

void QMailSearchActionPrivate::handleRemainingMessagesCount(quint64 action, uint count)
{
    if (validAction(action)) {
        _remainingMessagesCount = count;

        emit remainingMessagesCountChanged(count);
    }
}

void QMailSearchActionPrivate::handleMessagesCount(quint64 action, uint count)
{
    if (validAction(action)) {
        _messagesCount = count;

        emit messagesCountChanged(count);
    }
}

void QMailSearchActionPrivate::searchCompleted(quint64 action)
{
    if (validAction(action)) {
        setActivity(QMailServiceAction::Successful);
        emitChanges();
    }
}

void QMailSearchActionPrivate::finaliseSearch()
{
    emit messageIdsMatched(_matchingIds);

    setActivity(QMailServiceAction::Successful);
    emitChanges();
}


/*!
    \class QMailSearchAction

    \preliminary
    \ingroup messaginglibrary

    \brief The QMailSearchAction class provides the interface for identifying messages that match a set of search criteria.

    QMailSearchAction provides the mechanism for messaging clients to request that the message
    server perform a search for messages that match the supplied search criteria.  For criteria
    pertaining to message meta data, the message server will search within the locally held
    meta data.  For a textual content criterion, the message server will search locally where
    the message content is held locally, and on the external server for messages whose content
    has not been retrieved (provided the external service provides a search facility).

    A QMailSearchAction instance supports only a single request at any time. Attempting
    to initiate an operation on a QMailSearchAction instace while another operation is already
    in progress for that action is not supported.

    An application may however use multiple QMailSearchAction instances to send independent
    requests concurrently. Each QMailServiceAction instance will report only the changes
    pertaining to the request that instance delivered.  Whether or not concurrent requests are
    concurrently serviced by the messageserver depends on whether those requests must be
    serviced by the same QMailMessageService instance.
*/

/*!
    \enum QMailSearchAction::SearchSpecification

    This enum type is specify the form of search that the message server should perform.

    \value Local    Only the message data stored on the local device should be searched.
    \value Remote   The search should be extended to message data stored on external servers.
*/

/*!
    Constructs a new search action object with the supplied \a parent.
*/
QMailSearchAction::QMailSearchAction(QObject *parent)
    : QMailServiceAction(*(new QMailSearchActionPrivate(this)), parent)
{
    Q_D(const QMailSearchAction);
    connect(d, &QMailSearchActionPrivate::messageIdsMatched,
            this, &QMailSearchAction::messageIdsMatched);
    connect(d, &QMailSearchActionPrivate::remainingMessagesCountChanged,
            this, &QMailSearchAction::remainingMessagesCountChanged);
    connect(d, &QMailSearchActionPrivate::messagesCountChanged,
            this, &QMailSearchAction::messagesCountChanged);
}

/*! \internal */
QMailSearchAction::~QMailSearchAction()
{
}

/*!
    Requests that the message server identify all messages that match the criteria
    specified by \a filter.  If \a bodyText is non-empty then messages that
    contain the supplied text in their content will also be identified.

    If \a spec is \l{QMailSearchAction::Remote}{Remote}, then the device must be,
    online and an external service will be requested to perform the search for
    messages not stored locally.

    If \a sort is not empty, the external service will return matching messages in
    the ordering indicated by the sort criterion if possible.
*/
void QMailSearchAction::searchMessages(const QMailMessageKey &filter, const QString &bodyText,
                                       SearchSpecification spec, const QMailMessageSortKey &sort)
{
    Q_D(QMailSearchAction);
    d->searchMessages(filter, bodyText, spec, sort);
}

/*!
    Requests that the message server identify all messages that match the criteria
    specified by \a filter.  If \a bodyText is non-empty then messages that
    contain the supplied text in their content will also be identified.

    If \a spec is \l{QMailSearchAction::Remote}{Remote}, then the device must be,
    online and an external service will be requested to perform the search for
    messages not stored locally.

    A maximum of \a limit messages will be retrieved from the remote server.

    If \a sort is not empty, the external service will return matching messages in
    the ordering indicated by the sort criterion if possible.
*/
void QMailSearchAction::searchMessages(const QMailMessageKey &filter, const QString &bodyText,
                                       SearchSpecification spec, quint64 limit, const QMailMessageSortKey &sort)
{
    Q_D(QMailSearchAction);
    d->searchMessages(filter, bodyText, spec, limit, sort);
}

/*!
    Requests that the message server count all messages that match the criteria
    specified by \a filter, both local and on remote servers.  If \a bodyText
    is non-empty then messages that contain the supplied text in their content
    will also be matched and counted.

    This function requires the device to be online, it may initiate communication
    with external servers.
*/
void QMailSearchAction::countMessages(const QMailMessageKey &filter, const QString &bodyText)
{
    Q_D(QMailSearchAction);
    d->countMessages(filter, bodyText);
}

/*!
    Attempts to cancel the last requested search operation.
*/
void QMailSearchAction::cancelOperation()
{
    Q_D(QMailSearchAction);
    d->cancelOperation();
}

/*!
    Returns the list of message identifiers found to match the search criteria.
    Unless activity() returns \l Successful, an empty list is returned.
*/
QMailMessageIdList QMailSearchAction::matchingMessageIds() const
{
    Q_D(const QMailSearchAction);
    return d->_matchingIds;
}

/*!
    Returns the count of messages remaining on the remote server; that
    is the count of matching messages that will not be retrieved
    to the device.
*/
uint QMailSearchAction::remainingMessagesCount() const
{
    Q_D(const QMailSearchAction);
    return d->_remainingMessagesCount;
}

/*!
    Returns the count of matching messages on the remote server.
*/
uint QMailSearchAction::messagesCount() const
{
    Q_D(const QMailSearchAction);
    return d->_messagesCount;
}

/*!
    Returns a key matching messages that are temporary messages existing only as
    the result of a search action.
*/
QMailMessageKey QMailSearchAction::temporaryKey()
{
    return QMailMessageKey::status(QMailMessage::Temporary);
}

/*!
    \fn QMailSearchAction::messageIdsMatched(const QMailMessageIdList &ids)

    This signal is emitted when the messages in \a ids are discovered to match
    the criteria of the search in progress.

    \sa matchingMessageIds()
*/

/*!
    \fn QMailSearchAction::remainingMessagesCountChanged(uint count)

    This signal emits the \a count of messages remaining on the remote server; that
    is the count of matching messages that will not be retrieved to the device.

    Only emitted for remote searches.

    \sa remainingMessagesCount()
*/

/*!
    \fn QMailSearchAction::messagesCountChanged(uint count)

    This signal emits the \a count of matching messages on the remote server.

    Only emitted for remote searches.

    \sa messagesCount()
*/

QMailActionInfoPrivate::QMailActionInfoPrivate(const QMailActionData &data, QMailActionInfo *i,
                                               QSharedPointer<QMailMessageServer> server)
    : QMailServiceActionPrivate(i, server),
      _requestType(data.requestType()),
      _actionCompleted(false)
{
    // Service handler really should be sending the activity,
    // rather than us faking it..
    connect(_server.data(), &QMailMessageServer::retrievalCompleted,
            this, &QMailActionInfoPrivate::activityCompleted);
    connect(_server.data(), &QMailMessageServer::storageActionCompleted,
            this, &QMailActionInfoPrivate::activityCompleted);
    connect(_server.data(), &QMailMessageServer::searchCompleted,
            this, &QMailActionInfoPrivate::activityCompleted);
    connect(_server.data(), &QMailMessageServer::transmissionCompleted,
            this, &QMailActionInfoPrivate::activityCompleted);

    init();
    _progress = data.progressCurrent();
    _total = data.progressTotal();
    _status = QMailServiceAction::Status(QMailServiceAction::Status::ErrorCode(data.errorCode()),
                                         data.text(), data.accountId(), data.folderId(), data.messageId());
    setAction(data.id());
}

void QMailActionInfoPrivate::activityCompleted(quint64 action)
{
    if (validAction(action)) {
        setActivity(QMailServiceAction::Successful);
        emitChanges();
    }
}

float QMailActionInfoPrivate::totalProgress() const
{
    if (_total == 0)
        return 0.0;
    else
        return static_cast<float>(_progress) / _total;
}

void QMailActionInfoPrivate::theProgressChanged(uint, uint)
{
    emit totalProgressChanged(totalProgress());
}

void QMailActionInfoPrivate::theStatusChanged(const QMailServiceAction::Status &newStatus)
{
    if (_lastStatus.accountId != newStatus.accountId)
        emit statusAccountIdChanged(newStatus.accountId);
    if (_lastStatus.errorCode != newStatus.errorCode)
        emit statusErrorCodeChanged(newStatus.errorCode);
    if (_lastStatus.folderId != newStatus.folderId)
        emit statusFolderIdChanged(newStatus.folderId);
    if (_lastStatus.messageId != newStatus.messageId)
        emit statusMessageIdChanged(newStatus.messageId);

    _lastStatus = newStatus;
}

quint64 QMailActionInfoPrivate::actionId() const
{
    return _action;
}

QMailServerRequestType QMailActionInfoPrivate::requestType() const
{
    return _requestType;
}

QMailActionInfo::Status::ErrorCode QMailActionInfoPrivate::statusErrorCode() const
{
    return _status.errorCode;
}

QString QMailActionInfoPrivate::statusText() const
{
    return _status.text;
}

QMailAccountId QMailActionInfoPrivate::statusAccountId() const
{
    return _status.accountId;
}
QMailFolderId QMailActionInfoPrivate::statusFolderId() const
{
    return _status.folderId;
}

QMailMessageId QMailActionInfoPrivate::statusMessageId() const
{
    return _status.messageId;
}

/*!
    \class QMailActionInfo

    \preliminary
    \ingroup messaginglibrary

    \brief The QMailActionInfo class provides the interface for tracking individual actions

    QMailActionInfo provides the mechanism for messaging clients to track an action that is
    or has run on the server. QMailActionInfo objects can only be created by QMailActionObserver
    and derive most of their functionality and signals from \l QMailServiceAction.

    QMailActionInfo presents \l requestType() and \l id() to the the client. For future QML
    interoperability properties and notification signals have been added.
*/

/*!
    \typedef QMailActionInfo::StatusErrorCode

    A typedef for QMailServiceAction::Status::ErrorCode so moc can parse it
*/

/*!
  \fn QMailActionInfo::statusErrorCodeChanged(QMailActionInfo::StatusErrorCode error)

  Signal emitted when \l statusErrorCode() has changed to a new value of \a error
*/

/*!
  \fn QMailActionInfo::statusTextChanged(const QString &text)

  Signal emittted when \l statusText() has changed to a new \a text.
*/

/*!
  \fn QMailActionInfo::statusAccountIdChanged(const QMailAccountId &accountId)

  Signal emitted when \l statusAccountId() has changed to a new \a accountId.
*/

/*!
  \fn QMailActionInfo::statusFolderIdChanged(const QMailFolderId &folderId)

  Signal emitted when \l statusFolderId() has changed to a new \a folderId.
*/

/*!
  \fn QMailActionInfo::statusMessageIdChanged(const QMailMessageId &messageId)

  Signal emitted when \l statusMessageId() has changed to a new \a messageId.
*/

/*!
    \fn QMailActionInfo::totalProgressChanged(float progress)

    Signal emitted when \l totalProgress() has changed to a new \a progress.
*/

/*! \internal */
QMailActionInfo::QMailActionInfo(const QMailActionData &data, QSharedPointer<QMailMessageServer> server)
    : QMailServiceAction(*(new QMailActionInfoPrivate(data, this, server)), nullptr) // NB: No qobject parent!
{
    Q_D(QMailActionInfo);
    connect(d, &QMailActionInfoPrivate::statusAccountIdChanged,
            this, &QMailActionInfo::statusAccountIdChanged);
    connect(d, &QMailActionInfoPrivate::statusErrorCodeChanged,
            this, &QMailActionInfo::statusErrorCodeChanged);
    connect(d, &QMailActionInfoPrivate::statusTextChanged,
            this, &QMailActionInfo::statusTextChanged);
    connect(d, &QMailActionInfoPrivate::statusFolderIdChanged,
            this, &QMailActionInfo::statusFolderIdChanged);
    connect(d, &QMailActionInfoPrivate::statusMessageIdChanged,
            this, &QMailActionInfo::statusMessageIdChanged);
    connect(d, &QMailActionInfoPrivate::totalProgressChanged,
            this, &QMailActionInfo::totalProgressChanged);

    // Hack to get around _interface not being "ready" in the private class
    connect(this, &QMailActionInfo::progressChanged,
            d, &QMailActionInfoPrivate::theProgressChanged);
    connect(this, &QMailActionInfo::statusChanged,
            d, &QMailActionInfoPrivate::theStatusChanged);
}

/*!
    \fn QMailActionInfo::requestType() const
    Returns the \c QMailServiceRequestType of action is it tracking
*/
QMailServerRequestType QMailActionInfo::requestType() const
{
    Q_D(const QMailActionInfo);
    return d->requestType();
}

/*!
    Returns the unique action id that was assigned to the action QMailActionInfo is tracking
*/
quint64 QMailActionInfo::id() const
{
    Q_D(const QMailActionInfo);
    return d->actionId();
}

/*!
    \fn QMailActionInfo::totalProgress() const
    Returns a floating point between (0..1) indicating the actions progress. This is a convience
    method to \l QMailServiceAction::progress()
*/
float QMailActionInfo::totalProgress() const
{
    Q_D(const QMailActionInfo);
    return d->totalProgress();
}

/*!
  \fn QMailActionInfo::statusErrorCode() const
  This is a convience accessor to \l QMailServiceAction::Status::errorCode
*/
QMailActionInfo::StatusErrorCode QMailActionInfo::statusErrorCode() const
{
    Q_D(const QMailActionInfo);
    return d->statusErrorCode();
}

/*!
  \fn QMailActionInfo::statusText() const
  This is a convience accessor to \l QMailServiceAction::Status::text
*/
QString QMailActionInfo::statusText() const
{
    Q_D(const QMailActionInfo);
    return d->statusText();
}

/*!
  \fn QMailActionInfo::statusAccountId() const
  This is a convience accessor to \l QMailServiceAction::Status::accountId
*/
QMailAccountId QMailActionInfo::statusAccountId() const
{
    Q_D(const QMailActionInfo);
    return d->statusAccountId();
}

/*!
  \fn QMailActionInfo::statusFolderId() const
  This is a convience accessor to \l QMailServiceAction::Status::errorCode
*/
QMailFolderId QMailActionInfo::statusFolderId() const
{
    Q_D(const QMailActionInfo);
    return d->statusFolderId();
}

/*!
  \fn QMailActionInfo::statusMessageId() const
  This is a convience accessor for \l QMailServiceAction::Status::messageId
*/

QMailMessageId QMailActionInfo::statusMessageId() const
{
    Q_D(const QMailActionInfo);
    return d->statusMessageId();
}

QMailActionObserverPrivate::QMailActionObserverPrivate(QMailActionObserver *i)
    : QMailServiceActionPrivate(i),
      _isReady(false)
{
    connect(_server.data(), &QMailMessageServer::actionStarted,
            this, &QMailActionObserverPrivate::actionStarted);
    connect(_server.data(), &QMailMessageServer::actionsListed,
            this, &QMailActionObserverPrivate::actionsListed);
    connect(_server.data(), &QMailMessageServer::activityChanged,
            this, &QMailActionObserverPrivate::onActivityChanged);

    _server->listActions();

    init();
}

QList< QSharedPointer<QMailActionInfo> > QMailActionObserverPrivate::runningActions() const
{
    return _runningActions.values();
}

void QMailActionObserverPrivate::listActionsRequest()
{
    _server->listActions();
}

void QMailActionObserverPrivate::actionsListed(const QMailActionDataList &actions)
{
    if (_isReady)
        _runningActions.clear();
    else
        _isReady = true;

    Q_ASSERT(_runningActions.isEmpty());

    foreach (QMailActionData const& action, actions) {
        addAction(action);
    }

    _isReady = true;
    emit actionsChanged(runningActions());
}

void QMailActionObserverPrivate::actionStarted(const QMailActionData &action)
{
    if (_isReady) {
        addAction(action);
        emit actionsChanged(runningActions());
    }
}

QSharedPointer<QMailActionInfo> QMailActionObserverPrivate::addAction(const QMailActionData &action)
{
    QSharedPointer<QMailActionInfo> actionInfo(new QMailActionInfo(action, _server));
    _runningActions.insert(action.id(), actionInfo);

    return actionInfo;
}

void QMailActionObserverPrivate::onActivityChanged(quint64 id, QMailServiceAction::Activity activity)
{
    if (activity == QMailServiceAction::Successful || activity == QMailServiceAction::Failed) {
        QMap< QMailActionId, QSharedPointer<QMailActionInfo> >::Iterator it
            = _runningActions.find(QMailActionId(id));
        if (it != _runningActions.end()) {
            _runningActions.erase(it);
            emit actionsChanged(runningActions());
        }
    }
}

/*! \class QMailActionObserver

   \preliminary
   \ingroup messaginglibrary

   \brief The QMailActionObserver class provides an interface for monitoring currently running actions

    QMailActionObserver provides a mechanism for messaging clients to observe what actions
    the messageserver is currently running. A list of currently running actions can be retrieved
    with \l actions(). When actions are started or finished \l actionsChanged() is emitted with the
    new list of actions.

    QMailActionObserver is initialised asynchronously, thus reading \l actions() immediately will
    return an empty list.
*/

/*!
    \fn QMailActionObserver::actionsChanged(const QList< QSharedPointer<QMailActionInfo> > &newActions)

    This signal is emitted whenever the list of actions we are observing changes. This can be
    for three reasons: actions have started, actions have finished or the action list has just been
    initialized.

    New list given by \a newActions

    \sa actions()

*/

/*! Constructs a new QMailActionObserver with a supplied \a parent  */
QMailActionObserver::QMailActionObserver(QObject *parent)
    : QMailServiceAction(*(new QMailActionObserverPrivate(this)), parent)
{
    Q_D(QMailActionObserver);

    connect(d, &QMailActionObserverPrivate::actionsChanged,
            this, &QMailActionObserver::actionsChanged);
}

/*! Destructs QMailActionObserver */
QMailActionObserver::~QMailActionObserver()
{
}

/*!
    \fn QMailActionObserver::actions() const
    Returns a list of the currently running actions on the message server.

    \sa QMailActionInfo
*/
QList< QSharedPointer<QMailActionInfo> > QMailActionObserver::actions() const
{
    Q_D(const QMailActionObserver);
    return d->runningActions();
}

/*!
    \fn QMailActionObserver::listActionsRequest()
    Makes request of running actions. Result will be returned with actionsChanged() signal.
*/
void QMailActionObserver::listActionsRequest()
{
    Q_D(QMailActionObserver);
    d->listActionsRequest();
}

QMailProtocolActionPrivate::QMailProtocolActionPrivate(QMailProtocolAction *i)
    : QMailServiceActionPrivate(i)
{
    connect(_server.data(), &QMailMessageServer::protocolResponse,
            this, &QMailProtocolActionPrivate::handleProtocolResponse);
    connect(_server.data(), &QMailMessageServer::protocolRequestCompleted,
            this, &QMailProtocolActionPrivate::handleProtocolRequestCompleted);

    init();
}

void QMailProtocolActionPrivate::protocolRequest(const QMailAccountId &accountId, const QString &request,
                                                 const QVariantMap &data)
{
    _server->protocolRequest(newAction(), accountId, request, data);
}

void QMailProtocolActionPrivate::handleProtocolResponse(quint64 action, const QString &response,
                                                        const QVariantMap &data)
{
    if (validAction(action)) {
        emit protocolResponse(response, data);
    }
}

void QMailProtocolActionPrivate::handleProtocolRequestCompleted(quint64 action)
{
    if (validAction(action)) {
        setActivity(QMailServiceAction::Successful);
        emitChanges();
    }
}


/*!
    \class QMailProtocolAction

    \preliminary
    \ingroup messaginglibrary

    \brief The QMailProtocolAction class provides a mechanism for messageserver clients
    and services to collaborate without messageserver involvement.

    QMailProtocolAction provides a mechanism for messaging clients to request actions from
    message services that are not implemented by the messageserver.  If a client can
    determine that the service implementing a specific account supports a particular
    operation (by inspecting the output of QMailAccount::messageSources()), it may
    invoke that operation by passing appropriately formatted data to the service via the
    protocolRequest() slot.

    If the service is able to provide the requested service, and extract the necessary
    data from the received input, it should perform the requested operation.  If any
    output is produced, it may be passed back to the client application via the
    protocolResponse() signal.
*/

/*!
    Constructs a new protocol action object with the supplied \a parent.
*/
QMailProtocolAction::QMailProtocolAction(QObject *parent)
    : QMailServiceAction(*(new QMailProtocolActionPrivate(this)), parent)
{
    Q_D(QMailProtocolAction);
    connect(d, &QMailProtocolActionPrivate::protocolResponse,
            this, &QMailProtocolAction::protocolResponse);
}

/*! \internal */
QMailProtocolAction::~QMailProtocolAction()
{
}

/*!
    Requests that the message server forward the protocol-specific request \a request
    to the QMailMessageSource configured for the account identified by \a accountId.
    The request may have associated \a data, in a protocol-specific form.
    There might be limitations on what type of data is allowed.
*/
void QMailProtocolAction::protocolRequest(const QMailAccountId &accountId, const QString &request,
                                          const QVariantMap &data)
{
    Q_D(QMailProtocolAction);
    d->protocolRequest(accountId, request, data);
}

/*!
    \fn QMailProtocolAction::protocolResponse(const QString &response, const QVariantMap &data)

    This signal is emitted when the response \a response is emitted by the messageserver,
    with the associated \a data.
*/

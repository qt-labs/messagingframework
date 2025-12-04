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

#include "qmailipc.h"

#include "qmailid.h"
#include "qmailmessage.h"
#include "qmailmessagekey.h"
#include "qmailmessagesortkey.h"
#include "qmailserviceaction.h"

#include "qmailmessage_p.h"

#include <QDBusArgument>
#include <QDBusMetaType>

// all the qmailid.h / *Id classes are basically wrappers to uint64 with identical api.
// implement their serialization with a template
template<class IdType>
const QDBusArgument &operator<<(QDBusArgument &argument, const IdType &id)
{
    argument.beginStructure();
    argument << id.toULongLong();
    argument.endStructure();
    return argument;
}

// caution: this might catch enum types if not implemented explicitly
template<class IdType>
const QDBusArgument &operator>>(const QDBusArgument &argument, IdType &id)
{
    argument.beginStructure();
    quint64 val;
    argument >> val;
    id = IdType(val);
    argument.endStructure();
    return argument;
}

// bunch of enums with a macro for simplicity.
// could be done with template but it gets a bit hairier so it works for both id and enum.
//
// to consider:
// - should we pass these without the structure container? qdbusxml2cpp doesn't cope
//   with annotations for plain integer types, but that could be bypassed in c++
// - do we want all these passed as integers or should we do some textual representations

#define DBUS_ENUM_SERIALIZATION(TYPE) \
QDBusArgument &operator<<(QDBusArgument &argument, const TYPE &value) \
{ \
    argument.beginStructure(); \
    argument << static_cast<int>(value); \
    argument.endStructure(); \
    return argument; \
} \
const QDBusArgument &operator>>(const QDBusArgument &argument, TYPE &value) \
{ \
    argument.beginStructure(); \
    int intVal; \
    argument >> intVal; \
    value = static_cast<TYPE>(intVal); \
    argument.endStructure(); \
    return argument; \
}

DBUS_ENUM_SERIALIZATION(QMailMessageKey::Properties)
DBUS_ENUM_SERIALIZATION(QMailServiceAction::Status::ErrorCode)
DBUS_ENUM_SERIALIZATION(QMailServiceAction::Activity)
DBUS_ENUM_SERIALIZATION(QMailServiceAction::Connectivity)
DBUS_ENUM_SERIALIZATION(QMailRetrievalAction::RetrievalSpecification)
DBUS_ENUM_SERIALIZATION(QMailSearchAction::SearchSpecification)
DBUS_ENUM_SERIALIZATION(QMailStore::MessageRemovalOption)
DBUS_ENUM_SERIALIZATION(QMailStore::ChangeType)

const QDBusArgument &operator<<(QDBusArgument &argument, const QMailMessageMetaData &metadata)
{
    argument.beginStructure();
    argument << static_cast<int>(metadata.messageType());
    argument << metadata.status(); // quint64
    argument << static_cast<int>(metadata.contentCategory());
    argument << metadata.parentAccountId(); // qmailaccountid
    argument << metadata.serverUid(); // string
    argument << metadata.size(); // uint
    argument << metadata.id(); // messageid
    argument << metadata.parentFolderId(); // folderid
    argument << metadata.previousParentFolderId(); // folderid
    argument << metadata.subject(); // string
    argument << metadata.date().toString(); // qmailtimestamp
    argument << metadata.receivedDate().toString(); // qmailtimestamp
    argument << metadata.from().toString(); // qmailaddress
    argument << metadata.d->_to; // string, avoiding conversion by using member variable
    argument << metadata.copyServerUid(); // string
    argument << metadata.restoreFolderId(); // folderid
    argument << metadata.listId(); // string
    argument << metadata.rfcId(); // string
    argument << metadata.contentScheme(); // string
    argument << metadata.contentIdentifier(); // string
    argument << metadata.inResponseTo(); // messageid
    argument << static_cast<int>(metadata.responseType());
    argument << metadata.preview(); // string
    argument << metadata.parentThreadId(); // threadid
    argument << metadata.customFields(); // map<string,string>

    // to consider: does it make sense to pass these or should they come implicitly from the used methods?
    argument << metadata.d->_customFieldsModified;
    argument << metadata.d->_dirty;

    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, QMailMessageMetaData &metadata)
{
    int intVal;
    uint uintVal;
    quint64 uint64Val;
    QString stringVal;
    QMailMessageId messageId;
    QMailFolderId folderId;

    argument.beginStructure();

    argument >> intVal;
    metadata.setMessageType(static_cast<QMailMessageMetaData::MessageType>(intVal));

    argument >> uint64Val;
    metadata.setStatus(uint64Val);

    argument >> intVal;
    metadata.setContentCategory(static_cast<QMailMessageMetaData::ContentCategory>(intVal));

    QMailAccountId accountId;
    argument >> accountId;
    metadata.setParentAccountId(accountId);

    argument >> stringVal;
    metadata.setServerUid(stringVal);

    argument >> uintVal;
    metadata.setSize(uintVal);

    argument >> messageId;
    metadata.setId(messageId);

    argument >> folderId;
    metadata.setParentFolderId(folderId);

    argument >> folderId;
    metadata.setPreviousParentFolderId(folderId);

    argument >> stringVal;
    metadata.setSubject(stringVal);

    argument >> stringVal;
    metadata.setDate(QMailTimeStamp(stringVal));

    argument >> stringVal;
    metadata.setReceivedDate(QMailTimeStamp(stringVal));

    argument >> stringVal;
    metadata.setFrom(QMailAddress(stringVal));

    argument >> metadata.d->_to; // directly member variable to avoid extra conversions

    argument >> stringVal;
    metadata.setCopyServerUid(stringVal);

    argument >> folderId;
    metadata.setRestoreFolderId(folderId);

    argument >> stringVal;
    metadata.setListId(stringVal);

    argument >> stringVal;
    metadata.setRfcId(stringVal);

    argument >> stringVal;
    metadata.setContentScheme(stringVal);

    argument >> stringVal;
    metadata.setContentIdentifier(stringVal);

    argument >> messageId;
    metadata.setInResponseTo(messageId);

    argument >> intVal;
    metadata.setResponseType(static_cast<QMailMessageMetaData::ResponseType>(intVal));

    argument >> stringVal;
    metadata.setPreview(stringVal);

    QMailThreadId threadId;
    argument >> threadId;
    metadata.setParentThreadId(threadId);

    QMap<QString, QString> customFields;
    argument >> customFields;
    metadata.setCustomFields(customFields);

    // reset dirtiness on how it was passed
    argument >> metadata.d->_customFieldsModified;
    argument >> metadata.d->_dirty;

    argument.endStructure();
    return argument;
}

const QDBusArgument &operator<<(QDBusArgument &argument, const QMailMessagePartContainer::Location &location)
{
    argument.beginStructure();
    argument << location.d->_messageId;
    argument << location.d->_indices;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, QMailMessagePartContainer::Location &location)
{
    argument.beginStructure();

    QMailMessageId messageId;
    argument >> messageId;
    QList<uint> indices;
    argument >> indices;

    location.d->_messageId = messageId;
    location.d->_indices = indices;

    argument.endStructure();
    return argument;
}

// TODO reimplement the keys without binary data
const QDBusArgument &operator<<(QDBusArgument &argument, const QMailMessageSortKey &key)
{
    QByteArray data;
    QDataStream buffer(&data, QIODevice::WriteOnly);
    key.serialize(buffer);

    argument.beginStructure();
    argument << data;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, QMailMessageSortKey &key)
{
    QByteArray data;
    argument.beginStructure();
    argument >> data;
    QDataStream buffer(data);
    key.deserialize(buffer);
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator<<(QDBusArgument &argument, const QMailMessageKey &key)
{
    QByteArray data;
    QDataStream buffer(&data, QIODevice::WriteOnly);
    key.serialize(buffer);

    argument.beginStructure();
    argument << data;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, QMailMessageKey &key)
{
    QByteArray data;
    argument.beginStructure();
    argument >> data;
    QDataStream buffer(data);
    key.deserialize(buffer);
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator<<(QDBusArgument &argument, const QMailServiceAction::Status &status)
{
    argument.beginStructure();

    argument << status.errorCode;
    argument << status.text;
    argument << status.accountId;
    argument << status.folderId;
    argument << status.messageId;

    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, QMailServiceAction::Status &status)
{
    QMailServiceAction::Status::ErrorCode errorCode;
    QString text;
    QMailAccountId accountId;
    QMailFolderId folderId;
    QMailMessageId messageId;

    argument.beginStructure();

    argument >> errorCode;
    argument >> text;
    argument >> accountId;
    argument >> folderId;
    argument >> messageId;
    status = QMailServiceAction::Status(errorCode, text, accountId, folderId, messageId);

    argument.endStructure();
    return argument;
}

const QDBusArgument &operator<<(QDBusArgument &argument, const QMailActionData &value)
{
    argument.beginStructure();

    // internally QMailActionData has QMailServiceAction::Status for the latter value but api doesn't expose it.
    // could be an option to reuse the serialization of that.
    argument << value.id();
    argument << static_cast<int>(value.requestType());
    argument << value.progressCurrent();
    argument << value.progressTotal();
    argument << value.errorCode(); // int, not error code enum
    argument << value.text();
    argument << value.accountId();
    argument << value.folderId();
    argument << value.messageId();

    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, QMailActionData &value)
{
    QMailActionId id;
    int requestType;
    uint progressCurrent;
    uint progressTotal;
    int errorCode;
    QString text;
    QMailAccountId accountId;
    QMailFolderId folderId;
    QMailMessageId messageId;

    argument.beginStructure();

    argument >> id;
    argument >> requestType;
    argument >> progressCurrent;
    argument >> progressTotal;
    argument >> errorCode;
    argument >> text;
    argument >> accountId;
    argument >> folderId;
    argument >> messageId;

    value = QMailActionData(id, static_cast<QMailServerRequestType>(requestType), progressCurrent, progressTotal,
                            errorCode, text, accountId, folderId, messageId);

    argument.endStructure();
    return argument;
}

bool QMailIpc::init()
{
    static bool registrationDone = false;

    if (!registrationDone) {
        // ids
        qRegisterMetaType<QMailAccountId>("QMailAccountId");
        qDBusRegisterMetaType<QMailAccountId>();
        qRegisterMetaType<QMailFolderId>("QMailFolderId");
        qDBusRegisterMetaType<QMailFolderId>();
        qRegisterMetaType<QMailThreadId>("QMailThreadId");
        qDBusRegisterMetaType<QMailThreadId>();
        qRegisterMetaType<QMailMessageId>("QMailMessageId");
        qDBusRegisterMetaType<QMailMessageId>();

        qRegisterMetaType<QMailAccountIdList>("QMailAccountIdList");
        qDBusRegisterMetaType<QMailAccountIdList>();
        qRegisterMetaType<QMailFolderIdList>("QMailFolderIdList");
        qDBusRegisterMetaType<QMailFolderIdList>();
        qRegisterMetaType<QMailThreadIdList>("QMailThreadIdList");
        qDBusRegisterMetaType<QMailThreadIdList>();
        qRegisterMetaType<QMailMessageIdList>("QMailMessageIdList");
        qDBusRegisterMetaType<QMailMessageIdList>();

        // enums
        qRegisterMetaType<QMailServiceAction::Status::ErrorCode>("QMailServiceAction::Status::ErrorCode");
        qDBusRegisterMetaType<QMailServiceAction::Status::ErrorCode>();
        qRegisterMetaType<QMailServiceAction::Activity>("QMailServiceAction::Activity");
        qDBusRegisterMetaType<QMailServiceAction::Activity>();
        qRegisterMetaType<QMailServiceAction::Connectivity>("QMailServiceAction::Connectivity");
        qDBusRegisterMetaType<QMailServiceAction::Connectivity>();
        qRegisterMetaType<QMailRetrievalAction::RetrievalSpecification>("QMailRetrievalAction::RetrievalSpecification");
        qDBusRegisterMetaType<QMailRetrievalAction::RetrievalSpecification>();
        qRegisterMetaType<QMailSearchAction::SearchSpecification>("QMailSearchAction::SearchSpecification");
        qDBusRegisterMetaType<QMailSearchAction::SearchSpecification>();
        qRegisterMetaType<QMailStore::MessageRemovalOption>("QMailStore::MessageRemovalOption");
        qDBusRegisterMetaType<QMailStore::MessageRemovalOption>();
        qRegisterMetaType<QMailStore::ChangeType>("QMailStore::ChangeType");
        qDBusRegisterMetaType<QMailStore::ChangeType>();
        qRegisterMetaType<QMailMessageKey::Properties>("QMailMessageKey::Properties");
        qDBusRegisterMetaType<QMailMessageKey::Properties>();

        // classes
        qRegisterMetaType<QMailMessageMetaData>("QMailMessageMetaData");
        qDBusRegisterMetaType<QMailMessageMetaData>();
        qRegisterMetaType<QMailMessageMetaDataList>("QMailMessageMetaDataList");
        qDBusRegisterMetaType<QMailMessageMetaDataList>();

        qRegisterMetaType<QMailMessagePartContainer::Location>("QMailMessagePartContainer::Location");
        qDBusRegisterMetaType<QMailMessagePartContainer::Location>();

        qRegisterMetaType<QMailMessageSortKey>("QMailMessageSortKey");
        qDBusRegisterMetaType<QMailMessageSortKey>();

        qRegisterMetaType<QMailMessageKey>("QMailMessageKey");
        qDBusRegisterMetaType<QMailMessageKey>();

        qRegisterMetaType<QMailServiceAction::Status>("QMailServiceAction::Status");
        qDBusRegisterMetaType<QMailServiceAction::Status>();

        qRegisterMetaType<QMailActionData>("QMailActionData");
        qDBusRegisterMetaType<QMailActionData>();
        qRegisterMetaType<QMailActionDataList>("QMailActionDataList");
        qDBusRegisterMetaType<QMailActionDataList>();

        registrationDone = true;
    }

    return true;
}

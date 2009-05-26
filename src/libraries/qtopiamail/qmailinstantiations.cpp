/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#include "qmailmessage_p.h"
#include "qmailmessageset_p.h"
#include "qmailserviceaction_p.h"
#include "qprivateimplementationdef.h"

template class QPrivatelyImplemented<QMailMessageHeaderFieldPrivate>;
template class QPrivatelyImplemented<QMailMessageHeaderPrivate>;
template class QPrivatelyImplemented<QMailMessageBodyPrivate>;
template class QPrivatelyImplemented<QMailMessagePartContainerPrivate>;
template class QPrivatelyImplemented<QMailMessageMetaDataPrivate>;

template class QPrivatelyNoncopyable<QMailMessageSetContainerPrivate>;

template class QPrivatelyNoncopyable<QMailServiceActionPrivate>;
template class QPrivatelyNoncopyable<QMailRetrievalActionPrivate>;
template class QPrivatelyNoncopyable<QMailTransmitActionPrivate>;
template class QPrivatelyNoncopyable<QMailStorageActionPrivate>;
template class QPrivatelyNoncopyable<QMailSearchActionPrivate>;
template class QPrivatelyNoncopyable<QMailProtocolActionPrivate>;

Q_IMPLEMENT_USER_METATYPE_ENUM(QMailMessageBody::TransferEncoding)
Q_IMPLEMENT_USER_METATYPE_ENUM(QMailMessagePartContainer::MultipartType)
Q_IMPLEMENT_USER_METATYPE_ENUM(QMailMessage::MessageType)
Q_IMPLEMENT_USER_METATYPE_ENUM(QMailMessage::ContentType)
Q_IMPLEMENT_USER_METATYPE_ENUM(QMailMessage::ResponseType)
Q_IMPLEMENT_USER_METATYPE_ENUM(QMailMessage::AttachmentsAction)

Q_IMPLEMENT_USER_METATYPE(QMailMessage)
Q_IMPLEMENT_USER_METATYPE(QMailMessageMetaData)
Q_IMPLEMENT_USER_METATYPE(QMailMessagePart::Location)

Q_IMPLEMENT_USER_METATYPE_TYPEDEF(QMailMessageList, QMailMessageList)
Q_IMPLEMENT_USER_METATYPE_TYPEDEF(QMailMessageMetaDataList, QMailMessageMetaDataList)
Q_IMPLEMENT_USER_METATYPE_TYPEDEF(QMailMessageTypeList, QMailMessageTypeList)

Q_IMPLEMENT_USER_METATYPE(QMailServiceAction::Status)

Q_IMPLEMENT_USER_METATYPE_ENUM(QMailServiceAction::Connectivity)
Q_IMPLEMENT_USER_METATYPE_ENUM(QMailServiceAction::Activity)
Q_IMPLEMENT_USER_METATYPE_ENUM(QMailServiceAction::Status::ErrorCode)

Q_IMPLEMENT_USER_METATYPE_ENUM(QMailRetrievalAction::RetrievalSpecification)

Q_IMPLEMENT_USER_METATYPE_ENUM(QMailSearchAction::SearchSpecification)


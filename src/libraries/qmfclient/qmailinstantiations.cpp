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

#include "qmailmessage_p.h"
#include "qmailserviceaction_p.h"

#include "qprivateimplementationdef_p.h"

template class QPrivateImplementationPointer<QMailMessagePartContainerPrivate>;
template class QPrivatelyImplemented<QMailMessagePartContainerPrivate>;
template class QPrivateImplementationPointer<QMailMessageMetaDataPrivate>;
template class QPrivatelyImplemented<QMailMessageMetaDataPrivate>;

Q_IMPLEMENT_USER_METATYPE_ENUM(QMailMessageBody::TransferEncoding)
Q_IMPLEMENT_USER_METATYPE_ENUM(QMailMessagePartContainer::MultipartType)
Q_IMPLEMENT_USER_METATYPE_ENUM(QMailMessage::MessageType)
Q_IMPLEMENT_USER_METATYPE_ENUM(QMailMessage::ContentType)
Q_IMPLEMENT_USER_METATYPE_ENUM(QMailMessage::ResponseType)
Q_IMPLEMENT_USER_METATYPE_ENUM(QMailMessage::AttachmentsAction)

Q_IMPLEMENT_USER_METATYPE(QMailMessage)
Q_IMPLEMENT_USER_METATYPE(QMailMessageMetaData)
Q_IMPLEMENT_USER_METATYPE(QMailMessagePart)
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

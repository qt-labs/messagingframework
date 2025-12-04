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

Q_IMPLEMENT_USER_METATYPE_ENUM(QMailMessageBody::TransferEncoding)
Q_IMPLEMENT_USER_METATYPE_ENUM(QMailMessagePartContainer::MultipartType)
Q_IMPLEMENT_USER_METATYPE_ENUM(QMailMessage::MessageType)
Q_IMPLEMENT_USER_METATYPE_ENUM(QMailMessage::ContentCategory)
Q_IMPLEMENT_USER_METATYPE_ENUM(QMailMessage::ResponseType)
Q_IMPLEMENT_USER_METATYPE_ENUM(QMailMessage::AttachmentsAction)

Q_IMPLEMENT_USER_METATYPE(QMailMessage)
Q_IMPLEMENT_USER_METATYPE(QMailMessageMetaData)
Q_IMPLEMENT_USER_METATYPE(QMailMessagePart)
Q_IMPLEMENT_USER_METATYPE(QMailMessagePart::Location)

Q_IMPLEMENT_USER_METATYPE_NO_OPERATORS(QMailMessageList)
Q_IMPLEMENT_USER_METATYPE_NO_OPERATORS(QMailMessageMetaDataList)
Q_IMPLEMENT_USER_METATYPE_NO_OPERATORS(QMailMessageTypeList)

Q_IMPLEMENT_USER_METATYPE_NO_OPERATORS(QMailServiceAction::Status)

Q_IMPLEMENT_USER_METATYPE_NO_OPERATORS(QMailServiceAction::Connectivity)
Q_IMPLEMENT_USER_METATYPE_NO_OPERATORS(QMailServiceAction::Activity)
Q_IMPLEMENT_USER_METATYPE_NO_OPERATORS(QMailServiceAction::Status::ErrorCode)

Q_IMPLEMENT_USER_METATYPE_NO_OPERATORS(QMailRetrievalAction::RetrievalSpecification)

Q_IMPLEMENT_USER_METATYPE_NO_OPERATORS(QMailSearchAction::SearchSpecification)

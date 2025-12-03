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

#include "qmailmessageclassifier.h"
#include <qmailmessage.h>

/*!
    \class QMailMessageClassifier
    \inmodule QmfMessageServer

    \preliminary
    \brief The QMailMessageClassifier class provides a simple mechanism for determining the
    type of content contained by a message.

    QMailMessageClassifier inspects a message to determine what type of content it contains,
    according to the classification of \l{QMailMessageMetaData::ContentType}{QMailMessage::ContentType}.
*/

/*!
    Constructs a classifier object.
*/
QMailMessageClassifier::QMailMessageClassifier()
{
}

/*! \internal */
QMailMessageClassifier::~QMailMessageClassifier()
{
}

static QMailMessage::ContentType fromContentType(const QMailMessageContentType& contentType)
{
    QMailMessage::ContentType content = QMailMessage::UnknownContent;

    if (contentType.matches("text", "html")) {
        content = QMailMessage::HtmlContent;
    } else if (contentType.matches("text", "plain")) {
        content = QMailMessage::PlainTextContent;
    } else if (contentType.matches("text", "x-vcard")) {
        content = QMailMessage::VCardContent;
    } else if (contentType.matches("text", "x-vcalendar")) {
        content = QMailMessage::VCalendarContent;
    } else if (contentType.matches("image")) {
        content = QMailMessage::ImageContent;
    } else if (contentType.matches("audio")) {
        content = QMailMessage::AudioContent;
    } else if (contentType.matches("video")) {
        content = QMailMessage::VideoContent;
    }

    return content;
}

/*!
    Attempts to determine the type of content held within the message \a message, if it
    is currently set to \l{QMailMessageMetaData::UnknownContent}{QMailMessageMetaData::UnknownContent}.
    If the content type is determined, the message record is updated and true is returned.

    \sa QMailMessageMetaData::setContent()
*/
bool QMailMessageClassifier::classifyMessage(QMailMessage *message)
{
    if (message && message->content() == QMailMessage::UnknownContent) {
        QMailMessagePartContainer::MultipartType multipartType(message->multipartType());
        QMailMessageContentType contentType(message->contentType());

        // The content type is used to categorise the message more narrowly than
        // its transport categorisation
        QMailMessage::ContentType content = QMailMessage::UnknownContent;

        switch (message->messageType()) {
        case QMailMessage::Sms:
            content = fromContentType(contentType);
            if (content == QMailMessage::UnknownContent) {
                if (message->hasBody()) {
                    // Assume plain text
                    content = QMailMessage::PlainTextContent;
                } else {
                    // No content in this message beside the meta data
                    content = QMailMessage::NoContent;
                }
            }
            break;

        case QMailMessage::Mms:
            if (multipartType == QMailMessagePartContainer::MultipartNone) {
                content = fromContentType(contentType);
                if (content == QMailMessage::UnknownContent) {
                    if (contentType.matches("text")) {
                        // Assume some type of richer-than-plain text
                        content = QMailMessage::RichTextContent;
                    }
                }
            } else {
                if (multipartType == QMailMessagePartContainer::MultipartRelated) {
                    // Assume SMIL for now - we should test for 'application/smil' somewhere...
                    content = QMailMessage::SmilContent;
                } else {
                    content = QMailMessage::MultipartContent;
                }
            }
            break;

        case QMailMessage::Email:
            if (multipartType == QMailMessagePartContainer::MultipartNone) {
                content = fromContentType(contentType);
                if (content == QMailMessage::UnknownContent) {
                    if (contentType.matches("text")) {
                        // Assume some type of richer-than-plain text
                        content = QMailMessage::RichTextContent;
                    }
                }
            } else {
                // TODO: Much more goes here...
                content = QMailMessage::MultipartContent;
            }
            break;

        case QMailMessage::System:
            content = QMailMessage::RichTextContent;
            break;

        default:
            break;
        }

        if (content != QMailMessage::UnknownContent) {
            message->setContent(content);
            return true;
        }
    }

    return false;
}

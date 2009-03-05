/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#include "qmailmessageclassifier.h"
#include <qmailmessage.h>
#include <QSettings>


/*!
    \class QMailMessageClassifier
    \inpublicgroup QtMessagingModule
    \ingroup libmessageserver

    \preliminary
    \brief The QMailMessageClassifier class provides a simple mechanism for determining the
    type of content contained by a message.

    QMailMessageClassifier inspects a message to determine what type of content it contains, 
    according to the classification of \l{QMailMessageMetaDataFwd::ContentType}{QMailMessage::ContentType}.

    Messages of type \l{QMailMessageMetaDataFwd::Email}{QMailMessage::Email} may be classified as having 
    \l{QMailMessageMetaDataFwd::VoicemailContent}{QMailMessage::VoicemailContent} or 
    \l{QMailMessageMetaDataFwd::VideomailContent}{QMailMessage::VideomailContent} content if their 
    \l{QMailMessage::from()} address matches any of those configured in the \c{Trolltech/messageserver.conf} file.
*/

/*!
    Constructs a classifier object.
*/
QMailMessageClassifier::QMailMessageClassifier()
{
    QSettings settings("Trolltech", "messageserver");

    settings.beginGroup("global");

    int count = settings.beginReadArray("voicemail");
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        voiceMailAddresses.append(settings.value("address").toString());
    }
    settings.endArray();

    count = settings.beginReadArray("videomail");
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        videoMailAddresses.append(settings.value("address").toString());
    }
    settings.endArray();

    settings.endGroup();
}

/*! \internal */
QMailMessageClassifier::~QMailMessageClassifier()
{
}

static QMailMessage::ContentType fromContentType(const QMailMessageContentType& contentType)
{
    QString type(contentType.type().toLower());
    QString subtype(contentType.subType().toLower());

    QMailMessage::ContentType content = QMailMessage::UnknownContent;

    if (type == "text") {
        if (subtype == "html") {
            content = QMailMessage::HtmlContent;
        } else if (subtype == "plain") {
            content = QMailMessage::PlainTextContent;
        } else if (subtype == "x-vcard") {
            content = QMailMessage::VCardContent;
        } else if (subtype == "x-vcalendar") {
            content = QMailMessage::VCalendarContent;
        }
    } else if (contentType.type().toLower() == "image") {
        content = QMailMessage::ImageContent;
    } else if (contentType.type().toLower() == "audio") {
        content = QMailMessage::AudioContent;
    } else if (contentType.type().toLower() == "video") {
        content = QMailMessage::VideoContent;
    }

    return content;
}

/*!
    Attempts to determine the type of content held within the message described by \a metaData, 
    if it is currently set to \l{QMailMessageMetaDataFwd::UnknownContent}{QMailMessageMetaData::UnknownContent}.
    If the content type is determined, the message metadata record is updated and true is returned.

    \sa QMailMessageMetaData::setContent()
*/
bool QMailMessageClassifier::classifyMessage(QMailMessageMetaData& metaData)
{
    if (metaData.content() == QMailMessage::UnknownContent) {
        QMailMessage::ContentType content = QMailMessage::UnknownContent;

        switch (metaData.messageType()) {
        case QMailMessage::Email:
            // Handle voicemail emails, from pre-configured addresses
            if (voiceMailAddresses.contains(metaData.from().address())) {
                content = QMailMessage::VoicemailContent;
            } else if(videoMailAddresses.contains(metaData.from().address())) {
                content = QMailMessage::VideomailContent;
            }
            break;

        default:
            break;
        }

        if ((content != metaData.content()) && (content != QMailMessage::UnknownContent)) {
            metaData.setContent(content);
            return true;
        }
    }

    return false;
}

/*!
    Attempts to determine the type of content held within the message \a message, if it
    is currently set to \l{QMailMessageMetaDataFwd::UnknownContent}{QMailMessageMetaData::UnknownContent}.
    If the content type is determined, the message record is updated and true is returned.

    \sa QMailMessageMetaData::setContent()
*/
bool QMailMessageClassifier::classifyMessage(QMailMessage& message)
{
    if (message.content() == QMailMessage::UnknownContent) {
        QMailMessagePartContainer::MultipartType multipartType(message.multipartType());
        QMailMessageContentType contentType(message.contentType());

        // The content type is used to categorise the message more narrowly than 
        // its transport categorisation
        QMailMessage::ContentType content = QMailMessage::UnknownContent;

        switch (message.messageType()) {
        case QMailMessage::Sms:
            content = fromContentType(contentType);
            if (content == QMailMessage::UnknownContent) {
                if (message.hasBody()) {
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
                    if (contentType.type().toLower() == "text") {
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
                    if (contentType.type().toLower() == "text") {
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
            message.setContent(content);
            return true;
        }
    }

    return false;
}


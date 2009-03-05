/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/
#ifndef QMAILMESSAGEFWD_H
#define QMAILMESSAGEFWD_H

#include "qmailglobal.h"

class QMailMessageHeaderField;

class QTOPIAMAIL_EXPORT QMailMessageHeaderFieldFwd
{
public:
    enum FieldType
    {
        StructuredField = 1,
        UnstructuredField = 2
    };
};

class QMailMessageContentDisposition;

class QTOPIAMAIL_EXPORT QMailMessageContentDispositionFwd
{
public:
    enum DispositionType
    {
        None = 0,
        Inline = 1,
        Attachment = 2
    };
};

class QMailMessageBody;

class QTOPIAMAIL_EXPORT QMailMessageBodyFwd
{
public:
    enum TransferEncoding 
    {
        NoEncoding = 0,
        SevenBit = 1, 
        EightBit = 2, 
        Base64 = 3,
        QuotedPrintable = 4,
        Binary = 5, 
    };

    enum EncodingStatus
    {
        AlreadyEncoded = 1,
        RequiresEncoding = 2
    };

    enum EncodingFormat
    {
        Encoded = 1,
        Decoded = 2
    };
};

class QMailMessagePartContainer;

class QTOPIAMAIL_EXPORT QMailMessagePartContainerFwd
{
public:
    enum MultipartType 
    {
        MultipartNone = 0,
        MultipartSigned = 1,
        MultipartEncrypted = 2,
        MultipartMixed = 3,
        MultipartAlternative = 4,
        MultipartDigest = 5,
        MultipartParallel = 6,
        MultipartRelated = 7,
        MultipartFormData = 8,
        MultipartReport = 9
    };
};

class QMailMessagePart;

class QTOPIAMAIL_EXPORT QMailMessagePartFwd
{
public:
    enum ReferenceType {
        None = 0,
        MessageReference,
        PartReference
    };
};
        
class QMailMessageMetaData;

class QTOPIAMAIL_EXPORT QMailMessageMetaDataFwd
{
public:
    enum MessageType
    {
        Mms     = 0x1,
        // was: Ems = 0x2
        Sms     = 0x4,
        Email   = 0x8,
        System  = 0x10,
        Instant = 0x20,
        None    = 0,
        AnyType = Mms | Sms | Email | System | Instant
    };

    enum ContentType {
        UnknownContent        = 0,
        NoContent             = 1,
        PlainTextContent      = 2,
        RichTextContent       = 3,
        HtmlContent           = 4,
        ImageContent          = 5,
        AudioContent          = 6,
        VideoContent          = 7,
        MultipartContent      = 8,
        SmilContent           = 9,
        VoicemailContent      = 10,
        VideomailContent      = 11,
        VCardContent          = 12,
        VCalendarContent      = 13,
        ICalendarContent      = 14,
        DeliveryReportContent = 15,
        UserContent           = 64
    };

    enum ResponseType {
        NoResponse          = 0,
        Reply               = 1, 
        ReplyToAll          = 2,
        Forward             = 3,
        ForwardPart         = 4,
        Redirect            = 5
    };
};

class QMailMessage;

class QTOPIAMAIL_EXPORT QMailMessageFwd
{
public:
    enum AttachmentsAction {
        LinkToAttachments = 0,
        CopyAttachments,
        CopyAndDeleteAttachments
    };

    enum EncodingFormat
    {
        HeaderOnlyFormat = 1,
        StorageFormat = 2,
        TransmissionFormat = 3,
        IdentityFormat = 4,
    }; 
};

#endif


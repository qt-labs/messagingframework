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

#ifndef QMAILMESSAGE_H
#define QMAILMESSAGE_H

#include "qmailaddress.h"
#include "qmailid.h"
#include "qmailtimestamp.h"
#include "qprivateimplementation.h"

#include <QByteArray>
#include <QFlags>
#include <QList>
#include <QPair>
#include <QString>
#include <QStringList>
#include <QDebug>

class QMailMessagePart;
class QMailMessagePartContainerPrivate;

QT_BEGIN_NAMESPACE

class QDataStream;
class QTextStream;

QT_END_NAMESPACE

class QMailMessageHeaderFieldPrivate;

class QMF_EXPORT QMailMessageHeaderField : public QPrivatelyImplemented<QMailMessageHeaderFieldPrivate>
{
public:
    enum FieldType {
        StructuredField = 1,
        UnstructuredField = 2
    };

    typedef QMailMessageHeaderFieldPrivate ImplementationType;

    typedef QPair<QByteArray, QByteArray> ParameterType;

    QMailMessageHeaderField();
    QMailMessageHeaderField(const QByteArray& text, FieldType fieldType = StructuredField);
    QMailMessageHeaderField(const QByteArray& name, const QByteArray& text, FieldType fieldType = StructuredField);

    bool operator==(const QMailMessageHeaderField &other) const;

    bool isNull() const;

    QByteArray id() const;
    virtual void setId(const QByteArray& text);

    QByteArray content() const;
    void setContent(const QByteArray& text);

    QByteArray parameter(const QByteArray& name) const;
    void setParameter(const QByteArray& name, const QByteArray& value);

    bool isParameterEncoded(const QByteArray& name) const;
    void setParameterEncoded(const QByteArray& name);

    QList<ParameterType> parameters() const;

    virtual QByteArray toString(bool includeName = true, bool presentable = true) const;

    virtual QString decodedContent() const;

    static QByteArray encodeWord(const QString& input, const QByteArray& charset = "");
    static QString decodeWord(const QByteArray& input);

    static QByteArray encodeParameter(const QString& input, const QByteArray& charset = "", const QByteArray& language = "");
    static QString decodeParameter(const QByteArray& input);

    static QByteArray encodeContent(const QString& input, const QByteArray& charset = "");
    static QString decodeContent(const QByteArray& input);

    static QByteArray removeComments(const QByteArray& input);
    static QByteArray removeWhitespace(const QByteArray& input);

    template <typename Stream> void serialize(Stream &stream) const;
    template <typename Stream> void deserialize(Stream &stream);

protected:
    void parse(const QByteArray& text, FieldType fieldType);

private:
    friend class QMailMessageHeaderFieldPrivate;
    friend class QMailMessageHeaderPrivate;

    void output(QDataStream& out) const;
};

template <typename Stream>
Stream& operator<<(Stream &stream, const QMailMessageHeaderField& field) { field.serialize(stream); return stream; }

template <typename Stream>
Stream& operator>>(Stream &stream, QMailMessageHeaderField& field) { field.deserialize(stream); return stream; }


class QMF_EXPORT QMailMessageContentType : public QMailMessageHeaderField
{
public:
    QMailMessageContentType();
    QMailMessageContentType(const QByteArray& type);
    QMailMessageContentType(const QMailMessageHeaderField& field);

    QByteArray type() const;
    void setType(const QByteArray& type);

    QByteArray subType() const;
    void setSubType(const QByteArray& subType);

    QByteArray name() const;
    void setName(const QByteArray& name);

    QByteArray boundary() const;
    void setBoundary(const QByteArray& boundary);

    QByteArray charset() const;
    void setCharset(const QByteArray& charset);

    bool matches(const QByteArray& primary, const QByteArray& sub = QByteArray()) const;

    // can't be used. Class is about fixed type.
    void setId(const QByteArray& text) override;
};


class QMF_EXPORT QMailMessageContentDisposition : public QMailMessageHeaderField
{
public:
    enum DispositionType {
        None = 0,
        Inline = 1,
        Attachment = 2
    };

    QMailMessageContentDisposition();
    QMailMessageContentDisposition(const QByteArray& type);
    QMailMessageContentDisposition(DispositionType disposition);
    QMailMessageContentDisposition(const QMailMessageHeaderField& field);

    DispositionType type() const;
    void setType(DispositionType disposition);

    QByteArray filename() const;
    void setFilename(const QByteArray& filename);

    QMailTimeStamp creationDate() const;
    void setCreationDate(const QMailTimeStamp& timeStamp);

    QMailTimeStamp modificationDate() const;
    void setModificationDate(const QMailTimeStamp& timeStamp);

    QMailTimeStamp readDate() const;
    void setReadDate(const QMailTimeStamp& timeStamp);

    int size() const;
    void setSize(int size);

    // can't be used. Class is about fixed type.
    void setId(const QByteArray &text) override;
};


class QMailMessageHeaderPrivate;

// This class is not exposed to clients:
class QMailMessageHeader : public QPrivatelyImplemented<QMailMessageHeaderPrivate>
{
public:
    typedef QMailMessageHeaderPrivate ImplementationType;

    QMailMessageHeader();
    QMailMessageHeader(const QByteArray& input);

    void update(const QByteArray& id, const QByteArray& content);
    void append(const QByteArray& id, const QByteArray& content);
    void remove(const QByteArray& id);

    QMailMessageHeaderField field(const QByteArray& id) const;
    QList<QMailMessageHeaderField> fields(const QByteArray& id) const;

    QList<const QByteArray*> fieldList() const;

    template <typename Stream> void serialize(Stream &stream) const;
    template <typename Stream> void deserialize(Stream &stream);

private:
    friend class QMailMessageHeaderPrivate;
    friend class QMailMessagePartContainerPrivate;
    friend class QMailMessagePartPrivate;
    friend class QMailMessagePrivate;

    void output(QDataStream& out, const QList<QByteArray>& exclusions, bool stripInternal) const;
};

template <typename Stream>
Stream& operator<<(Stream &stream, const QMailMessageHeader& header) { header.serialize(stream); return stream; }

template <typename Stream>
Stream& operator>>(Stream &stream, QMailMessageHeader& header) { header.deserialize(stream); return stream; }


class QMailMessageBodyPrivate;
class LongString;

class QMF_EXPORT QMailMessageBody : public QPrivatelyImplemented<QMailMessageBodyPrivate>
{
public:
    enum TransferEncoding {
        NoEncoding = 0,
        SevenBit = 1,
        EightBit = 2,
        Base64 = 3,
        QuotedPrintable = 4,
        Binary = 5
    };

    enum EncodingStatus {
        AlreadyEncoded = 1,
        RequiresEncoding = 2
    };

    enum EncodingFormat {
        None = 0,
        Encoded = 1,
        Decoded = 2
    };

    typedef QMailMessageBodyPrivate ImplementationType;

    // Construction functions
    static QMailMessageBody fromFile(const QString& filename, const QMailMessageContentType& type, TransferEncoding encoding, EncodingStatus status);

    static QMailMessageBody fromStream(QDataStream& in, const QMailMessageContentType& type, TransferEncoding encoding, EncodingStatus status);
    static QMailMessageBody fromData(const QByteArray& input, const QMailMessageContentType& type, TransferEncoding encoding, EncodingStatus status);

    static QMailMessageBody fromStream(QTextStream& in, const QMailMessageContentType& type, TransferEncoding encoding);
    static QMailMessageBody fromData(const QString& input, const QMailMessageContentType& type, TransferEncoding encoding);

    // Output functions
    bool toFile(const QString& filename, EncodingFormat format) const;

    QByteArray data(EncodingFormat format) const;
    bool toStream(QDataStream& out, EncodingFormat format) const;

    QString data() const;
    bool toStream(QTextStream& out) const;

    // Property accessors
    TransferEncoding transferEncoding() const;
    QMailMessageContentType contentType() const;

    bool isEmpty() const;
    int length() const;

    template <typename Stream> void serialize(Stream &stream) const;
    template <typename Stream> void deserialize(Stream &stream);

private:
    friend class QMailMessagePartContainerPrivate;

    QMailMessageBody();

    uint indicativeSize() const;

    bool encoded() const;
    void setEncoded(bool value);

    void output(QDataStream& out, bool includeAttachments) const;

    static QMailMessageBody fromLongString(LongString& ls, const QMailMessageContentType& type, TransferEncoding encoding, EncodingStatus status);
};

template <typename Stream>
Stream& operator<<(Stream &stream, const QMailMessageBody& body) { body.serialize(stream); return stream; }

template <typename Stream>
Stream& operator>>(Stream &stream, QMailMessageBody& body) { body.deserialize(stream); return stream; }

class QMF_EXPORT QMailMessagePartContainer : public QPrivatelyImplemented<QMailMessagePartContainerPrivate>
{
public:
    enum MultipartType {
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

    typedef QMailMessagePartContainerPrivate ImplementationType;
    class LocationPrivate;

    class QMF_EXPORT Location
    {
    public:
        Location();
        Location(const QString& description);
        Location(const Location& other);

        ~Location();

        const QMailMessagePartContainer::Location &operator=(const QMailMessagePartContainer::Location &other);
        bool operator==(const QMailMessagePartContainer::Location &other) const;
        bool operator!=(const QMailMessagePartContainer::Location &other) const;

        bool isValid(bool extended = true) const;

        QMailMessageId containingMessageId() const;
        void setContainingMessageId(const QMailMessageId &id);

        QString toString(bool extended) const;

        template <typename Stream> void serialize(Stream &stream) const;
        template <typename Stream> void deserialize(Stream &stream);

    private:
        friend class QMailMessagePartContainerPrivate;
        friend class QMailMessagePart;

        Location(const QMailMessagePart& part);

        LocationPrivate *d;
    };

    // Parts management interface:
    MultipartType multipartType() const;
    void setMultipartType(MultipartType type, const QList<QMailMessageHeaderField::ParameterType> &parameters = QList<QMailMessageHeaderField::ParameterType>());

    uint partCount() const;

    void appendPart(const QMailMessagePart &part);
    void prependPart(const QMailMessagePart &part);

    void removePartAt(uint pos);

    const QMailMessagePart& partAt(uint pos) const;
    QMailMessagePart& partAt(uint pos);

    void clearParts();

    QByteArray boundary() const;
    void setBoundary(const QByteArray& text);

    QString contentDescription() const;
    void setContentDescription(const QString &s);

    QMailMessageContentDisposition contentDisposition() const;
    void setContentDisposition(const QMailMessageContentDisposition& disposition);

    // Body management interface:
    void setBody(const QMailMessageBody& body, QMailMessageBody::EncodingFormat encodingStatus = QMailMessageBody::None);
    QMailMessageBody body() const;

    bool hasBody() const;

    // Property accessors
    QMailMessageBody::TransferEncoding transferEncoding() const;
    QMailMessageContentType contentType() const;

    // Header fields describing this part container
    QString headerFieldText( const QString &id ) const;
    QMailMessageHeaderField headerField( const QString &id, QMailMessageHeaderField::FieldType fieldType = QMailMessageHeaderField::StructuredField ) const;

    QStringList headerFieldsText( const QString &id ) const;
    QList<QMailMessageHeaderField> headerFields( const QString &id, QMailMessageHeaderField::FieldType fieldType = QMailMessageHeaderField::StructuredField ) const;

    QList<QMailMessageHeaderField> headerFields() const;

    virtual void setHeaderField( const QString &id, const QString& content );
    virtual void setHeaderField( const QMailMessageHeaderField &field );

    virtual void appendHeaderField( const QString &id, const QString& content );
    virtual void appendHeaderField( const QMailMessageHeaderField &field );

    virtual void removeHeaderField( const QString &id );

    virtual bool contentAvailable() const = 0;
    virtual bool partialContentAvailable() const = 0;

    template <typename F>
    bool foreachPart(F func);

    template <typename F>
    bool foreachPart(F func) const;

    static MultipartType multipartTypeForName(const QByteArray &name);
    static QByteArray nameForMultipartType(MultipartType type);

    QMailMessagePartContainer* findPlainTextContainer() const;
    QMailMessagePartContainer* findHtmlContainer() const;
    QList<QMailMessagePartContainer::Location> findAttachmentLocations() const;
    QList<QMailMessagePartContainer::Location> findInlineImageLocations() const;
    QList<QMailMessagePartContainer::Location> findInlinePartLocations() const;
    bool hasPlainTextBody() const;
    bool hasHtmlBody() const;
    bool hasAttachments() const;
    bool isEncrypted() const;
    void setPlainTextBody(const QMailMessageBody& plainTextBody);
    void setHtmlAndPlainTextBody(const QMailMessageBody& htmlBody, const QMailMessageBody& plainTextBody);
    void setInlineImages(const QMap<QString, QString> &htmlImagesMap);
    void setInlineImages(const QList<const QMailMessagePart*> images);
    void setAttachments(const QStringList& attachments);
    void setAttachments(const QList<const QMailMessagePart*> attachments);
    void addAttachments(const QStringList& attachments);

protected:
    template<typename Subclass>
    QMailMessagePartContainer(Subclass* p);

    virtual void setHeader(const QMailMessageHeader& header, const QMailMessagePartContainerPrivate* parent = Q_NULLPTR);

private:
    friend class QMailMessagePartContainerPrivate;

    uint indicativeSize() const;

    void outputParts(QDataStream& out, bool includePreamble, bool includeAttachments, bool stripInternal) const;
    void outputBody(QDataStream& out, bool includeAttachments) const;
};

class QMailMessagePartPrivate;

class QMF_EXPORT QMailMessagePart : public QMailMessagePartContainer
{
public:
    enum ReferenceType {
        None = 0,
        MessageReference,
        PartReference
    };

    typedef QMailMessagePartPrivate ImplementationType;

    QMailMessagePart();

    // Construction functions
    static QMailMessagePart fromFile(const QString& filename, const QMailMessageContentDisposition& disposition,
                                     const QMailMessageContentType& type, QMailMessageBody::TransferEncoding encoding,
                                     QMailMessageBody::EncodingStatus status = QMailMessageBody::RequiresEncoding);

    static QMailMessagePart fromStream(QDataStream& in, const QMailMessageContentDisposition& disposition,
                                       const QMailMessageContentType& type, QMailMessageBody::TransferEncoding encoding,
                                       QMailMessageBody::EncodingStatus status = QMailMessageBody::RequiresEncoding);
    static QMailMessagePart fromData(const QByteArray& input, const QMailMessageContentDisposition& disposition,
                                     const QMailMessageContentType& type, QMailMessageBody::TransferEncoding encoding,
                                     QMailMessageBody::EncodingStatus status = QMailMessageBody::RequiresEncoding);

    static QMailMessagePart fromStream(QTextStream& in, const QMailMessageContentDisposition& disposition,
                                       const QMailMessageContentType& type, QMailMessageBody::TransferEncoding encoding);
    static QMailMessagePart fromData(const QString& input, const QMailMessageContentDisposition& disposition,
                                     const QMailMessageContentType& type, QMailMessageBody::TransferEncoding encoding);

    static QMailMessagePart fromMessageReference(const QMailMessageId &id, const QMailMessageContentDisposition& disposition,
                                                 const QMailMessageContentType& type, QMailMessageBody::TransferEncoding encoding);
    static QMailMessagePart fromPartReference(const QMailMessagePart::Location &partLocation, const QMailMessageContentDisposition& disposition,
                                              const QMailMessageContentType& type, QMailMessageBody::TransferEncoding encoding);

    void setReference(const QMailMessageId &id, const QMailMessageContentType& type, QMailMessageBody::TransferEncoding encoding);
    void setReference(const QMailMessagePart::Location &location, const QMailMessageContentType& type, QMailMessageBody::TransferEncoding encoding);

    QString contentID() const;
    void setContentID(const QString &s);

    QString contentLocation() const;
    void setContentLocation(const QString &s);

    QString contentLanguage() const;
    void setContentLanguage(const QString &s);

    int partNumber() const;

    Location location() const;

    QString displayName() const;
    QString identifier() const;

    ReferenceType referenceType() const;

    QMailMessageId messageReference() const;
    QMailMessagePart::Location partReference() const;

    QString referenceResolution() const;
    void setReferenceResolution(const QString &uri);

    virtual uint indicativeSize() const;

    bool contentAvailable() const override;
    bool partialContentAvailable() const override;

    QString writeBodyTo(const QString &path) const;

    virtual bool contentModified() const;

    template <typename Stream> void serialize(Stream &stream) const;
    template <typename Stream> void deserialize(Stream &stream);

    QByteArray toRfc2822() const;

    // Undecoded data handling:
    bool hasUndecodedData() const;
    QByteArray undecodedData() const;
    void setUndecodedData(const QByteArray &data);
    void appendUndecodedData(const QByteArray &data);

private:
    friend class QMailMessagePrivate;
    friend class QMailMessagePartContainerPrivate;

    virtual void setUnmodified();

    void output(QDataStream& out, bool includeAttachments, bool stripInternal) const;
};

template <typename Stream>
Stream& operator<<(Stream &stream, const QMailMessagePart& part) { part.serialize(stream); return stream; }

template <typename Stream>
Stream& operator>>(Stream &stream, QMailMessagePart& part) { part.deserialize(stream); return stream; }

template <typename F>
bool QMailMessagePartContainer::foreachPart(F func)
{
    for (uint i = 0; i < partCount(); ++i) {
        QMailMessagePart &part(partAt(i));

        if (!func(part)) {
            return false;
        } else if (part.multipartType() != QMailMessagePartContainer::MultipartNone) {
            if (!part.foreachPart<F>(func)) {
                return false;
            }
        }
    }

    return true;
}

template <typename F>
bool QMailMessagePartContainer::foreachPart(F func) const
{
    for (uint i = 0; i < partCount(); ++i) {
        const QMailMessagePart &part(partAt(i));

        if (!func(part)) {
            return false;
        } else if (part.multipartType() != QMailMessagePartContainer::MultipartNone) {
            if (!part.foreachPart<F>(func)) {
                return false;
            }
        }
    }

    return true;
}

class QMailMessageMetaDataPrivate;

class QMF_EXPORT QMailMessageMetaData : public QPrivatelyImplemented<QMailMessageMetaDataPrivate>
{
public:
    enum MessageType {
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
        Redirect            = 5,
        UnspecifiedResponse = 6
    };

    typedef QMailMessageMetaDataPrivate ImplementationType;

    static const quint64 &Incoming;
    static const quint64 &Outgoing;
    static const quint64 &Sent;
    static const quint64 &Replied;
    static const quint64 &RepliedAll;
    static const quint64 &Forwarded;
    static const quint64 &ContentAvailable;
    static const quint64 &Read;
    static const quint64 &Removed;
    static const quint64 &ReadElsewhere;
    static const quint64 &UnloadedData;
    static const quint64 &New;
    static const quint64 &ReadReplyRequested;
    static const quint64 &Trash;
    static const quint64 &PartialContentAvailable;
    static const quint64 &HasAttachments;
    static const quint64 &HasReferences;
    static const quint64 &HasSignature;
    static const quint64 &HasEncryption;
    static const quint64 &HasUnresolvedReferences;
    static const quint64 &Draft;
    static const quint64 &Outbox;
    static const quint64 &Junk;
    static const quint64 &TransmitFromExternal;
    static const quint64 &LocalOnly;
    static const quint64 &Temporary;
    static const quint64 &ImportantElsewhere;
    static const quint64 &Important;
    static const quint64 &HighPriority;
    static const quint64 &LowPriority;
    static const quint64 &CalendarInvitation;
    static const quint64 &Todo;
    static const quint64 &NoNotification;
    static const quint64 &CalendarCancellation;

    QMailMessageMetaData();
    QMailMessageMetaData(const QMailMessageId& id);
    QMailMessageMetaData(const QString& uid, const QMailAccountId& accountId);

    virtual QMailMessageId id() const;
    virtual void setId(const QMailMessageId &id);

    virtual QMailFolderId parentFolderId() const;
    virtual void setParentFolderId(const QMailFolderId &id);

    virtual MessageType messageType() const;
    virtual void setMessageType(MessageType t);

    virtual QMailAddress from() const;
    virtual void setFrom(const QMailAddress &s);

    virtual QString subject() const;
    virtual void setSubject(const QString &s);

    virtual QMailTimeStamp date() const;
    virtual void setDate(const QMailTimeStamp &s);

    virtual QMailTimeStamp receivedDate() const;
    virtual void setReceivedDate(const QMailTimeStamp &s);

    virtual QList<QMailAddress> recipients() const;

    virtual quint64 status() const;
    virtual void setStatus(quint64 newStatus);
    virtual void setStatus(quint64 mask, bool set);

    virtual QMailAccountId parentAccountId() const;
    virtual void setParentAccountId(const QMailAccountId& id);

    virtual QString serverUid() const;
    virtual void setServerUid(const QString &s);

    virtual uint size() const;
    virtual void setSize(uint i);

    virtual uint indicativeSize() const;

    virtual ContentType content() const;
    virtual void setContent(ContentType type);

    virtual QMailFolderId previousParentFolderId() const;
    virtual void setPreviousParentFolderId(const QMailFolderId &id);

    virtual QString contentScheme() const;
    virtual bool setContentScheme(const QString &s);

    virtual QString contentIdentifier() const;
    virtual bool setContentIdentifier(const QString &i);

    virtual QMailMessageId inResponseTo() const;
    virtual void setInResponseTo(const QMailMessageId &id);

    virtual ResponseType responseType() const;
    virtual void setResponseType(ResponseType type);

    virtual QString preview() const;
    virtual void setPreview(const QString &s);

    virtual bool contentAvailable() const;
    virtual bool partialContentAvailable() const;

    QString customField(const QString &name) const;
    void setCustomField(const QString &name, const QString &value);
    void setCustomFields(const QMap<QString, QString> &fields);

    void removeCustomField(const QString &name);

    const QMap<QString, QString> &customFields() const;

    virtual bool dataModified() const;

    virtual QString copyServerUid() const;
    virtual void setCopyServerUid(const QString &s);

    virtual QMailFolderId restoreFolderId() const;
    virtual void setRestoreFolderId(const QMailFolderId &s);

    virtual QString listId() const;
    virtual void setListId(const QString &s);

    virtual QString rfcId() const;
    virtual void setRfcId(const QString &s);

    virtual QMailThreadId parentThreadId() const;
    virtual void setParentThreadId(const QMailThreadId &id);

    static quint64 statusMask(const QString &flagName);

    template <typename Stream> void serialize(Stream &stream) const;
    template <typename Stream> void deserialize(Stream &stream);

protected:
    virtual void setRecipients(const QList<QMailAddress>& s);
    virtual void setRecipients(const QMailAddress& s);

private:
    friend class QMailMessage;
    friend class QMailMessagePrivate;
    friend class QMailStore;
    friend class QMailStorePrivate;
    friend class QMailStoreSql;

    virtual void setUnmodified();

    bool customFieldsModified() const;
    void setCustomFieldsModified(bool set);
};

class QMailMessagePrivate;

class QMF_EXPORT QMailMessage : public QMailMessageMetaData, public QMailMessagePartContainer
{
public:
    enum AttachmentsAction {
        LinkToAttachments = 0,
        CopyAttachments,
        CopyAndDeleteAttachments
    };

    enum EncodingFormat {
        HeaderOnlyFormat = 1,
        StorageFormat = 2,
        TransmissionFormat = 3,
        IdentityFormat = 4
    };

    enum ChunkType {
        Text = 0,
        Reference
    };

    enum DispositionNotificationMode {
        Manual = 0,
        Automatic
    };

    enum DispositionNotificationType {
        Displayed = 0,
        Deleted,
        Dispatched,
        Processed
    };

    typedef QPair<ChunkType, QByteArray> MessageChunk;

    using QMailMessageMetaData::MessageType;
    using QMailMessageMetaData::ContentType;
    using QMailMessageMetaData::ResponseType;

    static QMailMessage fromRfc2822(const QByteArray &ba);
    static QMailMessage fromRfc2822File(const QString& fileName);
    static QMailMessage fromSkeletonRfc2822File(const QString& fileName);
    static QMailMessage asReadReceipt(const QMailMessage& message, const QString &bodyText,
                                      const QString &subjectPrefix = QString(),
                                      const QString &reportingUA = QString(),
                                      DispositionNotificationMode mode = Manual,
                                      DispositionNotificationType type = Displayed);

    QMailMessage();
    QMailMessage(const QMailMessageId& id);
    QMailMessage(const QString& uid, const QMailAccountId& accountId);

    QByteArray toRfc2822(EncodingFormat format = TransmissionFormat) const;
    void toRfc2822(QDataStream& out, EncodingFormat format = TransmissionFormat) const;

    QList<QMailMessage::MessageChunk> toRfc2822Chunks(EncodingFormat format = TransmissionFormat) const;

    QMailAddress readReceiptRequestAddress() const;
    void requestReadReceipt(const QMailAddress &address);

    using QMailMessagePartContainer::partAt;

    bool contains(const QMailMessagePart::Location& location) const;
    const QMailMessagePart& partAt(const QMailMessagePart::Location& location) const;
    QMailMessagePart& partAt(const QMailMessagePart::Location& location);

    // Overrides of QMMPC functions where the data needs to be stored to the meta data also:

    void setHeaderField( const QString &id, const QString& content ) override;
    void setHeaderField( const QMailMessageHeaderField &field ) override;

    void appendHeaderField( const QString &id, const QString& content ) override;
    void appendHeaderField( const QMailMessageHeaderField &field ) override;

    void removeHeaderField( const QString &id ) override;

    // Overrides of QMMMD functions where the data needs to be stored to the part container also:

    void setId(const QMailMessageId &id) override;

    void setFrom(const QMailAddress &s) override;

    void setSubject(const QString &s) override;

    void setDate(const QMailTimeStamp &s) override;

    virtual QList<QMailAddress> to() const;
    virtual void setTo(const QList<QMailAddress>& s);
    virtual void setTo(const QMailAddress& s);

    uint indicativeSize() const override;

    // Convenience functions:

    virtual QList<QMailAddress> cc() const;
    virtual void setCc(const QList<QMailAddress>& s);
    virtual QList<QMailAddress> bcc() const;
    virtual void setBcc(const QList<QMailAddress>& s);

    QList<QMailAddress> recipients() const override;
    virtual bool hasRecipients() const;

    virtual QMailAddress replyTo() const;
    virtual void setReplyTo(const QMailAddress &s);

    virtual QString inReplyTo() const;
    virtual void setInReplyTo(const QString &s);

    void setReplyReferences(const QMailMessage &msg);

    virtual uint contentSize() const;
    virtual void setContentSize(uint size);

    virtual QString externalLocationReference() const;
    virtual void setExternalLocationReference(const QString &s);

    bool contentAvailable() const override;
    bool partialContentAvailable() const override;

    virtual bool hasCalendarInvitation() const;
    virtual bool hasCalendarCancellation() const;

    virtual bool contentModified() const;

    QString preview() const override;

    template <typename Stream> void serialize(Stream &stream) const;
    template <typename Stream> void deserialize(Stream &stream);

protected:
    void setHeader(const QMailMessageHeader& header, const QMailMessagePartContainerPrivate* parent = Q_NULLPTR) override;

private:
    friend class QMailStore;
    friend class QMailStorePrivate;
    friend class QMailStoreSql;

    QMailMessageMetaDataPrivate* metaDataImpl();
    const QMailMessageMetaDataPrivate* metaDataImpl() const;

    QMailMessagePrivate* partContainerImpl();
    const QMailMessagePrivate* partContainerImpl() const;

    virtual bool hasCalendarMethod(QByteArray const &method) const;
    void setUnmodified() override;

    QByteArray duplicatedData(const QString&) const;
    void updateMetaData(const QByteArray& id, const QString& value);

    bool extractUndecodedData(const LongString& ls);
    void refreshPreview();

    static QMailMessage fromRfc2822(LongString& ls);
};

QMF_EXPORT QDebug operator<<(QDebug dbg, const QMailMessagePart &part);
QMF_EXPORT QDebug operator<<(QDebug dbg, const QMailMessageHeaderField &field);

typedef QList<QMailMessage> QMailMessageList;
typedef QList<QMailMessageMetaData> QMailMessageMetaDataList;
typedef QList<QMailMessage::MessageType> QMailMessageTypeList;

Q_DECLARE_USER_METATYPE_ENUM(QMailMessageBody::TransferEncoding)
Q_DECLARE_USER_METATYPE_ENUM(QMailMessagePartContainer::MultipartType)
Q_DECLARE_USER_METATYPE_ENUM(QMailMessage::MessageType)
Q_DECLARE_USER_METATYPE_ENUM(QMailMessage::ContentType)
Q_DECLARE_USER_METATYPE_ENUM(QMailMessage::ResponseType)
Q_DECLARE_USER_METATYPE_ENUM(QMailMessage::AttachmentsAction)

Q_DECLARE_USER_METATYPE(QMailMessage)
Q_DECLARE_USER_METATYPE(QMailMessageMetaData)
Q_DECLARE_USER_METATYPE(QMailMessagePart)
Q_DECLARE_USER_METATYPE(QMailMessagePart::Location)

Q_DECLARE_METATYPE(QMailMessageList)
Q_DECLARE_METATYPE(QMailMessageMetaDataList)
Q_DECLARE_METATYPE(QMailMessageTypeList)

Q_DECLARE_USER_METATYPE_TYPEDEF(QMailMessageList, QMailMessageList)
Q_DECLARE_USER_METATYPE_TYPEDEF(QMailMessageMetaDataList, QMailMessageMetaDataList)
Q_DECLARE_USER_METATYPE_TYPEDEF(QMailMessageTypeList, QMailMessageTypeList)

#endif

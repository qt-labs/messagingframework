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

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#ifndef QMAILMESSAGE_P_H
#define QMAILMESSAGE_P_H

#include "qmailmessage.h"
#include "longstring_p.h"

#include <optional>

#include <QSharedData>

class QMailMessageHeaderFieldPrivate : public QSharedData
{
public:
    QMailMessageHeaderFieldPrivate();
    QMailMessageHeaderFieldPrivate(const QByteArray& text, bool structured);
    QMailMessageHeaderFieldPrivate(const QByteArray& name, const QByteArray& text, bool structured);

    bool operator==(const QMailMessageHeaderFieldPrivate &other) const;

    void addParameter(const QByteArray& name, const QByteArray& value);
    void parse(const QByteArray& text, bool structured);

    bool isNull() const;

    QByteArray id() const;
    void setId(const QByteArray& text);

    QByteArray content() const;
    void setContent(const QByteArray& text);

    QByteArray parameter(const QByteArray& name) const;
    void setParameter(const QByteArray& name, const QByteArray& value);

    bool isParameterEncoded(const QByteArray& name) const;
    void setParameterEncoded(const QByteArray& name);

    QList<QMailMessageHeaderField::ParameterType> parameters() const;

    QByteArray toString(bool includeName = true, bool presentable = true) const;

    void output(QDataStream& out) const;

    QString decodedContent() const;

    template <typename Stream> void serialize(Stream &stream) const;
    template <typename Stream> void deserialize(Stream &stream);

    QByteArray _id;
    QByteArray _content;
    bool _structured;
    QList<QMailMessageHeaderField::ParameterType> _parameters;
};


class QMailMessageHeaderPrivate : public QSharedData
{
public:
    QMailMessageHeaderPrivate();
    QMailMessageHeaderPrivate(const QByteArray& input);

    void update(const QByteArray &id, const QByteArray &content);
    void append(const QByteArray &id, const QByteArray &content);
    void remove(const QByteArray &id);

    QList<QMailMessageHeaderField> fields(const QByteArray& id, int maximum = -1) const;

    void output(QDataStream& out, const QList<QByteArray>& exclusions, bool excludeInternalFields) const;

    template <typename Stream> void serialize(Stream &stream) const;
    template <typename Stream> void deserialize(Stream &stream);

private:
    friend class QMailMessageHeader;

    QList<QByteArray> _headerFields;
};


class QMailMessageBodyPrivate : public QSharedData
{
public:
    QMailMessageBodyPrivate();

    void ensureCharsetExist();
    void fromLongString(LongString& ls, const QMailMessageContentType& type, QMailMessageBody::TransferEncoding encoding, QMailMessageBody::EncodingStatus status);
    void fromFile(const QString& filename, const QMailMessageContentType& type, QMailMessageBody::TransferEncoding encoding, QMailMessageBody::EncodingStatus status);
    void fromStream(QDataStream& in, const QMailMessageContentType& type, QMailMessageBody::TransferEncoding encoding, QMailMessageBody::EncodingStatus status);
    void fromStream(QTextStream& in, const QMailMessageContentType& type, QMailMessageBody::TransferEncoding encoding);

    bool toFile(const QString& filename, QMailMessageBody::EncodingFormat format) const;
    bool toStream(QDataStream& out, QMailMessageBody::EncodingFormat format) const;
    bool toStream(QTextStream& out) const;

    QMailMessageBody::TransferEncoding transferEncoding() const;
    QMailMessageContentType contentType() const;

    bool isEmpty() const;
    int length() const;

    bool encoded() const;
    void setEncoded(bool value);

    uint indicativeSize() const;

    void output(QDataStream& out, bool includeAttachments) const;

    // We will express the indicative size of the body in units of this:
    static const uint IndicativeSizeUnit = 1024;

    template <typename Stream> void serialize(Stream &stream) const;
    template <typename Stream> void deserialize(Stream &stream);

private:
    friend class QMailMessageBody;

    QMailMessageBody::TransferEncoding _encoding;
    LongString _bodyData;
    QString _filename;
    bool _encoded;
    QMailMessageContentType _type;
};


class QMF_EXPORT QMailMessagePartContainerPrivate : public QPrivateImplementationBase
{
public:
    template<typename Derived>
    QMailMessagePartContainerPrivate(Derived* p);

    void setLocation(const QMailMessageId& id, const QList<uint>& indices);
    int partNumber() const;

    const QMailMessagePartContainerPrivate* parentContainerPrivate() const;

    bool contains(const QMailMessagePart::Location& location) const;
    const QMailMessagePart& partAt(const QMailMessagePart::Location& location) const;
    QMailMessagePart& partAt(const QMailMessagePart::Location& location);

    void setHeader(const QMailMessageHeader& header, const QMailMessagePartContainerPrivate* parent = Q_NULLPTR);

    QByteArray headerField( const QByteArray &headerName ) const;
    QList<QByteArray> headerFields( const QByteArray &headerName, int maximum = 0 ) const;

    QList<QByteArray> headerFields() const;

    void updateHeaderField(const QByteArray &id, const QByteArray &content);
    void updateHeaderField(const QByteArray &id, const QString &content);

    void appendHeaderField(const QByteArray &id, const QByteArray &content);
    void appendHeaderField(const QByteArray &id, const QString &content);

    void removeHeaderField(const QByteArray &id);

    void setMultipartType(QMailMessagePartContainer::MultipartType type, const QList<QMailMessageHeaderField::ParameterType> &parameters);
    void appendPart(const QMailMessagePart &part);
    void prependPart(const QMailMessagePart &part);
    void removePartAt(uint pos);
    void clear();

    QMailMessageContentType contentType() const;
    QMailMessageBody::TransferEncoding transferEncoding() const;

    uint indicativeSize() const;

    void updateDefaultContentType(const QMailMessagePartContainerPrivate* parent);

    template <typename F>
    void outputParts(QDataStream **out, bool addMimePreamble, bool includeAttachments, bool excludeInternalFields, F *func) const;

    void outputBody(QDataStream& out, bool includeAttachments) const;

    QString headerFieldText( const QString &id ) const;

    QByteArray boundary() const;
    void setBoundary(const QByteArray& text);
    void generateBoundary();

    // Note: this returns a reference:
    QMailMessageBody& body();
    const QMailMessageBody& body() const;
    void setBody(const QMailMessageBody& body, QMailMessageBody::EncodingFormat encodingStatus = QMailMessageBody::None);
    void setBodyProperties(const QMailMessageContentType &type, QMailMessageBody::TransferEncoding encoding);

    bool hasBody() const;

    void parseMimeSinglePart(const QMailMessageHeader& partHeader, LongString body);
    void parseMimeMultipart(const QMailMessageHeader& partHeader, LongString body, bool insertIntoSelf);
    bool parseMimePart(LongString body);

    bool dirty(bool recursive = false) const;
    void setDirty(bool value = true, bool recursive = false);

    bool previewDirty() const;
    void setPreviewDirty(bool value);

    QMailMessagePartContainer *findPlainTextContainer() const;
    QMailMessagePartContainer *findHtmlContainer() const;

    static QMailMessagePartContainerPrivate* privatePointer(QMailMessagePart& part);

    template <typename Stream> void serialize(Stream &stream) const;
    template <typename Stream> void deserialize(Stream &stream);

    QMailMessagePartContainer::MultipartType _multipartType;
    QList<QMailMessagePart> _messageParts;
    mutable QByteArray _boundary;
    QMailMessageHeader _header;
    QMailMessageBody _body;
    QMailMessageId _messageId;
    QList<uint> _indices;
    bool _hasBody;
    bool _dirty;
    bool _previewDirty;
};


class QMailMessagePartPrivate : public QMailMessagePartContainerPrivate
{
public:
    QMailMessagePartPrivate();

    QMailMessagePart::ReferenceType referenceType() const;

    QMailMessageId messageReference() const;
    QMailMessagePart::Location partReference() const;

    QString referenceResolution() const;
    void setReferenceResolution(const QString &uri);

    bool contentAvailable() const;
    bool partialContentAvailable() const;

    QByteArray undecodedData() const;
    void setUndecodedData(const QByteArray &data);
    void appendUndecodedData(const QByteArray &data);

    template <typename F>
    void output(QDataStream **out, bool addMimePreamble, bool includeAttachments, bool excludeInternalFields, F *func) const;

    bool contentModified() const;
    void setUnmodified();

    template <typename Stream> void serialize(Stream &stream) const;
    template <typename Stream> void deserialize(Stream &stream);

private:
    friend class QMailMessagePart;

    void setReference(const QMailMessageId &id, const QMailMessageContentType& type, QMailMessageBody::TransferEncoding encoding);
    void setReference(const QMailMessagePart::Location &location, const QMailMessageContentType& type, QMailMessageBody::TransferEncoding encoding);

    QMailMessageId _referenceId;
    QMailMessagePart::Location _referenceLocation;
    QString _resolution;
    QByteArray _undecodedData;
};

class QMailMessageMetaDataPrivate : public QSharedData
{
public:
    QMailMessageMetaDataPrivate();

    void setMessageType(QMailMessage::MessageType type);
    void setParentFolderId(const QMailFolderId& id);
    void setPreviousParentFolderId(const QMailFolderId& id);
    void setId(const QMailMessageId& id);
    void setStatus(quint64 status);
    void setParentAccountId(const QMailAccountId& id);
    void setServerUid(const QString &server);
    void setSize(uint size);
    void setContent(QMailMessage::ContentType type);

    void setSubject(const QString& s);
    void setDate(const QMailTimeStamp& timeStamp);
    void setReceivedDate(const QMailTimeStamp& timeStamp);
    void setFrom(const QString& s);
    void setRecipients(const QString& s);

    void setCopyServerUid(const QString &copyServerUid);
    void setListId(const QString &listId);
    void setRestoreFolderId(const QMailFolderId &folderId);
    void setRfcId(const QString &rfcId);

    void setContentScheme(const QString& scheme);
    void setContentIdentifier(const QString& identifier);

    void setInResponseTo(const QMailMessageId &id);
    void setResponseType(QMailMessage::ResponseType type);

    void setPreview(const QString &s);

    uint indicativeSize() const;

    bool dataModified() const;
    void setUnmodified();

    const QMap<QString, QString> &customFields() const;
    QString customField(const QString &name) const;
    void setCustomField(const QString &name, const QString &value);
    void removeCustomField(const QString &name);
    void setCustomFields(const QMap<QString, QString> &fields);

    void setParentThreadId(const QMailThreadId &id);

    template <typename Stream> void serialize(Stream &stream) const;
    template <typename Stream> void deserialize(Stream &stream);

    QMailMessage::MessageType _messageType;
    quint64 _status;
    QMailMessage::ContentType _contentType;

    QMailAccountId _parentAccountId;
    QString _serverUid;
    uint _size;
    QMailMessageId _id;
    QMailFolderId _parentFolderId;
    QMailFolderId _previousParentFolderId;

    QString _subject;
    QMailTimeStamp _date;
    QMailTimeStamp _receivedDate;
    QString _from;
    QString _to;

    QString _copyServerUid;
    QMailFolderId _restoreFolderId;
    QString _listId;
    QString _rfcId;

    QString _contentScheme;
    QString _contentIdentifier;

    QMailMessageId _responseId;
    QMailMessage::ResponseType _responseType;
    QString _preview;
    QMailThreadId _parentThreadId;

    mutable std::optional< QMap<QString, QString> > _customFields;
    bool _customFieldsModified;

    template <typename T>
    void updateMember(T& value, const T& newValue)
    {
        if (value != newValue) {
            value = newValue;
            _dirty = true;
        }
    }

    void updateMember(QString& value, const QString& newValue)
    {
        if (newValue.isNull() && !value.isEmpty()) {
            value = QString::fromLatin1("");
            _dirty = true;
        } else if (value != newValue) {
            value = newValue;
            _dirty = true;
        }
    }

    bool _dirty;
    void ensureCustomFieldsLoaded() const;

private:
    static quint64 registerFlag(const QString &name);
};


class QMailMessagePrivate : public QMailMessagePartContainerPrivate
{
public:
    QMailMessagePrivate();

    void fromRfc2822(const LongString &ls);

    template <typename F>
    void toRfc2822(QDataStream **out, QMailMessage::EncodingFormat format, quint64 messageStatus, F *func) const;

    void setMessageType(QMailMessage::MessageType type);

    void setId(const QMailMessageId& id);
    void setSubject(const QString& s);
    void setDate(const QMailTimeStamp& timeStamp);
    void setFrom(const QString& from);
    void setReplyTo(const QString& s);
    void setInReplyTo(const QString& s);

    void setTo(const QString& s);
    void setCc(const QString& s);
    void setBcc(const QString& s);

    bool hasRecipients() const;

    bool contentModified() const;
    void setUnmodified();

    template <typename Stream> void serialize(Stream &stream) const;
    template <typename Stream> void deserialize(Stream &stream);

private:
    void outputHeaders(QDataStream& out, bool addTimeStamp, bool addContentHeaders, bool includeBcc, bool excludeInternalFields) const;
};

#endif

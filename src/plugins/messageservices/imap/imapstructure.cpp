/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#include "imapstructure.h"

#include <qmaillog.h>
#include <qmailmessage.h>


namespace {

template <typename Processor>
void processChars(Processor p, const QString& field, QString::const_iterator origin)
{
    bool quoted = false;
    bool escaped = false;
    int depth = 0;

    QString::const_iterator begin = origin, it = begin, end = field.end();

    do {
        char c((*it).toAscii());

        switch (c) {
        case ')':
            if (!quoted && !escaped) {
                if (it != begin) {
                    if (p(c, depth, quoted, field, begin, it)) {
                        begin = it + 1;
                    }
                }

                --depth;
            }
            break;

        case '(':
            if (!quoted && !escaped) {
                if (p(c, depth, quoted, field, begin, it)) {
                    begin = it + 1;
                }

                ++depth;
            }
            break;

        case '"':
            if (p(c, depth, quoted, field, begin, it)) {
                begin = it + 1;
            }
            if (!escaped) {
                quoted = !quoted;
            }
            break;

        default:
            if (p(c, depth, quoted, field, begin, it)) {
                begin = it + 1;
            }
            break;
        }

        escaped = (c == '\\');
    } while (++it != end);
    
    if ((it != begin) && (depth >= 0)) {
        // Terminate the sequence
        p('\0', depth, quoted, field, begin, it);
    }
}


struct StructureDecomposer
{
    int reportDepth;
    QStringList result;

    StructureDecomposer() : reportDepth(0) {}

    bool operator()(char c, int depth, bool, const QString &field, QString::const_iterator begin, QString::const_iterator it)
    {
        if ((c == ')') || (c == '\0')) {
            if (depth == reportDepth) {
                result.append(field.mid(begin - field.begin(), it - begin).trimmed());

                if (reportDepth > 0)
                    --reportDepth;

                return true;
            }
        } else if (c == '(') {
            if (it == begin) {
                if (reportDepth > 0) return false;

                ++reportDepth;
                return true;
            }
        }

        return false;
    }
};

QStringList decomposeStructure(const QString &structure, int offset)
{
    if (structure.isEmpty())
        return QStringList();

    StructureDecomposer sd;
    processChars<StructureDecomposer&>(sd, structure, structure.begin() + offset);
    return sd.result;
}


struct ElementDecomposer
{
    int reportDepth;
    QStringList result;

    ElementDecomposer() : reportDepth(0) {}

    void append(const QString &token, int begin, int length, int depth)
    {
        if ((depth == 0) && (!token.isEmpty()) && (token.at(begin) == QChar('"')) && (token.at(begin + length - 1) == QChar('"'))) {
            ++begin;
            length -= 2;
        }

        result.append(token.mid(begin, length));
    }

    bool operator()(char c, int depth, bool quoted, const QString &field, QString::const_iterator begin, QString::const_iterator it)
    {
        if (((c == ' ') && !quoted) || (c == '\0')) {
            if (depth == 0) {
                if (it != begin) {
                    append(field, begin - field.begin(), it - begin, depth);
                }

                return true;
            }
        } else if (c == ')') {
            if (depth == reportDepth) {
                append(field, begin - field.begin(), it - begin, depth);

                if (reportDepth > 0)
                    --reportDepth;

                return true;
            }
        } else if (c == '(') {
            if (it == begin) {
                if (reportDepth > 0) return false;

                ++reportDepth;
                return true;
            }
        }

        return false;
    }
};

QStringList decomposeElements(const QString &element)
{
    if (element.isEmpty() || (element.trimmed().toUpper() == "NIL"))
        return QStringList();

    ElementDecomposer ed;
    processChars<ElementDecomposer&>(ed, element, element.begin());
    return ed.result;
}


QMailMessageBody::TransferEncoding fromEncodingDescription(const QString &desc)
{
    if (!desc.isEmpty()) {
        const QString value(desc.trimmed().toUpper());

        // TODO: these strings aren't actually specified by RFC3501...
        if (value == "7BIT") {
            return QMailMessageBody::SevenBit;
        } else if (value == "8BIT") {
            return QMailMessageBody::EightBit;
        } else if (value == "BINARY") {
            return QMailMessageBody::Binary;
        } else if (value == "QUOTED-PRINTABLE") {
            return QMailMessageBody::QuotedPrintable;
        } else if (value == "BASE64") {
            return QMailMessageBody::Base64;
        }
    }

    return QMailMessageBody::NoEncoding;
}

QMailMessagePartContainer::MultipartType fromMultipartDescription(const QString &desc)
{
    if (!desc.isEmpty()) {
        const QString value(desc.trimmed().toUpper());

        // TODO: these strings aren't actually specified by RFC3501...
        if (value == "MIXED") {
            return QMailMessagePartContainer::MultipartMixed;
        } else if (value == "ALTERNATIVE") {
            return QMailMessagePartContainer::MultipartAlternative;
        } else if (value == "DIGEST") {
            return QMailMessagePartContainer::MultipartDigest;
        } else if (value == "SIGNED") {
            return QMailMessagePartContainer::MultipartSigned;
        } else if (value == "ENCRYPTED") {
            return QMailMessagePartContainer::MultipartEncrypted;
        } else if (value == "PARALLEL") {
            return QMailMessagePartContainer::MultipartParallel;
        } else if (value == "RELATED") {
            return QMailMessagePartContainer::MultipartRelated;
        } else if (value == "FORMDATA") {
            return QMailMessagePartContainer::MultipartFormData;
        } else if (value == "REPORT") {
            return QMailMessagePartContainer::MultipartReport;
        }
    }

    return QMailMessagePartContainer::MultipartNone;
}

QMailMessageContentDisposition fromDispositionDescription(const QString &desc, const QString &size)
{
    QMailMessageContentDisposition disposition;

    QStringList details(decomposeElements(desc));
    if (!details.isEmpty()) {
        const QString value(details.at(0).trimmed().toUpper());

        if (value == "INLINE") {
            disposition.setType(QMailMessageContentDisposition::Inline);
        } else {
            // Anything else should be treated as attachment
            disposition.setType(QMailMessageContentDisposition::Attachment);
        }

        if (details.count() > 1) {
            const QStringList parameters(decomposeElements(details.at(1)));
            if (parameters.count() % 2)
                qWarning() << "Incorrect fromDispositionDescription parameters:" << parameters;
            QStringList::const_iterator it = parameters.begin(), end = parameters.end();
            for ( ; (it != end) && (it + 1 != end); ++it) {
                disposition.setParameter((*it).toAscii(), (*(it + 1)).toAscii());
                ++it;
            }            
        }
    } else {
        // Default to inline for no specification
        disposition.setType(QMailMessageContentDisposition::Inline);
    }

    if (!size.isEmpty()) {
        disposition.setSize(size.toInt());
    }

    return disposition;
}

void setBodyFromDescription(const QStringList &details, QMailMessagePartContainer *container, uint *size)
{
    QMailMessageContentType type;

    // [0]: type
    // [1]: sub-type
    type.setType(details.at(0).toAscii());
    type.setSubType(details.at(1).toAscii());

    // [2]: parameter list
    const QStringList parameters(decomposeElements(details.at(2)));
    if (parameters.count() % 2)
        qWarning() << "Incorrect setBodyFromDescription parameters:" << parameters;
    QStringList::const_iterator it = parameters.begin(), end = parameters.end();
    for ( ; (it != end) && ((it + 1) != end); it += 2) {
        type.setParameter((*it).toAscii(), (*(it + 1)).toAscii());
    }

    // [3]: content-ID
    // [4]: content-description

    // [5]: content-encoding
    QMailMessageBody::TransferEncoding encoding(fromEncodingDescription(details.at(5)));

    // [6]: size
    if (size) {
        *size = details.at(6).toUInt();
    }

    container->setBody(QMailMessageBody::fromData(QByteArray(), type, encoding, QMailMessageBody::AlreadyEncoded));
}

void setPartContentFromStructure(const QStringList &structure, QMailMessagePart *part, uint *size);

void setPartFromDescription(const QStringList &details, QMailMessagePart *part)
{
    // [3]: content-ID
    const QString &id(details.at(3));
    if (!id.isEmpty() && (id.trimmed().toUpper() != "NIL")) {
        part->setContentID(id);
    }

    // [4]: content-description
    const QString &description(details.at(4));
    if (!description.isEmpty() && (description.trimmed().toUpper() != "NIL")) {
        part->setContentDescription(description);
    }

    // [6]: content size
    const QString &size(details.at(6));
    int next = 7;

    const QMailMessageContentType &type(part->contentType());
    if (type.type().toUpper() == "TEXT") {
        // The following field is the part's line count
        ++next;
    } else if ((type.type().toUpper() == "MESSAGE") && (type.subType().toUpper() == "RFC822")) {
        // For parts of type 'message/rfc822', there are three extra fields here...
        next += 3;
    }

    // [6 + n + 1]: MD5-sum of content
    // Ignore the potential MD5-sum for now
    ++next;

    // [6 + n + 2]: content-disposition
    QString disposition;
    if (next < details.count()) {
        disposition = details.at(next);
        if (disposition.trimmed().toUpper() == "NIL") {
            disposition = QString();
        }
    }
    part->setContentDisposition(fromDispositionDescription(disposition, size));
    ++next;

    // [6 + n + 3]: content-language
    if (next < details.count()) {
        const QString &language(details.at(next));

        // TODO: this may need compression from multiple tokens to one?
        if (!language.isEmpty() && (language.trimmed().toUpper() != "NIL"))
            part->setContentLanguage(language);
    }
    ++next;

    // [6 + n + 3]: content-location
    if (next < details.count()) {
        const QString &location(details.at(next));

        // TODO: this may need compression from multiple tokens to one?
        if (!location.isEmpty() && (location.trimmed().toUpper() != "NIL"))
            part->setContentLocation(location);
    }
    ++next;
}

void setMultipartFromDescription(const QStringList &structure, QMailMessagePartContainer *container, QMailMessagePart *part, uint *size)
{
    QStringList details = decomposeElements(structure.last());

    // [0]: type
    container->setMultipartType(fromMultipartDescription(details.at(0)));

    // [1]: parameter list
    if (details.count() > 1) {
        const QStringList parameters(decomposeElements(details.at(1)));
        QStringList::const_iterator it = parameters.begin(), end = parameters.end();
        if (parameters.count() % 2)
            qWarning() << "Incorrect setMultipartFromDescription parameter count" << parameters.last();
        for ( ; (it != end) && ((it + 1) != end); it += 2) {
            if ((*it).trimmed().toUpper() == "BOUNDARY") {
                container->setBoundary((*(it + 1)).toAscii());
            }
        }
    }

    if (part) {
        // [2]: content-disposition
        QString disposition;
        if (details.count() > 2) {
            disposition = details.at(2);
            if (disposition.trimmed().toUpper() == "NIL") {
                disposition = QString();
            }
        }
        part->setContentDisposition(fromDispositionDescription(disposition, QString()));

        // [3]: content-language
        if (details.count() > 3) {
            const QString &language(details.at(3));

            // TODO: this may need compression from multiple tokens to one?
            if (!language.isEmpty() && (language.trimmed().toUpper() != "NIL"))
                part->setContentLanguage(language);
        }
        
        // [4]: content-location
        if (details.count() > 4) {
            const QString &location(details.at(4));

            // TODO: this may need compression from multiple tokens to one?
            if (!location.isEmpty() && (location.trimmed().toUpper() != "NIL"))
                part->setContentLocation(location);
        }
    }

    // Create the other pieces described by the structure
    for (int i = 0; i < (structure.count() - 1); ++i) {
        QMailMessagePart part;
        uint partSize = 0;

        setPartContentFromStructure(decomposeStructure(structure.at(i), 0), &part, &partSize);
        container->appendPart(part);

        if (size) {
            *size += partSize;
        }
    }
}

void setPartContentFromStructure(const QStringList &structure, QMailMessagePart *part, uint *size)
{
    if (!structure.isEmpty()) {
        // The last element is the message
        const QString &message = structure.last();
        if (!message.isEmpty()) {
            if (structure.count() == 1) {
                QStringList details(decomposeElements(message));
                if (details.count() < 7) {
                    qWarning() << "Ill-formed part structure:" << details;
                } else {
                    setBodyFromDescription(details, part, size);

                    if (details.count() > 7) {
                        setPartFromDescription(details, part);
                    }
                }
            } else {
                // This is a multi-part message
                setMultipartFromDescription(structure, part, part, size);
            }
        }
    }
}

}

void setMessageContentFromStructure(const QStringList &structure, QMailMessage *message)
{
    if (!structure.isEmpty()) {
        // The last element is the message description
        const QString &description = structure.last();
        if (!description.isEmpty()) {
            uint size = 0;
            if (structure.count() == 1) {
                setBodyFromDescription(decomposeElements(description), message, &size);
            } else {
                // This is a multi-part message
                setMultipartFromDescription(structure, message, 0, &size);
            }

            message->setContentSize(size);
        }
    }
}

QStringList getMessageStructure(const QString &field)
{
    static const QString marker("BODYSTRUCTURE (");
    int index = field.indexOf(marker);
    if (index != -1) {
        return decomposeStructure(field, index + marker.length());
    }

    return QStringList();
}


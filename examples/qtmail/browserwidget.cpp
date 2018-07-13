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

#include "browserwidget.h"

#include <qmailaddress.h>
#include <qmailmessage.h>
#include <qmailtimestamp.h>
#include "qmaillog.h"

#include <QApplication>
#include <QImageReader>
#include <QKeyEvent>
#include <QMenu>
#include <QStyle>
#include <QTextBrowser>
#include <QVBoxLayout>
#ifdef USE_WEBKIT
#include <QBuffer>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTimer>
#include <QWebView>
#else
#include <QMap>
#include <QVariant>
#endif

#include <limits.h>

static QColor replyColor(Qt::darkGreen);

static QString dateString(const QDateTime& dt)
{
    QDateTime current = QDateTime::currentDateTime();
    if (dt.date() == current.date()) {
        //today
        return QString(qApp->translate("Browser", "Today %1")).arg(dt.toString("h:mm:ss ap"));
    } else if (dt.daysTo(current) == 1) {
        //yesterday
        return QString(qApp->translate("Browser", "Yesterday %1")).arg(dt.toString("h:mm:ss ap"));
    } else if (dt.daysTo(current) < 7) {
        //within 7 days
        return dt.toString("dddd h:mm:ss ap");
    } else {
        return dt.toString("dd/MM/yy h:mm:ss ap");
    }
}

#ifdef USE_WEBKIT
class ContentReply : public QNetworkReply
{
    Q_OBJECT

public:
    ContentReply(QObject *parent, QByteArray *data, const QString &contentType);
    ~ContentReply();

    qint64 bytesAvailable() const;
    qint64 readData(char *data, qint64 maxSize);

    void abort();

private:
    QBuffer m_buffer;
};

ContentReply::ContentReply(QObject *parent, QByteArray *data, const QString &contentType)
    : QNetworkReply(parent)
{
    setOpenMode(QIODevice::ReadOnly | QIODevice::Unbuffered);
    setHeader(QNetworkRequest::ContentTypeHeader, contentType);

    m_buffer.setBuffer(data);
    m_buffer.open(QIODevice::ReadOnly);

    QTimer::singleShot(0, this, SIGNAL(readyRead()));
    QTimer::singleShot(0, this, SIGNAL(finished()));
}

ContentReply::~ContentReply()
{
}

qint64 ContentReply::bytesAvailable() const
{
    return m_buffer.bytesAvailable() + QIODevice::bytesAvailable();
}

qint64 ContentReply::readData(char *data, qint64 maxSize)
{
    return m_buffer.read(data, maxSize);
}

void ContentReply::abort()
{
    m_buffer.close();
}

class ContentAccessManager : public QNetworkAccessManager
{
    Q_OBJECT

public:
    ContentAccessManager(QObject *parent);
    ~ContentAccessManager();

    void setResource(const QSet<QUrl> &identifiers, const QByteArray &data, const QString &contentType);

    void clear();

protected:
    QNetworkReply *createRequest(Operation op, const QNetworkRequest &req, QIODevice *outgoingData = Q_NULLPTR);

private:
    QMap<QUrl, QPair<QByteArray, QString> > m_data;
};

ContentAccessManager::ContentAccessManager(QObject *parent)
    : QNetworkAccessManager(parent)
{
}

ContentAccessManager::~ContentAccessManager()
{
}

void ContentAccessManager::setResource(const QSet<QUrl> &identifiers, const QByteArray &data, const QString &contentType)
{
    foreach (const QUrl url, identifiers) {
        m_data.insert(url, qMakePair(data, contentType));
    }
}

void ContentAccessManager::clear()
{
    m_data.clear();
}

QNetworkReply *ContentAccessManager::createRequest(Operation op, const QNetworkRequest &req, QIODevice *outgoingData)
{
    if (op == QNetworkAccessManager::GetOperation) {
        QUrl url(req.url());
        if ((url.scheme() == "cid") || (url.scheme() == "qmf-part")) {
            QString identifier(url.path().trimmed());
            // Remove any delimiters that are present
            if (identifier.startsWith('<') && identifier.endsWith('>')) {
                identifier = identifier.mid(1, identifier.length() - 2);
            }

            // See if we have any data for this content identifier   
            QMap<QUrl, QPair<QByteArray, QString> >::iterator it = m_data.find(QUrl("cid:" + identifier));
            if (it == m_data.end()) {
                it = m_data.find(QUrl("qmf-part:" + identifier));
            }
            
            if (it != m_data.end()) {
                return new ContentReply(this, &it.value().first, it.value().second);
            }
        }
    }

    return QNetworkAccessManager::createRequest(op, req, outgoingData);
}
#else
class ContentRenderer : public QTextBrowser
{
    Q_OBJECT

public:
    ContentRenderer(QWidget *parent);
    ~ContentRenderer();

    void setResource(const QUrl &name, const QVariant &var);
    virtual QVariant loadResource(int type, const QUrl &name);

    void clear();

private:
    QMap<QUrl, QVariant> m_data;
};

ContentRenderer::ContentRenderer(QWidget *parent)
    : QTextBrowser(parent)
{
}

ContentRenderer::~ContentRenderer()
{
}

void ContentRenderer::setResource(const QUrl& name, const QVariant &var)
{
    if (!m_data.contains(name)) {
        m_data.insert(name, var);
    }
}

void ContentRenderer::clear()
{
    m_data.clear();
}

QVariant ContentRenderer::loadResource(int type, const QUrl& name)
{
    if (m_data.contains(name)) {
        return m_data[name];
    }

    return QTextBrowser::loadResource(type, name);
}
#endif

BrowserWidget::BrowserWidget( QWidget *parent  )
    : QWidget( parent ),
      replySplitter( &BrowserWidget::handleReplies )
{
    QLayout* l = new QVBoxLayout(this);
    l->setSpacing(0);
    l->setContentsMargins(0,0,0,0);

#ifdef USE_WEBKIT
    m_accessManager = new ContentAccessManager(this);

    m_webView = new QWebView(this);
    m_webView->setObjectName("renderer");
    m_webView->page()->setNetworkAccessManager(m_accessManager);
    m_webView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_webView,SIGNAL(linkClicked(QUrl)),this,SIGNAL(anchorClicked(QUrl)));
    connect(m_webView,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(contextMenuRequested(QPoint)));

    l->addWidget(m_webView);
#else
    m_renderer = new ContentRenderer(this);
    m_renderer->setObjectName("renderer");
    m_renderer->setFrameStyle(QFrame::NoFrame);
    m_renderer->setContextMenuPolicy(Qt::CustomContextMenu);
    m_renderer->setOpenLinks(false);
    connect(m_renderer,SIGNAL(anchorClicked(QUrl)),this,SIGNAL(anchorClicked(QUrl)));
    connect(m_renderer,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(contextMenuRequested(QPoint)));

    l->addWidget(m_renderer);
#endif

    setFocusPolicy( Qt::StrongFocus );
}

#ifndef USE_WEBKIT
void BrowserWidget::setResource( const QUrl& name, QVariant var )
{
    m_renderer->setResource(name, var);
}
#endif

void BrowserWidget::clearResources()
{
#ifdef USE_WEBKIT
    m_accessManager->clear();
#else
    m_renderer->clear();
#endif
    numbers.clear();
}

QList<QString> BrowserWidget::embeddedNumbers() const
{
    QList<QString> result;
    return result;
}

void BrowserWidget::setTextResource(const QSet<QUrl>& names, const QString& textData, const QString &contentType)
{
#ifdef USE_WEBKIT
    m_accessManager->setResource(names, textData.toUtf8(), contentType);
#else
    QVariant data(textData);
    foreach (const QUrl &url, names) {
        setResource(url, data);
    }

    Q_UNUSED(contentType);
#endif
}

void BrowserWidget::setImageResource(const QSet<QUrl>& names, const QByteArray& imageData, const QString &contentType)
{
    // Create a image from the data
    QDataStream imageStream(&const_cast<QByteArray&>(imageData), QIODevice::ReadOnly);
    QImageReader imageReader(imageStream.device());

    // Max size should be bounded by our display window, which will possibly
    // have a vertical scrollbar (and a small fudge factor...)
    int maxWidth = (width() - style()->pixelMetric(QStyle::PM_ScrollBarExtent) - 4);

    QSize imageSize;
    if (imageReader.supportsOption(QImageIOHandler::Size)) {
        imageSize = imageReader.size();

        // See if the image needs to be down-scaled during load
        if (imageSize.width() > maxWidth) {
            // And the loaded size should maintain the image aspect ratio
            imageSize.scale(maxWidth, (INT_MAX >> 4), Qt::KeepAspectRatio);
            imageReader.setQuality( 49 ); // Otherwise Qt smooth scales
            imageReader.setScaledSize(imageSize);
#ifdef USE_WEBKIT
        } else {
            // We can use the image directly
            m_accessManager->setResource(names, imageData, contentType);
            return;
#endif
        }
    }

    QImage image = imageReader.read();

    if (!imageReader.supportsOption(QImageIOHandler::Size)) {
        // We need to scale it down now
        if (image.width() > maxWidth) {
            image = image.scaled(maxWidth, INT_MAX, Qt::KeepAspectRatio);
#ifdef USE_WEBKIT
        } else {
            // We can use the image directly
            m_accessManager->setResource(names, imageData, contentType);
            return;
#endif
        }
    }

#ifdef USE_WEBKIT
    QByteArray scaledData;
    {
        // Write the scaled image data to a buffer
        QBuffer buffer(&scaledData);
        buffer.open(QIODevice::WriteOnly);
        image.save(&buffer, "PNG"); // default format doesn't render in webkit
    }
    m_accessManager->setResource(names, scaledData, contentType);
#else
    QVariant data(image);
    foreach (const QUrl &url, names) {
        setResource(url, data);
    }

    Q_UNUSED(contentType);
#endif
}

void BrowserWidget::setPartResource(const QMailMessagePart& part)
{
    QSet<QUrl> names;

#ifdef USE_WEBKIT
    QString name(part.displayName());
    if (!name.isEmpty()) {
        // use 'qmf-part' url scheme to ensure inline images without a contentId are rendered
        names.insert(QUrl("qmf-part:" + name.toHtmlEscaped()));
    }

    name = part.contentID().toHtmlEscaped();
    if (!name.isEmpty()) {
        // We can only resolve URLs using the cid: scheme
        if (name.startsWith("cid:", Qt::CaseInsensitive)) {
            names.insert(QUrl(name));
        } else {
            names.insert(QUrl("cid:" + name));
        }
    }
#else
    QString name(part.displayName());
    if (!name.isEmpty()) {
        names.insert(QUrl(name.toHtmlEscaped()));
    }

    name = part.contentID().toHtmlEscaped();
    if (!name.isEmpty()) {
        // Add the content both with and without the cid: prefix
        names.insert(name);
        if (name.startsWith("cid:", Qt::CaseInsensitive)) {
            names.insert(QUrl(name.mid(4)));
        } else {
            names.insert(QUrl("cid:" + name));
        }
    }

    name = part.contentType().name();
    if (!name.isEmpty()) {
        names.insert(QUrl(name.toHtmlEscaped()));
    }
#endif

    const QMailMessageContentType &ct(part.contentType());

    // Ignore any charset info in the content type, since we extract string data as unicode
    QString contentType(ct.type() + '/' + ct.subType());

    if (ct.type().toLower() == "text") {
        setTextResource(names, part.body().data(), contentType);
    } else if (ct.type().toLower() == "image") {
        setImageResource(names, part.body().data(QMailMessageBody::Decoded), contentType);
    }
}

void BrowserWidget::setSource(const QUrl &name)
{
    Q_UNUSED(name)
// We deal with this ourselves.
//    QTextBrowser::setSource( name );
}

void BrowserWidget::contextMenuRequested(const QPoint& pos)
{
#ifdef USE_WEBKIT
    QMenu menu(this);
    menu.addAction(m_webView->pageAction(QWebPage::Copy));
    menu.addAction(m_webView->pageAction(QWebPage::CopyLinkToClipboard));
    menu.addSeparator();
    menu.addActions(m_webView->actions());
    menu.exec(mapToGlobal(pos));
#else
    QMenu* menu = m_renderer->createStandardContextMenu();
    menu->addSeparator();
    menu->addActions(m_renderer->actions());
    menu->exec(mapToGlobal(pos));
    delete menu;
#endif
}

void BrowserWidget::setMessage(const QMailMessage& email, bool plainTextMode)
{
    if (plainTextMode) {
        // Complete MMS messages must be displayed in HTML
        if (email.messageType() == QMailMessage::Mms) {
            QString mmsType = email.headerFieldText("X-Mms-Message-Type");
            if (mmsType.contains("m-retrieve-conf") || mmsType.contains("m-send-req"))
                plainTextMode = false;
        }
    }

    // Maintain original linelengths if display width allows it
    if (email.messageType() == QMailMessage::Sms) {
        replySplitter = &BrowserWidget::smsBreakReplies;
    } else {
        uint lineCharLength;
        if ( fontInfo().pointSize() >= 10 ) {
            lineCharLength = width() / (fontInfo().pointSize() - 4 );
        } else {
            lineCharLength = width() / (fontInfo().pointSize() - 3 );
        }

        // Determine how to split lines in text
        if ( lineCharLength >= 78 )
            replySplitter = &BrowserWidget::noBreakReplies;
        else
            replySplitter = &BrowserWidget::handleReplies;
    }

    if (plainTextMode)
        displayPlainText(&email);
    else
        displayHtml(&email);
}

void BrowserWidget::displayPlainText(const QMailMessage* mail)
{
    QString bodyText;

    if ( (mail->status() & QMailMessage::Incoming) && 
        !(mail->status() & QMailMessage::PartialContentAvailable) ) {
        if ( !(mail->status() & QMailMessage::Removed) ) {
            bodyText += '\n' + tr("Awaiting download") + '\n';
            bodyText += tr("Size of message") + ": " + describeMailSize(mail->size());
        } else {
            // TODO: what?
        }
    } else {
        if (mail->hasBody()) {
            bodyText += mail->body().data();
        } else {
            if ( mail->multipartType() == QMailMessagePartContainer::MultipartAlternative ) {
                const QMailMessagePart* bestPart = 0;

                // Find the best alternative for text rendering
                for ( uint i = 0; i < mail->partCount(); i++ ) {
                    const QMailMessagePart &part = mail->partAt( i );

                    // TODO: A good implementation would be able to extract the plain text parts
                    // from text/html and text/enriched...

                    if (part.contentType().type().toLower() == "text") {
                        if (part.contentType().subType().toLower() == "plain") {
                            // This is the best part for us
                            bestPart = &part;
                            break;
                        }
                        else if (part.contentType().subType().toLower() == "html") {
                            // This is the worst, but still acceptable, text part for us
                            if (bestPart == 0)
                                bestPart = &part;
                        }
                        else  {
                            // Some other text - better than html, probably
                            if ((bestPart != 0) && (bestPart->contentType().subType().toLower() == "html"))
                                bestPart = &part;
                        }
                    }
                }

                if (bestPart != 0)
                    bodyText += bestPart->body().data() + '\n';
                else
                    bodyText += QLatin1String("\n<") + tr("Message part is not displayable") +
                                QLatin1String(">\n");
            }
            else if ( mail->multipartType() == QMailMessagePartContainer::MultipartRelated ) {
                const QMailMessagePart* startPart = &mail->partAt(0);

                // If not specified, the first part is the start
                QByteArray startCID = mail->contentType().parameter("start");
                if (!startCID.isEmpty()) {
                    for ( uint i = 1; i < mail->partCount(); i++ ) 
                        if (mail->partAt(i).contentID() == startCID) {
                            startPart = &mail->partAt(i);
                            break;
                        }
                }

                // Render the start part, if possible
                if (startPart->contentType().type().toLower() == "text")
                    bodyText += startPart->body().data() + '\n';
                else
                    bodyText +=  QLatin1String("\n<") + tr("Message part is not displayable") +
                                  QLatin1String(">\n");
            }
            else {
                // According to RFC 2046, any unrecognised type should be treated as 'mixed'
                if (mail->multipartType() != QMailMessagePartContainer::MultipartMixed)
                    qWarning() << "Generic viewer: Unimplemented multipart type:" << mail->contentType().toString();

                // Render each succesive part to text, where possible
                for ( uint i = 0; i < mail->partCount(); i++ ) {
                    const QMailMessagePart &part = mail->partAt( i );

                    if (part.hasBody() && (part.contentType().type().toLower() == "text")) {
                        bodyText += part.body().data() + '\n';
                    } else {
                        bodyText += QLatin1String("\n<") + tr("Part") + ": " + part.displayName() +
                                    QLatin1String(">\n");
                    }
                }
            }
        }
    }

    QString text;

    if ((mail->messageType() != QMailMessage::Sms) && (mail->messageType() != QMailMessage::Instant))
        text += tr("Subject") +  QLatin1String(": ") + mail->subject().simplified() + '\n';

    QMailAddress fromAddress(mail->from());
    if (!fromAddress.isNull())
        text += tr("From") + ": " + fromAddress.toString() + '\n';

    if (mail->to().count() > 0) {
        text += tr("To") + ": ";
        text += QMailAddress::toStringList(mail->to()).join(QLatin1String(", "));
    }
    if (mail->cc().count() > 0) {
        text += '\n' + tr("CC") +  QLatin1String(": ");
        text += QMailAddress::toStringList(mail->cc()).join(QLatin1String(", "));
    }
    if (mail->bcc().count() > 0) {
        text += '\n' + tr("BCC") +  QLatin1String(": ");
        text += QMailAddress::toStringList(mail->bcc()).join(QLatin1String(", "));
    }
    if ( !mail->replyTo().isNull() ) {
        text += '\n' + tr("Reply-To") + QLatin1String(": ");
        text += mail->replyTo().toString();
    }

    text += '\n' + tr("Date") + QLatin1String(": ");
    text += dateString(mail->date().toLocalTime()) + '\n';

    if (mail->status() & QMailMessage::Removed) {
        if (!bodyText.isEmpty()) {
            // Don't include the notice - the most likely reason to view plain text
            // is for printing, and we don't want to print the notice
        } else {
            text += '\n';
            text += tr("Message deleted from server");
        }
    }

    if (!bodyText.isEmpty()) {
        text += '\n';
        text += bodyText;
    }
    
    setPlainText(text);
}

static QString replaceLast(const QString container, const QString& before, const QString& after)
{
    QString result(container);

    int index;
    if ((index = container.lastIndexOf(before)) != -1)
        result.replace(index, before.length(), after);

    return result;
}

QString BrowserWidget::renderSimplePart(const QMailMessagePart& part)
{
    QString result;

    QString partId = part.displayName().toHtmlEscaped();

    QMailMessageContentType contentType = part.contentType();
    if ( contentType.type().toLower() == "text") { // No tr
        if (part.hasBody()) {
            QString partText = part.body().data();
            if ( !partText.isEmpty() ) {
                if ( contentType.subType().toLower() == "html" ) {
                    result = partText + "<br>";
                } else {
                    result = formatText( partText );
                }
            }
        } else {
            result = renderAttachment(part);
        }
    } else if ( contentType.type().toLower() == "image") { // No tr
        setPartResource(part);
#ifdef USE_WEBKIT
        result = "<img src=\"qmf-part:" + partId + "\"></img>";
#else
        result = "<img src=\"" + partId + "\"></img>";
#endif
        
    } else {
        result = renderAttachment(part);
    }

    return result;
}

QString BrowserWidget::renderAttachment(const QMailMessagePart& part)
{
    QString partId = part.displayName().toHtmlEscaped();

    QString attachmentTemplate = 
"<hr><b>ATTACHMENT_TEXT</b>: <a href=\"attachment;ATTACHMENT_ACTION;ATTACHMENT_LOCATION\">NAME_TEXT</a>DISPOSITION<br>";

    attachmentTemplate = replaceLast(attachmentTemplate, "ATTACHMENT_TEXT", tr("Attachment"));
    attachmentTemplate = replaceLast(attachmentTemplate, "ATTACHMENT_ACTION", part.partialContentAvailable() ? "view" : "retrieve");
    attachmentTemplate = replaceLast(attachmentTemplate, "ATTACHMENT_LOCATION", part.location().toString(false));
    attachmentTemplate = replaceLast(attachmentTemplate, "NAME_TEXT", partId);
    return replaceLast(attachmentTemplate, "DISPOSITION", part.partialContentAvailable() ? "" : tr(" (on server)"));
}

QString BrowserWidget::renderPart(const QMailMessagePart& part)
{
    QString result;

    if (part.multipartType() != QMailMessage::MultipartNone) {
        result = renderMultipart(part);
    } else {
        bool displayAsAttachment(!part.contentAvailable());
        if (!displayAsAttachment) {
            QMailMessageContentDisposition disposition = part.contentDisposition();
            if (!disposition.isNull() && disposition.type() == QMailMessageContentDisposition::Attachment) {
                displayAsAttachment = true;
            }
        }

        result = (displayAsAttachment ? renderAttachment(part) : renderSimplePart(part));
    }

    return result;
}

QString BrowserWidget::renderMultipart(const QMailMessagePartContainer& partContainer)
{
    QString result;

    if (partContainer.multipartType() == QMailMessagePartContainer::MultipartAlternative) {
        int partIndex = -1;

        // Find the best alternative for rendering
        for (uint i = 0; i < partContainer.partCount(); ++i) {
            const QMailMessagePart &part = partContainer.partAt(i);

            // Parts are ordered simplest to most complex
            QString type(part.contentType().type().toLower());
            if ((type == "text") || (type == "image")) {
                // These parts are probably displayable
                partIndex = i;
            }
        }

        if (partIndex != -1) {
            result += renderPart(partContainer.partAt(partIndex));
        } else {
            result += "\n<" + tr("No displayable part") + ">\n";
        }
    } else if (partContainer.multipartType() == QMailMessagePartContainer::MultipartRelated) {
        uint startIndex = 0;

        // If not specified, the first part is the start
        QByteArray startCID = partContainer.contentType().parameter("start");
        if (!startCID.isEmpty()) {
            for (uint i = 1; i < partContainer.partCount(); ++i) {
                if (partContainer.partAt(i).contentID() == startCID) {
                    startIndex = i;
                    break;
                }
            }
        }

        // Add any other parts as resources
        QList<const QMailMessagePart*> absentParts;
        for (uint i = 0; i < partContainer.partCount(); ++i) {
            if (i != startIndex) {
                const QMailMessagePart &part = partContainer.partAt(i);
                if (part.partialContentAvailable()) {
                    setPartResource(partContainer.partAt(i));
                } else {
                    absentParts.append(&part);
                }
            }
        }

        // Render the start part
        result += renderPart(partContainer.partAt(startIndex));

        // Show any unavailable parts as attachments
        foreach (const QMailMessagePart *part, absentParts) {
            result += renderAttachment(*part);
        }
    } else {
        // According to RFC 2046, any unrecognised type should be treated as 'mixed'
        if (partContainer.multipartType() != QMailMessagePartContainer::MultipartMixed)
            qWarning() << "Unimplemented multipart type:" << partContainer.contentType().toString();

        // Render each part successively according to its disposition
        for (uint i = 0; i < partContainer.partCount(); ++i) {
            result += renderPart(partContainer.partAt(i));
        }
    }

    return result;
}

typedef QPair<QString, QString> TextPair;

void BrowserWidget::displayHtml(const QMailMessage* mail)
{
    QString subjectText, bodyText;
    QList<TextPair> metadata;

    // For SMS messages subject is the same as body, so for SMS don't 
    // show the message text twice (same for IMs)
    if ((mail->messageType() != QMailMessage::Sms) && (mail->messageType() != QMailMessage::Instant))
        subjectText = mail->subject();

    QString from = mail->headerFieldText("From");
    if (!from.isEmpty() && from != "\"\" <>") // ugh
        metadata.append(qMakePair(tr("From"), refMailTo( mail->from() )));

    if (mail->to().count() > 0) 
        metadata.append(qMakePair(tr("To"), listRefMailTo( mail->to() )));

    if (mail->cc().count() > 0) 
        metadata.append(qMakePair(tr("CC"), listRefMailTo( mail->cc() )));

    if (mail->bcc().count() > 0) 
        metadata.append(qMakePair(tr("BCC"), listRefMailTo( mail->bcc() )));

    if (!mail->replyTo().isNull())
        metadata.append(qMakePair(tr("Reply-To"), refMailTo( mail->replyTo() )));

    metadata.append(qMakePair(tr("Date"), dateString(mail->date().toLocalTime())));

    metadata.append(qMakePair(tr("Message Id"), QString::number(mail->id().toULongLong())));
    metadata.append(qMakePair(tr("CalendarInvitation"), (mail->status() & QMailMessage::CalendarInvitation) ? QString("true") : QString("false")));

    if ( (mail->status() & QMailMessage::Incoming) && 
        !(mail->status() & QMailMessage::PartialContentAvailable) ) {
        if ( !(mail->status() & QMailMessage::Removed) ) {
            bodyText = 
"<b>WAITING_TEXT</b><br>"
"SIZE_TEXT<br>"
"<br>"
"<a href=\"download\">DOWNLOAD_TEXT</a>";

            bodyText = replaceLast(bodyText, "WAITING_TEXT", tr("Awaiting download"));
            bodyText = replaceLast(bodyText, "SIZE_TEXT", tr("Size of message content") + ": " + describeMailSize(mail->contentSize()));
            bodyText = replaceLast(bodyText, "DOWNLOAD_TEXT", tr("Download this message"));
        } else {
            // TODO: what?
        }
    } else {
        if (mail->partCount() > 0) {
            bodyText = renderMultipart(*mail);
        } else {
            bodyText = (mail->content() == QMailMessage::PlainTextContent) ? formatText(mail->body().data()) : mail->body().data();

            if (!mail->contentAvailable()) {
                QString trailer =
"<br>"
"WAITING_TEXT<br>"
"SIZE_TEXT<br>"
"<a href=\"DOWNLOAD_ACTION\">DOWNLOAD_TEXT</a>";

                trailer = replaceLast(trailer, "WAITING_TEXT", tr("More data available"));
                trailer = replaceLast(trailer, "SIZE_TEXT", tr("Size") + ": " + describeMailSize(mail->body().length()) + tr(" of ") + describeMailSize(mail->contentSize()));
                if ((mail->contentType().type().toLower() == "text") && (mail->contentType().subType().toLower() == "plain")) {
                    trailer = replaceLast(trailer, "DOWNLOAD_ACTION", "download;5120");
                } else {
                    trailer = replaceLast(trailer, "DOWNLOAD_ACTION", "download");
                }
                trailer = replaceLast(trailer, "DOWNLOAD_TEXT", tr("Retrieve more data"));

                bodyText += trailer;
            }
        }
    }

    // Form our parts into a displayable page
    QString pageData;

    if (mail->status() & QMailMessage::Removed) {
        QString noticeTemplate =
"<div align=center>"
    "NOTICE_TEXT<br>"
"</div>";

        QString notice = tr("Message deleted from server");
        if (!bodyText.isEmpty()) {
            notice.prepend("<font size=\"-5\">[");
            notice.append("]</font>");
        }

        pageData += replaceLast(noticeTemplate, "NOTICE_TEXT", notice);
    }

    QString headerTemplate = \
"<div align=left>"
    "<table border=0 cellspacing=0 cellpadding=0 width=100%>"
        "<tr>"
            "<td bgcolor=\"#000000\">"
                "<table border=0 width=100% cellspacing=1 cellpadding=4>"
                    "<tr>"
                        "<td align=left bgcolor=\"HIGHLIGHT_COLOR\">"
                            "<b><font color=\"LINK_COLOR\">SUBJECT_TEXT</font></b>"
                        "</td>"
                    "</tr>"
                    "<tr>"
                        "<td bgcolor=\"WINDOW_COLOR\">"
                            "<table border=0>"
                                "METADATA_TEXT"
                            "</table>"
                        "</td>"
                    "</tr>"
                "</table>"
            "</td>"
        "</tr>"
    "</table>"
"</div>"
"<br>";

    headerTemplate = replaceLast(headerTemplate, "HIGHLIGHT_COLOR", palette().color(QPalette::Highlight).name());
    headerTemplate = replaceLast(headerTemplate, "LINK_COLOR", palette().color(QPalette::HighlightedText).name());
    headerTemplate = replaceLast(headerTemplate, "SUBJECT_TEXT", subjectText.toHtmlEscaped());
    headerTemplate = replaceLast(headerTemplate, "WINDOW_COLOR", palette().color(QPalette::Window).name());

    QString itemTemplate =
"<tr>"
    "<td align='right' valign='top'>"
        "<b>ID_TEXT: </b>"
    "</td>"
    "<td align='left' valign='top'>"
        "CONTENT_TEXT"
    "</td>"
"</tr>";

    QString metadataText;
    foreach (const TextPair item, metadata) {
        QString element = replaceLast(itemTemplate, "ID_TEXT", item.first.toHtmlEscaped());
        element = replaceLast(element, "CONTENT_TEXT", item.second);
        metadataText.append(element);
    }

    pageData += replaceLast(headerTemplate, "METADATA_TEXT", metadataText);

    QString bodyTemplate = 
 "<div align=left>BODY_TEXT</div>";

    pageData += replaceLast(bodyTemplate, "BODY_TEXT", bodyText);

    QString pageTemplate =
"<table width=100% height=100% border=0 cellspacing=8 cellpadding=0>"
    "<tr>"
        "<td valign=\"top\">"
            "PAGE_DATA"
        "</td>"
    "</tr>"
"</table>";

    QString html = replaceLast(pageTemplate,"PAGE_DATA",pageData);
#ifdef USE_WEBKIT
    m_webView->setHtml(html);
    m_webView->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
#else
    m_renderer->setHtml(html);
#endif
}

QString BrowserWidget::describeMailSize(uint bytes) const
{
    QString size;

    // No translation?
    if (bytes < 1024) {
        size.setNum(bytes);
        size += " Bytes";
    } else if (bytes < 1024*1024) {
        size.setNum( (bytes / 1024) );
        size += " KB";
    } else {
        float f = static_cast<float>( bytes )/ (1024*1024);
        size.setNum(f, 'g', 3);
        size += " MB";
    }

    return size;
}

QString BrowserWidget::formatText(const QString& txt) const
{
    return (*this.*replySplitter)(txt);
}

QString BrowserWidget::smsBreakReplies(const QString& txt) const
{
    /*  Preserve white space, add linebreaks so text is wrapped to
        fit the display width */
    QString str = "";
    QStringList p = txt.split('\n');

    QStringList::Iterator it = p.begin();
    while ( it != p.end() ) {
        str += buildParagraph( *it, "", true ) + "<br>";
        it++;
    }

    return str;
}

QString BrowserWidget::noBreakReplies(const QString& txt) const
{
    /*  Maintains the original linebreaks, but colours the lines
        according to reply level    */
    QString str = "";
    QStringList p = txt.split('\n');

    int x, levelList;
    QStringList::Iterator it = p.begin();
    while ( it != p.end() ) {

        x = 0;
        levelList = 0;
        while (x < (*it).length() ) {
            if ( (*it)[x] == '>' ) {
                levelList++;
            } else if ( (*it)[x] == ' ' ) {
            } else break;

            x++;
        }

        if (levelList == 0 ) {
            str += encodeUrlAndMail(*it) + "<br>";
        } else {
            const QString para("<font color=\"%1\">%2</font><br>");
            str += para.arg(replyColor.name()).arg(encodeUrlAndMail(*it));
        }

        it++;
    }

    while ( str.endsWith("<br>") ) {
        str.chop(4);   //remove trailing br
    }
    return str;
}

QString appendLine(const QString& preceding, const QString& suffix)
{
    if (suffix.isEmpty())
        return preceding;

    QString result(preceding);

    int nwsIndex = QRegExp("[^\\s]").indexIn(suffix);
    if (nwsIndex > 0) {
        // This line starts with whitespace, which we'll have to protect to keep

        // We can't afford to make huge tracts of whitespace; ASCII art will be broken!
        // Convert any run of up to 4 spaces to a tab; convert all tabs to two spaces each
        QString leader(suffix.left(nwsIndex));
        leader.replace(QRegExp(" {1,4}"), "\t");

        // Convert the spaces to non-breaking
        leader.replace("\t", "&nbsp;&nbsp;");
        result.append(leader).append(suffix.mid(nwsIndex));
    }
    else
        result.append(suffix);

    return result;
}

QString unwrap(const QString& txt, const QString& prepend)
{
    QStringList lines = txt.split('\n', QString::KeepEmptyParts);

    QString result;
    result.reserve(txt.length());

    QStringList::iterator it = lines.begin(), prev = it, end = lines.end();
    if (it != end) {
        for (++it; it != end; ++prev, ++it) {
            QString terminator = "<br>";

            int prevLength = (*prev).length();
            if (prevLength == 0) {
                // If the very first line is empty, skip it; otherwise include
                if (prev == lines.begin())
                    continue;
            } else {
                int wsIndex = (*it).indexOf(QRegExp("\\s"));
                if (wsIndex == 0) {
                    // This was probably an intentional newline
                } else {
                    if (wsIndex == -1)
                        wsIndex = (*it).length();

                    bool logicalEnd = false;

                    const QChar last = (*prev)[prevLength - 1];
                    logicalEnd = ((last == '.') || (last == '!') || (last == '?'));

                    if ((*it)[0].isUpper() && logicalEnd) {
                        // This was probably an intentional newline
                    } else {
                        int totalLength = prevLength + prepend.length();
                        if ((wsIndex != -1) && ((totalLength + wsIndex) > 78)) {
                            // This was probably a forced newline - convert it to a space
                            terminator = ' ';
                        }
                    }
                }
            }

            result = appendLine(result, BrowserWidget::encodeUrlAndMail(*prev) + terminator);
        }
        if (!(*prev).isEmpty())
            result = appendLine(result, BrowserWidget::encodeUrlAndMail(*prev));
    }

    return result;
}

/*  This one is a bit complicated.  It divides up all lines according
    to their reply level, defined as count of ">" before normal text
    It then strips them from the text, builds the formatted paragraph
    and inserts them back into the beginning of each line.  Probably not
    too speed efficient on large texts, but this manipulation greatly increases
    the readability (trust me, I'm using this program for my daily email reading..)
*/
QString BrowserWidget::handleReplies(const QString& txt) const
{
    QStringList out;
    QStringList p = txt.split('\n');
    QList<uint> levelList;
    QStringList::Iterator it = p.begin();
    uint lastLevel = 0, level = 0;

    // Skip the last string, if it's non-existent
    int offset = (txt.endsWith('\n') ? 1 : 0);

    QString str, line;
    while ( (it + offset) != p.end() ) {
        line = (*it);
        level = 0;

        if ( line.startsWith(">") ) {
            for (int x = 0; x < line.length(); x++) {
                if ( line[x] == ' ') {  
                    // do nothing
                } else if ( line[x] == '>' ) {
                    level++;
                    if ( (level > 1 ) && (line[x-1] != ' ') ) {
                        line.insert(x, ' ');    //we need it to be "> > " etc..
                        x++;
                    }
                } else {
                    // make sure it follows style "> > This is easier to format"
                    if ( line[x - 1] != ' ' )
                        line.insert(x, ' ');
                    break;
                }
            }
        }

        if ( level != lastLevel ) {
            if ( !str.isEmpty() ) {
                out.append( str );
                levelList.append( lastLevel );
            }

            str.clear();
            lastLevel = level;
            it--;
        } else {
            str += line.mid(level * 2) + '\n';
        }

        it++;
    }
    if ( !str.isEmpty() ) {
        out.append( str );
        levelList.append( level );
    }

    str = "";
    lastLevel = 0;
    int pos = 0;
    it = out.begin();
    while ( it != out.end() ) {
        if ( levelList[pos] == 0 ) {
            str += unwrap( *it, "" ) + "<br>";
        } else {
            QString pre = "";
            QString preString = "";
            for ( uint x = 0; x < levelList[pos]; x++) {
                pre += "&gt; ";
                preString += "> ";
            }

            QString segment = unwrap( *it, preString );

            const QString para("<font color=\"%1\">%2</font><br>");
            str += para.arg(replyColor.name()).arg(pre + segment);
        }

        lastLevel = levelList[pos];
        pos++;
        it++;
    }

    while ( str.endsWith("<br>") ) {
        str.chop(4);   //remove trailing br
    }

    return str;
}

QString BrowserWidget::buildParagraph(const QString& txt, const QString& prepend, bool preserveWs) const
{
    Q_UNUSED(prepend);
    QStringList out;

    QString input = encodeUrlAndMail( preserveWs ? txt : txt.simplified() );
    if (preserveWs)
        return input.replace('\n', "<br>");

    QStringList p = input.split( ' ', QString::SkipEmptyParts );
    return p.join(QString(' '));
}

QString BrowserWidget::encodeUrlAndMail(const QString& txt)
{
    QStringList result;

    // TODO: is this necessary?
    // Find and encode URLs that aren't already inside anchors
    //QRegExp anchorPattern("<\\s*a\\s*href.*/\\s*a\\s*>");
    //anchorPattern.setMinimal(true);

    // We should be optimistic in our URL matching - the link resolution can
    // always fail, but if we don't match it, then we can't make it into a link
    QRegExp urlPattern("("
                            "(?:http|https|ftp)://"
                       "|"
                            "mailto:"
                       ")?"                                     // optional scheme
                       "((?:[^: \\t]+:)?[^ \\t@]+@)?"           // optional credentials
                       "("                                      // either:
                            "localhost"                             // 'localhost'
                       "|"                                      // or:
                            "(?:"                                   // one-or-more: 
                            "[A-Za-z\\d]"                           // one: legal char, 
                            "(?:"                                   // zero-or-one:
                                "[A-Za-z\\d-]*[A-Za-z\\d]"              // (zero-or-more: (legal char or '-'), one: legal char)
                            ")?"                                    // end of optional group
                            "\\."                                   // '.'
                            ")+"                                    // end of mandatory group
                            "[A-Za-z\\d]"                           // one: legal char
                            "(?:"                                   // zero-or-one:
                                "[A-Za-z\\d-]*[A-Za-z\\d]"              // (zero-or-more: (legal char or '-'), one: legal char)
                            ")?"                                    // end of optional group
                       ")"                                      // end of alternation
                       "(:\\d+)?"                               // optional port number
                       "("                                      // optional location
                            "/"                                 // beginning with a slash
                            "[A-Za-z\\d\\.\\!\\#\\$\\%\\'"      // containing any sequence of legal chars
                             "\\*\\+\\-\\/\\=\\?\\^\\_\\`"
                             "\\{\\|\\}\\~\\&\\(\\)]*"       
                       ")?");

    // Find and encode file:// links
    QRegExp filePattern("(file://\\S+)");

    // Find and encode email addresses
    QRegExp addressPattern(QMailAddress::emailAddressPattern());

    int urlPos = urlPattern.indexIn(txt);
    int addressPos = addressPattern.indexIn(txt);
    int filePos = filePattern.indexIn(txt);

    int lastPos = 0;
    while ((urlPos != -1) || (addressPos != -1) || (filePos != -1)) {
        int *matchPos = 0;
        QRegExp *matchPattern = 0;

        // Which pattern has the first match?
        if ((urlPos != -1) && 
            ((addressPos == -1) || (addressPos >= urlPos)) && 
            ((filePos == -1) || (filePos >= urlPos))) {
            matchPos = &urlPos;
            matchPattern = &urlPattern;
        } else if ((addressPos != -1) &&
                   ((urlPos == -1) || (urlPos >= addressPos)) &&
                   ((filePos == -1) || (filePos >= addressPos))) {
            matchPos = &addressPos;
            matchPattern = &addressPattern;
        } else if ((filePos != -1) &&
                   ((urlPos == -1) || (urlPos >= filePos)) &&
                   ((addressPos == -1) || (addressPos >= filePos))) {
            matchPos = &filePos;
            matchPattern = &filePattern;
        } else {
            Q_ASSERT(false);
            return QString();
        }

        QString replacement;

        if (matchPattern == &urlPattern) {
            // Is this a valid URL?
            QString scheme = urlPattern.cap(1);
            QString credentials = urlPattern.cap(2);
            QString host(urlPattern.cap(3));

            // Ensure that the host is not purely a number
            // Also ignore credentials with no scheme
            if (scheme.isEmpty() && 
                ((host.indexOf(QRegExp("[^\\d\\.]")) == -1) || (!credentials.isEmpty()))) {
                // Ignore this match
                urlPos = urlPattern.indexIn(txt, urlPos + 1);
                continue;
            } else {
                char parenTypes[] = { '(', ')', '[', ']', '{', '}', '<', '>', '\0' };

                QString leading;
                QString trailing;
                QString url = urlPattern.cap(0);

                QChar firstChar(url.at(0));
                QChar lastChar(url.at(url.length() - 1));

                for (int i = 0; parenTypes[i] != '\0'; i += 2) {
                    if ((firstChar == parenTypes[i]) || (lastChar == parenTypes[i+1])) {
                        // Check for unbalanced parentheses
                        int left = url.count(parenTypes[i]);
                        int right = url.count(parenTypes[i+1]);

                        if ((right > left) && (lastChar == parenTypes[i+1])) {
                            // The last parenthesis is probably not part of the URL
                            url = url.left(url.length() - 1);
                            trailing.append(lastChar);
                            lastChar = url.at(url.length() - 1);
                        } else if ((right < left) && (firstChar == parenTypes[i])) {
                            // The first parenthesis is probably not part of the URL
                            url = url.mid(1);
                            leading.append(firstChar);
                            firstChar = url.at(0);
                        }
                    }
                }

                if ((lastChar == '.') || (lastChar == ',') || (lastChar == ';')) {
                    // The last character is probably part of the surrounding text
                    url = url.left(url.length() - 1);
                    trailing = lastChar;
                }

                replacement = refUrl(url, scheme, leading, trailing);
            }
        } else if (matchPattern == &addressPattern) {
            QString address = addressPattern.cap(0);

            replacement = refMailTo(QMailAddress(address));
        } else if (matchPattern == &filePattern) {
            QString file = filePattern.cap(0);

            replacement = refUrl(file, "file://", QString(), QString());
        }

        // Write the unmatched text out in escaped form
        result.append(txt.mid(lastPos, (*matchPos - lastPos)).toHtmlEscaped());

        result.append(replacement);

        // Find the following pattern match for this pattern
        lastPos = *matchPos + matchPattern->cap(0).length();
        *matchPos = matchPattern->indexIn(txt, lastPos);

        // Bypass any other matches contained within the matched text
        if ((urlPos != -1) && (urlPos < lastPos)) {
            urlPos = urlPattern.indexIn(txt, lastPos);
        }
        if ((addressPos != -1) && (addressPos < lastPos)) {
            addressPos = addressPattern.indexIn(txt, lastPos);
        }
        if ((filePos != -1) && (filePos < lastPos)) {
            filePos = filePattern.indexIn(txt, lastPos);
        }
    }

    if (lastPos < txt.length()) {
        result.append(txt.mid(lastPos).toHtmlEscaped());
    }

    return result.join("");
}

void BrowserWidget::scrollToAnchor(const QString& anchor)
{
#ifdef USE_WEBKIT
    // TODO
    Q_UNUSED(anchor);
#else
    m_renderer->scrollToAnchor(anchor);
#endif
}

void BrowserWidget::setPlainText(const QString& text)
{
#ifdef USE_WEBKIT
    QString html(text.toHtmlEscaped());
    html.replace("\n", "<br>");
    m_webView->setHtml("<html><body>" + html + "</body></html>");
    m_webView->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
#else
    m_renderer->setPlainText(text);
#endif
}

void BrowserWidget::addAction(QAction* action)
{
#ifdef USE_WEBKIT
    m_webView->addAction(action);
#else
    m_renderer->addAction(action);
#endif
}

void BrowserWidget::addActions(const QList<QAction*>& actions)
{
#ifdef USE_WEBKIT
    m_webView->addActions(actions);
#else
    m_renderer->addActions(actions);
#endif
}

void BrowserWidget::removeAction(QAction* action)
{
#ifdef USE_WEBKIT
    m_webView->removeAction(action);
#else
    m_renderer->removeAction(action);
#endif
}

QString BrowserWidget::listRefMailTo(const QList<QMailAddress>& list)
{
    QStringList result;
    foreach ( const QMailAddress& address, list )
        result.append( refMailTo( address ) );

    return result.join( ", " );
}

QString BrowserWidget::refMailTo(const QMailAddress& address)
{
    QString name = address.toString().toHtmlEscaped();
    if (name == "System")
        return name;

    if (address.isPhoneNumber() || address.isEmailAddress())
        return "<a href=\"mailto:" + address.address().toHtmlEscaped() + "\">" + name + "</a>";

    return name;
}

QString BrowserWidget::refNumber(const QString& number)
{
    return "<a href=\"dial;" + number.toHtmlEscaped() + "\">" + number + "</a>";
}

QString BrowserWidget::refUrl(const QString& url, const QString& scheme, const QString& leading, const QString& trailing)
{
    // Assume HTTP if there is no scheme
    QString escaped(url.toHtmlEscaped());
    QString target(scheme.isEmpty() ? "http://" + escaped : escaped);

    return leading.toHtmlEscaped() + "<a href=\"" + target + "\">" + escaped + "</a>" + trailing.toHtmlEscaped();
}

#include "browserwidget.moc"


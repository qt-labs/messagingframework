/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#include "emailcomposer.h"
#include <qmailmessage.h>
#include <qmailglobal.h>
#include <QAction>
#include <QFile>
#include <QFileInfo>
#include <QKeyEvent>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>
#include <QTextCursor>
#include <QTextEdit>
#include <QStackedWidget>
#include <qmailaccount.h>
#include <QInputContext>
#include <QStyle>
#include <QMenu>
#include <qmailnamespace.h>
#include <QApplication>
#include <QtPlugin>
#include <detailspage_p.h>

class BodyTextEdit : public QTextEdit
{
    Q_OBJECT

public:
    BodyTextEdit(EmailComposerInterface* composer, QWidget* parent = 0);

    bool isComposing();
    bool isEmpty();

signals:
    void finished();

protected:
    void keyPressEvent(QKeyEvent* e);

private:
    EmailComposerInterface* m_composer;
};

BodyTextEdit::BodyTextEdit(EmailComposerInterface* composer, QWidget* parent )
:
QTextEdit(parent),
m_composer(composer)
{
}

bool BodyTextEdit::isComposing()
{
    return (inputContext() && inputContext()->isComposing());
}

bool BodyTextEdit::isEmpty()
{
    if (!document()->isEmpty())
        return false;

    // Otherwise there may be pre-edit input queued in the input context
    return !isComposing();
}

void BodyTextEdit::keyPressEvent(QKeyEvent* e)
{
    static const bool keypadAbsent(style()->inherits("QThumbStyle"));

    if (keypadAbsent) {
        if (e->key() == Qt::Key_Back) {
            e->accept();
            emit finished();
            return;
        }
    } else {
        if (e->key() == Qt::Key_Select) {
            if (!m_composer->isEmpty()) {
                e->accept();
                emit finished();
            } else {
                e->ignore();
            }
            return;
        }

        if (e->key() == Qt::Key_Back) {
//            if( Qtopia::mousePreferred() ) {
//                e->ignore();
//                return;
//            } else if (isEmpty()) {
                if(isEmpty()) {
                e->accept();
                emit finished();
                return;
            }
        }
    }

    QTextEdit::keyPressEvent( e );
}

static void addActionsFromWidget(QWidget* sourceWidget, QMenu* targetMenu)
{
    if(!sourceWidget) return;
    foreach(QAction* a,sourceWidget->actions())
        targetMenu->addAction(a);
}

void EmailComposerInterface::updateLabel()
{
    static const bool keypadAbsent(style()->inherits("QThumbStyle"));
    if (keypadAbsent) {
        //if (isEmpty())
            //QSoftMenuBar::setLabel(m_bodyEdit, Qt::Key_Back, QSoftMenuBar::Cancel);
        //else
            //QSoftMenuBar::setLabel(m_bodyEdit, Qt::Key_Back, QSoftMenuBar::Next);
    } else {
        //if (isEmpty()) {
            //QSoftMenuBar::setLabel(m_bodyEdit, Qt::Key_Back, QSoftMenuBar::Cancel);
            //QSoftMenuBar::setLabel(m_bodyEdit, Qt::Key_Select, QSoftMenuBar::NoLabel);
        //} else {
            //QSoftMenuBar::clearLabel(m_bodyEdit, Qt::Key_Back);

          //  if (m_bodyEdit->isComposing())
                //QSoftMenuBar::clearLabel(m_bodyEdit, Qt::Key_Select);
          //  else
                //QSoftMenuBar::setLabel(m_bodyEdit, Qt::Key_Select, QSoftMenuBar::Next);
        //}
    }
}

void EmailComposerInterface::setCursorPosition()
{
    if (m_cursorIndex != -1) {
        QTextCursor cursor(m_bodyEdit->textCursor());
        cursor.setPosition(m_cursorIndex, QTextCursor::MoveAnchor);
        m_bodyEdit->setTextCursor(cursor);

        m_cursorIndex = -1;
    }
}

void EmailComposerInterface::updateAttachmentsLabel()
{
    /*
    int count = 0;
    int sizeKB = 0;

    foreach (const AttachmentItem* item, m_addAttDialog->attachedFiles()) {
        ++count;
        sizeKB += item->sizeKB();
    }

    if (count == 0) {
        m_attachmentsLabel->hide();
    } else {
        m_attachmentsLabel->setText(QString("<center><small>") + tr("%n Attachment(s): %1KB","", count).arg(sizeKB)+"</small></center>");
        m_attachmentsLabel->show();
    }
    */
}

void EmailComposerInterface::detailsPage()
{
}

void EmailComposerInterface::composePage()
{
}


void EmailComposerInterface::selectAttachment()
{
    /*
    if (m_attachments.isEmpty() && m_addAttDialog->documentSelector()->documents().isEmpty()) {
        QMessageBox::warning(this,
                             tr("No documents"),
                             tr("There are no existing documents to attach"),
                             tr("OK") );
    } else {
        if (QtopiaApplication::execDialog(m_addAttDialog) == QDialog::Accepted) {
            m_attachments.clear();
            foreach (const AttachmentItem *item, m_addAttDialog->attachedFiles())
                m_attachments.append(qMakePair(item->document(), item->action()));
        } else {
            m_addAttDialog->clear();
            foreach (const AttachmentDetail &att, m_attachments)
                m_addAttDialog->attach(att.first, att.second);
        }
    }
    */
}

EmailComposerInterface::EmailComposerInterface( QWidget *parent )
    : QMailComposerInterface( parent ),
    m_cursorIndex( -1 ),
    m_composerWidget(0),
    m_bodyEdit(0),
    m_attachmentsLabel(0),
    m_widgetStack(0),
    m_detailsWidget(0),
    m_title(QString())
{
    init();
}

EmailComposerInterface::~EmailComposerInterface()
{
    /*
    // Delete any temporary files we don't need
    foreach (const AttachmentDetail &att, m_attachments) {
        if (att.second == QMailMessage::CopyAndDeleteAttachments) {
            const_cast<QContent&>(att.first).removeFiles();
        }
    }
    */
}

void EmailComposerInterface::init()
{
    //main layout
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins(0,0,0,0);
    QWidget::setLayout(layout);

    m_detailsWidget = new DetailsPage(this,"send-message");
    m_detailsWidget->setType(QMailMessage::Email);
    connect( m_detailsWidget, SIGNAL(changed()), this, SIGNAL(changed()));
    connect( m_detailsWidget, SIGNAL(sendMessage()), this, SIGNAL(sendMessage()));
    connect( m_detailsWidget, SIGNAL(cancel()), this, SIGNAL(cancel()));
    connect( m_detailsWidget, SIGNAL(editMessage()), this, SLOT(composePage()));
    layout->addWidget(m_detailsWidget);


    //body edit
    m_bodyEdit = new BodyTextEdit(this,m_composerWidget);
    m_bodyEdit->setFrameStyle(QFrame::NoFrame);
    m_bodyEdit->setWordWrapMode(QTextOption::WordWrap);
    connect(m_bodyEdit, SIGNAL(textChanged()), this, SIGNAL(changed()) );
    connect(m_bodyEdit, SIGNAL(finished()),this,SLOT(detailsPage()));
    connect(m_bodyEdit->document(), SIGNAL(contentsChanged()), this, SLOT(updateLabel()));
    layout->addWidget(m_bodyEdit);

    //attachments label
    m_attachmentsLabel = new QLabel(this);
    layout->addWidget(m_attachmentsLabel);
    m_attachmentsLabel->hide();

//    m_addAttDialog = new AddAttDialog(this, "attachmentDialog");
//    connect(m_addAttDialog,SIGNAL(attachmentsChanged()),this,SLOT(updateAttachmentsLabel()));
//    connect(m_addAttDialog,SIGNAL(attachmentsChanged()),this,SLOT(updateLabel()));
//    connect(m_addAttDialog,SIGNAL(attachmentsChanged()),this,SIGNAL(changed()));

    //menus
    QAction *attachAction = new QAction( QIcon( ":icon/attach" ), tr("Attachments") + "...", this);
    connect( attachAction, SIGNAL(triggered()), this, SLOT(selectAttachment()) );
    //QMenu* bodyEditMenu = QSoftMenuBar::menuFor(m_bodyEdit);
    //bodyEditMenu->addSeparator();
    //bodyEditMenu->addAction(attachAction);
    //addActionsFromWidget(QWidget::parentWidget(),bodyEditMenu);

    setContext("Create " + displayName(QMailMessage::Email));

    composePage();
    updateLabel();
}

void EmailComposerInterface::setPlainText( const QString& text, const QString& signature )
{
    if (!signature.isEmpty()) {
        QString msgText(text);
        if (msgText.endsWith(signature)) {
            // Signature already exists
            m_cursorIndex = msgText.length() - (signature.length() + 1);
        } else {
            // Append the signature
            msgText.append('\n').append(signature);
            m_cursorIndex = text.length();
        }

        m_bodyEdit->setPlainText(msgText);

        // Move the cursor before the signature - setting directly fails...
        QTimer::singleShot(0, this, SLOT(setCursorPosition()));
    } else {
        m_bodyEdit->setPlainText(text);
        m_bodyEdit->moveCursor(QTextCursor::End);
    }
    //m_bodyEdit->setEditFocus(true);
}

void EmailComposerInterface::setContext(const QString& title)
{
    m_title = title;
    emit contextChanged();
}

bool EmailComposerInterface::isEmpty() const
{
    return m_bodyEdit->isEmpty();
    //return (m_bodyEdit->isEmpty() && m_attachments.isEmpty());
}

QMailMessage EmailComposerInterface::message() const
{
    QMailMessage mail;

    QString messageText( m_bodyEdit->toPlainText() );

    QMailMessageContentType type("text/plain; charset=UTF-8");
    //if (m_attachments.isEmpty()) {
        mail.setBody( QMailMessageBody::fromData( messageText, type, QMailMessageBody::Base64 ) );
    //} else {
    //    QMailMessagePart textPart;
    //    textPart.setBody(QMailMessageBody::fromData(messageText.toUtf8(), type, QMailMessageBody::Base64));
    //    mail.setMultipartType(QMailMessagePartContainer::MultipartMixed);
    //    mail.appendPart(textPart);

/*
        foreach (const AttachmentDetail &current, m_attachments) {
            const QContent &document(current.first);

            QFileInfo fi(document.fileName());
            QString partName(fi.fileName());
            QString filePath(fi.absoluteFilePath());

            QString mimeTypeName(document.type());
            if (mimeTypeName.isEmpty())
                mimeTypeName = QMimeType(filePath).id();

            QMailMessageContentType type(mimeTypeName.toLatin1());
            type.setName(document.name().toLatin1());

            QMailMessageContentDisposition disposition( QMailMessageContentDisposition::Attachment );
            disposition.setFilename(partName.toLatin1());

            QMailMessagePart part;

            QString location = document.property("qtopiamail/partLocation");
            if (!location.isEmpty()) {
                // This part is only a reference to another part
                QMailMessagePart::Location partLocation(location);
                const QMailMessage original(partLocation.containingMessageId());
                if (original.id().isValid()) {
                    const QMailMessagePart &srcPart(original.partAt(partLocation));

                    part = QMailMessagePart::fromPartReference(partLocation, srcPart.contentDisposition(), srcPart.contentType(), srcPart.transferEncoding());
                } else {
                    qWarning() << "Unable to locate referenced message:" << partLocation.toString(true);
                }
            } else {
                if ((current.second != QMailMessage::LinkToAttachments) ||
                    (filePath.startsWith(Qtopia::tempDir()))) {
                    // This file is temporary - extract the data and create a part from that
                    QFile dataFile(filePath);
                    if (dataFile.open(QIODevice::ReadOnly)) {
                        QDataStream in(&dataFile);

                        part = QMailMessagePart::fromStream(in, disposition, type, QMailMessageBody::Base64, QMailMessageBody::RequiresEncoding);
                    } else {
                        qWarning() << "Unable to open temporary file:" << filePath;
                    }
                } else {
                    part = QMailMessagePart::fromFile(filePath, disposition, type, QMailMessageBody::Base64, QMailMessageBody::RequiresEncoding);
                }
            }

            mail.appendPart(part);
        }
        */
    //}

    mail.setMessageType( QMailMessage::Email );
    m_detailsWidget->getDetails(mail);

    return mail;
}

void EmailComposerInterface::clear()
{
    m_bodyEdit->clear();
    //m_addAttDialog->clear();

    // Delete any temporary files we don't need
    /*
    foreach (const AttachmentDetail &att, m_attachments) {
        if (att.second == QMailMessage::CopyAndDeleteAttachments) {
            const_cast<QContent&>(att.first).removeFiles();
        }
    }

    m_attachments.clear();
    */
}

void EmailComposerInterface::setMessage( const QMailMessage &mail )
{
    if (mail.multipartType() == QMailMessagePartContainer::MultipartNone) {
        if (mail.hasBody())
            setBody( mail.body().data(), mail.contentType().content() );
    } else {
        // The only type of multipart message we currently compose is Mixed, with
        // all but the first part as out-of-line attachments
        int textPart = -1;
        for ( uint i = 0; i < mail.partCount(); ++i ) {
            QMailMessagePart &part = const_cast<QMailMessagePart&>(mail.partAt(i));

            if (textPart == -1 && part.hasBody() && (part.contentType().type().toLower() == "text")) {
                // This is the first text part, we will use as the forwarded text body
                textPart = i;
            } else {
                // Save the existing part data to a temporary file
                QString fileName(part.writeBodyTo(QMail::tempPath()));
                if (fileName.isEmpty()) {
                    qWarning() << "Unable to save part to temporary file!";
                } else {
                    /*
                    // Create a content object for the file
                    QContent doc(fileName);

                    if (part.referenceType() == QMailMessagePart::None) {
                        QMailMessageContentType type(part.contentType());

                        if (doc.drmState() == QContent::Unprotected)
                            doc.setType(type.content());
                    } else {
                        QString partLocation = part.location().toString(true);
                        if (!partLocation.isEmpty()) {
                            doc.setProperty("qtopiamail/partLocation", partLocation);
                        }
                    }

                    doc.setName(part.displayName());
                    doc.setRole(QContent::Data);
                    doc.commit();

                    attach(doc, QMailMessage::CopyAndDeleteAttachments);
                    */
                }
            }
        }

        if (textPart != -1) {
            const QMailMessagePart& part = mail.partAt(textPart);
            setBody( part.body().data(), part.contentType().content() );
        }
    }

    //set the details
    //m_detailsWidget->setDetails(mail);
}

void EmailComposerInterface::setBody( const QString &txt, const QString &type )
{
    setPlainText( txt, m_signature );
    Q_UNUSED(type)
}

/*
void EmailComposerInterface::attach( const QContent &lnk, QMailMessage::AttachmentsAction action )
{
    m_attachments.append(qMakePair(lnk, action));
    m_addAttDialog->attach(lnk, action);
}
*/

void EmailComposerInterface::setSignature( const QString &sig )
{
    QString msgText( m_bodyEdit->toPlainText() );

    if ( !msgText.isEmpty() && !m_signature.isEmpty() ) {
        // See if we need to remove the old signature
        if ( msgText.endsWith( m_signature ) )
            msgText.chop( m_signature.length() + 1 );
    }

    m_signature = sig;
    setPlainText( msgText, m_signature );
}

// sharp 1839 to here
static void checkOutlookString(QString &str)
{
    int  pos = 0;
    int  newPos;
    QString  oneAddr;

    QStringList  newStr;
    if (str.indexOf(";") == -1) {
        // not Outlook style
        return;
    }

    while ((newPos = str.indexOf(";", pos)) != -1) {
        if (newPos > pos + 1) {
            // found some string
            oneAddr = str.mid(pos, newPos-pos);

            if (oneAddr.indexOf("@") != -1) {
                // not Outlook comment
                newStr.append(oneAddr);
            }
        }
        if ((pos = newPos + 1) >= str.length()) {
            break;
        }
    }

    str = newStr.join(", ");
}

void EmailComposerInterface::reply(const QMailMessage& source, int action)
{
    const QString fwdIndicator(tr("Fwd"));
    const QString shortFwdIndicator(tr("Fw", "2 letter short version of Fwd for forward"));
    const QString replyIndicator(tr("Re"));

    const QString subject = source.subject().toLower();

    QString toAddress;
    QString fromAddress;
    QString ccAddress;
    QString subjectText;

    if (source.parentAccountId().isValid()) {
        QMailAccount sendingAccount(source.parentAccountId());
        fromAddress = sendingAccount.fromAddress().address();
    }

    // work out the kind of mail to response
    // type of reply depends on the type of message
    // a reply to an mms is just a new mms message with the sender as recipient
    // a reply to an sms is a new sms message with the sender as recipient

    // EMAIL
    QString originalText;
    int textPart = -1;
    QMailMessage mail;

    // Find the body of this message
    if ( source.hasBody() ) {
        originalText = source.body().data();
    } else {
        for ( uint i = 0; i < source.partCount(); ++i ) {
            const QMailMessagePart &part = source.partAt(i);

            if (part.contentType().type().toLower() == "text") {
                // This is the first text part, we will use as the forwarded text body
                originalText = part.body().data();
                textPart = i;
                break;
            }
        }
    }

    if ( action == Forward ) {
        // Copy the existing mail
        mail = source;

        if ((subject.left(fwdIndicator.length() + 1) == (fwdIndicator.toLower() + ":")) ||
                (subject.left(shortFwdIndicator.length() + 1) == (shortFwdIndicator.toLower() + ":"))) {
            subjectText = source.subject();
        } else {
            subjectText = fwdIndicator + ": " + source.subject();
        }
    } else {
        // Maintain the same ID in case we need part locations
        mail.setId(source.id());

        if (subject.left(replyIndicator.length() + 1) == (replyIndicator.toLower() + ":")) {
            subjectText = source.subject();
        } else {
            subjectText = replyIndicator + ": " + source.subject();
        }

        QMailAddress replyAddress(source.replyTo());
        if (replyAddress.isNull())
            replyAddress = source.from();

        QString str = replyAddress.address();
        checkOutlookString(str);
        toAddress = str;

        QString messageId = mail.headerFieldText( "message-id" ).trimmed();
        if ( !messageId.isEmpty() )
            mail.setInReplyTo( messageId );
    }

    QString bodyText;
    if (action == Forward) {
        bodyText = "\n------------ Forwarded Message ------------\n";
        bodyText += "Date: " + source.date().toString() + "\n";
        bodyText += "From: " + source.from().toString() + "\n";
        bodyText += "To: " + QMailAddress::toStringList(source.to()).join(", ") + "\n";
        bodyText += "Subject: " + source.subject() + "\n";
        bodyText += "\n" + originalText;
    } else {
        QDateTime dateTime = source.date().toLocalTime();
        bodyText = "\nOn " + dateTime.toString(Qt::ISODate) + ", ";
        bodyText += source.from().name() + " wrote:\n> ";

        int pos = bodyText.length();
        bodyText += originalText;
        while ((pos = bodyText.indexOf('\n', pos)) != -1)
            bodyText.insert(++pos, "> ");

        bodyText.append("\n");
    }

    // Whatever text subtype it was before, it's now plain...
    QMailMessageContentType contentType("text/plain; charset=UTF-8");

    if (mail.partCount() == 0) {
        // Set the modified text as the body
        mail.setBody(QMailMessageBody::fromData(bodyText, contentType, QMailMessageBody::Base64));
    } else if (textPart != -1) {
        // Replace the original text with our modified version
        QMailMessagePart& part = mail.partAt(textPart);
        part.setBody(QMailMessageBody::fromData(bodyText, contentType, QMailMessageBody::Base64));
    }

    if (action == ReplyToAll) {
        // Set the reply-to-all address list
        QList<QMailAddress> all;
        foreach (const QMailAddress& addr, source.to() + source.cc())
            if ((addr.address() != fromAddress) && (addr.address() != toAddress))
                all.append(addr);

        QString cc = QMailAddress::toStringList(all).join(", ");
        checkOutlookString( cc );
        ccAddress = cc;
    }

    mail.removeHeaderField("To");
    mail.removeHeaderField("Cc");
    mail.removeHeaderField("Bcc");
    mail.removeHeaderField("From");
    mail.removeHeaderField("Reply-To");

    if (!toAddress.isEmpty())
        mail.setTo(QMailAddress(toAddress));
    if (!fromAddress.isEmpty())
        mail.setFrom(QMailAddress(fromAddress));
    if (!ccAddress.isEmpty())
        mail.setCc(QMailAddress::fromStringList(ccAddress));
    if (!subjectText.isEmpty())
        mail.setSubject(subjectText);

    setMessage( mail );

    QString task;
    if ((action == Create) || (action == Forward)) {
        task = (action == Create ? tr("Create") : tr("Forward"));
        task += " " + displayName(QMailMessage::Email);
    } else if (action == Reply) {
        task = tr("Reply");
    } else if (action == ReplyToAll) {
        task = tr("Reply to all");
    }
    setContext(task);
}

void EmailComposerInterface::setTo(const QString& toAddress)
{
    m_detailsWidget->setTo(toAddress);
}

void EmailComposerInterface::setFrom(const QString& fromAddress)
{
    m_detailsWidget->setFrom(fromAddress);
}

void EmailComposerInterface::setCc(const QString& ccAddress)
{
    m_detailsWidget->setCc(ccAddress);
}

void EmailComposerInterface::setBcc(const QString& bccAddress)
{
    m_detailsWidget->setBcc(bccAddress);
}

void EmailComposerInterface::setSubject(const QString& subject)
{
    m_detailsWidget->setSubject(subject);
}

QString EmailComposerInterface::from() const
{
    return m_detailsWidget->from();
}

QString EmailComposerInterface::to() const
{
    return m_detailsWidget->to();
}

QString EmailComposerInterface::cc() const
{
    return m_detailsWidget->cc();
}

QString EmailComposerInterface::bcc() const
{
    return m_detailsWidget->bcc();
}

bool EmailComposerInterface::isReadyToSend() const
{
  bool ready = !from().trimmed().isEmpty() &&
           (!to().trimmed().isEmpty() ||
           !cc().trimmed().isEmpty() ||
           !bcc().trimmed().isEmpty());
  return ready;
}

bool EmailComposerInterface::isDetailsOnlyMode() const
{
    return false;
//    return m_detailsWidget->isDetailsOnlyMode();
}

void EmailComposerInterface::setDetailsOnlyMode(bool val)
{
    /*
    m_detailsWidget->setDetailsOnlyMode(val);
    if(val)
        detailsPage();
    */
}

QString EmailComposerInterface::contextTitle() const
{
    return m_title;
}

QMailAccount EmailComposerInterface::fromAccount() const
{
    return QMailAccount();
//    return m_detailsWidget->fromAccount();
}

QString EmailComposerInterface::key() const { return "EmailComposer"; }

QList<QMailMessage::MessageType> EmailComposerInterface::messageTypes() const
{
    return QList<QMailMessage::MessageType>() << QMailMessage::Email;
}

QList<QMailMessage::ContentType> EmailComposerInterface::contentTypes() const
{
    return QList<QMailMessage::ContentType>() << QMailMessage::RichTextContent
        << QMailMessage::PlainTextContent
        << QMailMessage::VCardContent
        << QMailMessage::MultipartContent;
}

QString EmailComposerInterface::name(QMailMessage::MessageType) const { return qApp->translate("EmailComposerPlugin","Email"); }

QString EmailComposerInterface::displayName(QMailMessage::MessageType) const { return qApp->translate("EmailComposerPlugin","Email"); }

QIcon EmailComposerInterface::displayIcon(QMailMessage::MessageType) const { return QIcon(":icon/email"); }

Q_EXPORT_PLUGIN( EmailComposerInterface)


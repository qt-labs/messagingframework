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
#include <QComboBox>
#include <QLineEdit>
#include <QToolButton>
#include <qmailaccountkey.h>
#include <qmailstore.h>
#include <QListWidget>
#include <QPushButton>
#include <QStringListModel>
#include <QFileDialog>
#include "attachmentlistwidget.h"
#include <support/qmailnamespace.h>
#include <QUrl>

static int minimumLeftWidth = 65;
static const QString placeholder("(no subject)");

enum RecipientType {To, Cc, Bcc };
typedef QPair<RecipientType,QString> Recipient;
typedef QList<Recipient> RecipientList;

class RecipientWidget : public QWidget
{
    Q_OBJECT

public:
    RecipientWidget(QWidget* parent = 0);

    bool isRemoveEnabled() const;
    void setRemoveEnabled(bool val);

    QString recipient() const;
    void setRecipient(const QString& r);
    bool isEmpty() const;
    void reset();
    void clear();

    RecipientType recipientType() const;
    void setRecipientType(RecipientType r);

signals:
    void removeClicked();
    void recipientChanged();

protected:
    bool eventFilter(QObject *obj, QEvent *event);

private:
    QComboBox* m_typeCombo;
    QLineEdit* m_recipientEdit;
    QToolButton* m_removeButton;
};

RecipientWidget::RecipientWidget(QWidget* parent)
:
QWidget(parent),
m_typeCombo(new QComboBox(this)),
m_recipientEdit(new QLineEdit(this)),
m_removeButton(new QToolButton(this))
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins(0,0,0,0);

    m_typeCombo->addItem("To",To);
    m_typeCombo->addItem("Cc",Cc);
    m_typeCombo->addItem("Bcc",Bcc);
    layout->addWidget(m_typeCombo);
    m_typeCombo->setFocusPolicy(Qt::NoFocus);
    m_typeCombo->setMinimumWidth(minimumLeftWidth);

    connect(m_recipientEdit,SIGNAL(textEdited(QString)),this,SIGNAL(recipientChanged()));
    layout->addWidget(m_recipientEdit);
    setFocusProxy(m_recipientEdit);
    m_recipientEdit->installEventFilter(this);

    m_removeButton->setIcon(QIcon(":icon/clear"));
    connect(m_removeButton,SIGNAL(clicked(bool)),this,SIGNAL(removeClicked()));
    layout->addWidget(m_removeButton);
    m_removeButton->setFocusPolicy(Qt::NoFocus);

    setFocusPolicy(Qt::StrongFocus);
}

bool RecipientWidget::isRemoveEnabled() const
{
    return m_removeButton->isEnabled();
}

void RecipientWidget::setRemoveEnabled(bool val)
{
    m_removeButton->setEnabled(val);
}

QString RecipientWidget::recipient() const
{
    return m_recipientEdit->text();
}

void RecipientWidget::setRecipient(const QString& r)
{
    m_recipientEdit->setText(r);
}

bool RecipientWidget::isEmpty() const
{
    return recipient().isEmpty();
}

void RecipientWidget::clear()
{
   m_recipientEdit->clear();
}

RecipientType RecipientWidget::recipientType() const
{
    return static_cast<RecipientType>(m_typeCombo->itemData(m_typeCombo->currentIndex()).toUInt());
}

void RecipientWidget::setRecipientType(RecipientType t)
{
    for(int index = 0; index < m_typeCombo->count(); index++)
    {
        RecipientType v = static_cast<RecipientType>(m_typeCombo->itemData(index).toUInt());
        if(v == t)
        {
            m_typeCombo->setCurrentIndex(index);
            break;
        }
    }
}

bool RecipientWidget::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == m_recipientEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if(keyEvent->key() == Qt::Key_Backspace)
            if(isEmpty())
            {
                emit removeClicked();
                return true;
            }
        return false;
    } else
        return QObject::eventFilter(obj, event);
}

class RecipientListWidget : public QWidget
{
    Q_OBJECT

public:
    RecipientListWidget(QWidget* parent = 0);
    QStringList recipients(RecipientType t) const;
    QStringList recipients() const;
    void setRecipients(RecipientType, const QStringList& list);
    void reset();
    void clear();

signals:
    void changed();

private:
    int emptyRecipientSlots() const;
    bool containRecipient(RecipientType t, const QString& address) const;

private slots:
    RecipientWidget* addRecipientWidget();
    void removeRecipientWidget();
    void recipientChanged();

private:
    QVBoxLayout* m_layout;
    QList<RecipientWidget*> m_widgetList;
};

RecipientListWidget::RecipientListWidget(QWidget* parent )
:
QWidget(parent),
m_layout(new QVBoxLayout(this))
{
    m_layout->setSpacing(0);
    m_layout->setContentsMargins(0,0,0,0);
    reset();
}

QStringList RecipientListWidget::recipients(RecipientType t) const
{
    QStringList results;

    foreach(RecipientWidget* r,m_widgetList)
        if(!r->isEmpty() && r->recipientType() == t)
            results.append(r->recipient());

    return results;
}

QStringList RecipientListWidget::recipients() const
{
    QStringList results;

    foreach(RecipientWidget* r,m_widgetList)
        if(!r->isEmpty())
            results.append(r->recipient());

    return results;
}

void RecipientListWidget::setRecipients(RecipientType t, const QStringList& addresses)
{
    if(addresses.isEmpty())
        return;

    foreach(RecipientWidget* r, m_widgetList)
    {
        if(r->isEmpty())
        {
            m_widgetList.removeAll(r);
            delete r;
        }
    }

    foreach(QString address, addresses)
    {
        if(!containRecipient(t,address))
        {
            RecipientWidget* r = addRecipientWidget();
            r->setRecipientType(t);
            r->setRecipient(address);
        }
    }
    addRecipientWidget();
}

void RecipientListWidget::reset()
{
    clear();
    addRecipientWidget();
}

void RecipientListWidget::clear()
{
	foreach(RecipientWidget* r, m_widgetList)
	{
		m_widgetList.removeAll(r);
        delete r;
	}
}

int RecipientListWidget::emptyRecipientSlots() const
{
    int emptyCount = 0;
    foreach(RecipientWidget* r,m_widgetList)
    {
        if(r->isEmpty())
            emptyCount++;
    }
    return emptyCount;
}

bool RecipientListWidget::containRecipient(RecipientType t, const QString& address) const
{
    foreach(RecipientWidget* r,m_widgetList)
    {
        if(r->recipientType() == t && r->recipient() == address)
            return true;
    }
    return false;
}

RecipientWidget* RecipientListWidget::addRecipientWidget()
{
    RecipientWidget* r = new RecipientWidget(this);
    connect(r,SIGNAL(removeClicked()),this,SLOT(removeRecipientWidget()));
    connect(r,SIGNAL(recipientChanged()),this,SLOT(recipientChanged()));
    connect(r,SIGNAL(removeClicked()),this,SIGNAL(changed()));
    connect(r,SIGNAL(recipientChanged()),this,SIGNAL(changed()));

    m_layout->addWidget(r);
    if(!m_widgetList.isEmpty())
        m_widgetList.last()->setTabOrder(m_widgetList.last(),r);

    r->setRemoveEnabled(!m_widgetList.isEmpty());
    m_widgetList.append(r);

    return r;
}

void RecipientListWidget::removeRecipientWidget()
{
    if(RecipientWidget* r = qobject_cast<RecipientWidget*>(sender()))
    {
        if(m_widgetList.count() <= 1)
            return;
        int index = m_widgetList.indexOf(r);
        m_widgetList.removeAll(r);
        delete r;

        if(index >= m_widgetList.count())
            index = m_widgetList.count()-1;

        if(m_widgetList.at(index)->isEmpty() && index > 0)
            index--;
        m_widgetList.at(index)->setFocus();

    }
}

void RecipientListWidget::recipientChanged()
{
    if(qobject_cast<RecipientWidget*>(sender()))
    {
        if(emptyRecipientSlots() == 0)
            addRecipientWidget();
    }
}

class BodyTextEdit : public QTextEdit
{
    Q_OBJECT

public:
    BodyTextEdit(EmailComposerInterface* composer, QWidget* parent = 0);

    bool isComposing();
    bool isEmpty();

signals:
    void finished();
    void positionChanged(int row, int col);

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

void EmailComposerInterface::selectAttachment()
{
    QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Select attachments"));
    m_attachmentListWidget->addAttachments(fileNames);
}

EmailComposerInterface::EmailComposerInterface( QWidget *parent )
    : QMailComposerInterface( parent ),
    m_cursorIndex( -1 ),
    m_composerWidget(0),
    m_bodyEdit(0),
    m_attachmentsLabel(0),
    m_widgetStack(0),
    m_attachmentAction(0),
    m_recipientListWidget(0),
    m_attachmentListWidget(0),
    m_subjectEdit(0),
    m_title(QString())
{
    init();
}

EmailComposerInterface::~EmailComposerInterface()
{
}

void EmailComposerInterface::init()
{
    //main layout
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins(0,0,0,0);
    QWidget::setLayout(layout);

    m_recipientListWidget = new RecipientListWidget(this);
    layout->addWidget(m_recipientListWidget);

    QWidget* subjectPanel = new QWidget(this);
    QHBoxLayout* subjectLayout = new QHBoxLayout(subjectPanel);
    subjectLayout->setSpacing(0);
    subjectLayout->setContentsMargins(0,0,0,0);
    QLabel* l = new QLabel("Subject:");
    l->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    l->setMinimumWidth(minimumLeftWidth);
    subjectLayout->addWidget(l);
    subjectLayout->addWidget(m_subjectEdit = new QLineEdit(subjectPanel));
    connect(m_subjectEdit,SIGNAL(textEdited(QString)),this,SIGNAL(statusChanged(QString)));
    subjectPanel->setLayout(subjectLayout);
    layout->addWidget(subjectPanel);

    connect(m_recipientListWidget,SIGNAL(changed()),this,SIGNAL(changed()));

    //body edit
    m_bodyEdit = new BodyTextEdit(this,m_composerWidget);
    m_bodyEdit->setWordWrapMode(QTextOption::WordWrap);
    connect(m_bodyEdit, SIGNAL(textChanged()), this, SIGNAL(changed()) );
    connect(m_bodyEdit->document(), SIGNAL(contentsChanged()), this, SLOT(updateLabel()));
    layout->addWidget(m_bodyEdit);

    //attachments label
    m_attachmentsLabel = new QLabel(this);
    layout->addWidget(m_attachmentsLabel);
    m_attachmentsLabel->hide();
    layout->addWidget(m_attachmentListWidget = new AttachmentListWidget(this));

    //menus
    m_attachmentAction = new QAction( QIcon( ":icon/attach" ), tr("Attachments") + "...", this);
    connect( m_attachmentAction, SIGNAL(triggered()), this, SLOT(selectAttachment()) );
    updateLabel();

    setTabOrder(m_recipientListWidget,m_subjectEdit);
    setTabOrder(m_subjectEdit,m_bodyEdit);
    setFocusProxy(m_bodyEdit);

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
    }
}

void EmailComposerInterface::getDetails(QMailMessage& mail) const
{
    mail.setTo(QMailAddress::fromStringList(m_recipientListWidget->recipients(To)));
    mail.setCc(QMailAddress::fromStringList(m_recipientListWidget->recipients(Cc)));
    mail.setBcc(QMailAddress::fromStringList(m_recipientListWidget->recipients(Bcc)));

    QString subjectText = m_subjectEdit->text();

    if (!subjectText.isEmpty())
        mail.setSubject(subjectText);
    else
        subjectText = placeholder;
}

void EmailComposerInterface::setDetails(const QMailMessage& mail)
{
    m_recipientListWidget->setRecipients(To,QMailAddress::toStringList(mail.to()));
    m_recipientListWidget->setRecipients(Cc,QMailAddress::toStringList(mail.cc()));
    m_recipientListWidget->setRecipients(Bcc,QMailAddress::toStringList(mail.bcc()));

    if ((mail.subject() != placeholder))
       m_subjectEdit->setText(mail.subject());
}

bool EmailComposerInterface::isEmpty() const
{
    return m_bodyEdit->isEmpty() && m_attachmentListWidget->isEmpty();
}

QMailMessage EmailComposerInterface::message() const
{
    QMailMessage mail;

    QString messageText( m_bodyEdit->toPlainText() );

    QMailMessageContentType type("text/plain; charset=UTF-8");
    if (m_attachmentListWidget->isEmpty()) {
        mail.setBody( QMailMessageBody::fromData( messageText, type, QMailMessageBody::Base64 ) );
    } else {
        QMailMessagePart textPart;
        textPart.setBody(QMailMessageBody::fromData(messageText.toUtf8(), type, QMailMessageBody::Base64));
        mail.setMultipartType(QMailMessagePartContainer::MultipartMixed);
        mail.appendPart(textPart);

        foreach (const QString& attachment, m_attachmentListWidget->attachments()) {
            QFileInfo fi(attachment);
            QString partName(fi.fileName());
            QString filePath(fi.absoluteFilePath());

            QMailMessageContentType type(QMail::mimeTypeFromFileName(attachment).toLatin1());
            type.setName(partName.toLatin1());

            QMailMessageContentDisposition disposition( QMailMessageContentDisposition::Attachment );
            disposition.setFilename(partName.toLatin1());

            QMailMessagePart part = QMailMessagePart::fromFile(filePath, disposition, type, QMailMessageBody::Base64, QMailMessageBody::RequiresEncoding);
            part.setContentLocation(QUrl::fromLocalFile(filePath).toString());
            mail.appendPart(part);
        }
    }

    mail.setMessageType( QMailMessage::Email );
    getDetails(mail);

    return mail;
}

void EmailComposerInterface::clear()
{
    m_subjectEdit->clear();
    m_recipientListWidget->reset();

    m_bodyEdit->clear();
    m_attachmentListWidget->clear();
}

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

void EmailComposerInterface::create(const QMailMessage& sourceMail)
{
    if (sourceMail.multipartType() == QMailMessagePartContainer::MultipartNone) {
        if (sourceMail.hasBody())
            setPlainText( sourceMail.body().data(), m_signature );
    } else {
        // The only type of multipart message we currently compose is Mixed, with
        // all but the first part as out-of-line attachments
        int textPart = -1;
        for ( uint i = 0; i < sourceMail.partCount(); ++i ) {
            QMailMessagePart &part = const_cast<QMailMessagePart&>(sourceMail.partAt(i));
            QString contentLocation = part.contentLocation().remove(QRegExp("\\s"));
            bool isLocalAttachment = part.hasBody() && QFile::exists(QUrl(contentLocation).toLocalFile());

            if (textPart == -1 && part.hasBody() && (part.contentType().type().toLower() == "text") && !isLocalAttachment) {
                // This is the first text part, we will use as the forwarded text body
                textPart = i;
            } else if(isLocalAttachment) {
                    m_attachmentListWidget->addAttachment(QUrl(contentLocation).toLocalFile());
                }
            }
        if (textPart != -1) {
            const QMailMessagePart& part = sourceMail.partAt(textPart);
            setPlainText( part.body().data(), m_signature );
        }
    }
    //set the details
    setDetails(sourceMail);

    m_bodyEdit->setFocus();
    m_bodyEdit->moveCursor(QTextCursor::Start);
    emit changed();
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

    create( mail );
}

bool EmailComposerInterface::isReadyToSend() const
{
  bool ready = !m_recipientListWidget->recipients().isEmpty();
  //QMailMessage m = message();
  //ready &= m.parentAccountId().isValid();
  return ready;
}

QString EmailComposerInterface::title() const
{
    return m_title;
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

void EmailComposerInterface::compose(ComposeContext context, const QMailMessage& sourceMail, QMailMessage::MessageType)
{
    switch(context)
    {
        case Create:
            create(sourceMail);
        break;
        case Reply:
            reply(sourceMail,Reply);
        break;
        case ReplyToAll:
            reply(sourceMail,ReplyToAll);
        break;
        case Forward:
            reply(sourceMail,Forward);
    }
}

QList<QAction*> EmailComposerInterface::actions() const
{
    return QList<QAction*>() << m_attachmentAction;
}

QString EmailComposerInterface::status() const
{
    return m_subjectEdit->text();
}

Q_EXPORT_PLUGIN( EmailComposerInterface)

#include <emailcomposer.moc>

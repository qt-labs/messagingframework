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
#include <QUrl>
#include <QSyntaxHighlighter>
#include <QCompleter>

static int minimumLeftWidth = 65;
static const QString placeholder("(no subject)");

enum RecipientType {To, Cc, Bcc };
typedef QPair<RecipientType,QString> Recipient;
typedef QList<Recipient> RecipientList;

static QCompleter* sentFolderCompleter()
{
    const int completionAddressLimit(1000);
    QSet<QString> addressSet;
    QMailMessageKey::Properties props(QMailMessageKey::Recipients);
    QMailMessageKey key(QMailMessageKey::status(QMailMessage::Sent));
    QMailMessageMetaDataList metaDataList(QMailStore::instance()->messagesMetaData(key, props, QMailStore::ReturnDistinct));
    foreach (const QMailMessageMetaData &metaData, metaDataList) {
        foreach(QMailAddress address, metaData.recipients()) {
            QString s(address.toString());
            if (!s.simplified().isEmpty()) {
                addressSet.insert(s);
            }
        }
        if (addressSet.count() >= completionAddressLimit)
            break;
    }

    QCompleter *completer(new QCompleter(addressSet.toList()));
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    return completer;
}

class RecipientWidget : public QWidget
{
    Q_OBJECT

public:
    RecipientWidget(QWidget* parent = Q_NULLPTR);

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
    
    m_recipientEdit->setCompleter(sentFolderCompleter());
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
    RecipientListWidget(QWidget* parent = Q_NULLPTR);
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

    setUpdatesEnabled(false);

    m_layout->addWidget(r);
    if(!m_widgetList.isEmpty())
        m_widgetList.last()->setTabOrder(m_widgetList.last(),r);

    r->setRemoveEnabled(!m_widgetList.isEmpty());
    m_widgetList.append(r);

    updateGeometry();
    setUpdatesEnabled(true);

    return r;
}

void RecipientListWidget::removeRecipientWidget()
{
    if(RecipientWidget* r = qobject_cast<RecipientWidget*>(sender()))
    {
        if(m_widgetList.count() <= 1)
            return;
        setUpdatesEnabled(false);
        int index = m_widgetList.indexOf(r);
        m_widgetList.removeAll(r);

        m_layout->removeWidget(r);
        r->deleteLater();

        if(index >= m_widgetList.count())
            index = m_widgetList.count()-1;

        if(m_widgetList.at(index)->isEmpty() && index > 0)
            index--;
        m_widgetList.at(index)->setFocus();

        updateGeometry();
        setUpdatesEnabled(true);

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

class Dictionary {
public:
    static Dictionary *instance();
    bool contains(const QString &word) { return _words.contains(word.toLower()); }
    bool empty() { return _words.isEmpty(); }

private:
    Dictionary();
    static Dictionary *_sDictionary;
    QSet<QString> _words;
};

Dictionary* Dictionary::_sDictionary = 0;

Dictionary* Dictionary::instance()
{
    if (!_sDictionary)
        _sDictionary = new Dictionary();
    return _sDictionary;
}

Dictionary::Dictionary()
{
    //TODO Consider using Hunspell
    QStringList dictionaryFiles;
    dictionaryFiles << "/usr/share/dict/words" << "/usr/dict/words";
    foreach(QString fileName, dictionaryFiles) {
        QFileInfo info(fileName);
        if (info.isReadable()) {
            QFile file(fileName);
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
                continue;
            QTextStream stream(&file);
            QString line;
            while (!stream.atEnd()) {
                line = stream.readLine();
                if (!line.isEmpty())
                    _words.insert(line.toLower());
            }
            file.close();
            return;
        }
    }
}

class SpellingHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
    
public:    
    SpellingHighlighter(QTextEdit *parent) :QSyntaxHighlighter(parent) {};

protected:
    virtual void highlightBlock(const QString &text);
};

void SpellingHighlighter::highlightBlock(const QString &text)
{
    if(text.startsWith(EmailComposerInterface::quotePrefix()))
        return; //don't find errors in quoted text

    Dictionary *dictionary = Dictionary::instance();
    QTextCharFormat spellingFormat;
    spellingFormat.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
    spellingFormat.setUnderlineColor(Qt::red);

    QRegExp wordExpression("\\b\\w+\\b");
    int index = text.indexOf(wordExpression);
    while (index >= 0) {
        int length = wordExpression.matchedLength();
        if (!dictionary->contains(text.mid(index, length)))
            setFormat(index, length, spellingFormat);
        index = text.indexOf(wordExpression, index + length);
    }
}

class BodyTextEdit : public QTextEdit
{
    Q_OBJECT

public:
    BodyTextEdit(EmailComposerInterface* composer, QWidget* parent = Q_NULLPTR);

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
    // ### FIXME always false since Qt 5 port
    return false;
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
    m_forwardLabel(0),
    m_forwardEdit(0),
    m_attachmentsLabel(0),
    m_widgetStack(0),
    m_attachmentAction(0),
    m_recipientListWidget(0),
    m_attachmentListWidget(0),
    m_subjectEdit(0),
    m_title(QString()),
    m_sourceStatus(0)
{
    init();
}

EmailComposerInterface::~EmailComposerInterface()
{
    // Delete any temporary files we don't need
    foreach (const QString file, m_temporaries) {
        if (!QFile::remove(file)) {
            qWarning() << "Unable to remove temporary file:" << file;
        }
    }
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
    QLabel* l = new QLabel(tr("Subject:"));
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
    new SpellingHighlighter(m_bodyEdit);
    m_bodyEdit->setWordWrapMode(QTextOption::WordWrap);
    connect(m_bodyEdit, SIGNAL(textChanged()), this, SIGNAL(changed()) );
    connect(m_bodyEdit->document(), SIGNAL(contentsChanged()), this, SLOT(updateLabel()));
    layout->addWidget(m_bodyEdit);

    m_forwardLabel = new QLabel(tr("Forwarded content:"));
    m_forwardLabel->setVisible(false);
    layout->addWidget(m_forwardLabel);

    //forwarded element edit
    m_forwardEdit = new QTextEdit(m_composerWidget);
    m_forwardEdit->setWordWrapMode(QTextOption::WordWrap);
    m_forwardEdit->setReadOnly(true);

    // Why doesn't this work?
    //m_forwardEdit->setAutoFillBackground(true);
    //m_forwardEdit->setBackgroundRole(QPalette::Window);
    QPalette p(m_forwardEdit->palette());
    p.setColor(QPalette::Active, QPalette::Base, p.color(QPalette::Window));
    m_forwardEdit->setPalette(p);

    m_forwardEdit->setVisible(false);
    layout->addWidget(m_forwardEdit);

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
    setTabOrder(m_bodyEdit,m_forwardEdit);
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
       m_subjectEdit->setText(mail.subject().simplified());
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
        mail.setBody( QMailMessageBody::fromData( messageText, type, QMailMessageBody::QuotedPrintable ) );
    } else {
        QMailMessagePart textPart;
        textPart.setBody(QMailMessageBody::fromData(messageText.toUtf8(), type, QMailMessageBody::QuotedPrintable));
        mail.setMultipartType(QMailMessagePartContainer::MultipartMixed);
        mail.appendPart(textPart);

        foreach (const QString& attachment, m_attachmentListWidget->attachments()) {
            if (attachment.startsWith("ref:")) {
                const QMailMessage referencedMessage(QMailMessageId(attachment.mid(4).toULongLong()));

                QMailMessageContentType type;
                if (referencedMessage.multipartType() == QMailMessage::MultipartNone) {
                    // The part will have the same type as the original message
                    type = referencedMessage.contentType();
                } else {
                    // Encapsulate the entire message
                    type = QMailMessageContentType("message/rfc822");
                }

                QMailMessageContentDisposition disposition(QMailMessageContentDisposition::Inline);
                disposition.setSize(referencedMessage.size());
                mail.appendPart(QMailMessagePart::fromMessageReference(referencedMessage.id(), disposition, type, referencedMessage.transferEncoding()));

                mail.setStatus(QMailMessage::HasReferences, true);
                mail.setStatus(QMailMessage::HasUnresolvedReferences, true);
            } else if (attachment.startsWith("partRef:")) {
                QMailMessagePart::Location partLocation(attachment.mid(8));
                const QMailMessage referencedMessage(partLocation.containingMessageId());
                const QMailMessagePart &existingPart(referencedMessage.partAt(partLocation));

                QMailMessageContentDisposition existingDisposition(existingPart.contentDisposition());
                
                QMailMessageContentDisposition disposition(QMailMessageContentDisposition::Inline);
                disposition.setFilename(existingDisposition.filename());
                disposition.setSize(existingDisposition.size());

                mail.appendPart(QMailMessagePart::fromPartReference(existingPart.location(), disposition, existingPart.contentType(), existingPart.transferEncoding()));

                mail.setStatus(QMailMessage::HasReferences, true);
                mail.setStatus(QMailMessage::HasUnresolvedReferences, true);
            } else {
                QFileInfo fi(attachment);
                QString partName(fi.fileName());
                QString filePath(fi.absoluteFilePath());

                QString mimeType(QMail::mimeTypeFromFileName(attachment));
                QMailMessageContentType type(mimeType.toLatin1());
                type.setName(partName.toLatin1());

                QMailMessageContentDisposition disposition( QMailMessageContentDisposition::Attachment );
                disposition.setFilename(partName.toLatin1());
                disposition.setSize(fi.size());

                QMailMessageBodyFwd::TransferEncoding encoding(QMailMessageBody::Base64);
                QMailMessageBodyFwd::EncodingStatus encodingStatus(QMailMessageBody::RequiresEncoding);
                if (mimeType == "message/rfc822") {
                    encoding = QMailMessageBody::NoEncoding;
                    encodingStatus = QMailMessageBody::AlreadyEncoded;
                }
                QMailMessagePart part = QMailMessagePart::fromFile(filePath, disposition, type, encoding, encodingStatus);
                mail.appendPart(part);

                // Store the location of this file for future reference
                const QMailMessagePart &mailPart(mail.partAt(mail.partCount() - 1));
                QString name("qmf-file-location-" + mailPart.location().toString(false));
                mail.setCustomField(name, filePath);
            }
        }
    }

    mail.setMessageType( QMailMessage::Email );
    getDetails(mail);

    if (m_sourceStatus & QMailMessage::LocalOnly) {
        mail.setStatus(QMailMessage::LocalOnly, true);
    }
    // Set the size estimate
    mail.setSize(mail.indicativeSize() * 1024);
    mail.setStatus(QMailMessage::HasAttachments, m_attachmentListWidget->attachments().count());

    return mail;
}

void EmailComposerInterface::clear()
{
    m_subjectEdit->clear();
    m_recipientListWidget->reset();

    m_bodyEdit->clear();
    m_attachmentListWidget->clear();

    // Delete any temporary files we don't need
    foreach (const QString file, m_temporaries) {
        if (!QFile::remove(file)) {
            qWarning() << "Unable to remove temporary file:" << file;
        }
    }

    // Any newly-composed messages are local messages only
    m_sourceStatus |= QMailMessage::LocalOnly;
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

void EmailComposerInterface::setSendingAccountId( const QMailAccountId &accountId )
{
    m_accountId = accountId;
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
    m_forwardLabel->setVisible(false);
    m_forwardEdit->setVisible(false);

    if (sourceMail.multipartType() == QMailMessagePartContainer::MultipartNone) {
        if (sourceMail.hasBody())
            setPlainText( sourceMail.body().data(), m_signature );
    } else {
        // The only type of multipart message we currently compose is Mixed, with
        // all but the first part as out-of-line attachments
        int textPart = -1;
        for ( uint i = 0; i < sourceMail.partCount(); ++i ) {
            const QMailMessagePart &part(sourceMail.partAt(i));

            // See if we have a filename to link to
            QString name("qmf-file-location-" + part.location().toString(false));
            QString contentLocation = sourceMail.customField(name);
            if (contentLocation.isEmpty()) {
                // See if we can use the value in the message (remove any folded whitespace)
                contentLocation = QUrl(part.contentLocation().remove(QRegExp("\\s"))).toLocalFile();
            }

            if (part.referenceType() != QMailMessagePart::None) {
                // Put the referenced data in the forwarded pane
                if (part.referenceType() == QMailMessagePart::MessageReference) {
                    QMailMessage referencedMessage(part.messageReference());

                    m_attachmentListWidget->addAttachment(QString("ref:%1").arg(QString::number(referencedMessage.id().toULongLong())));

                    if ((referencedMessage.multipartType() == QMailMessage::MultipartNone) &&
                        (referencedMessage.hasBody()) &&
                        (referencedMessage.contentType().type().toLower() == "text")) {
                            m_forwardEdit->setPlainText(referencedMessage.body().data());
                    } else {
                        m_forwardEdit->setPlainText(tr("<Message content on server>"));
                    }
                } else {
                    QMailMessagePart::Location partLocation(part.partReference());

                    QMailMessage referencedMessage(partLocation.containingMessageId());
                    const QMailMessagePart &referencedPart(referencedMessage.partAt(partLocation));

                    m_attachmentListWidget->addAttachment(QString("partRef:%1").arg(referencedPart.location().toString(true)));

                    if (referencedPart.hasBody() &&
                        (referencedMessage.contentType().type().toLower() == "text")) {
                        m_forwardEdit->setPlainText(referencedPart.body().data());
                    } else {
                        m_forwardEdit->setPlainText(tr("<Message content on server>"));
                    }
                }

                m_forwardLabel->setVisible(true);
                m_forwardEdit->setVisible(true);
            } else if (part.hasBody() || !contentLocation.isEmpty()) {
                bool localFile(!contentLocation.isEmpty() && QFile::exists(contentLocation));

                if ((textPart == -1) && (!localFile) && (part.contentType().type().toLower() == "text")) {
                    // This is the first text part, we will use as the forwarded text body
                    textPart = i;
                } else {
                    if (!localFile) {
                        // We need to create a temporary copy of this part's data to link to
                        QString fileName(part.writeBodyTo(QMail::tempPath()));
                        if (fileName.isEmpty()) {
                            qWarning() << "Unable to save part to temporary file!";
                            continue;
                        } else {
                            m_temporaries.append(fileName);
                            contentLocation = fileName;
                        }
                    }

                    m_attachmentListWidget->addAttachment(contentLocation);
                }
            }
        }

        if (textPart != -1) {
            const QMailMessagePart& part = sourceMail.partAt(textPart);
            setPlainText( part.body().data(), m_signature );
        }
    }

    //set the details
    setDetails(sourceMail);

    m_sourceStatus = sourceMail.status(); 
    if (!sourceMail.id().isValid()) {
        // This is a new message
        m_sourceStatus |= QMailMessage::LocalOnly;
    }

    m_bodyEdit->setFocus();
    m_bodyEdit->moveCursor(QTextCursor::Start);
    emit changed();
}

void EmailComposerInterface::respond(QMailMessage::ResponseType type, const QMailMessage& source, const QMailMessagePart::Location& partLocation)
{
    const QString fwdIndicator(tr("Fwd"));
    const QString shortFwdIndicator(tr("Fw", "2 letter short version of Fwd for forward"));
    const QString replyIndicator(tr("Re"));

    const QString subject = source.subject().toLower();

    QString toAddress;
    QString ccAddress;
    QString subjectText;

    QMailAccount sendingAccount(m_accountId);
    QString fromAddress(sendingAccount.fromAddress().address());

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

    if ((type == QMailMessage::Forward) || (type == QMailMessage::ForwardPart)) {
        // Copy the existing mail
        mail = source;

        if ((subject.left(fwdIndicator.length() + 1) == (fwdIndicator.toLower() + ":")) ||
            (subject.left(shortFwdIndicator.length() + 1) == (shortFwdIndicator.toLower() + ":"))) {
            subjectText = source.subject().simplified();
        } else {
            subjectText = fwdIndicator + ": " + source.subject().simplified();
        }
    } else {
        // Maintain the same ID in case we need part locations
        mail.setId(source.id());

        if (subject.left(replyIndicator.length() + 1) == (replyIndicator.toLower() + ":")) {
            subjectText = source.subject().simplified();
        } else {
            subjectText = replyIndicator + ": " + source.subject().simplified();
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

    QMailMessageContentType textContentType("text/plain; charset=UTF-8");

    QString bodyText;
    if ((type == QMailMessage::Forward) || (type == QMailMessage::ForwardPart)) {
        QString forwardBlock = "\n------------ Forwarded Message ------------\n";
        forwardBlock += "Date: " + source.date().toString() + '\n';
        forwardBlock += "From: " + source.from().toString() + '\n';
        forwardBlock += "To: " + QMailAddress::toStringList(source.to()).join(QLatin1String(", ")) + '\n';
        forwardBlock += "Subject: " + source.subject().simplified() + '\n';

        QMailAccount originAccount(source.parentAccountId());
        bool viaReference((originAccount.status() & QMailAccount::CanReferenceExternalData) &&
                          (sendingAccount.status() & QMailAccount::CanTransmitViaReference));

        if (type == QMailMessage::ForwardPart) {
            mail.clearParts();
            mail.setMultipartType(QMailMessage::MultipartMixed);

            mail.appendPart(QMailMessagePart::fromData(forwardBlock, QMailMessageContentDisposition(QMailMessageContentDisposition::Inline), textContentType, QMailMessageBody::Base64));

            // Add only the single relevant part
            QMailMessageContentDisposition disposition(QMailMessageContentDisposition::Attachment);
            
            const QMailMessagePart &sourcePart(source.partAt(partLocation));
            if (viaReference) {
                mail.appendPart(QMailMessagePart::fromPartReference(partLocation, disposition, sourcePart.contentType(), sourcePart.transferEncoding()));
            } else {
                // We need to create a copy of the original part
                QByteArray partData(sourcePart.body().data(QMailMessageBody::Decoded));
                mail.appendPart(QMailMessagePart::fromData(partData, disposition, sourcePart.contentType(), sourcePart.transferEncoding()));
            }
            
            bodyText = forwardBlock + '\n' + originalText;
            textPart = -1;
        } else {
            if (viaReference) {
                // We can send this message by reference
                mail.clearParts();
                mail.setMultipartType(QMailMessage::MultipartMixed);

                QMailMessageContentDisposition disposition(QMailMessageContentDisposition::Inline);

                mail.appendPart(QMailMessagePart::fromData(forwardBlock, disposition, textContentType, QMailMessageBody::Base64));
                mail.appendPart(QMailMessagePart::fromMessageReference(source.id(), disposition, source.contentType(), source.transferEncoding()));

                textPart = -1;
            } else {
                bodyText = forwardBlock + '\n' + originalText;
            }
        }
    } else {
        QDateTime dateTime = source.date().toLocalTime();
        bodyText = "\nOn " + dateTime.toString(Qt::ISODate) + ", ";
        bodyText += source.from().name() + " wrote:\n" + EmailComposerInterface::quotePrefix();

        int pos = bodyText.length();
        bodyText += originalText;
        while ((pos = bodyText.indexOf('\n', pos)) != -1)
            bodyText.insert(++pos, EmailComposerInterface::quotePrefix());

        bodyText.append('\n');
    }

    // Whatever text subtype it was before, it's now plain...
    if (mail.partCount() == 0) {
        // Set the modified text as the body
        mail.setBody(QMailMessageBody::fromData(bodyText, textContentType, QMailMessageBody::Base64));
    } else if (textPart != -1) {
        // Replace the original text with our modified version
        QMailMessagePart& part = mail.partAt(textPart);
        part.setBody(QMailMessageBody::fromData(bodyText, textContentType, QMailMessageBody::Base64));
    }

    if (type == QMailMessage::ReplyToAll) {
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

void EmailComposerInterface::compose(QMailMessage::ResponseType type, const QMailMessage& sourceMail, const QMailMessagePart::Location& sourceLocation, QMailMessage::MessageType)
{
    if (type == QMailMessage::NoResponse) {
        create(sourceMail);
    } else if (type == QMailMessage::Redirect) {
        // TODO: Implement redirect
        qWarning() << "Unable to handle request to redirect!";
    } else {
        respond(type, sourceMail, sourceLocation);
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

QString EmailComposerInterface::quotePrefix()
{
    return "> ";
}

#include "emailcomposer.moc"

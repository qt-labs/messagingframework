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

#include "writemail.h"
#include "selectcomposerwidget.h"
#include "readmail.h"
#include <qmailaccountkey.h>
#include <qmailstore.h>
#include <qmaillog.h>
#include <qmailaccount.h>
#include <qmailcomposer.h>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QStackedWidget>
#include <QToolBar>
#include "qtmailnamespace.h"

static const int defaultWidth = 500;
static const int defaultHeight = 400;

static QMailAccountIdList sendingAccounts(QMailMessage::MessageType messageType)
{
    QMailAccountKey statusKey(QMailAccountKey::status(QMailAccount::CanTransmit, QMailDataComparator::Includes));
    statusKey &= QMailAccountKey::status(QMailAccount::Enabled, QMailDataComparator::Includes);
    QMailAccountKey typeKey(QMailAccountKey::messageType(messageType));
    QMailAccountIdList accountIds = QMailStore::instance()->queryAccounts(statusKey & typeKey);
    return accountIds;

}

WriteMail::WriteMail(QWidget* parent)
    :
    QMainWindow(parent),
    m_composerInterface(0),
    m_cancelAction(0),
    m_draftAction(0),
    m_sendAction(0),
    m_widgetStack(0),
    m_selectComposerWidget(0),
    m_replyAction(QMailMessage::NoResponse),
    m_toolbar(0),
    m_accountSelection(0)
{
    init();
}

void WriteMail::init()
{
    m_toolbar = new QToolBar(this);
    addToolBar(m_toolbar);
    m_widgetStack = new QStackedWidget(this);

    m_cancelAction = new QAction(Qtmail::icon("cancel"),tr("Close"),this);
    connect( m_cancelAction, SIGNAL(triggered()), this, SLOT(discard()) );
    addAction(m_cancelAction);

    m_draftAction = new QAction(Qtmail::icon("saveToDraft"),tr("Save in drafts"),this);
    connect( m_draftAction, SIGNAL(triggered()), this, SLOT(draft()) );
    m_draftAction->setWhatsThis( tr("Save this message as a draft.") );
    addAction(m_draftAction);

    m_sendAction = new QAction(Qtmail::icon("sendmail"),tr("Send"),this);
    connect( m_sendAction, SIGNAL(triggered()), this, SLOT(sendStage()) );
    m_sendAction->setWhatsThis( tr("Send the message.") );
    addAction(m_sendAction);

    m_accountSelection = new QComboBox(m_toolbar);
    connect( m_accountSelection, SIGNAL(currentIndexChanged(int)), this, SLOT(accountSelectionChanged(int)) );

    m_selectComposerWidget = new SelectComposerWidget(this);
    m_selectComposerWidget->setObjectName("selectComposer");
    connect(m_selectComposerWidget, SIGNAL(selected(QPair<QString,QMailMessage::MessageType>)),
            this, SLOT(composerSelected(QPair<QString,QMailMessage::MessageType>)));
    connect(m_selectComposerWidget, SIGNAL(cancel()), this, SLOT(discard()));
    m_widgetStack->addWidget(m_selectComposerWidget);

    setCentralWidget(m_widgetStack);

    QMenuBar* mainMenuBar = new QMenuBar();
    setMenuBar(mainMenuBar);

    QMenu* file = mainMenuBar->addMenu("File");
    file->addAction(m_sendAction);
    file->addAction(m_draftAction);
    file->addSeparator();
    file->addAction(m_cancelAction);

    m_toolbar->addAction(m_sendAction);
    m_toolbar->addAction(m_draftAction);
    m_toolbar->addWidget(m_accountSelection);
    m_toolbar->addSeparator();

    setWindowTitle(tr("Compose"));

    setGeometry(0,0,defaultWidth,defaultHeight);
}

bool WriteMail::sendStage()
{
    if (!isComplete()) {
        // The user must either complete the message, save as draft or explicitly cancel
        return false;
    }

    if (buildMail(true)) {
        if (largeAttachments()) {
            // Ensure this is intentional
            if (QMessageBox::question(qApp->activeWindow(),
                                      tr("Large attachments"),
                                      tr("The message has large attachments. Send now?"),
                                      QMessageBox::Yes | QMessageBox::No) == QMessageBox::No) {
                draft();
                QMessageBox::warning(qApp->activeWindow(),
                                     tr("Message saved"),
                                     tr("The message has been saved in the Drafts folder"),
                                     QMessageBox::Ok);
                return true;
            }
        }

        emit enqueueMail(mail);
    } else {
        qMailLog(Messaging) << "Unable to build mail for transmission!";
    }

    // Prevent double deletion of composer textedit that leads to crash on exit
    reset();
    close();

    return true;
}

void WriteMail::accountSelectionChanged(int index)
{
    if (m_composerInterface) {
        QMailAccountId accountId(m_accountSelection->itemData(index).value<QMailAccountId>());
        m_composerInterface->setSendingAccountId(accountId);
    }
}

bool WriteMail::isComplete() const
{
    if (m_composerInterface && m_composerInterface->isReadyToSend()) {
        if (changed() || (QMessageBox::question(qApp->activeWindow(),
                                                tr("Empty message"),
                                                tr("The message is currently empty. Do you wish to send an empty message?"),
                                                QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)) {
            return true;
        }
    } else {
        QMessageBox::warning(qApp->activeWindow(),
                             tr("Incomplete message"),
                             tr("The message cannot be sent until at least one recipient has been entered."),
                             QMessageBox::Ok);
    }

    return false;
}

bool WriteMail::saveChangesOnRequest()
{
    // ideally, you'd also check to see if the message is the same as it was
    // when we started working on it
    // don't bother with a prompt for empty composers. Not likely to want to save empty draft.
    if (m_hasMessageChanged && hasContent() &&
        QMessageBox::warning(this,
                             tr("Save to drafts"),
                             tr("Do you wish to save the message to drafts?"),
                             QMessageBox::Yes,
                             QMessageBox::No) == QMessageBox::Yes) {
        draft();
    } else {
        discard();
    }

    return true;
}

bool WriteMail::buildMail(bool includeSignature)
{
    QMailAccountId accountId(m_accountSelection->itemData(m_accountSelection->currentIndex()).value<QMailAccountId>());

    // Ensure the signature of the selected account is used
    if (accountId.isValid() && includeSignature) {
        m_composerInterface->setSignature(signature(accountId));
    }

    // Extract the message from the composer
    QMailMessage newMail = m_composerInterface->message();

    // Retain the old mail properties if they're configured
    newMail.setId(mail.id());
    newMail.setParentFolderId(mail.parentFolderId());
    newMail.setContentScheme(mail.contentScheme());
    newMail.setContentIdentifier(mail.contentIdentifier());
    newMail.setServerUid(mail.serverUid());
    newMail.setCustomFields(mail.customFields());

    newMail.setDate(QMailTimeStamp::currentDateTime());
    newMail.setStatus(QMailMessage::Outgoing, true);
    newMail.setStatus(QMailMessage::ContentAvailable, true);
    newMail.setStatus(QMailMessage::PartialContentAvailable, true);
    newMail.setStatus(QMailMessage::Read, true);

    if (accountId.isValid()) {
        newMail.setParentAccountId(accountId);
        newMail.setFrom(QMailAccount(accountId).fromAddress());
    }

    if (!newMail.parentFolderId().isValid()) {
        newMail.setParentFolderId(QMailFolder::LocalStorageFolderId);
    }

    if (m_precursorId.isValid()) {
        newMail.setInResponseTo(m_precursorId);
        newMail.setResponseType(m_replyAction);

        QMailMessage precursor(m_precursorId);

        // Set the In-Reply-To and References headers in our outgoing message
        QString references(precursor.headerFieldText("References"));
        if (references.isEmpty()) {
            references = precursor.headerFieldText("In-Reply-To");
        }

        QString precursorId(precursor.headerFieldText("Message-ID"));
        if (!precursorId.isEmpty()) {
            newMail.setHeaderField("In-Reply-To", precursorId);

            if (!references.isEmpty()) {
                references.append(' ');
            }
            references.append(precursorId);
        }

        if (!references.isEmpty()) {
            // TODO: Truncate references if they're too long
            newMail.setHeaderField("References", references);
        }
    }

    mail = newMail;
    return true;
}

/*  TODO: connect this method to the input widgets so we can actually know whether
    the mail was changed or not */
bool WriteMail::changed() const
{
    if (!m_composerInterface || m_composerInterface->isEmpty())
        return false;

    return true;
}

void WriteMail::create(const QMailMessage& initMessage)
{
    prepareComposer(initMessage.messageType(), initMessage.parentAccountId());
    if (composer().isEmpty())
        return;

    m_composerInterface->compose(QMailMessage::NoResponse, initMessage);
    m_hasMessageChanged = true;
}

void WriteMail::respond(const QMailMessage& source, QMailMessage::ResponseType type)
{
    prepareComposer(source.messageType(), source.parentAccountId());
    if (composer().isEmpty())
        return;

    m_composerInterface->compose(type, source);
    m_hasMessageChanged = true;
    m_precursorId = source.id();
    m_replyAction = type;
}

void WriteMail::respond(const QMailMessagePart::Location& sourceLocation, QMailMessage::ResponseType type)
{
    QMailMessage source(sourceLocation.containingMessageId());

    prepareComposer(source.messageType(), source.parentAccountId());
    if (composer().isEmpty())
        return;

    m_composerInterface->compose(type, source, sourceLocation);
    m_hasMessageChanged = true;
    m_precursorId = source.id();
    m_replyAction = type;
}

void WriteMail::modify(const QMailMessage& previousMessage)
{
    QString recipients = "";

    prepareComposer(previousMessage.messageType(), previousMessage.parentAccountId());
    if (composer().isEmpty())
        return;

    // Record any message properties we should retain
    mail.setId(previousMessage.id());
    mail.setParentFolderId(previousMessage.parentFolderId());
    mail.setContentScheme(previousMessage.contentScheme());
    mail.setContentIdentifier(previousMessage.contentIdentifier());
    mail.setTo(previousMessage.to());
    mail.setFrom(previousMessage.from());
    mail.setCustomFields(previousMessage.customFields());
    mail.setServerUid(previousMessage.serverUid());

    m_composerInterface->compose(QMailMessage::NoResponse, previousMessage);

    // ugh. we need to do this everywhere
    m_hasMessageChanged = false;
    m_precursorId = mail.inResponseTo();
    m_replyAction = mail.responseType();
}

bool WriteMail::hasContent()
{
    if (!m_composerInterface)
        return false;
    return !m_composerInterface->isEmpty();
}

void WriteMail::reset()
{
    mail = QMailMessage();

    if (m_composerInterface) {
        // Remove any associated widgets
        m_widgetStack->removeWidget(m_composerInterface);
        m_composerInterface = 0;
    }

    m_hasMessageChanged = false;
    m_precursorId = QMailMessageId();
    m_replyAction = QMailMessage::NoResponse;
}

bool WriteMail::prepareComposer(QMailMessage::MessageType type, const QMailAccountId &accountId)
{
    bool success = false;

    // Don't discard mail being composed without user confirmation
    if (changed()) {
        if (QMessageBox::question(qApp->activeWindow(),
                                  tr("Compose new message"),
                                  tr("A message is currently being composed. Do you wish to save the message in drafts?"),
                                  QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            draft();
        } else {
            return false;
        }
    }
    
    reset();

    if (type == QMailMessage::AnyType) {
        bool displaySelector(true);

        // If we have no options, prompt to set up an account
        if (m_selectComposerWidget->availableTypes().isEmpty()) {
            displaySelector = false;

            if (QMessageBox::question(qApp->activeWindow(),
                                      tr("No accounts configured"),
                                      tr("No accounts are configured to send messages with. Do you wish to configure one now?"),
                                      QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
                emit editAccounts();
            }
        }

        // If there's just one composer option, select it immediately
        QString key = m_selectComposerWidget->singularKey();
        if (!key.isEmpty()) {
            displaySelector = false;
            success = composerSelected(qMakePair(key, QMailComposerFactory::messageTypes(key).first()));
        }

        if (displaySelector) {
            m_selectComposerWidget->setSelected(composer(), QMailMessage::AnyType);
            setWindowTitle( tr("Select type") );
            m_widgetStack->setCurrentWidget(m_selectComposerWidget);
            success = true;
        }
    } else {
        QString key = QMailComposerFactory::defaultKey(type);
        if (!key.isEmpty()) {
            success = composerSelected(qMakePair(key, type));
        } else {
            qWarning() << "Cannot edit message of type:" << type;
        }
    }

    updateAccountSelection(type, accountId);
    if(m_composerInterface)
        m_composerInterface->setSendingAccountId(accountId);

    return success;
}

void WriteMail::discard()
{
    reset();
    emit discardMail();
    close();
}

bool WriteMail::draft()
{
    bool result(false);

    if (!buildMail(false)) {
        qWarning() << "draft() - Unable to buildMail for saveAsDraft!";
    } else {
        result = true;
        emit saveAsDraft( mail );
    }

    // Since the existing details page may have hijacked the focus, we need to reset
    reset();
    close();

    return result;
}

void WriteMail::statusChanged(const QString& status)
{
    if(status.isEmpty())
        setWindowTitle(tr("(Unnamed)"));
    else
        setWindowTitle(status);
}

void WriteMail::closeEvent(QCloseEvent *event)
{
    reset();
    QMainWindow::closeEvent(event);
}


bool WriteMail::composerSelected(const QPair<QString, QMailMessage::MessageType> &selection)
{
    // We need to ensure that we can send for this composer
    QMailMessage::MessageType messageType = selection.second;

    if (!m_selectComposerWidget->availableTypes().contains(messageType)) {
        // See if the user wants to add/edit an account to resolve this
        QString type(QMailComposerFactory::displayName(selection.first, messageType));
        if (QMessageBox::question(qApp->activeWindow(),
                                  tr("No accounts configured"),
                                  tr("No accounts are configured to send %1.\nDo you wish to configure one now?").arg(type),
                                  QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            emit editAccounts();
        }
        return false;
    }
    if (!m_selectComposerWidget->availableTypes().contains(messageType)) {
        // If still not possible, then just fail
        emit noSendAccount(messageType);
        return false;
    }

    setComposer(selection.first);
    m_composerInterface->clear();
    m_widgetStack->setCurrentWidget(m_composerInterface);

    // Can't save as draft until there has been a change
    m_draftAction->setEnabled(false);
    return true;
}

void WriteMail::setComposer( const QString &key )
{
    if (m_composerInterface && m_composerInterface->key() == key) {
        // We can reuse the existing composer object
        m_composerInterface->clear();
        return;
    }

    if (m_composerInterface) {
        // Remove any associated widgets
        m_widgetStack->removeWidget(m_composerInterface);
        //delete m_composerInterface;
        m_composerInterface = 0;
    }

    m_composerInterface = QMailComposerFactory::create( key, this );
    if (!m_composerInterface)
        return;

    connect( m_composerInterface, SIGNAL(changed()), this, SLOT(messageModified()) );
    connect( m_composerInterface, SIGNAL(cancel()), this, SLOT(saveChangesOnRequest()));
    connect( m_composerInterface, SIGNAL(statusChanged(QString)), this, SLOT(statusChanged(QString)));

    m_widgetStack->addWidget(m_composerInterface);

    foreach(QAction* a, m_composerInterface->actions())
        m_toolbar->addAction(a);

    m_composerInterface->setFocus();
}

void WriteMail::updateAccountSelection(QMailMessage::MessageType messageType, const QMailAccountId &suggestedId)
{
    m_accountSelection->clear();

    QMailAccountIdList accountIds = sendingAccounts(messageType);
    m_accountSelection->setEnabled(accountIds.count() > 1);

    if (accountIds.isEmpty()) {
        return;
    }

    int suggestedIndex = -1;
    int preferredIndex = -1;

    foreach (QMailAccountId id, accountIds) {
        QMailAccount a(id);

        if (a.id() == suggestedId) {
            suggestedIndex = m_accountSelection->count();
        }
        if (a.status() & QMailAccount::PreferredSender) {
            preferredIndex = m_accountSelection->count();
        }

        m_accountSelection->addItem(a.name(), a.id());
    }

    int index(suggestedIndex != -1 ? suggestedIndex : (preferredIndex != -1 ? preferredIndex : 0));
    m_accountSelection->setCurrentIndex(index);
}

QString WriteMail::composer() const
{
    QString key;
    if (m_composerInterface)
        key = m_composerInterface->key();
    return key;
}

void WriteMail::messageModified()
{
    m_hasMessageChanged = true;

    if (m_composerInterface) {
        m_draftAction->setEnabled( hasContent() );
        m_sendAction->setEnabled(m_composerInterface->isReadyToSend());
    }
}

bool WriteMail::forcedClosure()
{
    if (changed() && draft())
        return true;

    discard();
    return false;
}

void WriteMail::setVisible(bool visible)
{
    if (visible) {

        //center the window on the parent
    QWidget* w = qobject_cast<QWidget*>(parent());

    QPoint p;

    if (w) {
        // Use mapToGlobal rather than geometry() in case w might
        // be embedded in another application
        QPoint pp = w->mapToGlobal(QPoint(0,0));
        p = QPoint(pp.x() + w->width()/2,
                    pp.y() + w->height()/ 2);
    }

    p = QPoint(p.x()-width()/2,
            p.y()-height()/2);

    move(p);
    }
    QWidget::setVisible(visible);
}

bool WriteMail::largeAttachments()
{
    static int limit = 0;
    if (limit == 0) {
        const QString key("emailattachlimitkb");
        QSettings mailconf("QtProject","qtmail");

        mailconf.beginGroup("qtmailglobal");
        if (mailconf.contains(key))
            limit = mailconf.value(key).value<int>();
        else
            limit = 2048; //default to 2MB
        mailconf.endGroup();

        limit *= 1024;
    }

    // Determine if the message attachments exceed our limit
    int totalAttachmentSize = 0;

    for (unsigned int i = 0; i < mail.partCount(); ++i) {
        const QMailMessagePart part = mail.partAt(i);

        if (part.hasBody())
            totalAttachmentSize += part.body().length();
    }

    return (totalAttachmentSize > limit);
}

QString WriteMail::signature(const QMailAccountId& accountId) const
{
    if (accountId.isValid()) {
        QMailAccount account(accountId);
        if (account.status() & QMailAccount::AppendSignature)
            return account.signature();
    }

    return QString();
}


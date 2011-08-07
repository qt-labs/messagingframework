/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "genericviewer.h"
#include "browserwidget.h"
#include "attachmentoptions.h"
#include <QAction>
#include <QGridLayout>
#include <QKeyEvent>
#include <qmailmessage.h>
#include <QMenu>
#include <QPushButton>
#include <QToolButton>
#include <QtPlugin>

GenericViewer::GenericViewer(QWidget* parent)
    : QMailViewerInterface(parent),
      browser(new BrowserWidget(parent)),
      attachmentDialog(0),
      message(0),
      plainTextMode(false)
{

    connect(browser, SIGNAL(anchorClicked(QUrl)), this, SLOT(linkClicked(QUrl)));

    plainTextModeAction = new QAction(QIcon(":icon/txt"), tr("Plain text"), this);
    plainTextModeAction->setVisible(!plainTextMode);
    plainTextModeAction->setWhatsThis(tr("Display the message contents in Plain text format."));

    richTextModeAction = new QAction(QIcon(":icon/txt"), tr("Rich text"), this);
    richTextModeAction->setVisible(plainTextMode);
    richTextModeAction->setWhatsThis(tr("Display the message contents in Rich text format."));

    installEventFilter(this);

    browser->addAction(plainTextModeAction);
    connect(plainTextModeAction, SIGNAL(triggered(bool)),
            this, SLOT(triggered(bool)));

    browser->addAction(richTextModeAction);
    connect(richTextModeAction, SIGNAL(triggered(bool)),
            this, SLOT(triggered(bool)));
 
}

QWidget* GenericViewer::widget() const
{
    return browser;
}

void GenericViewer::addActions(const QList<QAction*>& actions)
{
    browser->addActions(actions);
}

void GenericViewer::removeAction(QAction* action)
{
    browser->removeAction(action);
}

void GenericViewer::scrollToAnchor(const QString& a)
{
    browser->scrollToAnchor(a);
}

QString GenericViewer::key() const
{
    return "GenericViewer";
}

QMailViewerFactory::PresentationType GenericViewer::presentation() const
{
    return QMailViewerFactory::StandardPresentation;
}

QList<QMailMessage::ContentType> GenericViewer::types() const 
{ 
    return QList<QMailMessage::ContentType>() << QMailMessage::PlainTextContent
                                              << QMailMessage::RichTextContent
                                              << QMailMessage::ImageContent
                                              << QMailMessage::AudioContent
                                              << QMailMessage::VideoContent
                                              << QMailMessage::MultipartContent
                                              << QMailMessage::HtmlContent         // temporary...
                                              << QMailMessage::VCardContent        // temporary...
                                              << QMailMessage::VCalendarContent    // temporary...
                                              << QMailMessage::ICalendarContent;   // temporary...
}

bool GenericViewer::setMessage(const QMailMessage& mail)
{
    if (attachmentDialog) {
        attachmentDialog->close();
        attachmentDialog = 0;
    }

    message = &mail;

    setPlainTextMode(plainTextMode);

    return true;
}

void GenericViewer::setResource(const QUrl& name, QVariant var)
{
#ifndef USE_WEBKIT
    browser->setResource(name, var);
#else
    Q_UNUSED(name)
    Q_UNUSED(var)
#endif
}

void GenericViewer::clear()
{
    if (attachmentDialog) {
        attachmentDialog->close();
        attachmentDialog = 0;
    }

    plainTextMode = false;

    browser->setPlainText("");
    browser->clearResources();
}

void GenericViewer::triggered(bool)
{
    if (sender() == plainTextModeAction) {
        setPlainTextMode(true);
    } else if (sender() == richTextModeAction) {
        setPlainTextMode(false);
    }
}

void GenericViewer::setPlainTextMode(bool plainTextMode)
{
    this->plainTextMode = plainTextMode;

    browser->setMessage(*message, plainTextMode);

    plainTextModeAction->setVisible(!plainTextMode && message->messageType() != QMailMessage::Mms);
    richTextModeAction->setVisible(plainTextMode);
}

void GenericViewer::linkClicked(const QUrl& link)
{
    QString command = link.toString();

    if (command.startsWith(QLatin1String("attachment"))) {
        QRegExp splitter("attachment;([^;]+)(?:;([\\d\\.]*))?");
        if (splitter.exactMatch(command)) {
            QString cmd = splitter.cap(1);
            QString location = splitter.cap(2);
            if (!location.isEmpty()) {
                QMailMessagePart::Location partLocation(location);

                // Show the attachment dialog
                attachmentDialog = new AttachmentOptions(browser);
                attachmentDialog->setAttribute(Qt::WA_DeleteOnClose);

                attachmentDialog->setAttachment(message->partAt(partLocation));
                connect(attachmentDialog, SIGNAL(retrieve(QMailMessagePart)), this, SIGNAL(retrieveMessagePart(QMailMessagePart)));
                connect(attachmentDialog, SIGNAL(retrievePortion(QMailMessagePart, uint)), this, SIGNAL(retrieveMessagePartPortion(QMailMessagePart, uint)));
                connect(attachmentDialog, SIGNAL(respondToPart(QMailMessagePart::Location, QMailMessage::ResponseType)), this, SIGNAL(respondToMessagePart(QMailMessagePart::Location, QMailMessage::ResponseType)));
                connect(attachmentDialog, SIGNAL(finished(int)), this, SLOT(dialogFinished(int)));

                attachmentDialog->exec();
                return;
            }
        }
    } else if (command.startsWith(QLatin1String("download"))) {
        QRegExp splitter("download(?:;(\\d+))?");
        if (splitter.exactMatch(command)) {
            QString bytes = splitter.cap(1);
            if (!bytes.isEmpty()) {
                emit retrieveMessagePortion(bytes.toUInt());
            } else {
                emit retrieveMessage();
            }
            return;
        }
    }

    emit anchorClicked(link);
}

bool GenericViewer::eventFilter(QObject*, QEvent* event)
{
    if (event->type() == QEvent::KeyPress) {
        if (QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event)) {
            if (keyEvent->key() == Qt::Key_Back) {
                emit finished();
                return true;
            }
        }
    }

    return false;
}

void GenericViewer::dialogFinished(int)
{
    attachmentDialog = 0;
}

Q_EXPORT_PLUGIN2(generic,GenericViewer)


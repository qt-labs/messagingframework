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

#ifndef EMAILCOMPOSER_H
#define EMAILCOMPOSER_H

#include <QList>
#include <qmailcomposer.h>

class AddAttDialog;
class BodyTextEdit;
class RecipientListWidget;
class AttachmentListWidget;
class DetailsPage;

QT_BEGIN_NAMESPACE

class QLabel;
class QLineEdit;
class QStackedWidget;
class QTextEdit;

QT_END_NAMESPACE

class EmailComposerInterface : public QMailComposerInterface
{
    Q_OBJECT

public:
    EmailComposerInterface( QWidget *parent = 0 );
    virtual ~EmailComposerInterface();

    bool isEmpty() const;
    QMailMessage message() const;

    bool isReadyToSend() const;
    QString title() const;

    virtual QString key() const;
    virtual QList<QMailMessage::MessageType> messageTypes() const;
    virtual QList<QMailMessage::ContentType> contentTypes() const;
    virtual QString name(QMailMessage::MessageType type) const;
    virtual QString displayName(QMailMessage::MessageType type) const;
    virtual QIcon displayIcon(QMailMessage::MessageType type) const;

    void compose(QMailMessage::ResponseType type, const QMailMessage& source, const QMailMessagePart::Location& sourceLocation, QMailMessage::MessageType messageType);

    QList<QAction*> actions() const;
    QString status() const;
    static QString quotePrefix();

public slots:
    void clear();
    void setSignature(const QString &sig);
    void setSendingAccountId(const QMailAccountId &accountId);

protected slots:
    void selectAttachment();
    void updateLabel();
    void setCursorPosition();

private:
    void init();
    void create(const QMailMessage& source);
    void respond(QMailMessage::ResponseType type, const QMailMessage& source, const QMailMessagePart::Location& partLocation);
    void setPlainText( const QString& text, const QString& signature );
    void getDetails(QMailMessage& message) const;
    void setDetails(const QMailMessage& message);

private:
    AddAttDialog *m_addAttDialog;
    int m_cursorIndex;
    QWidget* m_composerWidget;
    BodyTextEdit* m_bodyEdit;
    QLabel* m_forwardLabel;
    QTextEdit* m_forwardEdit;
    QLabel* m_attachmentsLabel;
    QStackedWidget* m_widgetStack;
    QAction* m_attachmentAction;
    RecipientListWidget* m_recipientListWidget;
    AttachmentListWidget* m_attachmentListWidget;
    QLineEdit* m_subjectEdit;
    QString m_signature;
    QString m_title;
    QLabel* m_columnLabel;
    QLabel* m_rowLabel;
    QStringList m_temporaries;
    QMailAccountId m_accountId;
    quint64 m_sourceStatus;
};

#endif


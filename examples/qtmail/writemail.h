/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef WRITEMAIL_H
#define WRITEMAIL_H

#include <qmailmessage.h>
#include <QDialog>
#include <QMainWindow>
#include <QString>

class SelectComposerWidget;
class QMailComposerInterface;

QT_BEGIN_NAMESPACE

class QAction;
class QComboBox;
class QContent;
class QStackedWidget;

QT_END_NAMESPACE

class WriteMail : public QMainWindow
{
    Q_OBJECT

public:
    WriteMail(QWidget* parent = 0);

    void create(const QMailMessage& initMessage = QMailMessage());
    void respond(const QMailMessage& source, QMailMessage::ResponseType type);
    void respond(const QMailMessagePart::Location& sourceLocation, QMailMessage::ResponseType type);
    void modify(const QMailMessage& previousMessage);

    bool hasContent();
    QString composer() const;
    bool forcedClosure();

    void setVisible(bool visible);

public slots:
    bool saveChangesOnRequest();
    bool prepareComposer(QMailMessage::MessageType = QMailMessage::AnyType, const QMailAccountId & = QMailAccountId());

signals:
    void editAccounts();
    void noSendAccount(QMailMessage::MessageType);
    void saveAsDraft(QMailMessage&);
    void enqueueMail(QMailMessage&);
    void discardMail();

protected slots:
    bool sendStage();
    void accountSelectionChanged(int);
    void messageModified();
    void reset();
    void discard();
    bool draft();
    bool composerSelected(const QPair<QString, QMailMessage::MessageType> &selection);
    void statusChanged(const QString& status);

protected:
    void closeEvent(QCloseEvent * event);

private:
    bool largeAttachments();
    bool buildMail(bool includeSignature);
    void init();
    QString signature(const QMailAccountId& id) const;
    bool isComplete() const;
    bool changed() const;
    void setComposer( const QString &id );
    void setTitle(const QString& title);
    void updateAccountSelection(QMailMessage::MessageType messageType, const QMailAccountId &accountId);

private:
    QMailMessage mail;
    QMailComposerInterface *m_composerInterface;
    QAction *m_cancelAction, *m_draftAction, *m_sendAction;
    QStackedWidget* m_widgetStack;
    bool m_hasMessageChanged;
    SelectComposerWidget* m_selectComposerWidget;
    QMailMessageId m_precursorId;
    QMailMessage::ResponseType m_replyAction;
    QToolBar *m_toolbar;
    QComboBox *m_accountSelection;
};

#endif

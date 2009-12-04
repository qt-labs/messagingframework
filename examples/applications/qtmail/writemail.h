/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Qt Software Information (qt-info@nokia.com)
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the either Technology Preview License Agreement or the
** Beta Release License Agreement.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef WRITEMAIL_H
#define WRITEMAIL_H

#include <qmailmessage.h>
#include <QDialog>
#include <QMainWindow>
#include <QString>

class QAction;
class QComboBox;
class QContent;
class QMailComposerInterface;
class QStackedWidget;
class SelectComposerWidget;

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

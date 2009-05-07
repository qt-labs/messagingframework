/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef WRITEMAIL_H
#define WRITEMAIL_H

#include <QDialog>
#include <QMainWindow>
#include <QString>
#include <qmailmessage.h>

class QAction;
class QContent;
class QMailComposerInterface;
class QStackedWidget;
class SelectComposerWidget;
class QToolButton;

class WriteMail : public QMainWindow
{
    Q_OBJECT

public:
    WriteMail(QWidget* parent = 0);
    virtual ~WriteMail();

    void create(const QMailMessage& initMessage = QMailMessage());
    void forward(const QMailMessage& forwardMail);
    void reply(const QMailMessage& replyMail);
    void modify(const QMailMessage& previousMessage);

    bool hasContent();
    QString composer() const;
    bool forcedClosure();

public slots:
    bool saveChangesOnRequest();
    bool prepareComposer( QMailMessage::MessageType = QMailMessage::AnyType);

signals:
    void editAccounts();
    void noSendAccount(const QMailMessage::MessageType);
    void saveAsDraft(const QMailMessage&);
    void enqueueMail(const QMailMessage&);
    void discardMail();

protected slots:
    bool sendStage();
    void messageModified();
    void reset();
    void discard();
    bool draft();
    bool composerSelected(const QPair<QString, QMailMessage::MessageType> &selection);
    void statusChanged(const QString& status);

private:
    bool largeAttachments();
    bool buildMail(const QMailAccountId& accountId, bool includeSignature = true);
    void init();
    QString signature(const QMailAccountId& id) const;
    bool isComplete() const;
    bool changed() const;
    void setComposer( const QString &id );
    void setTitle(const QString& title);
    void updateSendAction(QMailMessage::MessageType messageType);

private:
    QMailMessage mail;
    QMailComposerInterface *m_composerInterface;
    QAction *m_cancelAction, *m_draftAction, *m_sendAction;
    QMenu* m_sendViaMenu;
    QToolButton* m_sendButton;
    QStackedWidget* m_widgetStack;
    QWidget *m_mainWindow;
    bool m_hasMessageChanged;
    SelectComposerWidget* m_selectComposerWidget;
    QMailMessageId m_precursorId;
    int m_replyAction;
    QToolBar *m_toolbar;
};

#endif

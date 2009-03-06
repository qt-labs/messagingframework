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

class WriteMail : public QMainWindow
{
    Q_OBJECT

public:
    WriteMail(QWidget* parent = 0);
    virtual ~WriteMail();

    void reply(const QMailMessage& replyMail, int action);
    void modify(const QMailMessage& previousMessage);
    void setRecipients(const QString &emails, const QString &numbers);
    void setRecipient(const QString &recipient);
    void setSubject(const QString &subject);
    void setBody(const QString &text, const QString &type);
    bool hasContent();
    QString composer() const;
    bool forcedClosure();

public slots:
    bool saveChangesOnRequest();
    bool prepareComposer( QMailMessage::MessageType = QMailMessage::AnyType, bool detailsOnly = false );

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
    void contextChanged();

private:
    bool largeAttachments();
    bool buildMail();
    void init();
    QString signature() const;
    bool isComplete() const;
    bool changed() const;
    void setComposer( const QString &id );
    void setTitle(const QString& title);

private:
    QMailMessage mail;
    QMailComposerInterface *m_composerInterface;
    QAction *m_cancelAction, *m_draftAction;
    QStackedWidget* m_widgetStack;
    QWidget *m_mainWindow;
    bool hasMessageChanged;
    bool _detailsOnly;
    SelectComposerWidget* m_selectComposerWidget;
};

#endif

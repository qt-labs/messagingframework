/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef EMAILCOMPOSER_H
#define EMAILCOMPOSER_H

#include <QList>
#include <qmailcomposer.h>

class AddAttDialog;
class QLabel;
class BodyTextEdit;
class QStackedWidget;
class DetailsPage;
class RecipientListWidget;
class QLineEdit;
class AttachmentListWidget;

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

    void compose(ComposeContext context, const QMailMessage& source, QMailMessage::MessageType mtype);
    QList<QAction*> actions() const;
    QString status() const;

public slots:
    void clear();
    //void attach( const QContent &lnk, QMailMessage::AttachmentsAction = QMailMessage::LinkToAttachments );
    void setSignature( const QString &sig );

protected slots:
    void selectAttachment();
    void updateLabel();
    void setCursorPosition();
    void updateAttachmentsLabel();

private:
    void create(const QMailMessage& source);
    void reply(const QMailMessage& source, int action);
    void init();
    void setPlainText( const QString& text, const QString& signature );
    void getDetails(QMailMessage& message) const;
    void setDetails(const QMailMessage& message);

private:
    AddAttDialog *m_addAttDialog;
    int m_cursorIndex;
    QWidget* m_composerWidget;
    BodyTextEdit* m_bodyEdit;
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
};

#endif


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

class EmailComposerInterface : public QMailComposerInterface
{
    Q_OBJECT

public:
    EmailComposerInterface( QWidget *parent = 0 );
    virtual ~EmailComposerInterface();

    bool isEmpty() const;
    QMailMessage message() const;

    void setTo(const QString& toAddress);
    void setFrom(const QString& fromAddress);
    void setCc(const QString& ccAddress);
    void setBcc(const QString& bccAddress);
    void setSubject(const QString& subject);
    QString from() const;
    QString to() const;
    QString cc() const;
    QString bcc() const;
    bool isReadyToSend() const;
    bool isDetailsOnlyMode() const;
    void setDetailsOnlyMode(bool val);
    QString contextTitle() const;
    QMailAccount fromAccount() const;

    virtual QString key() const;
    virtual QList<QMailMessage::MessageType> messageTypes() const;
    virtual QList<QMailMessage::ContentType> contentTypes() const;
    virtual QString name(QMailMessage::MessageType type) const;
    virtual QString displayName(QMailMessage::MessageType type) const;
    virtual QIcon displayIcon(QMailMessage::MessageType type) const;


public slots:
    void setMessage( const QMailMessage &mail );
    void setBody( const QString &txt, const QString &type );
    void clear();
//    void attach( const QContent &lnk, QMailMessage::AttachmentsAction = QMailMessage::LinkToAttachments );
    void setSignature( const QString &sig );
    void reply(const QMailMessage& source, int action);

protected slots:
    void selectAttachment();
    void updateLabel();
    void setCursorPosition();
    void updateAttachmentsLabel();
    void detailsPage();
    void composePage();

private:
    void init();
    void setPlainText( const QString& text, const QString& signature );
    void setContext(const QString& title);

private:
    AddAttDialog *m_addAttDialog;
    int m_cursorIndex;
    QWidget* m_composerWidget;
    BodyTextEdit* m_bodyEdit;
    QLabel* m_attachmentsLabel;
    QStackedWidget* m_widgetStack;
    DetailsPage* m_detailsWidget;

//    typedef QPair<QContent, QMailMessage::AttachmentsAction> AttachmentDetail;
//    QList<AttachmentDetail> m_attachments;

    QString m_signature;
    QString m_title;
};

#endif


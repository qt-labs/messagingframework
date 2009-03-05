/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef DESKPHONE_EMAILCOMPOSER_H
#define DESKPHONE_EMAILCOMPOSER_H

#include <QContent>
#include <QList>
#include <QWidget>
#include <qmailcomposer.h>

class AddAttDialog;
class QLabel;
class QWidget;
class QTextEdit;
class QLineEdit;
class QPushButton;
class QToolButton;
class HomeActionButton;
class HomeFieldButton;
class ColumnSizer;

class EmailComposerInterface : public QMailComposerInterface
{
    Q_OBJECT

public:
    EmailComposerInterface( QWidget *parent = 0 );
    virtual ~EmailComposerInterface();

    bool isEmpty() const;
    QMailMessage message() const;

public:
    void setTo(const QString& toAddress);
    void setFrom(const QString& fromAddress);
    void setSubject(const QString& subject);
    QString from() const;
    QString to() const;
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

signals:
    void attachmentsChanged();

public slots:
    void setMessage( const QMailMessage &mail );
    void setBody( const QString &txt, const QString &type );
    void clear();
    void attach( const QContent &lnk, QMailMessage::AttachmentsAction = QMailMessage::LinkToAttachments );
    void setSignature( const QString &sig );
    void reply(const QMailMessage& source, int action);

protected slots:
    void selectAttachments();
    void selectRecipients();
    void updateAttachmentsLabel();
    void setCursorPosition();
    void subjectClicked();
    void toClicked();

protected:
    void keyPressEvent( QKeyEvent *e );

private:
    void init();
    void setPlainText( const QString& text, const QString& signature );
    void setContext(const QString& context);

private:
    AddAttDialog *m_addAttDialog;
    int m_index;
    QTextEdit* m_bodyEdit;
    HomeActionButton* m_contactsButton;
    HomeFieldButton* m_toEdit;
    HomeFieldButton* m_subjectEdit;
    ColumnSizer* m_sizer;
    QToolButton* m_attachmentsButton;
    QWidget *m_widget;

    typedef QPair<QContent, QMailMessage::AttachmentsAction> AttachmentDetail;
    QList<AttachmentDetail> m_attachments;

    QString m_signature;
    QString m_title;
    QString m_from;
};

#endif

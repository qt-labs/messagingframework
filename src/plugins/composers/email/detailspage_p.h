/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef DETAILSPAGE_P_H
#define DETAILSPAGE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt Extended API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qwidget.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qmailmessage.h>

class QLineEdit;
class QHBoxLayout;
class QComboBox;
class QToolButton;
class QLabel;
class QCheckBox;
class QMailAccount;
class QMailAccountId;

class QTOPIAMAIL_EXPORT DetailsPage : public QWidget
{
    Q_OBJECT

public:

    DetailsPage( QWidget *parent, const char *name = 0 );

    void setDefaultAccount( const QMailAccountId& defaultId );

    void setTo( const QString &a_to );
    QString to() const;

    void setBcc( const QString &a_bcc );
    QString bcc() const;

    void setCc( const QString &a_cc );
    QString cc() const;

    QString subject() const;
    void setSubject( const QString &sub );

    QString from() const;
    void setFrom( const QString &from );
    void setFrom( const QMailAccountId &id );

    QMailAccount fromAccount() const;

    void setType( int t );

    void setDetails( const QMailMessage &mail );
    void getDetails( QMailMessage &mail ) const;

    bool isDetailsOnlyMode() const;
    void setDetailsOnlyMode(bool val);

public slots:
    void clear();

signals:
    void changed();
    void sendMessage();
    void cancel();
    void editMessage();

private slots:
    void editRecipients();
    void copy();
    void paste();

private:
    bool m_allowPhoneNumbers, m_allowEmails;
    bool m_ignoreFocus;
    int m_type;
    QCheckBox *m_readReplyField;
    QCheckBox *m_deliveryReportField;
    QLineEdit *m_subjectField;
    QLabel *m_subjectFieldLabel, *m_fromFieldLabel;
    QComboBox *m_fromField;
    QLabel *m_toFieldLabel, *m_ccFieldLabel, *m_bccFieldLabel;
    QLineEdit *m_ccField, *m_bccField, *m_toField;
    QHBoxLayout *m_toBox, *m_ccBox, *m_bccBox;
    QAction *m_previousAction;
};

#endif

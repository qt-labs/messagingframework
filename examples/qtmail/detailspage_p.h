/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
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

QT_BEGIN_NAMESPACE

class QLineEdit;
class QHBoxLayout;
class QComboBox;
class QToolButton;
class QLabel;
class QCheckBox;

QT_END_NAMESPACE

class QMailAccount;
class QMailAccountId;

class DetailsPage : public QWidget
{
    Q_OBJECT

public:

    DetailsPage( QWidget *parent, const char *name = Q_NULLPTR );

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

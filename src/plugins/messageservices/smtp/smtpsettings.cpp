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

#include "smtpsettings.h"
#include "smtpconfiguration.h"
#include <QGridLayout>
#include <QLineEdit>
#include <QTextEdit>
#include <QValidator>
#include <qmailaccount.h>
#include <qmailaccountconfiguration.h>
#include <qmailtransport.h>
#include <QDialog>
#include <QPointer>

namespace {

const QString serviceKey("smtp");

class SigEntry : public QDialog
{
    Q_OBJECT

public:
    SigEntry(QWidget *parent, const char* name, Qt::WindowFlags fl = 0);

    void setEntry(QString sig);
    QString entry() const;

protected:
    void closeEvent(QCloseEvent *event);

private:
    QTextEdit *input;
};

SigEntry::SigEntry(QWidget *parent, const char *name, Qt::WindowFlags fl)
    : QDialog(parent, fl)
{
    setObjectName(name);
    setWindowTitle(tr("Signature"));

    QGridLayout *grid = new QGridLayout(this);
    input = new QTextEdit(this);
    grid->addWidget(input, 0, 0);
}

void SigEntry::setEntry(QString sig)
{
    input->insertPlainText(sig);
}

QString SigEntry::entry() const
{
    return input->toPlainText();
}

void SigEntry::closeEvent(QCloseEvent *)
{
    accept();
}


class PortValidator : public QValidator
{
public:
    PortValidator(QWidget *parent = Q_NULLPTR, const char *name = Q_NULLPTR);

    QValidator::State validate(QString &str, int &) const;
};

PortValidator::PortValidator(QWidget *parent, const char *name)
    : QValidator(parent)
{
    setObjectName(name);
}

QValidator::State PortValidator::validate(QString &str, int &) const
{
    // allow empty strings, as it's a bit awkward to edit otherwise
    if ( str.isEmpty() )
        return QValidator::Acceptable;

    bool ok = false;
    int i = str.toInt(&ok);
    if ( !ok )
        return QValidator::Invalid;

    if ( i <= 0 || i >= 65536 )
        return QValidator::Invalid;

    return QValidator::Acceptable;
}


const SmtpConfiguration::AuthType authenticationType[] = {
    SmtpConfiguration::Auth_NONE,
#ifndef QT_NO_SSL
    SmtpConfiguration::Auth_LOGIN,
    SmtpConfiguration::Auth_PLAIN,
#endif
    SmtpConfiguration::Auth_CRAMMD5,
    SmtpConfiguration::Auth_INCOMING
};

#ifndef QT_NO_SSL
int authenticationIndex(int type)
{
    const int numTypes = sizeof(authenticationType)/sizeof(SmtpConfiguration::AuthType);
    for (int i = 0; i < numTypes; ++i)
        if (type == authenticationType[i])
            return i;

    return 0;
}
#endif

}


SmtpSettings::SmtpSettings()
    : QMailMessageServiceEditor(),
      addressModified(false)
{
    setupUi(this);
    setLayoutDirection(qApp->layoutDirection());

    connect(setSignatureButton, SIGNAL(clicked()), this, SLOT(sigPressed()));
    connect(authentication, SIGNAL(currentIndexChanged(int)), this, SLOT(authChanged(int)));
    connect(emailInput, SIGNAL(textChanged(QString)), this, SLOT(emailModified()));
    connect(sigCheckBox,SIGNAL(clicked(bool)),setSignatureButton,SLOT(setEnabled(bool)));

    const QString uncapitalised("email noautocapitalization");

    // These fields should not be autocapitalised

    smtpPortInput->setValidator(new PortValidator(this));

    smtpPasswordInput->setEchoMode(QLineEdit::Password);

#ifdef QT_NO_SSL
    encryption->hide();
    lblEncryption->hide();
    authentication->hide();
    lblAuthentication->hide();
    smtpUsernameInput->hide();
    lblSmtpUsername->hide();
    smtpPasswordInput->hide();
    lblSmtpPassword->hide();
#endif
}

void SmtpSettings::sigPressed()
{
    if (sigCheckBox->isChecked()) {
        QString sigText;
        if (signature.isEmpty())
            sigText = QLatin1String("~~\n") + nameInput->text();
        else
            sigText = signature;

        QPointer<SigEntry> sigEntry(new SigEntry(this, "sigEntry", static_cast<Qt::WindowFlags>(1)));
        sigEntry->setEntry(sigText);
        if (sigEntry->exec() == QDialog::Accepted)
            signature = sigEntry->entry();

        delete sigEntry;

    }
}

void SmtpSettings::emailModified()
{
    addressModified = true;
}

void SmtpSettings::authChanged(int index)
{
#ifndef QT_NO_SSL
    SmtpConfiguration::AuthType type = authenticationType[index];
    bool enableCredentials = (type == SmtpConfiguration::Auth_LOGIN
                              || type == SmtpConfiguration::Auth_PLAIN
                              || type == SmtpConfiguration::Auth_CRAMMD5);

    smtpUsernameInput->setEnabled(enableCredentials);
    lblSmtpUsername->setEnabled(enableCredentials);
    smtpPasswordInput->setEnabled(enableCredentials);
    lblSmtpPassword->setEnabled(enableCredentials);

    if (!enableCredentials) {
        smtpUsernameInput->clear();
        smtpPasswordInput->clear();
    }
#else
    Q_UNUSED(index);
#endif
}

void SmtpSettings::displayConfiguration(const QMailAccount &account, const QMailAccountConfiguration &config)
{
#ifndef QT_NO_SSL
    // Any reason to re-enable this facility?
    //authentication->setItemText(3, tr("Incoming");
#endif

    if (!config.services().contains(serviceKey)) {
        // New account
        nameInput->setText("");
        emailInput->setText("");
        smtpServerInput->setText("");
        smtpPortInput->setText("25");
#ifndef QT_NO_SSL
        smtpUsernameInput->setText("");
        smtpPasswordInput->setText("");
        encryption->setCurrentIndex(0);
        authentication->setCurrentIndex(0);
        smtpUsernameInput->setEnabled(false);
        lblSmtpUsername->setEnabled(false);
        smtpPasswordInput->setEnabled(false);
        lblSmtpPassword->setEnabled(false);
#endif
        signature.clear();
    } else {
        SmtpConfiguration smtpConfig(config);
        nameInput->setText(smtpConfig.userName());;
        emailInput->setText(smtpConfig.emailAddress());
        smtpServerInput->setText(smtpConfig.smtpServer());
        smtpPortInput->setText(QString::number(smtpConfig.smtpPort()));
#ifndef QT_NO_SSL
        smtpUsernameInput->setText(smtpConfig.smtpUsername());
        smtpPasswordInput->setText(smtpConfig.smtpPassword());
        authentication->setCurrentIndex(authenticationIndex(smtpConfig.smtpAuthentication()));
        encryption->setCurrentIndex(static_cast<int>(smtpConfig.smtpEncryption()));

        int index(authentication->currentIndex());
        Q_ASSERT(index >= 0);
        SmtpConfiguration::AuthType type = authenticationType[index];
        const bool enableCredentials(type == SmtpConfiguration::Auth_LOGIN || type == SmtpConfiguration::Auth_PLAIN || type == SmtpConfiguration::Auth_CRAMMD5);
        smtpUsernameInput->setEnabled(enableCredentials);
        lblSmtpUsername->setEnabled(enableCredentials);
        smtpPasswordInput->setEnabled(enableCredentials);
        lblSmtpPassword->setEnabled(enableCredentials);
#endif
        defaultMailCheckBox->setChecked(account.status() & QMailAccount::PreferredSender);
        sigCheckBox->setChecked(account.status() & QMailAccount::AppendSignature);
        setSignatureButton->setEnabled(sigCheckBox->isChecked());
        signature = account.signature();
    }
}

bool SmtpSettings::updateAccount(QMailAccount *account, QMailAccountConfiguration *config)
{
    QString username(nameInput->text());
    QString address(emailInput->text());

    if (!username.isEmpty() || !address.isEmpty()) {
        account->setFromAddress(QMailAddress(username, address)); 
    }

    bool result;
    int port = smtpPortInput->text().toInt(&result);
    if ( (!result) ) {
        // should only happen when the string is empty, since we use a validator.
        port = 25;
    }

    if (!config->services().contains(serviceKey))
        config->addServiceConfiguration(serviceKey);

    SmtpConfigurationEditor smtpConfig(config);

    smtpConfig.setVersion(100);
    smtpConfig.setType(QMailServiceConfiguration::Sink);

    if ((!addressModified) && (address.isEmpty())) {
        // Try to guess email address
        QString server(smtpConfig.smtpServer());
        if (server.count('.')) {
            address = username + "@" + server.mid(server.indexOf('.') + 1, server.length());
        } else if (server.count('.') == 1) {
            address = username + "@" + server;
        }
    }
    smtpConfig.setUserName(username);
    smtpConfig.setEmailAddress(address);
    smtpConfig.setSmtpServer(smtpServerInput->text());
    smtpConfig.setSmtpPort(port);
#ifndef QT_NO_SSL
    smtpConfig.setSmtpUsername(smtpUsernameInput->text());
    smtpConfig.setSmtpPassword(smtpPasswordInput->text());
    int index(authentication->currentIndex());
    Q_ASSERT(index >= 0);
    smtpConfig.setSmtpAuthentication(authenticationType[index]);
    smtpConfig.setSmtpEncryption(static_cast<QMailTransport::EncryptType>(encryption->currentIndex()));
#endif

    account->setStatus(QMailAccount::PreferredSender, defaultMailCheckBox->isChecked());
    account->setStatus(QMailAccount::AppendSignature, sigCheckBox->isChecked());
    account->setSignature(signature);

    // Do we have a configuration we can use?
    if (!smtpConfig.smtpServer().isEmpty() && !smtpConfig.emailAddress().isEmpty())
        account->setStatus(QMailAccount::CanTransmit, true);

    account->setStatus(QMailAccount::UseSmartReply, false);

    return true;
}


#include "smtpsettings.moc"


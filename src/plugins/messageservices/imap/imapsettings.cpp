/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#include "imapsettings.h"
#include "imapconfiguration.h"
#include <QLineEdit>
#include <QMessageBox>
#include <qmailaccount.h>
#include <qmailaccountconfiguration.h>
#include <qmailtransport.h>

namespace {

const QString serviceKey("imap4"); 

class PortValidator : public QValidator
{
public:
    PortValidator(QWidget *parent = 0, const char *name = 0);

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

}


ImapSettings::ImapSettings()
    : QMailMessageServiceEditor(),
      warningEmitted(false)
{
    setupUi(this);
    setLayoutDirection(qApp->layoutDirection());

    connect(pushCheckBox, SIGNAL(stateChanged(int)), this, SLOT(pushCheckChanged(int)));
    connect(intervalCheckBox, SIGNAL(stateChanged(int)), this, SLOT(intervalCheckChanged(int)));

    const QString uncapitalised("email noautocapitalization");

    // These fields should not be autocapitalised
    mailPortInput->setValidator(new PortValidator(this));

    mailPasswInput->setEchoMode(QLineEdit::PasswordEchoOnEdit);

    // This functionality is not currently used:
    mailboxButton->hide();

#ifdef QT_NO_OPENSSL
    encryptionIncoming->hide();
    lblEncryptionIncoming->hide();
#endif

}

void ImapSettings::pushCheckChanged(int enabled)
{
    intervalCheckBox->setEnabled(!enabled);
    intervalPeriod->setEnabled(!enabled && intervalCheckBox->isChecked());
    roamingCheckBox->setEnabled(!enabled && intervalCheckBox->isChecked());
}

void ImapSettings::intervalCheckChanged(int enabled)
{
    intervalPeriod->setEnabled(enabled);
    roamingCheckBox->setEnabled(enabled);
}

void ImapSettings::displayConfiguration(const QMailAccount &, const QMailAccountConfiguration &config)
{
    if (!config.services().contains(serviceKey)) {
        // New account
        mailUserInput->setText("");
        mailPasswInput->setText("");
        mailServerInput->setText("");
        mailPortInput->setText("143");
#ifndef QT_NO_OPENSSL
        encryptionIncoming->setCurrentIndex(0);
#endif
        preferHtml->setChecked(true);
        pushCheckBox->setChecked(false);
        intervalCheckBox->setChecked(false);
        roamingCheckBox->setChecked(false);
    } else {
        ImapConfiguration imapConfig(config);

        mailUserInput->setText(imapConfig.mailUserName());
        mailPasswInput->setText(imapConfig.mailPassword());
        mailServerInput->setText(imapConfig.mailServer());
        mailPortInput->setText(QString::number(imapConfig.mailPort()));
#ifndef QT_NO_OPENSSL
        encryptionIncoming->setCurrentIndex(static_cast<int>(imapConfig.mailEncryption()));
#endif
        deleteCheckBox->setChecked(imapConfig.canDeleteMail());
        maxSize->setValue(imapConfig.maxMailSize());
        thresholdCheckBox->setChecked(imapConfig.maxMailSize() != -1);
        preferHtml->setChecked(imapConfig.preferredTextSubtype() == "html");
        pushCheckBox->setChecked(imapConfig.pushEnabled());
        intervalCheckBox->setChecked(imapConfig.checkInterval() > 0);
        intervalPeriod->setValue(qAbs(imapConfig.checkInterval()));
        roamingCheckBox->setChecked(!imapConfig.intervalCheckRoamingEnabled());
        imapBaseDir->setText(imapConfig.baseFolder());
    }

    intervalCheckBox->setEnabled(!pushCheckBox->isChecked());
    intervalPeriod->setEnabled(false);
    roamingCheckBox->setEnabled(intervalCheckBox->isChecked());
}

bool ImapSettings::updateAccount(QMailAccount *account, QMailAccountConfiguration *config)
{
    bool result;
    int port = mailPortInput->text().toInt(&result);
    if ( (!result) ) {
        // should only happen when the string is empty, since we use a validator.
        port = -1;
    }

    if (!config->services().contains(serviceKey))
        config->addServiceConfiguration(serviceKey);

    ImapConfigurationEditor imapConfig(config);

    imapConfig.setVersion(100);
    imapConfig.setType(QMailServiceConfiguration::Source);

    imapConfig.setMailUserName(mailUserInput->text());
    imapConfig.setMailPassword(mailPasswInput->text());
    imapConfig.setMailServer(mailServerInput->text());
    imapConfig.setMailPort(port == -1 ? 143 : port);
#ifndef QT_NO_OPENSSL
    imapConfig.setMailEncryption(static_cast<QMailTransport::EncryptType>(encryptionIncoming->currentIndex()));
#endif
    imapConfig.setDeleteMail(deleteCheckBox->isChecked());
    imapConfig.setMaxMailSize(thresholdCheckBox->isChecked() ? maxSize->value() : -1);
    imapConfig.setPreferredTextSubtype(preferHtml->isChecked() ? "html" : "plain");
    imapConfig.setAutoDownload(false);
    imapConfig.setPushEnabled(pushCheckBox->isChecked());
    imapConfig.setCheckInterval(intervalPeriod->value() * (intervalCheckBox->isChecked() ? 1 : -1));
    imapConfig.setIntervalCheckRoamingEnabled(!roamingCheckBox->isChecked());
    imapConfig.setBaseFolder(imapBaseDir->text());

    // Do we have a configuration we can use?
    if (!imapConfig.mailServer().isEmpty() && !imapConfig.mailUserName().isEmpty())
        account->setStatus(QMailAccount::CanRetrieve, true);

    return true;
}


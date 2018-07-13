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

#include "popsettings.h"
#include "popconfiguration.h"
#include <QLineEdit>
#include <QMessageBox>
#include <qmailaccount.h>
#include <qmailaccountconfiguration.h>
#include <qmailtransport.h>

namespace {

const QString serviceKey("pop3");

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

}


PopSettings::PopSettings()
    : QMailMessageServiceEditor(),
      warningEmitted(false)
{
    setupUi(this);
    setLayoutDirection(qApp->layoutDirection());

    connect(intervalCheckBox, SIGNAL(stateChanged(int)), this, SLOT(intervalCheckChanged(int)));

    const QString uncapitalised("email noautocapitalization");

    // These fields should not be autocapitalised

    mailPortInput->setValidator(new PortValidator(this));
    mailPasswInput->setEchoMode(QLineEdit::Password);

#ifdef QT_NO_SSL
    encryptionIncoming->hide();
    lblEncryptionIncoming->hide();
#endif

}

void PopSettings::intervalCheckChanged(int enabled)
{
    intervalPeriod->setEnabled(enabled);
    roamingCheckBox->setEnabled(enabled);
}

void PopSettings::displayConfiguration(const QMailAccount &, const QMailAccountConfiguration &config)
{
    if (!config.services().contains(serviceKey)) {
        // New account
        mailUserInput->setText("");
        mailPasswInput->setText("");
        mailServerInput->setText("");
        mailPortInput->setText("110");
#ifndef QT_NO_SSL
        encryptionIncoming->setCurrentIndex(0);
#endif
        intervalCheckBox->setChecked(false);
        roamingCheckBox->setChecked(false);
    } else {
        PopConfiguration popConfig(config);

        mailUserInput->setText(popConfig.mailUserName());
        mailPasswInput->setText(popConfig.mailPassword());
        mailServerInput->setText(popConfig.mailServer());
        mailPortInput->setText(QString::number(popConfig.mailPort()));
#ifndef QT_NO_SSL
        encryptionIncoming->setCurrentIndex(static_cast<int>(popConfig.mailEncryption()));
#endif
        deleteCheckBox->setChecked(popConfig.canDeleteMail());
        maxSize->setValue(popConfig.maxMailSize());
        thresholdCheckBox->setChecked(popConfig.maxMailSize() != -1);
        intervalCheckBox->setChecked(popConfig.checkInterval() > 0);
        intervalPeriod->setValue(qAbs(popConfig.checkInterval() ));
        roamingCheckBox->setChecked(!popConfig.intervalCheckRoamingEnabled());
    }

    intervalPeriod->setEnabled(false);
    roamingCheckBox->setEnabled(intervalCheckBox->isChecked());
}

bool PopSettings::updateAccount(QMailAccount *account, QMailAccountConfiguration *config)
{
    bool result;
    int port = mailPortInput->text().toInt(&result);
    if ( (!result) ) {
        // should only happen when the string is empty, since we use a validator.
        port = -1;
    }

    if (!config->services().contains(serviceKey))
        config->addServiceConfiguration(serviceKey);

    PopConfigurationEditor popConfig(config);

    popConfig.setVersion(100 );
    popConfig.setType(QMailServiceConfiguration::Source );

    popConfig.setMailUserName(mailUserInput->text());
    popConfig.setMailPassword(mailPasswInput->text());
    popConfig.setMailServer(mailServerInput->text());
    popConfig.setMailPort(port == -1 ? 110 : port);
#ifndef QT_NO_SSL
    popConfig.setMailEncryption(static_cast<QMailTransport::EncryptType>(encryptionIncoming->currentIndex()));
#endif
    popConfig.setDeleteMail(deleteCheckBox->isChecked());
    popConfig.setMaxMailSize(thresholdCheckBox->isChecked() ? maxSize->value() : -1);
    popConfig.setAutoDownload(false );
    popConfig.setCheckInterval(intervalPeriod->value() * (intervalCheckBox->isChecked() ? 1 : -1));
    popConfig.setIntervalCheckRoamingEnabled(!roamingCheckBox->isChecked());

    account->setStatus(QMailAccount::CanCreateFolders, false);
    // Do we have a configuration we can use?
    if (!popConfig.mailServer().isEmpty() && !popConfig.mailUserName().isEmpty())
        account->setStatus(QMailAccount::CanRetrieve, true);

    return true;
}


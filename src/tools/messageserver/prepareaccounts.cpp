/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#include <QCoreApplication>
#include <qmailaccount.h>
#include <qmailaccountconfiguration.h>
#include <qmailserviceconfiguration.h>
#include <qmailstore.h>
#include <qmaillog.h>

void prepareAccounts()
{
#ifdef QMAIL_QTOPIA
    // Find the set of sources for the existing accounts
    QSet<QString> existingSources;
    foreach (const QMailAccountId &id, QMailStore::instance()->queryAccounts()) {
        QMailAccount account(id);
        foreach (const QString &source, account.messageSources())
            existingSources.insert(source.toLower());
    }

    // See if we need to create any missing account types
    // Note: the order of account creation will yield default account list model ordering
    QString sourceType;
    QList<QPair<QMailAccount, QMailAccountConfiguration> > missingAccounts;
#ifndef QTOPIA_NO_SMS
    sourceType = "sms";
    if (!existingSources.contains(sourceType)) {
        QMailAccount account;
        account.setName(qApp->translate("MessageServer", "SMS"));
        account.setMessageType(QMailMessage::Sms);
        account.setStatus(QMailAccount::MessageSource, true);
        account.setStatus(QMailAccount::MessageSink, true);
        account.setStatus(QMailAccount::CanTransmit, true);

        QMailAccountConfiguration config;
        config.addServiceConfiguration(sourceType);
        QMailServiceConfiguration svcCfg(&config, sourceType);
        if (svcCfg.isValid()) {
            svcCfg.setVersion(100);
            svcCfg.setType(QMailServiceConfiguration::SourceAndSink);

            qMailLog(Messaging) << "Creating SMS account";
            missingAccounts.append(qMakePair(account, config));
        } else {
            qMailLog(Messaging) << "Unable to create SMS account!";
        }
    }
#endif
#ifndef QTOPIA_NO_MMS
    sourceType = "mms";
    if (!existingSources.contains(sourceType)) {
        QMailAccount account;
        account.setName(qApp->translate("MessageServer", "MMS"));
        account.setMessageType(QMailMessage::Mms);
        account.setStatus(QMailAccount::UserEditable, true);
        account.setStatus(QMailAccount::MessageSource, true);
        account.setStatus(QMailAccount::MessageSink, true);
        account.setStatus(QMailAccount::CanTransmit, true);

        QMailAccountConfiguration config;
        config.addServiceConfiguration(sourceType);
        QMailServiceConfiguration svcCfg(&config, sourceType);
        if (svcCfg.isValid()) {
            svcCfg.setVersion(100);
            svcCfg.setType(QMailServiceConfiguration::SourceAndSink);

            qMailLog(Messaging) << "Creating MMS account";
            missingAccounts.append(qMakePair(account, config));
        } else {
            qMailLog(Messaging) << "Unable to create MMS account!";
        }
    }
#endif
#ifndef QTOPIA_NO_COLLECTIVE
    sourceType = "jabber";
    if (!existingSources.contains(sourceType)) {
        QMailAccount account;
        account.setName(qApp->translate("MessageServer", "Collective"));
        account.setMessageType(QMailMessage::Instant);
        account.setStatus(QMailAccount::UserEditable, true);
        account.setStatus(QMailAccount::MessageSource, true);
        account.setStatus(QMailAccount::MessageSink, true);
        account.setStatus(QMailAccount::CanTransmit, true);

        QMailAccountConfiguration config;
        config.addServiceConfiguration(sourceType);
        QMailServiceConfiguration svcCfg(&config, sourceType);
        if (svcCfg.isValid()) {
            svcCfg.setVersion(100);
            svcCfg.setType(QMailServiceConfiguration::SourceAndSink);

            qMailLog(Messaging) << "Creating Collective account";
            missingAccounts.append(qMakePair(account, config));
        } else {
            qMailLog(Messaging) << "Unable to create Collective account!";
        }
    }
#endif
    sourceType = "qtopia-system";
    if (!existingSources.contains(sourceType)) {
        QMailAccount account;
        account.setName(qApp->translate("MessageServer", "System"));
        account.setMessageType(QMailMessage::System);
        account.setStatus(QMailAccount::MessageSource, true);

        QMailAccountConfiguration config;
        config.addServiceConfiguration(sourceType);
        QMailServiceConfiguration svcCfg(&config, sourceType);
        if (svcCfg.isValid()) {
            svcCfg.setVersion(100);
            svcCfg.setType(QMailServiceConfiguration::Source);

            qMailLog(Messaging) << "Creating System account";
            missingAccounts.append(qMakePair(account, config));
        } else {
            qMailLog(Messaging) << "Unable to create System account!";
        }
    }

    if (!missingAccounts.isEmpty()) {
        // Create the missing accounts
        QList<QPair<QMailAccount, QMailAccountConfiguration> >::iterator it = missingAccounts.begin(), end = missingAccounts.end(); 
        for ( ; it != end; ++it) {
            QMailAccount &account((*it).first);
            QMailAccountConfiguration &config((*it).second);
            account.setStatus(QMailAccount::Enabled, true);

            QMailStore::instance()->addAccount(&account, &config);
        }
    }
#endif //QMAIL_QTOPIA
}


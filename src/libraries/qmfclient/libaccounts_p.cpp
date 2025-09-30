/****************************************************************************
**
** Copyright (C) 2025 Damien Caliste
** Contact: Damien Caliste <dcaliste@free.fr>
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "libaccounts_p.h"

#include "qmaillog.h"

namespace {
void reportAccountError(const Accounts::Error& error)
{
    switch (error.type()) {
    case Accounts::Error::NoError:
        break;
    case Accounts::Error::Deleted:
    case Accounts::Error::AccountNotFound:
        qCWarning(lcMessaging) << "Accounts:" << error.message();
        break;
    case Accounts::Error::Unknown:
    case Accounts::Error::Database:
    case Accounts::Error::DatabaseLocked:
        qCritical() << "Accounts:" << error.message();
        break;
    default:
        Q_ASSERT (false);
    }
}

QStringList toStringList(const QMailAddress &address)
{
    if (address.isNull()) {
        return QStringList();
    } else if (address.isGroup()) {
        QStringList values;
        for (const QMailAddress &sub : address.groupMembers()) {
            values.append(sub.toString(true));
        }
        return values;
    } else {
        return QStringList() << address.toString(true);
    }
}

bool AccountSatisfyTheKey(Accounts::Account* account, const QMailAccountKey& key);

template <typename Property>
bool AccountCompareProperty(Accounts::Account* account, Property value, QMailKey::Comparator op, const QMailAccountKey::ArgumentType::ValueList& arguments)
{
    // Argument list should not be empty.
    // Otherwise we have nothing to compare.
    Q_ASSERT(arguments.count());

    if (arguments.count() == 1) {
        if (!arguments.front().canConvert<Property>()) {
            if (arguments.front().canConvert<QMailAccountKey>()) {
                QMailAccountKey accountKey = arguments.front().value<QMailAccountKey>();
                return AccountSatisfyTheKey(account, accountKey);
            }

            qCWarning(lcMessaging) << "Failed to convert argument";
            return false;
        }

        Property argument = arguments.front().value<Property>();
        switch (op) {
            case QMailKey::Equal:
                return value == argument;

            case QMailKey::NotEqual:
                return value != argument;

            default:
                qCWarning(lcMessaging) << "This comparator is not supported" << op;
                break;
        }

    } else {
        switch (op) {
            case QMailKey::Includes:
            case QMailKey::Present:
                foreach (const QVariant& argument, arguments) {
                    if (argument.canConvert<Property>() && argument.value<Property>() == value)
                        return true;
                }
                return false;

            case QMailKey::Excludes:
            case QMailKey::Absent:
                foreach (const QVariant& argument, arguments) {
                    if (argument.canConvert<Property>() && argument.value<Property>() == value)
                        return false;
                }
                return true;

            default:
                qCWarning(lcMessaging) << "This comparator is not supported" << op;
                break;
        }
    }

    Q_ASSERT(false);
    return false;
}

template <>
bool AccountCompareProperty(Accounts::Account*, quint64 value, QMailKey::Comparator op, const QMailAccountKey::ArgumentType::ValueList& arguments)
{
    // Argument list should not be empty.
    // Otherwise we have nothing to compare.
    Q_ASSERT(arguments.count());

    if (arguments.count() == 1) {
        bool ok = false;
        quint64 argument = arguments.front().toULongLong(&ok);
        if (!ok) {
            qCWarning(lcMessaging) << "Failed to convert to quing64";
            return false;
        }

        switch (op) {
            case QMailKey::LessThan:
                return value < argument;

            case QMailKey::LessThanEqual:
                return value <= argument;

            case QMailKey::GreaterThan:
                return value > argument;

            case QMailKey::GreaterThanEqual:
                return value >= argument;

            case QMailKey::Equal:
                return value == argument;

            case QMailKey::NotEqual:
                return value != argument;

            case QMailKey::Includes:
            case QMailKey::Present:
                return ((value & argument) == argument);

            case QMailKey::Excludes:
            case QMailKey::Absent:
                return !(value & argument);

            default:
                qCWarning(lcMessaging) << "This comparator is not supported" << op;
                break;
        }
    } else {
        switch (op) {
            case QMailKey::Includes:
            case QMailKey::Present:
                foreach (const QVariant& argument, arguments) {
                    if (value == argument.toULongLong())
                        return true;
                }
                return false;

            case QMailKey::Excludes:
            case QMailKey::Absent:
                foreach (const QVariant& argument, arguments) {
                    if (value == argument.toULongLong())
                        return false;
                }
                return true;

            default:
                qCWarning(lcMessaging) << "This comparator is not supported" << op;
                break;
        }
    }
    Q_ASSERT(false);
    return false;
}

template <>
bool AccountCompareProperty(Accounts::Account*, const QString& value, QMailKey::Comparator op, const QMailAccountKey::ArgumentType::ValueList& arguments)
{
    // Argument list should not be empty.
    // Otherwise we have nothing to compare.
    Q_ASSERT(arguments.count());

    if (arguments.count() == 1) {
        if (!arguments.front().canConvert<QString>()) {
            qCWarning(lcMessaging) << "Failed to convert to string";
            return false;
        }

        QString argument = arguments.front().toString();
        switch (op) {
            case QMailKey::LessThan:
                return value < argument;

            case QMailKey::LessThanEqual:
                return value <= argument;

            case QMailKey::GreaterThan:
                return value > argument;

            case QMailKey::GreaterThanEqual:
                return value >= argument;

            case QMailKey::Equal:
                return value == argument;

            case QMailKey::NotEqual:
                return value != argument;

            case QMailKey::Includes:
            case QMailKey::Present:
                return value.contains(argument);

            case QMailKey::Excludes:
            case QMailKey::Absent:
                return !value.contains(argument);

            default:
                qCWarning(lcMessaging) << "This comparator is not supported" << op;
                break;
        }
    } else {
        switch (op) {
            case QMailKey::Includes:
            case QMailKey::Present:
                foreach (const QVariant& argument, arguments) {
                    if (value == argument.toString())
                        return true;
                }
                return false;

            case QMailKey::Excludes:
            case QMailKey::Absent:
                foreach (const QVariant& argument, arguments) {
                    if (value == argument.toString())
                        return false;
                }
                return true;

            default:
                qCWarning(lcMessaging) << "This comparator is not supported" << op;
                break;
        }
    }
    Q_ASSERT(false);
    return false;
}

bool AccountCompareProperty(Accounts::Account* account, QMailKey::Comparator op, const QMailAccountKey::ArgumentType::ValueList& arguments)
{
    // Argument list should not be empty.
    // Otherwise we have nothing to compare.
    Q_ASSERT(arguments.count() == 1);

    QStringList argument = arguments.front().toStringList();

    QString key = argument.front();
    QString value = argument.count() == 2 ? argument.back() : QString();

    account->beginGroup(QLatin1String("customFields"));

    bool result = false;
    switch (op) {
        case QMailKey::Equal:
            result = account->contains(key) && (account->valueAsString(key) == value);
            break;

        case QMailKey::NotEqual:
            result = !account->contains(key) || (account->valueAsString(key) != value);
            break;

        case QMailKey::Includes:
        case QMailKey::Present:
            result = account->contains(key) &&  account->valueAsString(key).contains(value);
            break;

        case QMailKey::Excludes:
        case QMailKey::Absent:
            result = !(account->contains(key) && account->valueAsString(key).contains(value));
            break;

        default:
            qCWarning(lcMessaging) << "This comparator is not supported" << op;
            break;
    }
    account->endGroup();
    return result;
}

bool AccountSatisfyTheProperty(Accounts::Account* account, const QMailAccountKey::ArgumentType& argument)
{
    Q_ASSERT(account);

    switch (argument.property) {
        case QMailAccountKey::Id:
            return AccountCompareProperty<QMailAccountId>
                (account, QMailAccountId(account->id()),
                 argument.op, argument.valueList);

        case QMailAccountKey::Name:
            return AccountCompareProperty<const QString&>
                (account, account->displayName(),
                 argument.op, argument.valueList);

        case QMailAccountKey::MessageType:
            return AccountCompareProperty<quint64>
                (account, account->valueAsInt(QLatin1String("type")),
                 argument.op, argument.valueList);

        case QMailAccountKey::FromAddress:
            return AccountCompareProperty<const QString&>
                (account, QMailAddress(account->valueAsString(QLatin1String("emailaddress"))).address(),
                 argument.op, argument.valueList);

        case QMailAccountKey::Status: {
            Accounts::Service service = account->selectedService();

            account->selectService();
            const bool enabled = account->enabled();
            account->selectService(service);

            quint64 status = account->valueAsUInt64(QLatin1String("status"));
            status &= (~QMailAccount::Enabled);
            status |= enabled ? (QMailAccount::Enabled) : 0;

            bool canTransmit = account->valueAsBool(QLatin1String("canTransmit"), true);
            status &= (~QMailAccount::CanTransmit);
            status |= canTransmit ? (QMailAccount::CanTransmit) : 0;

            bool appendSignature = account->valueAsBool(QLatin1String("signatureEnabled"), true);
            status &= (~QMailAccount::AppendSignature);
            status |= appendSignature ? (QMailAccount::AppendSignature) : 0;

            bool hasPersistentConnection = account->valueAsBool(QLatin1String("hasPersistentConnection"), false);
            status &= (~QMailAccount::HasPersistentConnection);
            status |= hasPersistentConnection ? (QMailAccount::HasPersistentConnection) : 0;

            return AccountCompareProperty<quint64>(account,
                                                   status,
                                                   argument.op, argument.valueList);
        }

        case QMailAccountKey::Custom:
            return AccountCompareProperty(account, argument.op, argument.valueList);

        default:
            qCWarning(lcMessaging) << "This property is not supported" << argument.property;
            break;
    }
    return false;
}

bool AccountSatisfyTheKey(Accounts::Account* account, const QMailAccountKey& key)
{
    Q_ASSERT(account);

    if (key.isNonMatching())
        return false;

    if (key.isEmpty())
        return true;

    // In case of it is not compound key and has got a list of arguments
    // follow the list of arguments and compare
    if (!key.arguments().isEmpty()) {
        typedef QList<QMailAccountKey::ArgumentType> ListOfArguments;
        ListOfArguments::const_iterator it = key.arguments().begin();

        bool result = AccountSatisfyTheProperty(account, *it);
        while (++it != key.arguments().end()) {
            switch (key.combiner()) {
                case QMailKey::And:
                    result = result && AccountSatisfyTheProperty(account, *it);
                    break;
                case QMailKey::Or:
                    result = result || AccountSatisfyTheProperty(account, *it);
                    break;
                default:
                    Q_ASSERT(false);
                    break;
            }
        }

        // Return negated value if key was negeated
        return key.isNegated() ? !result : result;
    }

    // In case of compound key, process each subkey separatelly
    if (!key.subKeys().isEmpty()) {
        typedef QList<QMailAccountKey> ListOfKeys;
        ListOfKeys::const_iterator it = key.subKeys().begin();

        bool result = AccountSatisfyTheKey(account, *it);
        while (++it != key.subKeys().end()) {
            switch (key.combiner()) {
                case QMailKey::And:
                    result = result && AccountSatisfyTheKey(account, *it);
                    break;
                case QMailKey::Or:
                    result = result || AccountSatisfyTheKey(account, *it);
                    break;
                default:
                    Q_ASSERT(false);
                    break;
            }
        }

        // Return negated value if key was negeated
        return key.isNegated() ? !result : result;
    }

    // This key is not empty and has neither subkeys nor arguments.
    Q_ASSERT(false);
    return false;
}

}

static QSharedPointer<Accounts::Manager> managerInstance;

LibAccountManager::LibAccountManager(QObject *parent)
    : QMailAccountManager(parent)
{
    if (!managerInstance) {
        managerInstance = QSharedPointer<Accounts::Manager>(new Accounts::Manager(QLatin1String("e-mail")));
        managerInstance->setAbortOnTimeout(true);
    }
    manager = managerInstance;
    // manager to notify QMailStore about the changes
    connect(manager.data(), &Accounts::Manager::accountCreated,
            this, &LibAccountManager::onAccountCreated);
    connect(manager.data(), &Accounts::Manager::accountRemoved,
            this, &LibAccountManager::onAccountRemoved);
    connect(manager.data(), &Accounts::Manager::accountUpdated,
            this, &LibAccountManager::onAccountUpdated);
}

LibAccountManager::~LibAccountManager()
{
}

QMailAccount LibAccountManager::account(const QMailAccountId &id) const
{
    QMailAccount result;

    if (!id.isValid())
        return result;

    QSharedPointer<Accounts::Account> account = getAccount(id);
    if (!account) {
        // On the contrary to updateAccount, not finding an account is not a failure
        // here, because the purpose of this function is to inquire for one.
        // We return success and we keep result as invalid / empty.
        return result;
    }
    Accounts::ServiceList services = account->enabledServices();
    if (services.count() != 1) {
        qCWarning(lcMessaging) << "Cannot handle several email services for account" << id;
        return result;
    }
    Accounts::Service service = services.first();
    Q_ASSERT(service.serviceType() == QLatin1String("e-mail"));

    result.setId(QMailAccountId(account->id()));

    account->selectService();
    const bool enabled = account->enabled();

    account->selectService(service);
    QString name = account->valueAsString(QLatin1String("email/email_box_name"));
    if (name.isEmpty())
        name = account->displayName();
    result.setName(name);
    result.setMessageType(static_cast<QMailMessageMetaData::MessageType>(account->valueAsInt(QLatin1String("type"))));
    result.setStatus(account->valueAsUInt64(QLatin1String("status")));
    const bool isDefault = account->valueAsBool(QLatin1String("email/default"));
    const bool canTransmit = account->valueAsBool(QLatin1String("canTransmit"), true);
    const bool appendSignature = account->valueAsBool(QLatin1String("signatureEnabled"), true);
    const bool hasPersistentConnection = account->valueAsBool(QLatin1String("hasPersistentConnection"), false);

    result.setStatus(QMailAccount::Enabled, enabled);
    result.setStatus(QMailAccount::PreferredSender, isDefault);
    result.setStatus(QMailAccount::CanTransmit, canTransmit);
    result.setStatus(QMailAccount::AppendSignature, appendSignature);
    result.setStatus(QMailAccount::HasPersistentConnection, hasPersistentConnection);

    result.setSignature(account->valueAsString(QLatin1String("signature")));
    result.setFromAddress(account->contains(QLatin1String("fullName"))
                          ? QMailAddress(account->valueAsString(QLatin1String("fullName")), account->valueAsString(QLatin1String("emailaddress")))
                          : QMailAddress(account->valueAsString(QLatin1String("emailaddress"))));
    const QStringList aliases = account->value(QLatin1String("emailAliases")).toStringList();
    if (!aliases.isEmpty())
        result.setFromAliases(QMailAddress(aliases.join(';')));

    if ((static_cast<uint>(account->valueAsUInt64(QLatin1String("lastSynchronized")))) == 0) {
        result.setLastSynchronized(QMailTimeStamp());
    } else {
        result.setLastSynchronized(QMailTimeStamp(QDateTime::fromSecsSinceEpoch(static_cast<uint>(account->valueAsUInt64(QLatin1String("lastSynchronized"))))));
    }

    result.setIconPath(account->valueAsString(QLatin1String("iconPath")));

    // Find any custom fields for this account
    QMap<QString, QString> fields;
    account->beginGroup(QLatin1String("customFields"));
    foreach (const QString& key, account->allKeys()) {
        fields.insert(key, account->valueAsString(key));
    }
    account->endGroup();
    result.setCustomFields(fields);
    setCustomFieldsModified(&result, false);

    // Find the type of the account
    foreach (const QString& group, account->childGroups()) {
        if (group != QLatin1String("customFields")) {
            account->beginGroup(group);

            QString serviceType = account->valueAsString(QLatin1String("servicetype"));
            if (serviceType.contains(QLatin1String("source")))
                addMessageSource(&result, group);

            if (serviceType.contains(QLatin1String("sink")))
                addMessageSink(&result, group);

            account->endGroup();
        }
    }

    return result;
}

QMailAccountConfiguration LibAccountManager::accountConfiguration(const QMailAccountId &id) const
{
    QMailAccountConfiguration result;

    QSharedPointer<Accounts::Account> account = getAccount(id);
    if (!account)
        return result;

    Accounts::ServiceList services = account->enabledServices();
    if (services.count() != 1) {
        qCWarning(lcMessaging) << "Cannot handle several email services for account" << id;
        return result;
    }

    Accounts::Service service = services.first();
    Q_ASSERT(service.serviceType() == QLatin1String("e-mail"));

    account->selectService(service);

    foreach (const QString& group, account->childGroups()) {
        if (group != QLatin1String("customFields")) {
            if (!result.services().contains(group)) {
                // Add this service to the configuration
                result.addServiceConfiguration(group);
            }

            QMailAccountConfiguration::ServiceConfiguration* serviceConfig = &result.serviceConfiguration(group);
            Q_ASSERT(serviceConfig);

            account->beginGroup(group);
            foreach (const QString& key, account->allKeys()) {
                const QVariant &value = account->value(key);
                if (value.typeName() == QStringLiteral("QStringList")) {
                    serviceConfig->setValue(key, value.toStringList());
                } else {
                    serviceConfig->setValue(key, value.toString());
                }
            }
            account->endGroup();
        }
    }

    result.setId(id);
    setModified(&result, false);

    return result;
}

QMailAccountIdList LibAccountManager::queryAccounts(const QMailAccountKey &key,
                                                    const QMailAccountSortKey &sortKey,
                                                    uint limit, uint offset) const
{
    Q_UNUSED (sortKey);

    Accounts::AccountIdList accountIDList = manager->accountList(QLatin1String("e-mail"));

    // Populate all E-Mail accounts
    QMailAccountIdList accountList;

    foreach (const Accounts::AccountId& accountID, accountIDList) {
        Accounts::Account* account = Accounts::Account::fromId(manager.data(), accountID);
        if (!account) {
            reportAccountError(manager->lastError());
            continue;
        }

        Accounts::ServiceList services = account->enabledServices();
        const int count = services.count();
        switch (count) {
        case 0: // ignore such accounts
            break;
        case 1: {
            Accounts::Service service = services.first();
            account->selectService(service);
            if (AccountSatisfyTheKey(account, key))
                accountList.append(QMailAccountId(account->id()));
        } break;
        default:
            qCWarning(lcMessaging) << "Cannot handle several email services for account" << accountID;
            return QMailAccountIdList();
        }

        delete account;
    }

    /*
     * TBD: Use sortKey to sort found accounts properly
     */

    return accountList.mid(offset, limit ? limit : -1);
}

bool LibAccountManager::addAccount(QMailAccount *account,
                                   QMailAccountConfiguration *config)
{
    if (account->id().isValid() && getAccount(account->id())) {
        qCWarning(lcMessaging) << "Account already exists in database, use update instead";
        return false;
    }

    // Create new account in Accounts subsystem
    QSharedPointer<Accounts::Account> sharedAccount(manager->createAccount(QLatin1String("email")));
    if (!sharedAccount) {
        qCWarning(lcMessaging) << "Failed to create account";
        return false;
    }

    sharedAccount->setDisplayName(account->name());
    sharedAccount->setEnabled(account->status() & QMailAccount::Enabled);

    Accounts::ServiceList services = sharedAccount->services(QLatin1String("e-mail"));
    if (!services.count()) {
        qCWarning(lcMessaging) << "E-mail Services not found, make sure that *.service and *.provider files are properly installed.";
        return false;
    }
    if (services.count() != 1) {
        qCWarning(lcMessaging) << "Cannot handle several email services for account" << account->id();
        return false;
    }
    Accounts::Service service = services.first();
    Q_ASSERT(service.serviceType() == QLatin1String("e-mail"));

    sharedAccount->selectService(service);
    sharedAccount->setEnabled(true); // service is enabled anyway
    sharedAccount->setValue(QLatin1String("type"), static_cast<int>(account->messageType()));
    sharedAccount->setValue(QLatin1String("status"), account->status());
    const bool appendSignature = (account->status() & QMailAccount::AppendSignature);
    sharedAccount->setValue(QLatin1String("signatureEnabled"), appendSignature);
    const bool hasPersistentConnection = (account->status() & QMailAccount::HasPersistentConnection);
    sharedAccount->setValue(QLatin1String("hasPersistentConnection"), hasPersistentConnection);
    sharedAccount->setValue(QLatin1String("signature"), account->signature());
    sharedAccount->setValue(QLatin1String("emailaddress"), account->fromAddress().address());
    sharedAccount->setValue(QLatin1String("fullName"), account->fromAddress().name());
    sharedAccount->setValue(QLatin1String("emailAliases"), toStringList(account->fromAliases()));
    //Account was never synced
    sharedAccount->setValue(QLatin1String("lastSynchronized"), quint64(0));
    sharedAccount->setValue(QLatin1String("iconPath"), account->iconPath());
    const bool canTransmit = (account->status() & QMailAccount::CanTransmit);
    sharedAccount->setValue(QLatin1String("canTransmit"), canTransmit);

    // Insert any custom fields belonging to this account
    QMap<QString, QString> fields = account->customFields();
    if (!fields.isEmpty()) {
        sharedAccount->beginGroup(QLatin1String("customFields"));

        // Insert any custom fields belonging to this account
        QMap<QString, QString>::const_iterator it = fields.begin(), end = fields.end();
        for ( ; it != end; ++it) {
            sharedAccount->setValue(it.key(), QVariant(it.value()));
        }
        sharedAccount->endGroup();
    }

    if (config) {
        foreach (const QString &service, config->services()) {
            QMailAccountConfiguration::ServiceConfiguration &serviceConfig(config->serviceConfiguration(service));
            const QMap<QString, QString> &fields = serviceConfig.values();
            QString serviceName = serviceConfig.service();

            // Open configuration group
            sharedAccount->beginGroup(serviceName);

            // Insert any configuration fields belonging to this account
            QMap<QString, QString>::const_iterator it = fields.begin(), end = fields.end();
            for ( ; it != end; ++it) {
                bool isList;
                const QStringList list = QMailAccountConfiguration::ServiceConfiguration::asList(*it, &isList);
                if (isList) {
                    sharedAccount->setValue(it.key(), QVariant(list));
                } else {
                    sharedAccount->setValue(it.key(), QVariant(it.value()));
                }
            }
            // Close group of keys
            sharedAccount->endGroup();
        }
    }

    // Save all changes
    if (!sharedAccount->syncAndBlock()) {
        sharedAccount->remove();
        sharedAccount->syncAndBlock();
        return false;
    }

    //Extract the insert id
    QMailAccountId insertId = QMailAccountId(sharedAccount->id());
    account->setId(insertId);
    if (config) {
        config->setId(insertId);
    }
    return true;
}

bool LibAccountManager::removeAccounts(const QMailAccountIdList &ids)
{
    bool success = true;
    for (const QMailAccountId& accountID : ids) {
        QSharedPointer<Accounts::Account> account(Accounts::Account::fromId(manager.data(), accountID.toULongLong()));
        if (account) {
            account->remove();
            if (!account->syncAndBlock()) {
                qCWarning(lcMessaging) << "cannot remove account" << accountID;
                success = false;
            }
        }
    }
    return success;
}

void LibAccountManager::updateAccountCustomFields(QSharedPointer<Accounts::Account>& account, const QMap<QString, QString> &fields)
{
    account->beginGroup(QLatin1String("customFields"));

    QMap<QString, QString> existing;
    foreach (const QString& name, account->allKeys()) {
        existing.insert(name, account->valueAsString(name));
    }

    QVariantList obsoleteFields;
    QVariantList modifiedFields;
    QVariantList modifiedValues;
    QVariantList addedFields;
    QVariantList addedValues;

    // Compare the sets
    QMap<QString, QString>::const_iterator fend = fields.end(), eend = existing.end();
    QMap<QString, QString>::const_iterator it = existing.begin();
    for ( ; it != eend; ++it) {
        QMap<QString, QString>::const_iterator current = fields.find(it.key());
        if (current == fend) {
            obsoleteFields.append(QVariant(it.key()));
        } else if (*current != *it) {
            modifiedFields.append(QVariant(current.key()));
            modifiedValues.append(QVariant(current.value()));
        }
    }

    for (it = fields.begin(); it != fend; ++it) {
        if (existing.find(it.key()) == eend) {
            addedFields.append(QVariant(it.key()));
            addedValues.append(QVariant(it.value()));
        }
    }

    // Remove the obsolete fields
    foreach (const QVariant& obsolete, obsoleteFields) {
        account->remove(obsolete.toString());
    }

    if (!modifiedFields.isEmpty()) {
        // Batch update of the modified fields
        QVariantList::const_iterator field = modifiedFields.begin();
        QVariantList::const_iterator value = modifiedValues.begin();
        while (field != modifiedFields.end() && value != modifiedValues.end())
            account->setValue(field++->toString(), *value++);
    }

    if (!addedFields.isEmpty()) {
        // Batch insert of the added fields
        QVariantList::const_iterator field = addedFields.begin();
        QVariantList::const_iterator value = addedValues.begin();
        while (field != addedFields.end() && value != addedValues.end())
            account->setValue(field++->toString(), *value++);

    }

    account->endGroup();
}

bool LibAccountManager::updateSharedAccount(QMailAccount *account,
                                            QMailAccountConfiguration *config)
{
    QMailAccountId id(account ? account->id() : config ? config->id() : QMailAccountId());
    if (!id.isValid())
        return false;

    QSharedPointer<Accounts::Account> sharedAccount = getAccount(id);
    if (!sharedAccount)
        return false;

    Accounts::ServiceList services = sharedAccount->enabledServices();
    if (services.count() != 1) {
        qCWarning(lcMessaging) << "Cannot handle several email services for account" << id;
        return false;
    }

    Accounts::Service service = services.first();
    Q_ASSERT(service.isValid());
    Q_ASSERT(service.serviceType() == QLatin1String("e-mail"));

    if (account) {
        sharedAccount->selectService(service);
        bool isEmailBoxName = false;
        if (!sharedAccount->valueAsString(QLatin1String("email/email_box_name")).isEmpty()) {
            isEmailBoxName = true;
            sharedAccount->setValue(QLatin1String("email/email_box_name"), account->name());
        }
        sharedAccount->selectService();
        if (isEmailBoxName) {
            sharedAccount->setDisplayName(sharedAccount->valueAsString(QLatin1String("username")));
        } else {
            sharedAccount->setDisplayName(account->name());
        }
        sharedAccount->setEnabled(account->status() & QMailAccount::Enabled);
        sharedAccount->selectService(service);
        sharedAccount->setValue(QLatin1String("type"), static_cast<int>(account->messageType()));
        sharedAccount->setValue(QLatin1String("status"), account->status());
        bool signatureEnabled = account->status() & QMailAccount::AppendSignature;
        sharedAccount->setValue(QLatin1String("signatureEnabled"), signatureEnabled);
        bool hasPersistentConnection = account->status() & QMailAccount::HasPersistentConnection;
        sharedAccount->setValue(QLatin1String("hasPersistentConnection"), hasPersistentConnection);
        sharedAccount->setValue(QLatin1String("signature"), account->signature());
        sharedAccount->setValue(QLatin1String("emailaddress"), account->fromAddress().address());
        sharedAccount->setValue(QLatin1String("fullName"), account->fromAddress().name());
        sharedAccount->setValue(QLatin1String("emailAliases"), toStringList(account->fromAliases()));
        if (account->lastSynchronized().isValid()) {
            sharedAccount->setValue(QLatin1String("lastSynchronized"), static_cast<quint64>(account->lastSynchronized().toLocalTime().toSecsSinceEpoch()));
        } else {
            sharedAccount->setValue(QLatin1String("lastSynchronized"), quint64(0));
        }
        bool isDefault = account->status() & QMailAccount::PreferredSender;
        bool canTransmit = account->status() & QMailAccount::CanTransmit;
        sharedAccount->setValue(QLatin1String("email/default"), isDefault);
        sharedAccount->setValue(QLatin1String("canTransmit"), canTransmit);
        sharedAccount->setValue(QLatin1String("iconPath"), account->iconPath());

        if (customFieldsModified(*account)) {
            updateAccountCustomFields(sharedAccount, account->customFields());
        }
    }

    if (config) {
        sharedAccount->selectService(service);
        // Find the complete set of configuration fields
        QMap<QPair<QString, QString>, QString> fields;

        foreach (const QString &service, config->services()) {
            QMailAccountConfiguration::ServiceConfiguration &serviceConfig(config->serviceConfiguration(service));
            const QMap<QString, QString> &values = serviceConfig.values();

            // Insert any configuration fields belonging to this account
            QMap<QString, QString>::const_iterator it = values.begin(), end = values.end();
            for ( ; it != end; ++it)
                fields.insert(qMakePair(service, it.key()), it.value());
        }

        // Find the existing fields in the database
        QMap<QPair<QString, QString>, QString> existing;
        foreach (const QString& group, sharedAccount->childGroups()) {
            if (group != QLatin1String("customFields")) {
                sharedAccount->beginGroup(group);
                foreach (const QString& name, sharedAccount->allKeys()) {
                    const QVariant &value = sharedAccount->value(name);
                    if (value.typeName() == QStringLiteral("QStringList")) {
                        existing.insert(qMakePair(group, name), QMailAccountConfiguration::ServiceConfiguration::fromList(value.toStringList()));
                    } else {
                        existing.insert(qMakePair(group, name), value.toString());
                    }
                }
                sharedAccount->endGroup();
            }
        }

        QMap<QString, QVariantList> obsoleteFields;
        QMap<QString, QVariantList> modifiedFields;
        QMap<QString, QVariantList> modifiedValues;
        QMap<QString, QVariantList> addedFields;
        QMap<QString, QVariantList> addedValues;

        // Compare the sets
        QMap<QPair<QString, QString>, QString>::const_iterator fend = fields.end(), eend = existing.end();
        QMap<QPair<QString, QString>, QString>::const_iterator it = existing.begin();
        for ( ; it != eend; ++it) {
            const QPair<QString, QString> &name = it.key();
            QMap<QPair<QString, QString>, QString>::const_iterator current = fields.find(name);
            if (current == fend) {
                obsoleteFields[name.first].append(QVariant(name.second));
            } else if (*current != *it) {
                modifiedFields[name.first].append(QVariant(name.second));
                bool isList;
                const QStringList list = QMailAccountConfiguration::ServiceConfiguration::asList(current.value(), &isList);
                if (isList) {
                    modifiedValues[name.first].append(QVariant(list));
                } else {
                    modifiedValues[name.first].append(QVariant(current.value()));
                }
            }
        }

        for (it = fields.begin(); it != fend; ++it) {
            const QPair<QString, QString> &name = it.key();
            if (existing.find(name) == eend) {
                addedFields[name.first].append(QVariant(name.second));
                bool isList;
                const QStringList list = QMailAccountConfiguration::ServiceConfiguration::asList(it.value(), &isList);
                if (isList) {
                    addedValues[name.first].append(QVariant(list));
                } else {
                    addedValues[name.first].append(QVariant(it.value()));
                }
            }
        }

        if (!obsoleteFields.isEmpty()) {
            // Remove the obsolete fields
            QMap<QString, QVariantList>::const_iterator it = obsoleteFields.begin(), end = obsoleteFields.end();
            for ( ; it != end; ++it) {
                const QString &service = it.key();
                const QVariantList &fields = it.value();
                sharedAccount->beginGroup(service);
                foreach (const QVariant& field, fields) {
                    sharedAccount->remove(field.toString());
                }
                sharedAccount->endGroup();
            }
        }

        if (!modifiedFields.isEmpty()) {
            // Batch update the modified fields
            QMap<QString, QVariantList>::const_iterator it = modifiedFields.begin(), end = modifiedFields.end();
            for (QMap<QString, QVariantList>::const_iterator vit = modifiedValues.begin(); it != end; ++it, ++vit) {
                const QString &service = it.key();
                const QVariantList &fields = it.value();
                const QVariantList &values = vit.value();
                QVariantList::const_iterator field = fields.begin();
                QVariantList::const_iterator value = values.begin();

                sharedAccount->beginGroup(service);
                while (field != fields.end() && value != values.end())
                    sharedAccount->setValue(field++->toString(), *value++);

                sharedAccount->endGroup();
            }
        }

        if (!addedFields.isEmpty()) {
            // Batch insert the added fields
            QMap<QString, QVariantList>::const_iterator it = addedFields.begin(), end = addedFields.end();
            for (QMap<QString, QVariantList>::const_iterator vit = addedValues.begin(); it != end; ++it, ++vit) {
                const QString &service = it.key();
                const QVariantList &fields = it.value();
                const QVariantList &values = vit.value();

                QVariantList::const_iterator field = fields.begin();
                QVariantList::const_iterator value = values.begin();

                sharedAccount->beginGroup(service);
                while (field != fields.end() && value != values.end())
                    sharedAccount->setValue(field++->toString(), *value++);

                sharedAccount->endGroup();
            }
        }
    }

    return sharedAccount->syncAndBlock();
}

bool LibAccountManager::updateAccount(QMailAccount *account,
                                      QMailAccountConfiguration *config)
{
    return updateSharedAccount(account, config);
}

bool LibAccountManager::updateAccountConfiguration(QMailAccountConfiguration *config)
{
    return updateSharedAccount(nullptr, config);
}

void LibAccountManager::clearContent()
{
    // Remove all email accounts
    Accounts::AccountIdList accountIDList = manager->accountList(QLatin1String("e-mail"));

    // Populate all E-Mail accounts
    foreach (Accounts::AccountId accountID, accountIDList) {
        // Remove account
        QSharedPointer<Accounts::Account> account(Accounts::Account::fromId(manager.data(), accountID));

        if (account) {
            account->remove();
            account->syncAndBlock();
        } else {
            qCWarning(lcMessaging) << Q_FUNC_INFO << "E-mail Services not found, make sure that *.service and *.provider files are properly installed and e-mail services are enabled.";
            reportAccountError(manager->lastError());
        }
    }
}

QSharedPointer<Accounts::Account> LibAccountManager::getAccount(const QMailAccountId &id) const
{
    //get account from the manager
    QSharedPointer<Accounts::Account> account(Accounts::Account::fromId(manager.data(), id.toULongLong()));

    if (!account) {
        qCWarning(lcMessaging) << Q_FUNC_INFO << "Account with was not found" ;
        reportAccountError(manager->lastError());
        return account;
    }

    // check if it is an e-mail account
    Accounts::ServiceList services = account->enabledServices();
    if (!services.count()) {
        account = QSharedPointer<Accounts::Account>();
    }

    return account;
}

bool LibAccountManager::accountValid(Accounts::AccountId id) const
{
    QSharedPointer<Accounts::Account> account(Accounts::Account::fromId(manager.data(), id));

    if (!account) {
        reportAccountError(manager->lastError());
        return false;
    }

    // Account should already have the type "e-mail",
    // ignore extra checks

    return true;
}

void LibAccountManager::onAccountCreated(Accounts::AccountId id)
{
    // ignore non-email accounts
    if (!accountValid(id))
        return;

    emit accountCreated(QMailAccountId(id));
}

void LibAccountManager::onAccountRemoved(Accounts::AccountId id)
{
    // ignore non-email accounts
    if (!accountValid(id))
        return;

    emit accountRemoved(QMailAccountId(id));
}

void LibAccountManager::onAccountUpdated(Accounts::AccountId id)
{
    if (!accountValid(id))
        return;

    emit accountUpdated(QMailAccountId(id));
}

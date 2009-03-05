/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef QMAILACCOUNTCONFIG_H
#define QMAILACCOUNTCONFIG_H

#include "qmailid.h"
#include "qmailglobal.h"
#include <QMap>
#include <QObject>
#include <QSharedData>
#include <QString>

class QMailAccountConfigurationPrivate;

class QTOPIAMAIL_EXPORT QMailAccountConfiguration
{
    class ConfigurationValues;
    class ServiceConfigurationPrivate;

public:
    class QTOPIAMAIL_EXPORT ServiceConfiguration
    {
    public:
        ServiceConfiguration();
        ServiceConfiguration(const ServiceConfiguration &other);

        ~ServiceConfiguration();

        QString service() const;
        QMailAccountId id() const;

        QString value(const QString &name) const;
        void setValue(const QString &name, const QString &value);

        void removeValue(const QString &name);

        const QMap<QString, QString> &values() const;

        const ServiceConfiguration &operator=(const ServiceConfiguration &other);

    private:
        friend class QMailAccountConfiguration;
        friend class QMailAccountConfigurationPrivate;

        ServiceConfiguration(QMailAccountConfigurationPrivate *, const QString *, ConfigurationValues *);

        ServiceConfigurationPrivate *d;
    };

    QMailAccountConfiguration();
    explicit QMailAccountConfiguration(const QMailAccountId& id);
    QMailAccountConfiguration(const QMailAccountConfiguration &other);

    ~QMailAccountConfiguration();

    const QMailAccountConfiguration &operator=(const QMailAccountConfiguration &other);

    void setId(const QMailAccountId &id);
    QMailAccountId id() const;

    ServiceConfiguration &serviceConfiguration(const QString &service);
    const ServiceConfiguration &serviceConfiguration(const QString &service) const;

    bool addServiceConfiguration(const QString &service);
    bool removeServiceConfiguration(const QString &service);

    QStringList services() const;

private:
    friend class QMailAccountConfigurationPrivate;
    friend class QMailStorePrivate;

    bool modified() const;
    void setModified(bool set);

    QSharedDataPointer<QMailAccountConfigurationPrivate> d;
};

#endif

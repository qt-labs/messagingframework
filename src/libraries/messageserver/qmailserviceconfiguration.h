/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef QMAILSERVICECONFIGURATION_H
#define QMAILSERVICECONFIGURATION_H

#include <qmailaccountconfiguration.h>
#include <QString>


class MESSAGESERVER_EXPORT QMailServiceConfiguration
{
public:
    enum ServiceType { Unknown = 0, Source, Sink, SourceAndSink, Storage };

    QMailServiceConfiguration(QMailAccountConfiguration *config, const QString &service);
    QMailServiceConfiguration(const QMailAccountConfiguration &config, const QString &service);
    QMailServiceConfiguration(const QMailAccountConfiguration::ServiceConfiguration &config);

    ~QMailServiceConfiguration();

    QString service() const;
    QMailAccountId id() const;

    int version() const;
    void setVersion(int version);

    ServiceType type() const;
    void setType(ServiceType type);

    bool isValid() const;
    bool isEmpty() const;

    QString value(const QString &name, const QString &defaultValue = QString()) const;
    void setValue(const QString &name, const QString &value);

protected:
    static QString encodeValue(const QString &value);
    static QString decodeValue(const QString &value);

private:
    QMailAccountConfiguration::ServiceConfiguration *_config;
};


#endif

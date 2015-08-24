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

#ifndef QMAILACCOUNTCONFIG_H
#define QMAILACCOUNTCONFIG_H

#include "qmailid.h"
#include "qmailglobal.h"
#include <QMap>
#include <QObject>
#include <QSharedData>
#include <QString>

class QMailAccountConfigurationPrivate;

class QMF_EXPORT QMailAccountConfiguration
{
    class ConfigurationValues;
    class ServiceConfigurationPrivate;

public:
    class QMF_EXPORT ServiceConfiguration
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

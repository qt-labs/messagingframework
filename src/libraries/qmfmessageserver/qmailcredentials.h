/****************************************************************************
**
** Copyright (C) 2024 Damien Caliste
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

#ifndef QMAILCREDENTIALSPLUGIN_H
#define QMAILCREDENTIALSPLUGIN_H

#include "qmailglobal.h"
#include <QObject>
#include <QString>

#include <qmailaccountconfiguration.h>
#include <qmailserviceconfiguration.h>

class QMailCredentialsInterfacePrivate;

class MESSAGESERVER_EXPORT QMailCredentialsInterface : public QObject
{
    Q_OBJECT

public:
    enum Status {
        Invalid,
        Fetching,
        Ready,
        Failed
    };

    typedef QMailCredentialsInterfacePrivate ImplementationType;

    QMailCredentialsInterface(QObject* parent = nullptr);
    ~QMailCredentialsInterface();

    virtual bool init(const QMailServiceConfiguration &svcCfg);

    virtual void authSuccessNotice(const QString &source = QString());
    virtual void authFailureNotice(const QString &source = QString());
    virtual bool shouldRetryAuth() const;

    virtual Status status() const = 0;
    virtual QString lastError() const = 0;

    virtual QMailAccountId id() const;
    virtual QString service() const;

    virtual QString username() const;
    virtual QString password() const;
    virtual QString accessToken() const;

Q_SIGNALS:
    void statusChanged();

protected:
    virtual void invalidate(const QString &source);
    virtual bool isInvalidated();

private:
    QScopedPointer<QMailCredentialsInterfacePrivate> d;
};
QT_BEGIN_NAMESPACE
Q_DECLARE_INTERFACE(QMailCredentialsInterface, "org.qt-project.Qt.QMailCredentialsInterface")
QT_END_NAMESPACE

class MESSAGESERVER_EXPORT QMailCredentialsPlugin : public QObject
{
    Q_OBJECT

public:
    QMailCredentialsPlugin(QObject* parent = nullptr);
    ~QMailCredentialsPlugin();

    virtual QString key() const = 0;
    virtual QMailCredentialsInterface *createCredentialsHandler(QObject *parent = nullptr) = 0;
};
QT_BEGIN_NAMESPACE
Q_DECLARE_INTERFACE(QMailCredentialsPlugin, "org.qt-project.Qt.QMailCredentialsPlugin")
QT_END_NAMESPACE

class QMF_EXPORT QMailCredentialsFactory
{
public:
    static QStringList keys();
    static QMailCredentialsInterface *createCredentialsHandler(const QString& key,
                                                               QObject *parent = nullptr);
    static QMailCredentialsInterface *defaultCredentialsHandler(QObject *parent = nullptr);

    static QMailCredentialsInterface *getCredentialsHandlerForAccount(const QMailAccountConfiguration& config,
                                                                      QObject *parent = nullptr);
};

#endif // QMAILCREDENTIALSPLUGIN_H

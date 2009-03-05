/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef NEWCOUNTNOTIFIER_H
#define NEWCOUNTNOTIFIER_H

#include <QString>
#include <QObject>
#include <qmailmessage.h>

class NewCountNotifierPrivate;

class NewCountNotifier : public QObject
{
    Q_OBJECT

public:
    NewCountNotifier(QMailMessage::MessageType type, int count);
    ~NewCountNotifier();

    bool notify();

signals:
    void response(bool handled);
    void error(const QString& msg);

public:
    static void notify(QMailMessage::MessageType type, int count);

private:
    NewCountNotifierPrivate* d;
};

#endif

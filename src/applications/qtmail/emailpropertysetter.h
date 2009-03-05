/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef EMAILPROPERTYSETTER_H
#define EMAILPROPERTYSETTER_H

#include <QMap>

class QMailMessage;

// An object that sets the properties of an Email object, given property information in key/value form
class EmailPropertySetter
{
public:
    typedef void (EmailPropertySetter::*SetterFunction)(const QString&);
    typedef QMap<QString, SetterFunction> PropertySetterMap;

    // Create a setter to modify the supplied email object
    explicit EmailPropertySetter(QMailMessage& email);

    // Set the email to have the supplied property
    void setProperty(const QString& key, const QString& value);

    // Set the email to have all the supplied properties
    void setProperties(const QMap<QString, QString>& properties);

private:
    void invoke(SetterFunction fn, const QString& s);

    void setFrom(const QString& s);
    void setSubject(const QString& s);
    void setDate(const QString& s);
    void setTo(const QString& s);
    void setCc(const QString& s);
    void setBcc(const QString& s);
    void setReplyTo(const QString& s);
    void setMessageId(const QString& s);
    void setInReplyTo(const QString& s);
    void setPlainTextBody(const QString& s);
    void setAttachment(const QString& s);

    static PropertySetterMap setterMapInit();

private:
    QMailMessage& target;
};

#endif


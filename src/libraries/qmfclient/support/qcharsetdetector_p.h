/***************************************************************************
**
** Copyright (C) 2010, 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (directui@nokia.com)
**
** This file is part of libmeegotouch.
**
** If you have questions regarding the use of this file, please contact
** Nokia at directui@nokia.com.
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation
** and appearing in the file LICENSE.LGPL included in the packaging
** of this file.
**
****************************************************************************/

#ifndef MCHARSETDETECTOR_P_H
#define MCHARSETDETECTR_P_H

#include <unicode/utypes.h>

#include <QByteArray>
#include <QString>

class UCharsetDetector;
class QCharsetMatchPrivate;

class QCharsetMatchPrivate
{
    Q_DECLARE_PUBLIC(QCharsetMatch)

public:
    QCharsetMatchPrivate();
    QCharsetMatchPrivate(const QCharsetMatchPrivate &other);

    virtual ~QCharsetMatchPrivate();

    QCharsetMatchPrivate &operator=(const QCharsetMatchPrivate &other);

    QString _name;
    QString _language;
    qint32 _confidence;

    QCharsetMatch *q_ptr;
};

class QCharsetDetectorPrivate
{
    Q_DECLARE_PUBLIC(QCharsetDetector)

public:
    QCharsetDetectorPrivate();

    virtual ~QCharsetDetectorPrivate();

    bool hasError() const;
    void clearError();
    QString errorString() const;

    QByteArray _ba;
    QByteArray _baExtended;

    UErrorCode _status;
    UCharsetDetector *_uCharsetDetector;

    QString _declaredLocale;
    QString _declaredEncoding;

    QStringList _allDetectableCharsets;

    QCharsetDetector *q_ptr;
private:
    Q_DISABLE_COPY(QCharsetDetectorPrivate)
};

#endif

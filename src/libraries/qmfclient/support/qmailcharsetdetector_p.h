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

#ifndef QMAILCHARSETDETECTOR_P_H
#define QMAILCHARSETDETECTOR_P_H

#include <unicode/utypes.h>

#include <QByteArray>
#include <QString>

class UCharsetDetector;
class QMailCharsetMatchPrivate;

class QMailCharsetMatchPrivate
{
    Q_DECLARE_PUBLIC(QMailCharsetMatch)

public:
    QMailCharsetMatchPrivate();
    QMailCharsetMatchPrivate(const QMailCharsetMatchPrivate &other);
    virtual ~QMailCharsetMatchPrivate();

    QMailCharsetMatchPrivate &operator=(const QMailCharsetMatchPrivate &other);

    QString _name;
    QString _language;
    qint32 _confidence;

    QMailCharsetMatch *q_ptr;
};

class QMailCharsetDetectorPrivate
{
    Q_DECLARE_PUBLIC(QMailCharsetDetector)

public:
    QMailCharsetDetectorPrivate();
    virtual ~QMailCharsetDetectorPrivate();

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

    QMailCharsetDetector *q_ptr;

private:
    Q_DISABLE_COPY(QMailCharsetDetectorPrivate)
};

#endif

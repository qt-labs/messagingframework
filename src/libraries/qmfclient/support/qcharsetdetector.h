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

#ifndef QCHARSETDETECTOR_H
#define QCHARSETDETECTOR_H

#include <QList>
#include <QStringList>

class QCharsetDetectorPrivate;
class QCharsetMatchPrivate;

class QCharsetMatch
{
public:
    QCharsetMatch();
    explicit QCharsetMatch(const QString name, const QString language = QString(), const qint32 confidence = 0);
    QCharsetMatch(const QCharsetMatch &other);
    virtual ~QCharsetMatch();
    QCharsetMatch &operator=(const QCharsetMatch &other);
    bool operator<(const QCharsetMatch &other) const;
    bool operator>(const QCharsetMatch &other) const;
    QString name() const;
    void setName(QString name);
    QString language() const;
    void setLanguage(QString language);
    qint32 confidence() const;
    void setConfidence(qint32 confidence);

private:
    QCharsetMatchPrivate *const d_ptr;
    Q_DECLARE_PRIVATE(QCharsetMatch)
};

class QCharsetDetector
{
public:
    QCharsetDetector();
    QCharsetDetector(const QByteArray &ba);
    explicit QCharsetDetector(const char *str);
    QCharsetDetector(const char *data, int size);
    virtual ~QCharsetDetector();
    bool hasError() const;
    void clearError();
    QString errorString() const;
    void setText(const QByteArray &ba);
    QCharsetMatch detect();
    QList<QCharsetMatch> detectAll();
    QString text(const QCharsetMatch &charsetMatch);
    void setDeclaredLocale(const QString &locale);
    void setDeclaredEncoding(const QString &encoding);
    QStringList getAllDetectableCharsets();
    void enableInputFilter(const bool enable);
    bool isInputFilterEnabled();

private:
    Q_DISABLE_COPY(QCharsetDetector)
    QCharsetDetectorPrivate *const d_ptr;
    Q_DECLARE_PRIVATE(QCharsetDetector)
};

#endif

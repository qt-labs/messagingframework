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

#ifndef QMAILCHARSETDETECTOR_H
#define QMAILCHARSETDETECTOR_H

#include <QList>
#include <QStringList>

class QMailCharsetDetectorPrivate;
class QMailCharsetMatchPrivate;

class QMailCharsetMatch
{
public:
    QMailCharsetMatch();
    explicit QMailCharsetMatch(const QString &name, const QString &language = QString(), qint32 confidence = 0);
    QMailCharsetMatch(const QMailCharsetMatch &other);
    virtual ~QMailCharsetMatch();

    QMailCharsetMatch &operator=(const QMailCharsetMatch &other);
    bool operator<(const QMailCharsetMatch &other) const;
    bool operator>(const QMailCharsetMatch &other) const;

    QString name() const;
    void setName(const QString &name);
    QString language() const;
    void setLanguage(const QString &language);
    qint32 confidence() const;
    void setConfidence(qint32 confidence);

private:
    QMailCharsetMatchPrivate *const d_ptr;
    Q_DECLARE_PRIVATE(QMailCharsetMatch)
};

class QMailCharsetDetector
{
public:
    QMailCharsetDetector();
    QMailCharsetDetector(const QByteArray &ba);
    explicit QMailCharsetDetector(const char *str);
    QMailCharsetDetector(const char *data, int size);
    virtual ~QMailCharsetDetector();

    bool hasError() const;
    void clearError();
    QString errorString() const;
    void setText(const QByteArray &ba);
    QMailCharsetMatch detect();
    QList<QMailCharsetMatch> detectAll();
    QString text(const QMailCharsetMatch &charsetMatch);
    void setDeclaredLocale(const QString &locale);
    void setDeclaredEncoding(const QString &encoding);
    QStringList getAllDetectableCharsets();
    void enableInputFilter(const bool enable);
    bool isInputFilterEnabled();

private:
    Q_DISABLE_COPY(QMailCharsetDetector)
    QMailCharsetDetectorPrivate *const d_ptr;
    Q_DECLARE_PRIVATE(QMailCharsetDetector)
};

#endif

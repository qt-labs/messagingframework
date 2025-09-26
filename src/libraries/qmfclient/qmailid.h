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

#ifndef QMAILID_H
#define QMAILID_H

#include "qmailglobal.h"
#include "qmailipc.h"

#include <QDebug>
#include <QScopedPointer>
#include <QString>
#include <QVariant>

class QMailIdPrivate;

class QMF_EXPORT QMailAccountId
{
    QScopedPointer<QMailIdPrivate> d;
public:
    QMailAccountId();
    explicit QMailAccountId(quint64 value);
    QMailAccountId(const QMailAccountId& other);
    virtual ~QMailAccountId();

    QMailAccountId& operator=(const QMailAccountId& other);

    bool isValid() const;
    quint64 toULongLong() const;

    operator QVariant() const;

    bool operator!=(const QMailAccountId& other) const;
    bool operator==(const QMailAccountId& other) const;
    bool operator<(const QMailAccountId& other) const;

    template <typename Stream> void serialize(Stream &stream) const;
    template <typename Stream> void deserialize(Stream &stream);
};


class QMF_EXPORT QMailFolderId
{
    QScopedPointer<QMailIdPrivate> d;
public:
    enum PredefinedFolderId { LocalStorageFolderId = 1 };

    QMailFolderId();
    QMailFolderId(QMailFolderId::PredefinedFolderId id);
    explicit QMailFolderId(quint64 value);
    QMailFolderId(const QMailFolderId& other);
    virtual ~QMailFolderId();

    QMailFolderId& operator=(const QMailFolderId& other);

    bool isValid() const;
    quint64 toULongLong() const;

    operator QVariant() const;

    bool operator!=(const QMailFolderId& other) const;
    bool operator==(const QMailFolderId& other) const;
    bool operator<(const QMailFolderId& other) const;

    template <typename Stream> void serialize(Stream &stream) const;
    template <typename Stream> void deserialize(Stream &stream);
};


class QMF_EXPORT QMailMessageId
{
    QScopedPointer<QMailIdPrivate> d;
public:
    QMailMessageId();
    explicit QMailMessageId(quint64 value);
    QMailMessageId(const QMailMessageId& other);
    virtual ~QMailMessageId();

    QMailMessageId& operator=(const QMailMessageId& other);

    bool isValid() const;
    quint64 toULongLong() const;

    operator QVariant() const;

    bool operator!=(const QMailMessageId& other) const;
    bool operator==(const QMailMessageId& other) const;
    bool operator<(const QMailMessageId& other) const;

    template <typename Stream> void serialize(Stream &stream) const;
    template <typename Stream> void deserialize(Stream &stream);
};

class QMF_EXPORT QMailThreadId
{
    QScopedPointer<QMailIdPrivate> d;
public:
    QMailThreadId();
    explicit QMailThreadId(quint64 value);
    QMailThreadId(const QMailThreadId& other);
    virtual ~QMailThreadId();

    QMailThreadId& operator=(const QMailThreadId& other);

    bool isValid() const;
    quint64 toULongLong() const;

    operator QVariant() const;

    bool operator!=(const QMailThreadId& other) const;
    bool operator==(const QMailThreadId& other) const;
    bool operator<(const QMailThreadId& other) const;

    template <typename Stream> void serialize(Stream &stream) const;
    template <typename Stream> void deserialize(Stream &stream);
};


typedef QList<QMailAccountId> QMailAccountIdList;
typedef QList<QMailFolderId> QMailFolderIdList;
typedef QList<QMailMessageId> QMailMessageIdList;
typedef QList<QMailThreadId> QMailThreadIdList;

QMF_EXPORT QDebug& operator<< (QDebug& debug, const QMailAccountId &id);
QMF_EXPORT QDebug& operator<< (QDebug& debug, const QMailFolderId &id);
QMF_EXPORT QDebug& operator<< (QDebug& debug, const QMailMessageId &id);
QMF_EXPORT QDebug& operator<< (QDebug& debug, const QMailThreadId &id);

QMF_EXPORT QTextStream& operator<< (QTextStream& s, const QMailAccountId &id);
QMF_EXPORT QTextStream& operator<< (QTextStream& s, const QMailFolderId &id);
QMF_EXPORT QTextStream& operator<< (QTextStream& s, const QMailMessageId &id);
QMF_EXPORT QTextStream& operator<< (QTextStream& s, const QMailThreadId &id);

Q_DECLARE_USER_METATYPE(QMailAccountId)
Q_DECLARE_USER_METATYPE(QMailFolderId)
Q_DECLARE_USER_METATYPE(QMailMessageId)
Q_DECLARE_USER_METATYPE(QMailThreadId)

Q_DECLARE_METATYPE(QMailAccountIdList)
Q_DECLARE_METATYPE(QMailFolderIdList)
Q_DECLARE_METATYPE(QMailMessageIdList)
Q_DECLARE_METATYPE(QMailThreadIdList)

Q_DECLARE_USER_METATYPE_TYPEDEF(QMailAccountIdList, QMailAccountIdList)
Q_DECLARE_USER_METATYPE_TYPEDEF(QMailFolderIdList, QMailFolderIdList)
Q_DECLARE_USER_METATYPE_TYPEDEF(QMailMessageIdList, QMailMessageIdList)
Q_DECLARE_USER_METATYPE_TYPEDEF(QMailThreadIdList, QMailThreadIdList)

uint QMF_EXPORT qHash(const QMailAccountId &id);
uint QMF_EXPORT qHash(const QMailFolderId &id);
uint QMF_EXPORT qHash(const QMailMessageId &id);
uint QMF_EXPORT qHash(const QMailThreadId &id);

#endif

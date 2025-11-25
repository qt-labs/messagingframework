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

#ifndef QMAILIPC_H
#define QMAILIPC_H

#include <QDataStream>
#include <QVariant>

#include "qmailglobal.h"

namespace QMailIpc
{
    QMF_EXPORT bool init();
}

template <typename T>
struct QmfMetaTypeRegister
{
    static int registerType() { return 1; }
};

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#define QMF_REGISTER_STREAMABLE_TYPE(TYPE) \
    qRegisterMetaType<TYPE>( #TYPE ); \
    qRegisterMetaTypeStreamOperators<TYPE>( #TYPE );
#else
#define QMF_REGISTER_STREAMABLE_TYPE(TYPE) \
    qRegisterMetaType<TYPE>( #TYPE );
#endif

#define Q_DECLARE_USER_METATYPE(TYPE) \
    Q_DECLARE_METATYPE(TYPE) \
    QMF_EXPORT QDataStream &operator<<(QDataStream &stream, const TYPE &var); \
    QMF_EXPORT QDataStream &operator>>(QDataStream &stream, TYPE &var);

#define Q_IMPLEMENT_USER_METATYPE_NO_OPERATORS(TYPE) \
    template<> \
    struct QmfMetaTypeRegister< TYPE > \
    { \
        static int registerType() { \
            qRegisterMetaType<TYPE>(#TYPE); \
            return 1; \
        } \
        static int __init_variable__; \
    }; \
    int QmfMetaTypeRegister< TYPE >::__init_variable__ = QmfMetaTypeRegister< TYPE >::registerType();

#define Q_IMPLEMENT_USER_METATYPE_WITH_OPERATORS(TYPE) \
    template<> \
    struct QmfMetaTypeRegister< TYPE > \
    { \
        static int registerType() { \
            QMF_REGISTER_STREAMABLE_TYPE(TYPE) \
            return 1; \
        } \
        static int __init_variable__; \
    }; \
    int QmfMetaTypeRegister< TYPE >::__init_variable__ = QmfMetaTypeRegister< TYPE >::registerType();

#define Q_IMPLEMENT_USER_METATYPE(TYPE) \
    QDataStream &operator<<(QDataStream &stream, const TYPE &var) \
    { \
        var.serialize(stream); \
        return stream; \
    } \
    QDataStream &operator>>(QDataStream &stream, TYPE &var) \
    { \
        var.deserialize(stream); \
        return stream; \
    } \
    Q_IMPLEMENT_USER_METATYPE_WITH_OPERATORS(TYPE)

#define Q_IMPLEMENT_USER_METATYPE_ENUM(TYPE) \
    QDataStream& operator<<(QDataStream& stream, const TYPE &v) \
    { \
        stream << static_cast<qint32>(v); \
        return stream; \
    } \
    QDataStream& operator>>(QDataStream& stream, TYPE& v) \
    { \
        qint32 _v; \
        stream >> _v; \
        v = static_cast<TYPE>(_v); \
        return stream; \
    } \
    Q_IMPLEMENT_USER_METATYPE_WITH_OPERATORS(TYPE)

#endif //QMAILIPC_H

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

template <typename T>
struct QMetaTypeRegister
{
    static int registerType() { return 1; }
};

#ifdef Q_CC_GNU
# define _QATOMIC_ONCE() do {} while(0)
#else
# define _QATOMIC_ONCE()                \
    static QAtomicInt once;             \
    if ( once.fetchAndStoreOrdered(1) ) \
        return 1
#endif

#define Q_DECLARE_USER_METATYPE_NO_OPERATORS(TYPE) \
    Q_DECLARE_METATYPE(TYPE) \
    template<> \
    struct QMetaTypeRegister< TYPE > \
    { \
        static int registerType() \
        { \
            _QATOMIC_ONCE(); \
            int id = qMetaTypeId<TYPE>(); \
            if ( id >= static_cast<int>(QMetaType::User) ) \
                qRegisterMetaTypeStreamOperators< TYPE >( #TYPE ); \
            return 1; \
        } \
        static int __init_variable__; \
    };

#define Q_DECLARE_USER_METATYPE(TYPE) \
    Q_DECLARE_USER_METATYPE_NO_OPERATORS(TYPE) \
    QMF_EXPORT QDataStream &operator<<(QDataStream &stream, const TYPE &var); \
    QMF_EXPORT QDataStream &operator>>( QDataStream &stream, TYPE &var );

#define Q_DECLARE_USER_METATYPE_TYPEDEF(TAG,TYPE)       \
    template <typename T> \
    struct QMetaTypeRegister##TAG \
    { \
        static int registerType() { return 1; } \
    }; \
    template<> struct QMetaTypeRegister##TAG< TYPE > { \
        static int registerType() { \
            _QATOMIC_ONCE(); \
            qRegisterMetaType< TYPE >( #TYPE ); \
            qRegisterMetaTypeStreamOperators< TYPE >( #TYPE ); \
            return 1; \
        } \
        static int __init_variable__; \
    };

#define Q_DECLARE_USER_METATYPE_ENUM(TYPE)      \
    Q_DECLARE_USER_METATYPE(TYPE)

#define Q_IMPLEMENT_USER_METATYPE_NO_OPERATORS(TYPE) \
    int QMetaTypeRegister< TYPE >::__init_variable__ = \
        QMetaTypeRegister< TYPE >::registerType();

#define Q_IMPLEMENT_USER_METATYPE(TYPE) \
    QDataStream &operator<<(QDataStream &stream, const TYPE &var) \
    { \
        var.serialize(stream); \
        return stream; \
    } \
    \
    QDataStream &operator>>( QDataStream &stream, TYPE &var ) \
    { \
        var.deserialize(stream); \
        return stream; \
    } \
    Q_IMPLEMENT_USER_METATYPE_NO_OPERATORS(TYPE)

#define Q_IMPLEMENT_USER_METATYPE_TYPEDEF(TAG,TYPE)     \
    int QMetaTypeRegister##TAG< TYPE >::__init_variable__ = \
        QMetaTypeRegister##TAG< TYPE >::registerType();

#define Q_IMPLEMENT_USER_METATYPE_ENUM(TYPE)    \
    QDataStream& operator<<( QDataStream& stream, const TYPE &v ) \
    { \
        stream << static_cast<qint32>(v); \
        return stream; \
    } \
    QDataStream& operator>>( QDataStream& stream, TYPE& v ) \
    { \
        qint32 _v; \
        stream >> _v; \
        v = static_cast<TYPE>(_v); \
        return stream; \
    } \
    Q_IMPLEMENT_USER_METATYPE_NO_OPERATORS(TYPE)

#endif //QMAILIPC_H


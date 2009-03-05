#ifndef QMAILIPC_H
#define QMAILIPC_H

#ifdef QMAIL_QTOPIA

#include <QtopiaIpcAdaptor>
#include <QtopiaIpcEnvelope>
#include <qtopiaipcmarshal.h>

#else

#include "qcopadaptor.h"
#include "qcopchannel.h"
#include <QDataStream>
#include "qcopserver.h"

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
            int id = qMetaTypeId( reinterpret_cast<TYPE *>(0) ); \
            if ( id >= static_cast<int>(QMetaType::User) ) \
                qRegisterMetaTypeStreamOperators< TYPE >( #TYPE ); \
            return 1; \
        } \
        static int __init_variable__; \
    };

#define Q_DECLARE_USER_METATYPE(TYPE) \
    Q_DECLARE_USER_METATYPE_NO_OPERATORS(TYPE) \
    QTOPIAMAIL_EXPORT QDataStream &operator<<(QDataStream &stream, const TYPE &var); \
    QTOPIAMAIL_EXPORT QDataStream &operator>>( QDataStream &stream, TYPE &var );

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

#endif //QMAIL_QTOPIA
#endif //QMAILIPC_H


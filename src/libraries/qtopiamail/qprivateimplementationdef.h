/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef QPRIVATEIMPLEMENTATIONDEF_H
#define QPRIVATEIMPLEMENTATIONDEF_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt Extended API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qprivateimplementation.h"

template <typename T>
void QPrivateImplementationPointer<T>::increment(T*& p)
{
    if (p) p->ref();
}

template <typename T>
void QPrivateImplementationPointer<T>::decrement(T*& p)
{
    if (p) {
        if (p->deref())  {
            p = reinterpret_cast<T*>(~0);
        }
    }
}

template<typename ImplementationType>
QTOPIAMAIL_EXPORT QPrivatelyImplemented<ImplementationType>::QPrivatelyImplemented(ImplementationType* p)
    : d(p)
{
}

template<typename ImplementationType>
QTOPIAMAIL_EXPORT QPrivatelyImplemented<ImplementationType>::QPrivatelyImplemented(const QPrivatelyImplemented& other)
    : d(other.d)
{
}

template<typename ImplementationType>
template<typename A1>
QTOPIAMAIL_EXPORT QPrivatelyImplemented<ImplementationType>::QPrivatelyImplemented(ImplementationType* p, A1 a1)
    : d(p, a1)
{
}

template<typename ImplementationType>
QTOPIAMAIL_EXPORT QPrivatelyImplemented<ImplementationType>::~QPrivatelyImplemented()
{
}

template<typename ImplementationType>
QTOPIAMAIL_EXPORT const QPrivatelyImplemented<ImplementationType>& QPrivatelyImplemented<ImplementationType>::operator=(const QPrivatelyImplemented<ImplementationType>& other)
{
    d = other.d;
    return *this;
}


template<typename T>
QPrivateNoncopyablePointer<T>::~QPrivateNoncopyablePointer()
{
    if (d) d->delete_self();
}

template<typename ImplementationType>
QTOPIAMAIL_EXPORT QPrivatelyNoncopyable<ImplementationType>::QPrivatelyNoncopyable(ImplementationType* p)
    : d(p)
{
}

template<typename ImplementationType>
template<typename A1>
QTOPIAMAIL_EXPORT QPrivatelyNoncopyable<ImplementationType>::QPrivatelyNoncopyable(ImplementationType* p, A1 a1)
    : d(p, a1)
{
}

template<typename ImplementationType>
QTOPIAMAIL_EXPORT QPrivatelyNoncopyable<ImplementationType>::~QPrivatelyNoncopyable()
{
}

#endif

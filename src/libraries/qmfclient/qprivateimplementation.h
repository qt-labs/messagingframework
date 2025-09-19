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

#ifndef QPRIVATEIMPLEMENTATION_H
#define QPRIVATEIMPLEMENTATION_H

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

/* Rationale:

QSharedDataPointer has some deficiencies when used for implementation hiding,
which are exposed by its use in the messaging library:
1. It cannot easily be used with incomplete classes, requiring client classes
   to unnecessarily define destructors, copy constructors and assignment
   operators.
2. It is not polymorphic, and must be reused redundantly where a class with
   a hidden implementation is derived from another class with a hidden
   implementation.
3. Type-bridging functions are required to provide a supertype with access
   to the supertype data allocated within a subtype object.

The QPrivateImplementation class stores a pointer to the correct destructor
and copy-constructor, given the type of the actual object instantiated.
This allows it to be copied and deleted in contexts where the true type of
the implementation object is not recorded.

The QPrivateImplementationPointer provides QSharedDataPointer semantics,
providing the pointee type with necessary derived-type information. The
pointee type must derive from QPrivateImplementation.

The QPrivatelyImplemented<> template provides correct copy and assignment
functions, and allows the shared implementation object to be cast to the
different object types in the implementation type hierarchy.

*/

#include "qmailglobal.h"
#include <QtCore/qglobal.h>
#include <QtCore/qatomic.h>

class QPrivateImplementationBase
{
public:
    template<typename Subclass>
    inline QPrivateImplementationBase(Subclass* p)
        : ref_count(0),
          self(p),
          delete_function(&QPrivateImplementationBase::typed_delete<Subclass>),
          copy_function(&QPrivateImplementationBase::typed_copy_construct<Subclass>)
    {
    }

    inline QPrivateImplementationBase(const QPrivateImplementationBase& other)
        : ref_count(0),
          self(other.self),
          delete_function(other.delete_function),
          copy_function(other.copy_function)
    {
    }

    inline int count() const
    {
        return ref_count.loadRelaxed();
    }

    inline void ref()
    {
        ref_count.ref();
    }

    inline bool deref()
    {
        if (ref_count.deref() == 0 && delete_function && self) {
            (*delete_function)(self);
            return true;
        } else {
            return false;
        }
    }

    inline void* clone()
    {
        void* copy = (*copy_function)(self);
        reinterpret_cast<QPrivateImplementationBase*>(copy)->self = copy;
        return copy;
    }

private:
    QAtomicInt ref_count;

    void *self;
    void (*delete_function)(void *p);
    void *(*copy_function)(const void *p);

    template<class T>
    static inline void typed_delete(void *p)
    {
        delete reinterpret_cast<T*>(p);
    }

    template<class T>
    static inline void* typed_copy_construct(const void *p)
    {
        return new T(*reinterpret_cast<const T*>(p));
    }

    // using the assignment operator would lead to corruption in the ref-counting
    QPrivateImplementationBase &operator=(const QPrivateImplementationBase &);
};

template <class T> class QPrivateImplementationPointer
{
public:
    typedef T Type;
    typedef T *pointer;

    inline void detach() { if (d && d->count() != 1) detach_helper(); }
    inline T &operator*() { detach(); return *d; }
    inline const T &operator*() const { return *d; }
    inline T *operator->() { detach(); return d; }
    inline const T *operator->() const { return d; }
    inline operator T *() { detach(); return d; }
    inline operator const T *() const { return d; }
    inline T *data() { detach(); return d; }
    inline const T *data() const { return d; }
    inline const T *constData() const { return d; }

    inline bool operator==(const QPrivateImplementationPointer<T> &other) const { return d == other.d; }
    inline bool operator!=(const QPrivateImplementationPointer<T> &other) const { return d != other.d; }

    inline QPrivateImplementationPointer() { d = 0; }
    ~QPrivateImplementationPointer();

    explicit QPrivateImplementationPointer(T *data);
    QPrivateImplementationPointer(const QPrivateImplementationPointer<T> &o);
    inline QPrivateImplementationPointer<T> & operator=(const QPrivateImplementationPointer<T> &o) {
        if (o.d != d) {
            if (o.d)
                o.d->ref();
            T *old = d;
            d = o.d;
            if (old)
                old->deref();
        }
        return *this;
    }
    inline QPrivateImplementationPointer &operator=(T *o) {
        if (o != d) {
            if (o)
                o->ref();
            T *old = d;
            d = o;
            if (old)
                old->deref();
        }
        return *this;
    }

    inline bool operator!() const { return !d; }

    inline void swap(QPrivateImplementationPointer &other)
    { qSwap(d, other.d); }

protected:
    T *clone();

private:
    void detach_helper();

    T *d;
};

namespace std {
    template <class T>
    Q_INLINE_TEMPLATE void swap(QPrivateImplementationPointer<T> &p1, QPrivateImplementationPointer<T> &p2)
    { p1.swap(p2); }
}

template <class T>
Q_INLINE_TEMPLATE QPrivateImplementationPointer<T>::QPrivateImplementationPointer(T *adata) : d(adata)
{ if (d) d->ref(); }

template <class T>
Q_INLINE_TEMPLATE T *QPrivateImplementationPointer<T>::clone()
{
    return reinterpret_cast<T *>(d->clone());
}

template <class T>
Q_OUTOFLINE_TEMPLATE void QPrivateImplementationPointer<T>::detach_helper()
{
    T *x = clone();
    x->ref();
    d->deref();
    d = x;
}

template <class T>
Q_INLINE_TEMPLATE void qSwap(QPrivateImplementationPointer<T> &p1, QPrivateImplementationPointer<T> &p2)
{ p1.swap(p2); }

QT_BEGIN_NAMESPACE

template<typename T> Q_DECLARE_TYPEINFO_BODY(QPrivateImplementationPointer<T>, Q_MOVABLE_TYPE);

QT_END_NAMESPACE

template<typename ImplementationType>
class QMF_EXPORT QPrivatelyImplemented
{
public:
    QPrivatelyImplemented(ImplementationType* p);
    QPrivatelyImplemented(const QPrivatelyImplemented& other);

    virtual ~QPrivatelyImplemented();

    const QPrivatelyImplemented<ImplementationType>& operator=(const QPrivatelyImplemented<ImplementationType>& other);

    template<typename ImplementationSubclass>
    inline ImplementationSubclass* impl()
    {
        return static_cast<ImplementationSubclass*>(static_cast<ImplementationType*>(d));
    }

    template<typename InterfaceType>
    inline typename InterfaceType::ImplementationType* impl(InterfaceType*)
    {
        return impl<typename InterfaceType::ImplementationType>();
    }

    template<typename ImplementationSubclass>
    inline const ImplementationSubclass* impl() const
    {
        return static_cast<const ImplementationSubclass*>(static_cast<const ImplementationType*>(d));
    }

    template<typename InterfaceType>
    inline const typename InterfaceType::ImplementationType* impl(const InterfaceType*) const
    {
        return impl<const typename InterfaceType::ImplementationType>();
    }

protected:
    QPrivateImplementationPointer<ImplementationType> d;
};


class QPrivateNoncopyableBase
{
public:
    template<typename Subclass>
    inline QPrivateNoncopyableBase(Subclass* p)
        : self(p),
          delete_function(&QPrivateNoncopyableBase::typed_delete<Subclass>)
    {
    }

    inline void delete_self()
    {
        if (delete_function && self) {
            (*delete_function)(self);
        }
    }

private:
    void *self;
    void (*delete_function)(void *p);

    template<class T>
    static inline void typed_delete(void *p)
    {
        delete static_cast<T*>(p);
    }

    // do not permit copying
    QPrivateNoncopyableBase(const QPrivateNoncopyableBase &);

    QPrivateNoncopyableBase &operator=(const QPrivateNoncopyableBase &);
};

template <class T> class QPrivateNoncopyablePointer
{
public:
    inline T &operator*() { return *d; }
    inline const T &operator*() const { return *d; }

    inline T *operator->() { return d; }
    inline const T *operator->() const { return d; }

    inline operator T *() { return d; }
    inline operator const T *() const { return d; }

    inline T *data() { return d; }
    inline const T *data() const { return d; }

    inline const T *constData() const { return d; }

    inline bool operator==(const QPrivateNoncopyablePointer<T> &other) const { return d == other.d; }
    inline bool operator!=(const QPrivateNoncopyablePointer<T> &other) const { return d != other.d; }

    inline QPrivateNoncopyablePointer()
        : d(0)
    {
    }

    inline explicit QPrivateNoncopyablePointer(T *p)
        : d(p)
    {
    }

    template<typename U>
    inline explicit QPrivateNoncopyablePointer(U *p)
        : d(static_cast<T*>(p))
    {
    }

    ~QPrivateNoncopyablePointer();

    inline bool operator!() const { return !d; }

private:
    inline QPrivateNoncopyablePointer<T> &operator=(T *) { return *this; }

    inline QPrivateNoncopyablePointer<T> &operator=(const QPrivateNoncopyablePointer<T> &) { return *this; }

public:
    T *d;
};

template<typename ImplementationType>
class QMF_EXPORT QPrivatelyNoncopyable
{
public:
    QPrivatelyNoncopyable(ImplementationType* p);

    template<typename A1>
    QMF_EXPORT QPrivatelyNoncopyable(ImplementationType* p, A1 a1);

    virtual ~QPrivatelyNoncopyable();

    template<typename ImplementationSubclass>
    inline ImplementationSubclass* impl()
    {
        return static_cast<ImplementationSubclass*>(static_cast<ImplementationType*>(d));
    }

    template<typename InterfaceType>
    inline typename InterfaceType::ImplementationType* impl(InterfaceType*)
    {
        return impl<typename InterfaceType::ImplementationType>();
    }

    template<typename ImplementationSubclass>
    inline const ImplementationSubclass* impl() const
    {
        return static_cast<const ImplementationSubclass*>(static_cast<const ImplementationType*>(d));
    }

    template<typename InterfaceType>
    inline const typename InterfaceType::ImplementationType* impl(const InterfaceType*) const
    {
        return impl<const typename InterfaceType::ImplementationType>();
    }

protected:
    QPrivateNoncopyablePointer<ImplementationType> d;
};

#endif

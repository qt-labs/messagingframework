/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#ifndef QMF_QMFLIST_H
#define QMF_QMFLIST_H

#include <algorithm>
#include <list>
#include <QList>

/*
 * QMF made use of stable-reference semantics of QList in Qt5.
 * In Qt6, non-const operations will cause reference and
 * iterator invalidation, and thus QList cannot be used in
 * those cases.
 *
 * This class provides some QList-like API sugar (at the cost
 * of O(N) performance for the sugar operations) around a
 * std::list (which provides reference-stability).
 */

template<class T>
class QmfList : public std::list<T>
{
public:
    ~QmfList() {}
    QmfList() : std::list<T>() {}
    QmfList(const QmfList<T> &other) : std::list<T>(other) {}
    QmfList(std::initializer_list<T> t) : std::list<T>(t) {}
    template <typename InputIterator> QmfList(InputIterator start, InputIterator end) : std::list<T>(start, end) {}
    QmfList(const std::list<T> &stdlist) : std::list<T>(stdlist) {}
    QmfList(const QList<T> &qlist) : std::list<T>() { for (const T &t : qlist) this->push_back(t); }

    QmfList<T> &operator=(const QmfList<T> &other) { this->clear(); for (const T &t : other) this->push_back(t); return *this; }
    QmfList<T> &operator+(const QmfList<T> &other) { this->append(other); return *this; }

    qsizetype count() const { return this->size(); }
    bool isEmpty() const { return this->count() == 0; };
    void append(const T &t) { this->push_back(t); }
    void append(const QmfList<T> &list) { for (const T &t : list) this->push_back(t); }
    const T& last() const { return this->back(); }
    const T& first() const { return this->front(); }
    T takeFirst() { T t = this->front(); this->pop_front(); return t; }
    void removeAt(qsizetype index) { this->erase(std::next(this->begin(), index)); }
    void removeOne(const T &t) { auto it = std::find(this->begin(), this->end(), t); if (it != this->end()) this->erase(it); }
    void removeAll(const T &t) { this->remove(t); }
    const T& at(qsizetype index) const { return *std::next(this->cbegin(), index); }
    T& operator[](qsizetype index) { return *std::next(this->begin(), index); }
    const T& operator[](qsizetype index) const { return *std::next(this->cbegin(), index); }
    bool contains(const T &t) const { return std::find(this->cbegin(), this->cend(), t) != this->cend(); }
    qsizetype indexOf(const T &t) const { qsizetype i = 0; for (const T &v : *this) { if (t == v) return i; else i++; } return -1; }

    QmfList<T>& operator<<(const T &t) { this->append(t); return *this; }

    QList<T> toQList() const { return QList<T>(this->cbegin(), this->cend()); }
    static QmfList<T> fromQList(const QList<T> &list) { return QmfList<T>(list.constBegin(), list.constEnd()); }
};

template<typename T>
QmfList<T> operator+(const QmfList<T> &first, const QmfList<T> &second)
{
    QmfList<T> ret = first;
    ret.append(second);
    return ret;
}

template <typename T>
QDataStream &operator<<(QDataStream &out, const QmfList<T> &list)
{
    out << list.toQList();
    return out;
}
template <typename T>
QDataStream &operator>>(QDataStream &in, QmfList<T> &list)
{
    QList<T> qlist;
    in >> qlist;
    list = QmfList<T>::fromQList(qlist);
    return in;
}

#endif // QMF_QMFLIST_H


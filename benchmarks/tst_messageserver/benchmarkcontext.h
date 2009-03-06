/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef BENCHMARKCONTEXT_H
#define BENCHMARKCONTEXT_H

#include <QtGlobal>

class BenchmarkContextPrivate;
class BenchmarkContext
{
public:
    BenchmarkContext(bool xml = false);
    ~BenchmarkContext();

private:
    BenchmarkContextPrivate* d;
};

#endif


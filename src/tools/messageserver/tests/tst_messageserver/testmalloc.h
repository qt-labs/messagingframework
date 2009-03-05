/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef TESTMALLOC_H
#define TESTMALLOC_H

class TestMalloc
{
public:
    static int peakUsable();
    static int peakTotal();
    static int nowUsable();
    static int nowTotal();
    static void resetPeak();
    static void resetNow();
};

#endif


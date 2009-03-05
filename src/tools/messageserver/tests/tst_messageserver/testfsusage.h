/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef TESTFSUSAGE_H
#define TESTFSUSAGE_H

#include <QString>

class TestFsUsage
{
public:
    static qint64 usage(QString const&);
};

#endif


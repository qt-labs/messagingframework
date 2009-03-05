/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#include "testfsusage.h"

#include <QDir>
#include <QFileInfo>

qint64 TestFsUsage::usage(QString const& path)
{
    qint64 ret = 0;
    QFileInfo fi(path);
    ret += fi.size();
    if (!fi.isSymLink() && fi.isDir()) {
        QDir dir(fi.absoluteFilePath());
        foreach (QString const& name, dir.entryList(QDir::NoDotAndDotDot|QDir::AllEntries)) {
            ret += usage(path + "/" + name);
        }
    }
    return ret;
}


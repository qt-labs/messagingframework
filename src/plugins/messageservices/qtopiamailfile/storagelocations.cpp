/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#include <QCoreApplication>
#include <QPair>
#include <QString>

QList<QPair<QString, QString> > storageLocations()
{
    QList<QPair<QString, QString> > locations;
    locations.append(qMakePair(QString("qtopiamailfile"),QString("")));
    return locations;
}


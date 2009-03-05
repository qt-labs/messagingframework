/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef IMAPSTRUCTURE_H
#define IMAPSTRUCTURE_H

#include <qstring.h>
#include <qstringlist.h>

class QMailMessagePartContainer;
class QString;
class QStringList;


void setMessageContentFromStructure(const QStringList &structure, QMailMessagePartContainer *container);

QStringList getMessageStructure(const QString &field);

#endif

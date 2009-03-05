/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef FOLDERMODEL_H
#define FOLDERMODEL_H

#include <QMailMessageSetModel>

class FolderModel : public QMailMessageSetModel
{
    Q_OBJECT

public:
    explicit FolderModel(QObject* parent = 0);
    virtual ~FolderModel();

    virtual QVariant data(QMailMessageSet* item, int role, int column) const;
};

#endif

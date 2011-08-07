/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef EMAILFOLDERVIEW_H
#define EMAILFOLDERVIEW_H

#include "emailfoldermodel.h"
#include "folderdelegate.h"
#include "folderview.h"

class EmailFolderDelegate;

class QMFUTIL_EXPORT EmailFolderView : public FolderView
{
    Q_OBJECT

public:
    EmailFolderView(QWidget *parent);

    virtual EmailFolderModel *model() const;
    void setModel(EmailFolderModel *model);

private:
    virtual void setModel(QAbstractItemModel *model);

    EmailFolderModel *mModel;
    EmailFolderDelegate* mDelegate;
};


class EmailFolderDelegate : public FolderDelegate
{
    Q_OBJECT

public:
    EmailFolderDelegate(EmailFolderView *parent = 0);

    virtual void drawDisplay(QPainter *painter, const QStyleOptionViewItem &option, const QRect &rect, const QString &text) const;
    virtual void drawDecoration(QPainter *painter, const QStyleOptionViewItem &option, const QRect &rect, const QVariant &decoration) const;

private:
    virtual void init(const QStyleOptionViewItem &option, const QModelIndex &index);

    bool _unsynchronized;
};

#endif

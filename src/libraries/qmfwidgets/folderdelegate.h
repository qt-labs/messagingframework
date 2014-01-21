/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef FOLDERDELEGATE_H
#define FOLDERDELEGATE_H

#include "foldermodel.h"
#include <QItemDelegate>

QT_BEGIN_NAMESPACE

class QAbstractItemView;
class QScrollBar;

QT_END_NAMESPACE

class FolderDelegate : public QItemDelegate
{
    Q_OBJECT

public:
    FolderDelegate(QAbstractItemView *parent = 0);
    FolderDelegate(QWidget *parent);

    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    virtual void drawDisplay(QPainter *painter, const QStyleOptionViewItem &option, const QRect &rect, const QString &text) const;
    virtual void drawDecoration(QPainter *painter, const QStyleOptionViewItem &option, const QRect &rect, const QVariant &decoration) const;

    virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;

    bool showStatus() const;
    void setShowStatus(bool val);

protected:
    virtual void init(const QStyleOptionViewItem &option, const QModelIndex &index);

    QWidget *_parent;
    QScrollBar *_scrollBar;
    QString _statusText;
    bool m_showStatus;
};

#endif

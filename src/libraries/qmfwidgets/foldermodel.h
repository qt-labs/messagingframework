/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#ifndef FOLDERMODEL_H
#define FOLDERMODEL_H

#include <qtmailnamespace.h>
#include <qmailmessageset.h>
#include <QIcon>
#include <QPair>
#include <QString>

class QMFUTIL_EXPORT FolderModel : public QMailMessageSetModel
{
    Q_OBJECT

public:
    enum Roles
    {
        FolderIconRole = Qt::DecorationRole,
        FolderStatusRole = QMailMessageSetModel::SubclassUserRole,
        FolderStatusDetailRole,
        FolderIdRole
    };

    FolderModel(QObject *parent = Q_NULLPTR);
    ~FolderModel();

    static QString excessIndicator();

    using QMailMessageSetModel::data;
    virtual QVariant data(QMailMessageSet *item, int role, int column) const;

protected Q_SLOTS:
    void processUpdatedItems();

protected:
    typedef QPair<QString, QString> StatusText;

    void scheduleUpdate(QMailMessageSet *item);

    virtual void appended(QMailMessageSet *item);
    virtual void updated(QMailMessageSet *item);
    virtual void removed(QMailMessageSet *item);

    virtual QIcon itemIcon(QMailMessageSet *item) const;
    virtual QString itemStatus(QMailMessageSet *item) const;
    virtual QString itemStatusDetail(QMailMessageSet *item) const;

    virtual StatusText itemStatusText(QMailMessageSet *item) const;
    virtual StatusText folderStatusText(QMailFolderMessageSet *item) const;
    virtual StatusText accountStatusText(QMailAccountMessageSet *item) const;
    virtual StatusText filterStatusText(QMailFilterMessageSet *item) const;

    enum SubTotalType { Unread, New, Unsent };

    static QMailMessageKey unreadKey();

    static QString describeFolderCount(int total, int subTotal, SubTotalType type = Unread);
    static QString formatCounts(int total, int unread, bool excessTotal = false, bool excessUnread = false);

    QMap<QMailMessageSet*, StatusText> statusMap;

    QList<QMailMessageSet*> updatedItems;
};


#endif


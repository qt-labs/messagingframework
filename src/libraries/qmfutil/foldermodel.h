/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Qt Software Information (qt-info@nokia.com)
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the either Technology Preview License Agreement or the
** Beta Release License Agreement.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef FOLDERMODEL_H
#define FOLDERMODEL_H

#include <QIcon>
#include <qmailmessageset.h>
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

    FolderModel(QObject *parent = 0);
    ~FolderModel();

    static QString excessIndicator();

    using QMailMessageSetModel::data;
    virtual QVariant data(QMailMessageSet *item, int role, int column) const;

protected slots:
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


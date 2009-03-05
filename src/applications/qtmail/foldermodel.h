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

#include <QIcon>
#include <qmailmessageset.h>
#include <QPair>
#include <QString>

class FolderModel : public QMailMessageSetModel
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

    static QString describeFolderCount(int total, int subTotal, SubTotalType type = Unread);

    QMap<QMailMessageSet*, StatusText> statusMap;

    QList<QMailMessageSet*> updatedItems;
};


#endif


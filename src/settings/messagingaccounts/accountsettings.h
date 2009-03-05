/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef ACCOUNTSETTINGS_H
#define ACCOUNTSETTINGS_H

#include <qmailid.h>
#include <qmailserviceaction.h>
#include <QMap>
#include <QModelIndex>
#include <QDialog>

class StatusDisplay;
class QMailAccount;
class QMenu;
class QAction;
class QSmoothList;
class QMailAccountListModel;
class QListView;

class AccountSettings : public QDialog
{
    Q_OBJECT
public:
    AccountSettings(QWidget *parent = 0, Qt::WFlags flags = 0);

signals:
    void deleteAccount(const QMailAccountId &id);

public slots:
    void addAccount();

protected:
    void showEvent(QShowEvent* e);
    void hideEvent(QHideEvent* e);

private slots:
    void removeAccount();
    void accountSelected(QModelIndex index);
    void updateActions();
    void displayProgress(uint, uint);
    void activityChanged(QMailServiceAction::Activity activity);
    void retrieveFolders();
    void deleteMessages();

private:
    void editAccount(QMailAccount *account);

private:
    QMap<int,int> listToAccountIdx;
    QMailAccountListModel *accountModel;
    QListView* accountView;
    QMenu *context;
    QAction *addAccountAction;
    QAction *removeAccountAction;
    StatusDisplay *statusDisplay;
    QPoint cPos;
    bool preExisting;
    QMailRetrievalAction *retrievalAction;
    QMailAccountId deleteAccountId;
    QMailMessageIdList deleteMessageIds;
    int deleteBatchSize;
    int deleteProgress;
};

#endif

/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef MESSAGENAVIGATOR_H
#define MESSAGENAVIGATOR_H
#include "ui_messagenavigatorbase.h"

class QMailMessageId;

class MessageNavigator : public QWidget, public Ui_MessageNavigatorBase
{
    Q_OBJECT
public:
    MessageNavigator( QWidget *parent = 0, Qt::WFlags f = 0 );
    ~MessageNavigator();

private slots:
    void showFolderTree();
    void showMessageList();

    void viewMessage(const QMailMessageId& id);

private:
    QWidget* folderSelector;
    QWidget* messageSelector;
};

#endif

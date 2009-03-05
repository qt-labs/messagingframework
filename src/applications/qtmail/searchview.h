/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef SEARCHVIEW_H
#define SEARCHVIEW_H

#include "ui_searchviewbasephone.h"
#include <QDialog>
#include <qmailmessagekey.h>

class QScrollArea;
class QAction;

class SearchView : public QDialog, public Ui::SearchViewBase
{
    Q_OBJECT

public:
    SearchView(QWidget* parent = 0);
    ~SearchView();

    QMailMessageKey searchKey() const;
    QString bodyText() const;

    void reset();

private:
    void init();

private slots:
    void updateActions();
    void pickAddressSlot();

private:
    QDate dateBefore, dateAfter;
    QScrollArea *sv;
    QAction *pickAddressAction;
};

#endif

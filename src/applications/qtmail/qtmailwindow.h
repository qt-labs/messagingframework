/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef QTMAILWINDOW_H
#define QTMAILWINDOW_H

#include <qevent.h>
#include <qlist.h>
#include <QMainWindow>

class MailListView;
class WriteMail;
class StatusDisplay;
class QStackedWidget;
class EmailClient;
class QMenu;
class QToolBar;

class QTMailWindow : public QMainWindow
{
    Q_OBJECT

public:
    QTMailWindow(QWidget *parent = 0, Qt::WindowFlags f = 0);
    ~QTMailWindow();

    static QTMailWindow *singleton();

    void forceHidden(bool hidden);
    void setVisible(bool visible);

    QWidget* currentWidget() const;
    QMenu* contextMenu() const;
    QToolBar* toolBar() const;

public slots:
    void closeEvent(QCloseEvent *e);

protected:
    void init();

    EmailClient *emailClient;
    QStackedWidget *views;
    StatusDisplay *status;
    bool noShow;

    static QTMailWindow *self; //singleton

private:
    QMenu* m_contextMenu;
    QToolBar* m_toolBar;
};

#endif

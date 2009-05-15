/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#include "qtmailwindow.h"
#include "statusdisplay_p.h"
#include <qdatetime.h>
#include <qtimer.h>
#include <QDebug>
#include <QStackedWidget>
#include <qmailaccount.h>
#include <QVBoxLayout>
#include "emailclient.h"
#include <qmaillog.h>
#include <QStatusBar>
#include <QMenuBar>
#include <QApplication>
#include <QToolBar>

QTMailWindow *QTMailWindow::self = 0;

QTMailWindow::QTMailWindow(QWidget *parent, Qt::WindowFlags f)
    : QMainWindow(parent, f), noShow(false), m_contextMenu(0)
{
    qMailLog(Messaging) << "QTMailWindow ctor begin";
    init();
}

void QTMailWindow::init()
{
    self = this;

    views = new QStackedWidget();
    views->setFrameStyle(QFrame::NoFrame);
    setCentralWidget(views);

    QStatusBar* mainStatusBar = new QStatusBar();
    status = new StatusDisplay;
    mainStatusBar->addPermanentWidget(status,0);
    setStatusBar(status);

    QMenuBar* mainMenuBar = new QMenuBar();
    setMenuBar(mainMenuBar);
    QMenu* file = mainMenuBar->addMenu("File");
    QMenu* help = mainMenuBar->addMenu("Help");
    QAction* aboutQt = help->addAction("About Qt");
    connect(aboutQt,SIGNAL(triggered()),qApp,SLOT(aboutQt()));

    m_contextMenu = file;

    m_toolBar = new QToolBar(this);
    addToolBar(m_toolBar);


    // Add the email client to our central widget stack
    emailClient = new EmailClient(views);

    // Connect the email client as the primary interface widget
    connect(emailClient, SIGNAL(statusVisible(bool)),
            status, SLOT(showStatus(bool)) );
    connect(emailClient, SIGNAL(updateStatus(QString)),
            status, SLOT(displayStatus(QString)) );
    connect(emailClient, SIGNAL(updateProgress(uint,uint)),
            status, SLOT(displayProgress(uint,uint)) );
    connect(emailClient, SIGNAL(clearStatus()),
            status, SLOT(clearStatus()) );

    views->addWidget(emailClient);
    views->setCurrentWidget(emailClient);


    setWindowTitle( emailClient->windowTitle() );

}

QTMailWindow::~QTMailWindow()
{
    qMailLog(Messaging) << "QTMailWindow dtor end";
}

void QTMailWindow::closeEvent(QCloseEvent *e)
{
    if (emailClient->closeImmediately()) {
        emailClient->quit();
    }
    e->ignore();
}

void QTMailWindow::forceHidden(bool hidden)
{
    noShow = hidden;
}

void QTMailWindow::setVisible(bool visible)
{
    if (noShow && visible)
        return;

    QWidget::setVisible(visible);
}

QWidget* QTMailWindow::currentWidget() const
{
    return views->currentWidget();
}

QMenu* QTMailWindow::contextMenu() const
{
    return m_contextMenu;
}

QToolBar* QTMailWindow::toolBar() const
{
    return m_toolBar;
}

QTMailWindow* QTMailWindow::singleton()
{
    return self;
}



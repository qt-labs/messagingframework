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
#include <QDesktopWidget>

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

    QPoint p(0, 0);
    int extraw = 0, extrah = 0, scrn = 0;
    QWidget* w = 0;
    if (w)
        w = w->window();
    QRect desk;
    if (w) {
        scrn = QApplication::desktop()->screenNumber(w);
    } else if (QApplication::desktop()->isVirtualDesktop()) {
        scrn = QApplication::desktop()->screenNumber(QCursor::pos());
    } else {
        scrn = QApplication::desktop()->screenNumber(this);
    }
    desk = QApplication::desktop()->availableGeometry(scrn);

    QWidgetList list = QApplication::topLevelWidgets();
    for (int i = 0; (extraw == 0 || extrah == 0) && i < list.size(); ++i) {
        QWidget * current = list.at(i);
        if (current->isVisible()) {
            int framew = current->geometry().x() - current->x();
            int frameh = current->geometry().y() - current->y();

            extraw = qMax(extraw, framew);
            extrah = qMax(extrah, frameh);
        }
    }

    // sanity check for decoration frames. With embedding, we
    // might get extraordinary values
    if (extraw == 0 || extrah == 0 || extraw >= 10 || extrah >= 40) {
        extrah = 40;
        extraw = 10;
    }

    p = QPoint(desk.x() + desk.width()/2, desk.y() + desk.height()/2);

    // p = origin of this
    p = QPoint(p.x()-width()/2 - extraw,
                p.y()-height()/2 - extrah);


    if (p.x() + extraw + width() > desk.x() + desk.width())
        p.setX(desk.x() + desk.width() - width() - extraw);
    if (p.x() < desk.x())
        p.setX(desk.x());

    if (p.y() + extrah + height() > desk.y() + desk.height())
        p.setY(desk.y() + desk.height() - height() - extrah);
    if (p.y() < desk.y())
        p.setY(desk.y());

    move(p);
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



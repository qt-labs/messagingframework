/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#include "statusdisplay_p.h"
#include <QApplication>
#include <QProgressBar>

StatusDisplay::StatusDisplay(QWidget* parent)
    : QStatusBar(parent), suppressed(false)
{
    m_progressBar = new QProgressBar(this);
    m_progressBar->setMaximumWidth(150);
    addPermanentWidget(m_progressBar);
    m_progressBar->hide();
}

void StatusDisplay::showStatus(bool visible)
{
    suppressed = !visible;
    if(!visible) clearMessage();
}

void StatusDisplay::displayStatus(const QString& txt)
{
    showMessage(txt);
}

void StatusDisplay::displayProgress(uint value, uint range)
{
    if (range == 0) {
        m_progressBar->hide();
    } else {
        m_progressBar->setVisible(true);

        if (static_cast<int>(range) != m_progressBar->maximum())
            m_progressBar->setRange(0, range);

        m_progressBar->setValue(qMin(value, range));
    }
}

void StatusDisplay::clearStatus()
{
    m_progressBar->reset();
    clearMessage(); 

    if (suppressed)
        m_progressBar->setVisible(false);
}


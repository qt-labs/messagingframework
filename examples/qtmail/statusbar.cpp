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

#include <statusbar.h>
#include <QProgressBar>
#include <QLabel>
#include <QToolButton>
#include <QHBoxLayout>
#include <QIcon>
#include <qtmailnamespace.h>

class ArrowButton : public QToolButton
{
    Q_OBJECT

public:
    ArrowButton(QWidget* parent = Q_NULLPTR);
    void reset();
    bool upState() const;

private slots:
    void thisClicked();

signals:
    void upClicked();
    void downClicked();

private:
    bool up;
    static QIcon upIcon(){static QIcon i(Qtmail::icon("uparrow")); return i;}
    static QIcon downIcon(){static QIcon i(Qtmail::icon("downarrow")); return i;}
};

ArrowButton::ArrowButton(QWidget* parent)
:
QToolButton(parent),
up(true)
{
    connect(this,SIGNAL(clicked()),this,SLOT(thisClicked()));
    setIcon(upIcon());
}

void ArrowButton::reset()
{
    up = true;
    setIcon(upIcon());
}

bool ArrowButton::upState() const
{
    return up;
}

void ArrowButton::thisClicked()
{
    if(up)
        emit upClicked();
    else
        emit downClicked();

    up=!up;
    setIcon(up ? upIcon() : downIcon());
}

StatusBar::StatusBar(QWidget* parent)
:
QWidget(parent),
m_showDetailsButton(true)
{
    init();
    clearProgress();
}

void StatusBar::setProgress(unsigned int min, unsigned int max)
{
    if(min == max)
    {
        clearProgress();
        return;
    }
    m_progressBar->show();
    m_detailsButton->setVisible(m_showDetailsButton);

    m_progressBar->setRange(0,max);
    m_progressBar->setValue(min);
    if(m_detailsButton->isVisible() && !m_detailsButton->upState())
        emit showDetails();
}

void StatusBar::clearProgress()
{
    m_progressBar->hide();
    m_detailsButton->hide();
    emit hideDetails();
}

void StatusBar::setStatus(const QString& msg)
{
    m_statusLabel->setText(msg);
}

void StatusBar::clearStatus()
{
    m_statusLabel->clear();
}

void StatusBar::setDetailsButtonVisible(bool val)
{
    m_detailsButton->setVisible(val);
    m_showDetailsButton = val;
}

void StatusBar::init()
{
    QHBoxLayout* l = new QHBoxLayout(this);
    l->setContentsMargins(0,0,0,0);
    l->setSpacing(0);

    m_statusLabel = new QLabel(this);
    l->addWidget(m_statusLabel);

    l->addStretch();

    m_detailsButton = new ArrowButton(this);
    connect(m_detailsButton,SIGNAL(upClicked()),this,SIGNAL(showDetails()));
    connect(m_detailsButton,SIGNAL(downClicked()),this,SIGNAL(hideDetails()));
    l->addWidget(m_detailsButton);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Expanding);

    l->addWidget(m_progressBar);

}

#include <statusbar.moc>

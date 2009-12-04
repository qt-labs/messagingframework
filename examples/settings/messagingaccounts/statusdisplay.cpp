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

#include "statusdisplay.h"
#include <QApplication>

//#include <QPhoneStyle>
/*
class StatusProgressBarStyle : public QPhoneStyle
{
public:
    void drawControl( ControlElement ce, const QStyleOption *opt, QPainter *p, const QWidget *widget ) const
    {
        if (ce != CE_ProgressBarGroove)
            QPhoneStyle::drawControl(ce, opt, p, widget);
    }
};
*/

StatusProgressBar::StatusProgressBar( QWidget* parent ) :
    QProgressBar(parent), txtchanged(false)
{
    QPalette p(palette());
    p.setBrush(QPalette::Base,p.brush(QPalette::Window));
    p.setBrush(QPalette::HighlightedText,p.brush(QPalette::WindowText));
    setPalette(p);
    setAlignment(Qt::AlignHCenter);
    //setStyle(new StatusProgressBarStyle);
}

StatusProgressBar::~StatusProgressBar()
{
    //delete style();
}

QSize StatusProgressBar::sizeHint() const
{
    return QProgressBar::sizeHint()-QSize(0,8);
}

void StatusProgressBar::setText(const QString& s)
{
    if ( txt != s ) {
        txt = s;
        txtchanged = true;

        if ( value() == maximum() )
            reset();

        update();
    }
}

QString StatusProgressBar::text() const
{
    static const Qt::TextElideMode mode(QApplication::isRightToLeft() ? Qt::ElideLeft : Qt::ElideRight);
    static QString last;

    if (txtchanged) {
        QFontMetrics fm(font());
        last = fm.elidedText(txt, mode, width());
        txtchanged = false;
    } 
    return last;
}


StatusDisplay::StatusDisplay(QWidget* parent)
    : StatusProgressBar(parent), suppressed(false)
{
}

void StatusDisplay::showStatus(bool visible)
{
    suppressed = !visible;
    setVisible(visible);
}

void StatusDisplay::displayStatus(const QString& txt)
{
    setVisible(true);
    setText(txt);
}

void StatusDisplay::displayProgress(uint value, uint range)
{
    if (range == 0) {
        reset();
    } else {
        setVisible(true);

        if (static_cast<int>(range) != maximum())
            setRange(0, range);

        setValue(qMin(value, range));
    }
}

void StatusDisplay::clearStatus()
{
    reset();
    setText(QString());

    if (suppressed)
        setVisible(false);
}


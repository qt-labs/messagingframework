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

#ifndef QMAILMESSAGEDELEGATE_H
#define QMAILMESSAGEDELEGATE_H

#include <QtopiaItemDelegate>
#include <qtopiaglobal.h>

class QMailMessageDelegatePrivate;

class QMFUTIL_EXPORT QMailMessageDelegate : public QtopiaItemDelegate
{
public:
    enum DisplayMode
    {
        QtmailMode,
        AddressbookMode
    };

    QMailMessageDelegate(DisplayMode mode, QWidget* parent);
    virtual ~QMailMessageDelegate();

    DisplayMode displayMode() const;
    void setDisplayMode(DisplayMode mode);

    bool displaySelectionState() const;
    void setDisplaySelectionState(bool set);

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const;

private:
    QMailMessageDelegatePrivate* d;
};

#ifdef QTOPIA_HOMEUI

class QtopiaHomeMailMessageDelegatePrivate;

class QMFUTIL_EXPORT QtopiaHomeMailMessageDelegate : public QtopiaItemDelegate
{
    Q_OBJECT
public:
    enum DisplayMode
    {
        QtmailMode,
        QtmailUnifiedMode,
        AddressbookMode
    };

    QtopiaHomeMailMessageDelegate(DisplayMode mode, QWidget* parent);
    virtual ~QtopiaHomeMailMessageDelegate();

    DisplayMode displayMode() const;
    void setDisplayMode(DisplayMode mode);

    bool displaySelectionState() const;
    void setDisplaySelectionState(bool set);

    QFont titleFont(const QStyleOptionViewItem &option) const;

    QRect replyButtonRect(const QRect &rect) const;

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const;

private:
    QtopiaHomeMailMessageDelegatePrivate* d;
};

#endif // QTOPIA_HOMEUI

#endif

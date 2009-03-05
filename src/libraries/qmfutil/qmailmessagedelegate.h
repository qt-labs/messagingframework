/****************************************************************************
**
** This file is part of the $PACKAGE_NAME$.
**
** Copyright (C) $THISYEAR$ $COMPANY_NAME$.
**
** $QT_EXTENDED_DUAL_LICENSE$
**
****************************************************************************/

#ifndef QMAILMESSAGEDELEGATE_H
#define QMAILMESSAGEDELEGATE_H

#include <QtopiaItemDelegate>
#include <qtopiaglobal.h>

class QMailMessageDelegatePrivate;

class QTOPIAMAIL_EXPORT QMailMessageDelegate : public QtopiaItemDelegate
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

class QTOPIAMAIL_EXPORT QtopiaHomeMailMessageDelegate : public QtopiaItemDelegate
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

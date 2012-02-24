/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qloggers.h"

BaseLoggerFoundation::BaseLoggerFoundation(const LogLevel min_lvl_)
                     : min_lvl(min_lvl_), is_ready(true)
{
};

void BaseLoggerFoundation::setMinLogLvl(const LogLevel _min_lvl)
{
    min_lvl = _min_lvl;
};

bool BaseLoggerFoundation::isReady(QString& _err) const
{
    if(!is_ready) _err = err_msg;
    return is_ready;
};

void BaseLoggerFoundation::setUnReady(const QString& _err)
{
    is_ready = false;
    err_msg = _err;
};

void BaseLoggerFoundation::setReady(void)
{
    is_ready = true;
    err_msg.clear();
};

/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Messaging Framework.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "symbianfileinfo.h"

#include "symbiandir.h"
#include <bautils.h>
#include <QString>

SymbianFileInfo::SymbianFileInfo(const QString &file)
    : m_file(file)
{
    m_dataStorage.connect();
}

SymbianFileInfo::~SymbianFileInfo()
{
}

QString SymbianFileInfo::absoluteFilePath() const
{
    return m_file;
}

QString SymbianFileInfo::fileName() const
{
    return m_file.mid(m_file.lastIndexOf('/')+1);
}

SymbianDir SymbianFileInfo::dir() const
{
    int index = m_file.lastIndexOf('/');
    if (index == -1) {
        return SymbianDir(".");
    }
    return m_file.mid(0,index);
}

bool SymbianFileInfo::isRelative() const
{
    if (m_file.startsWith(QLatin1Char('/'))) {
        return false;
    }

    if (m_file.length() >= 2) {
        if (m_file.at(0).isLetter() && m_file.at(1) == QLatin1Char(':')) {
            return false;
        }
    }

    return true;
}

bool SymbianFileInfo::isDir() const
{
    return m_dataStorage.protectedDirectoryExists(m_file);
}

// End of File

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

#include "symbiandir.h"

#include <QString>
#include <QStringList>
#include <QDir>
#include <QVector>
#include <QRegExp>

SymbianDir::SymbianDir(const SymbianDir &other)
{
    m_dataStorage.connect();
    m_path = other.m_path;
}

SymbianDir::SymbianDir(const QString &path)
    : m_path(path)
{
    m_dataStorage.connect();
}

SymbianDir::~SymbianDir()
{
}

QString SymbianDir::absolutePath() const
{
    if (!m_path.startsWith('/')) {
        return rootPath()+m_path;
    }
    return m_path;
}

void SymbianDir::setPath(const QString &path)
{
    m_path = path;
}

void SymbianDir::setNameFilters(const QStringList &nameFilters)
{
    m_nameFilters = nameFilters;
}

QStringList SymbianDir::entryList(int filters) const
{
    QStringList entries = m_dataStorage.listDirectoryEntries(m_path);
    if (filters & NoDotAndDotDot) {
        for (int i=entries.count()-1; i >= 0; i--) {
            if ((entries[i] == ".") || (entries[i] == "..")) {
                entries.removeAt(i);
            }
        }
    }

    if (!m_nameFilters.isEmpty()) {
        QVector<QRegExp> regExps;
        regExps.reserve(m_nameFilters.size());
        for (int i = 0; i < m_nameFilters.size(); ++i) {
            regExps.append(QRegExp(m_nameFilters.at(i),
                                   (filters & CaseSensitive) ? Qt::CaseSensitive : Qt::CaseInsensitive,
                                   QRegExp::Wildcard));
        }
        for (int i=entries.count()-1; i >= 0; i--) {
            for (QVector<QRegExp>::const_iterator iter = regExps.constBegin(), end = regExps.constEnd();
                 iter != end;
                 ++iter) {
                if (!iter->exactMatch(entries[i])) {
                    entries.removeAt(i);
                }
            }
        }
    }

    return entries;
}

bool SymbianDir::rmdir(const QString &dirName) const
{
    return m_dataStorage.removeProtectedDirectory(dirName);
}

bool SymbianDir::mkpath(const QString &dirPath) const
{
    return m_dataStorage.makeProtectedDirectory(dirPath);
}

bool SymbianDir::rmpath(const QString &dirPath) const
{
    return m_dataStorage.removeProtectedPath(dirPath);
}

bool SymbianDir::remove(const QString &fileName)
{
    return m_dataStorage.removeProtectedFile(filePath(fileName));
}

bool SymbianDir::exists() const
{
    return m_dataStorage.protectedDirectoryExists(m_path);
}

QString SymbianDir::rootPath()
{
    return QDir::rootPath();
}

QString SymbianDir::filePath(const QString &fileName) const
{
    QString filePath;

    filePath.append(m_path);
    if (!m_path.endsWith('/')) {
        filePath.append('/');
    }
    filePath.append(fileName);

    return filePath;
}

// End of File

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

#ifndef SYMBIANFILE_H
#define SYMBIANFILE_H

#include "qmfdatastorage.h"
#include <f32file.h>
#include <QSharedDataPointer>
#include <QIODevice>

class QString;
class SymbianFilePrivate;

class SymbianFile : public QObject
{
    Q_OBJECT

public:
    SymbianFile();
    SymbianFile(const QString &name);
    SymbianFile(const SymbianFile& other);
    ~SymbianFile();

    SymbianFile& operator=(const SymbianFile& other);

    bool open(QIODevice::OpenMode flags);
    virtual void close();

    bool rename(const QString &newName);
    bool flush();

    static bool exists(const QString &fileName);
    static bool remove(const QString &fileName);
    static bool rename(const QString &oldName, const QString &newName);

    QByteArray readAll();
    qint64 write(const QByteArray &byteArray);

private: // Data
    QSharedDataPointer<SymbianFilePrivate> d;
};

#endif // SYMBIANFILE_H

// End of File

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

#ifndef SYMBIANSQLQUERY_H
#define SYMBIANSQLQUERY_H

#include "sqldatabase.h"

#include <QSharedDataPointer>
#include <QSqlQuery>
#include <QString>
#include <QMap>

class QVariant;
class QSqlError;
class QSqlResult;
class SymbianSqlQueryPrivate;

class SymbianSqlQuery
{
public:
    SymbianSqlQuery(const QString& query = QString(), SymbianSqlDatabase db = SymbianSqlDatabase());
    explicit SymbianSqlQuery(SymbianSqlDatabase db);
    SymbianSqlQuery(const SymbianSqlQuery& other);
    ~SymbianSqlQuery();

    SymbianSqlQuery& operator=(const SymbianSqlQuery& other);

    QSqlError lastError() const;
    QString lastQuery() const;
    QMap<QString, QVariant> boundValues() const;

    bool next();
    bool first();

    QVariant value(int i) const;
    QSqlRecord record() const;

    bool prepare(const QString& query);
    void addBindValue(const QVariant& val, QSql::ParamType type = QSql::In);

    void setForwardOnly(bool forward);

    bool exec(const QString& query);
    bool exec();
    bool execBatch();

    QVariant lastInsertId() const;

private:
    void startBindingValues();
    bool bindValuesLeft();
    bool bindValues();
    bool openDatabase();

private:
    QSharedDataPointer<SymbianSqlQueryPrivate> d;
};

#endif // SYMBIANSQLQUERY_H

// End of File

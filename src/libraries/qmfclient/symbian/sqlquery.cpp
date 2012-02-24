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

#include "sqlquery.h"

#include <QVariant>
#include <QSqlError>
#include <QSqlResult>
#include <QSqlRecord>
#include <QSqlField>
#include <QDateTime>
#include "sqldatabase.h"

#include <QDebug>

class SymbianSqlQueryPrivate : public QSharedData
{
public:
    SymbianSqlQueryPrivate();
    SymbianSqlQueryPrivate(const SymbianSqlQueryPrivate &other);
    ~SymbianSqlQueryPrivate();

    SymbianSqlQueryPrivate& operator=(const SymbianSqlQueryPrivate& other);

    QString m_query;
    QVector<QVariant> m_boundValues;
    bool m_prepared;
    int m_row;
    int m_paramIndex;
    int m_bindValueIndex;
    mutable SymbianSqlDatabase m_database;
    mutable RSqlStatement m_sqlStatement;
};

SymbianSqlQueryPrivate::SymbianSqlQueryPrivate()
    : m_prepared(false),
      m_row(0),
      m_paramIndex(0)
{
}

SymbianSqlQueryPrivate::SymbianSqlQueryPrivate(const SymbianSqlQueryPrivate &other)
    : QSharedData(other),
      m_query(other.m_query),
      m_prepared(other.m_prepared),
      m_row(other.m_row),
      m_paramIndex(other.m_paramIndex),
      m_sqlStatement(other.m_sqlStatement)
{
}

SymbianSqlQueryPrivate::~SymbianSqlQueryPrivate()
{
    m_sqlStatement.Close();
}

SymbianSqlQueryPrivate& SymbianSqlQueryPrivate::operator=(const SymbianSqlQueryPrivate& other)
{
    if (this != &other) {
        m_sqlStatement.Close();
        m_query = other.m_query;
        m_prepared = other.m_prepared;
        m_row = other.m_row;
        m_paramIndex = other.m_paramIndex;
        m_sqlStatement = other.m_sqlStatement;
    }

    return *this;
}

SymbianSqlQuery::SymbianSqlQuery(const QString& query, SymbianSqlDatabase db)
{
    d = new SymbianSqlQueryPrivate;
    if (db.isValid()) {
        d->m_database = db;
    } else {
        d->m_database = SymbianSqlDatabase::database();
    }
    d->m_query = query;
    d->m_query = d->m_query.remove('\"');
    d->m_query = d->m_query.trimmed();
}

SymbianSqlQuery::SymbianSqlQuery(SymbianSqlDatabase db)
{
    d = new SymbianSqlQueryPrivate;
    if (db.isValid()) {
        d->m_database = db;
    } else {
        d->m_database = SymbianSqlDatabase::database();
    }
}

SymbianSqlQuery::SymbianSqlQuery(const SymbianSqlQuery& other)
    : d(other.d)
{
}


SymbianSqlQuery::~SymbianSqlQuery()
{
}

SymbianSqlQuery& SymbianSqlQuery::operator=(const SymbianSqlQuery& other)
{
    if (this != &other) {
        d = other.d;
    }

    return *this;
}
QSqlError SymbianSqlQuery::lastError() const
{
    return QSqlError("","");
}

QString SymbianSqlQuery::lastQuery() const
{
    return d->m_query;
}

QMap<QString, QVariant> SymbianSqlQuery::boundValues() const
{
    QMap<QString,QVariant> boundValueMap;

    for (int i = 0; i < d->m_boundValues.count(); ++i) {
        boundValueMap[QString::number(i)] = d->m_boundValues.at(i);
    }

    return boundValueMap;
}

bool SymbianSqlQuery::next()
{
    d->m_row++;

    if (d->m_row == 1 && d->m_sqlStatement.AtRow()) {
        return true;
    }

    if (d->m_sqlStatement.Next() == KSqlAtRow) {
        return true;
    }
    return false;
}

bool SymbianSqlQuery::first()
{
    if (next() && (d->m_row == 1)) {
        return true;
    }
    return false;
}

QVariant SymbianSqlQuery::value(int i) const
{
    QVariant retVal;

    TSqlColumnType columnType = d->m_sqlStatement.ColumnType(i);
    switch (columnType) {
    case ESqlNull:
        break;
    case ESqlInt:
        retVal = d->m_sqlStatement.ColumnInt(i);
        break;
    case ESqlInt64:
        retVal = d->m_sqlStatement.ColumnInt64(i);
        break;
    case ESqlReal:
        retVal = d->m_sqlStatement.ColumnReal(i);
        break;
    case ESqlText:
        {
            RBuf text;
            TInt textSize = d->m_sqlStatement.ColumnSize(i);
            TRAPD(err, text.CreateL(textSize));
            if (err == KErrNone) {
                err = d->m_sqlStatement.ColumnText(i, text);
                if (err == KErrNone) {
                    TBuf<1> emptyStringBuf;
                    emptyStringBuf.Append(0);
                    if (text.Compare(emptyStringBuf) == 0) {
                        retVal = QString("");
                    } else {
                        retVal = QString::fromUtf16(text.Ptr(), text.Length());
                    }
                }
                text.Close();
            }
        }
        break;
    case ESqlBinary:
        break;
    }

    return retVal;
}

QSqlRecord SymbianSqlQuery::record() const
{
    QSqlRecord record;

    TPtrC columnName(KNullDesC);
    TInt count = d->m_sqlStatement.ColumnCount();
    for (TInt i=0; i < count; i++) {
        d->m_sqlStatement.ColumnName(i, columnName);
        QString columnNameString = QString::fromUtf16(columnName.Ptr(), columnName.Length());

        TSqlColumnType columnType = d->m_sqlStatement.ColumnType(i);
        switch (columnType) {
        case ESqlNull:
            record.append(QSqlField(columnNameString, QVariant::Int));
            break;
        case ESqlInt:
            record.append(QSqlField(columnNameString, QVariant::Int));
            break;
        case ESqlInt64:
            record.append(QSqlField(columnNameString, QVariant::LongLong));
            break;
        case ESqlReal:
            record.append(QSqlField(columnNameString, QVariant::Double));
            break;
        case ESqlText:
            record.append(QSqlField(columnNameString, QVariant::String));
            break;
        case ESqlBinary:
            break;
        }
        record.setValue(columnNameString, value(i));
    }

    return record;
}

bool SymbianSqlQuery::prepare(const QString& query)
{
    d->m_query = query;
    d->m_query = d->m_query.remove('\"');
    d->m_query = d->m_query.trimmed();

    if (d->m_prepared) {
        d->m_sqlStatement.Close();
    }

    TPtrC16 stringPtr(KNullDesC);
    stringPtr.Set(reinterpret_cast<const TUint16*>(query.utf16()));
    TInt retVal = d->m_sqlStatement.Prepare(d->m_database.symbianDatabase(), stringPtr);
    if (retVal != KErrNone) {
        return false;
    }

    d->m_prepared = true;
    return true;
}

void SymbianSqlQuery::addBindValue(const QVariant& val, QSql::ParamType /*type*/)
{
    d->m_boundValues.append(val);
    d->m_paramIndex++;
}

void SymbianSqlQuery::setForwardOnly(bool /*forward*/)
{
}

void SymbianSqlQuery::startBindingValues()
{
    d->m_bindValueIndex = 0;
}

bool SymbianSqlQuery::bindValuesLeft()
{
    if (d->m_boundValues.count() > 0) {
        if (d->m_boundValues.at(0).type() == QVariant::List) {
            if (d->m_bindValueIndex < d->m_boundValues.at(0).toList().count()) {
                return true;
            }
        } else {
            if (d->m_bindValueIndex == 0) {
                return true;
            }
        }
    }

    return false;
}

bool SymbianSqlQuery::bindValues()
{
    TPtrC16 stringPtr(KNullDesC);
    TInt paramIndex = 0;
    foreach (QVariant value, d->m_boundValues) {
        QVariant val;
        if (value.type() == QVariant::List) {
            QList<QVariant> variantList = value.toList();
            if (d->m_bindValueIndex >= variantList.count()) {
                return false;
            }
            val = variantList.at(d->m_bindValueIndex);
        } else {
            val = value;
        }

        if (val.isNull()) {
            d->m_sqlStatement.BindNull(paramIndex);
        } else {
            switch (val.type()) {
            case QVariant::Invalid:
                d->m_sqlStatement.BindNull(paramIndex);
                break;
            case QVariant::String:
                if (val.toString().isEmpty()) {
                    TBuf<1> emptyStringBuf;
                    emptyStringBuf.Append(0);
                    d->m_sqlStatement.BindText(paramIndex, emptyStringBuf);
                } else {
                    stringPtr.Set(reinterpret_cast<const TUint16*>(val.toString().utf16()));
                    d->m_sqlStatement.BindText(paramIndex, stringPtr);
                }
                break;
            case QVariant::UInt:
                d->m_sqlStatement.BindInt(paramIndex, val.toUInt());
                break;
            case QVariant::Int:
                d->m_sqlStatement.BindInt(paramIndex, val.toInt());
                break;
            case QVariant::ULongLong:
                d->m_sqlStatement.BindInt64(paramIndex, val.toULongLong());
                break;
            case QVariant::LongLong:
                d->m_sqlStatement.BindInt64(paramIndex, val.toLongLong());
                break;
            case QVariant::Double:
                d->m_sqlStatement.BindReal(paramIndex, val.toDouble());
                break;
            case QVariant::DateTime:
                if (val.toDateTime().isNull()) {
                    d->m_sqlStatement.BindInt(paramIndex, 0);
                } else {
                    d->m_sqlStatement.BindInt(paramIndex, val.toDateTime().toTime_t());
                }
                break;
            default:
                stringPtr.Set(reinterpret_cast<const TUint16*>(val.toString().utf16()));
                d->m_sqlStatement.BindText(paramIndex, stringPtr);
                break;
            }
        }

        paramIndex++;
    }

    d->m_bindValueIndex++;

    return true;
}

bool SymbianSqlQuery::exec(const QString& query)
{
    if (query.isEmpty()) {
        return false;
    }

    d->m_query = query;
    d->m_query = d->m_query.remove('\"');
    d->m_query = d->m_query.trimmed();

    if (d->m_prepared) {
        d->m_sqlStatement.Close();
    }

    TPtrC16 stringPtr(KNullDesC);
    stringPtr.Set(reinterpret_cast<const TUint16*>(d->m_query.utf16()));
    TInt retVal = d->m_sqlStatement.Prepare(d->m_database.symbianDatabase(), stringPtr);
    if (retVal != KErrNone) {
        return false;
    }

    startBindingValues();
    if (!bindValues()) {
        return false;
    }

    if (!query.startsWith("SELECT")) {
        TInt retVal = d->m_sqlStatement.Exec();
        if (retVal < 0) {
            return false;
        }
    }

    return true;
}

bool SymbianSqlQuery::exec()
{
    if (d->m_query.isEmpty()) {
        return false;
    }

    if (!d->m_prepared) {
        TPtrC16 stringPtr(KNullDesC);
        stringPtr.Set(reinterpret_cast<const TUint16*>(d->m_query.utf16()));
        TInt retVal = d->m_sqlStatement.Prepare(d->m_database.symbianDatabase(), stringPtr);
        if (retVal != KErrNone) {
            return false;
        }
    }

    startBindingValues();
    if (!bindValues()) {
        return false;
    }

    if (!d->m_query.startsWith("SELECT")) {
        TInt retVal = d->m_sqlStatement.Exec();
        if (retVal < KErrNone) {
            return false;
        }
    }

    return true;
}

bool SymbianSqlQuery::execBatch()
{
    if (d->m_query.isEmpty()) {
        return false;
    }

    if (d->m_prepared) {
        d->m_sqlStatement.Close();
    }

    TPtrC16 stringPtr(KNullDesC);
    stringPtr.Set(reinterpret_cast<const TUint16*>(d->m_query.utf16()));
    TInt retVal = d->m_sqlStatement.Prepare(d->m_database.symbianDatabase(), stringPtr);
    if (retVal != KErrNone) {
        return false;
    }

    startBindingValues();
    while (bindValuesLeft() && (retVal >= KErrNone)) {
        if (bindValues()) {
            retVal = d->m_sqlStatement.Exec();
            d->m_sqlStatement.Reset();
        } else {
            retVal = KErrGeneral;
        }
    }

    if (retVal < KErrNone) {
        return false;
    }

    return true;
}

QVariant SymbianSqlQuery::lastInsertId() const
{
    QVariant id;

    SymbianSqlQuery query;
    query.prepare("SELECT last_insert_rowid();");
    if (query.next()) {
        id = query.value(0);
    }

    return id;
}

// End of File

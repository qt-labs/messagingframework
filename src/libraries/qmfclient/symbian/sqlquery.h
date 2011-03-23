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

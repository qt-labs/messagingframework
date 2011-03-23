#ifndef SQLDATABASE_H
#define SQLDATABASE_H

#include <QSharedDataPointer>
#include <SqlDb.h>

class QSqlError;
class QString;
class QStringList;
class SymbianSqlQuery;
class SymbianSqlDatabasePrivate;

class SymbianSqlDatabase
{
public:
    SymbianSqlDatabase();
    SymbianSqlDatabase(const SymbianSqlDatabase &other);
    ~SymbianSqlDatabase();

    SymbianSqlDatabase &operator=(const SymbianSqlDatabase &other);

    bool isValid() const;

    bool open();
    bool isOpen() const;
    QStringList tables() const;

    void setDatabaseName(const QString& name);
    QString databaseName() const;
    QString driverName() const;

    QSqlError lastError() const;

    bool transaction();
    bool commit();
    bool rollback();

    QString lastQuery() const;

    static SymbianSqlDatabase addDatabase(const QString& type);
    static SymbianSqlDatabase database();

protected:
    RSqlDatabase& symbianDatabase();

private:
    friend class SymbianSqlQuery;
    QSharedDataPointer<SymbianSqlDatabasePrivate> d;
};

#endif // SQLDATABASE_H

// End of File

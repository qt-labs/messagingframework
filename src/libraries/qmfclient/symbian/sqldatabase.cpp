#include "sqldatabase.h"

#include <QThreadStorage>
#include <QStringList>
#include <QSqlError>
#include <QVariant>
#include <sqldb.h>
#include "sqlquery.h"
#include "qmfdatastorage.h"

#define QMFSecureDbNamePrefix "[2003A67A]"

class SymbianDatabase {
public:
    SymbianDatabase();
    ~SymbianDatabase();

    static SymbianDatabase* instance();

    QString m_type;
    QString m_name;
    bool m_open;
    RSqlDatabase m_database;
};

Q_GLOBAL_STATIC(QThreadStorage<SymbianDatabase *>, symbianDatabase)

SymbianDatabase::SymbianDatabase()
    : m_open(false)
{
};

SymbianDatabase::~SymbianDatabase()
{
    if (m_open) {
        m_database.Close();
    }
};

SymbianDatabase* SymbianDatabase::instance()
{
    if (!symbianDatabase()->hasLocalData()) {
        symbianDatabase()->setLocalData(new SymbianDatabase);
    }

    SymbianDatabase* instance = symbianDatabase()->localData();

    return instance;
}

class SymbianSqlDatabasePrivate : public QSharedData
{
public:
    SymbianSqlDatabasePrivate();
    SymbianSqlDatabasePrivate(const SymbianSqlDatabasePrivate &other);
    ~SymbianSqlDatabasePrivate();

    SymbianSqlDatabasePrivate& operator=(const SymbianSqlDatabasePrivate& other);

    SymbianDatabase* m_db;
};

SymbianSqlDatabasePrivate::SymbianSqlDatabasePrivate()
    : m_db(0)
{
}

SymbianSqlDatabasePrivate::SymbianSqlDatabasePrivate(const SymbianSqlDatabasePrivate &other)
    : QSharedData(other),
      m_db(other.m_db)
{
}

SymbianSqlDatabasePrivate::~SymbianSqlDatabasePrivate()
{
}

SymbianSqlDatabasePrivate& SymbianSqlDatabasePrivate::operator=(const SymbianSqlDatabasePrivate& other)
{
    if (this != &other) {
        m_db = other.m_db;
    }
    return *this;
}

SymbianSqlDatabase::SymbianSqlDatabase()
{
    d = 0;
}

SymbianSqlDatabase::SymbianSqlDatabase(const SymbianSqlDatabase& other)
    : d(other.d)
{
}

SymbianSqlDatabase::~SymbianSqlDatabase()
{
}

SymbianSqlDatabase& SymbianSqlDatabase::operator=(const SymbianSqlDatabase& other)
{
    if (this != &other) {
        d = other.d;
    }
    return *this;
}

bool SymbianSqlDatabase::isValid() const
{
    if (d) {
        return true;
    }
    return false;
}

bool SymbianSqlDatabase::open()
{
    if (!d->m_db->m_open) {
        TInt error;

        QString dbname = QString(QMFSecureDbNamePrefix)+d->m_db->m_name;
        TPtrC16 symbianDbName(KNullDesC);
        symbianDbName.Set(reinterpret_cast<const TUint16*>(dbname.utf16()));

        error = d->m_db->m_database.Open(symbianDbName);
        if (error == KErrNone) {
            d->m_db->m_open = true;
        } else {
            SymbianQMFDataStorage dataStorage;
            dataStorage.connect();
            dataStorage.createDatabase(d->m_db->m_name);

            error = d->m_db->m_database.Open(symbianDbName);
            if (error == KErrNone) {
                d->m_db->m_open = true;
            } else {
                d->m_db->m_open = false;
            }
        }
    }

    return d->m_db->m_open;
}

bool SymbianSqlDatabase::isOpen() const
{
    return d->m_db->m_open;
}

QStringList SymbianSqlDatabase::tables() const
{
    QStringList tables;

    SymbianSqlQuery query("SELECT name FROM sqlite_master WHERE type='table' ORDER BY name;");
    query.exec();
    while (query.next()) {
        QString tablename = query.value(0).toString();
        if (!tablename.startsWith("symbian_") && !tablename.startsWith("sqlite_")) {
            tables.append(tablename);
        }
    }

    return tables;
}

void SymbianSqlDatabase::setDatabaseName(const QString& name)
{
    d->m_db->m_name = name;
}

QString SymbianSqlDatabase::databaseName() const
{
    return d->m_db->m_name;
}

QString SymbianSqlDatabase::driverName() const
{
    return "QSQLITE";
}

QSqlError SymbianSqlDatabase::lastError() const
{
    return QSqlError();
}

bool SymbianSqlDatabase::transaction()
{
    if (d->m_db->m_database.Exec(_L("BEGIN")) < KErrNone) {
        return false;
    }
    return true;
}

bool SymbianSqlDatabase::commit()
{
    if (d->m_db->m_database.Exec(_L("COMMIT")) < KErrNone) {
        return false;
    }
    return true;
}

bool SymbianSqlDatabase::rollback()
{
    if (d->m_db->m_database.Exec(_L("ROLLBACK")) < KErrNone) {
        return false;
    }
    return true;
}

SymbianSqlDatabase SymbianSqlDatabase::addDatabase(const QString& type)
{
    SymbianSqlDatabase newDb;

    SymbianDatabase* database = SymbianDatabase::instance();
    database->m_type = type;
    newDb.d = new SymbianSqlDatabasePrivate;
    newDb.d->m_db = database;

    return newDb;
}

SymbianSqlDatabase SymbianSqlDatabase::database()
{
    SymbianSqlDatabase newDb;

    newDb.d = new SymbianSqlDatabasePrivate;
    newDb.d->m_db = SymbianDatabase::instance();

    return newDb;
}

RSqlDatabase& SymbianSqlDatabase::symbianDatabase()
{
    return d->m_db->m_database;
}

// End of File

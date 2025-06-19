#include "DatabaseManager.h"
#include <QStandardPaths>
#include <QDir>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDateTime>
#include <QUuid>

DatabaseManager* DatabaseManager::instance = nullptr;
QMutex DatabaseManager::instanceMutex;

DatabaseManager& DatabaseManager::getInstance()
{
    if (instance == nullptr) {
        QMutexLocker locker(&instanceMutex);
        if (instance == nullptr) {
            instance = new DatabaseManager();
        }
    }
    return *instance;
}

DatabaseManager::DatabaseManager(QObject *parent)
    : QObject(parent)
{
    dbPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) 
             + "/vms.db";
}

DatabaseManager::~DatabaseManager()
{
    close();
}

bool DatabaseManager::initialize()
{
    QDir().mkpath(QFileInfo(dbPath).path());
    
    db = QSqlDatabase::addDatabase("QSQLITE", "main");
    db.setDatabaseName(dbPath);
    
    if (!db.open()) {
        qCritical() << "Failed to open database:" << db.lastError().text();
        return false;
    }
    
    if (!initializeEncryption()) {
        qCritical() << "Failed to initialize database encryption";
        return false;
    }
    
    if (!initializeTables()) {
        qCritical() << "Failed to initialize database tables";
        return false;
    }
    
    return true;
}

void DatabaseManager::close()
{
    QString connection = db.connectionName();
    db.close();
    db = QSqlDatabase();
    QSqlDatabase::removeDatabase(connection);
}

bool DatabaseManager::initializeEncryption()
{
    QSqlQuery query(db);
    
    // Enable SQLite encryption
    if (!query.exec("PRAGMA key = 'your_encryption_key'")) {
        qCritical() << "Failed to set encryption key:" << query.lastError().text();
        return false;
    }
    
    // Enable secure delete
    if (!query.exec("PRAGMA secure_delete = ON")) {
        qCritical() << "Failed to enable secure delete:" << query.lastError().text();
        return false;
    }
    
    return true;
}

bool DatabaseManager::initializeTables()
{
    QSqlQuery query(db);
    
    // Create version table
    if (!query.exec("CREATE TABLE IF NOT EXISTS version ("
                   "id INTEGER PRIMARY KEY,"
                   "version INTEGER NOT NULL,"
                   "updated_at DATETIME NOT NULL)")) {
        qCritical() << "Failed to create version table:" << query.lastError().text();
        return false;
    }
    
    // Create users table
    if (!query.exec("CREATE TABLE IF NOT EXISTS users ("
                   "id TEXT PRIMARY KEY,"
                   "name TEXT NOT NULL,"
                   "email TEXT NOT NULL,"
                   "phone TEXT,"
                   "company TEXT,"
                   "created_at DATETIME NOT NULL,"
                   "updated_at DATETIME NOT NULL,"
                   "retention_period INTEGER NOT NULL,"
                   "UNIQUE(email))")) {
        qCritical() << "Failed to create users table:" << query.lastError().text();
        return false;
    }
    
    // Create consent table
    if (!query.exec("CREATE TABLE IF NOT EXISTS consent ("
                   "id TEXT PRIMARY KEY,"
                   "user_id TEXT NOT NULL,"
                   "purpose TEXT NOT NULL,"
                   "granted BOOLEAN NOT NULL,"
                   "granted_at DATETIME NOT NULL,"
                   "expires_at DATETIME,"
                   "FOREIGN KEY(user_id) REFERENCES users(id))")) {
        qCritical() << "Failed to create consent table:" << query.lastError().text();
        return false;
    }
    
    // Create audit_log table
    if (!query.exec("CREATE TABLE IF NOT EXISTS audit_log ("
                   "id TEXT PRIMARY KEY,"
                   "action TEXT NOT NULL,"
                   "entity_type TEXT NOT NULL,"
                   "entity_id TEXT NOT NULL,"
                   "user_id TEXT,"
                   "details TEXT,"
                   "created_at DATETIME NOT NULL)")) {
        qCritical() << "Failed to create audit_log table:" << query.lastError().text();
        return false;
    }
    
    return validateSchema();
}

bool DatabaseManager::validateSchema()
{
    QSqlQuery query(db);
    
    // Check current schema version
    if (!query.exec("SELECT version FROM version ORDER BY id DESC LIMIT 1")) {
        qCritical() << "Failed to check schema version:" << query.lastError().text();
        return false;
    }
    
    int currentVersion = 0;
    if (query.next()) {
        currentVersion = query.value(0).toInt();
    }
    
    if (currentVersion < CURRENT_SCHEMA_VERSION) {
        return upgradeSchema(currentVersion, CURRENT_SCHEMA_VERSION);
    }
    
    return true;
}

bool DatabaseManager::upgradeSchema(int fromVersion, int toVersion)
{
    // Implement schema upgrades here
    QSqlQuery query(db);
    
    if (!db.transaction()) {
        qCritical() << "Failed to start transaction for schema upgrade";
        return false;
    }
    
    try {
        // Add schema upgrade steps here
        
        // Update version
        query.prepare("INSERT INTO version (version, updated_at) VALUES (?, ?)");
        query.addBindValue(toVersion);
        query.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
        
        if (!query.exec()) {
            throw std::runtime_error("Failed to update schema version");
        }
        
        if (!db.commit()) {
            throw std::runtime_error("Failed to commit schema upgrade");
        }
    }
    catch (const std::exception& e) {
        db.rollback();
        qCritical() << "Schema upgrade failed:" << e.what();
        return false;
    }
    
    return true;
}

bool DatabaseManager::purgeExpiredData()
{
    QSqlQuery query(db);
    
    if (!db.transaction()) {
        qCritical() << "Failed to start transaction for data purge";
        return false;
    }
    
    try {
        // Delete expired consent records
        if (!query.exec("DELETE FROM consent WHERE expires_at < datetime('now')")) {
            throw std::runtime_error("Failed to purge expired consent records");
        }
        
        // Delete users with expired retention period
        if (!query.exec("DELETE FROM users WHERE datetime(created_at, '+' || retention_period || ' days') < datetime('now')")) {
            throw std::runtime_error("Failed to purge expired user records");
        }
        
        if (!db.commit()) {
            throw std::runtime_error("Failed to commit data purge");
        }
    }
    catch (const std::exception& e) {
        db.rollback();
        qCritical() << "Data purge failed:" << e.what();
        return false;
    }
    
    return true;
}

bool DatabaseManager::exportUserData(const QString& userId, const QString& format)
{
    // TODO: Implement user data export in various formats (JSON, CSV, etc.)
    return false;
}

bool DatabaseManager::updateUserConsent(const QString& userId, const QString& purpose, bool consent)
{
    QSqlQuery query(db);
    
    query.prepare("INSERT OR REPLACE INTO consent (id, user_id, purpose, granted, granted_at, expires_at) "
                 "VALUES (?, ?, ?, ?, ?, ?)");
    
    query.addBindValue(QUuid::createUuid().toString());
    query.addBindValue(userId);
    query.addBindValue(purpose);
    query.addBindValue(consent);
    query.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
    query.addBindValue(QDateTime::currentDateTime().addYears(1).toString(Qt::ISODate));
    
    return query.exec();
}

QSqlDatabase DatabaseManager::getConnection()
{
    QMutexLocker locker(&connectionMutex);
    
    QString connectionName = QUuid::createUuid().toString();
    QSqlDatabase connection = QSqlDatabase::cloneDatabase(db, connectionName);
    
    if (!connection.open()) {
        qCritical() << "Failed to open database connection:" << connection.lastError().text();
    }
    
    return connection;
}

void DatabaseManager::releaseConnection(const QString& connectionName)
{
    QMutexLocker locker(&connectionMutex);
    
    QSqlDatabase::database(connectionName).close();
    QSqlDatabase::removeDatabase(connectionName);
} 
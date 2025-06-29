#include "DatabaseManager.h"
#include <QStandardPaths>
#include <QDir>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDateTime>
#include <QUuid>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <openssl/rand.h>
#include "../utils/ErrorHandler.h"

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
    // Get or generate encryption key
    QString encryptionKey = getStoredEncryptionKey();
    if (encryptionKey.isEmpty()) {
        encryptionKey = generateDatabaseKey();
        storeEncryptionKey(encryptionKey);
        ErrorHandler::getInstance().logInfo("DatabaseManager", "Generated new database encryption key");
    }
    
    QSqlQuery query(db);
    
    // Enable SQLite encryption with secure key
    QString pragmaKey = QString("PRAGMA key = '%1'").arg(encryptionKey);
    if (!query.exec(pragmaKey)) {
        ErrorHandler::getInstance().logError("DatabaseManager", 
            QString("Failed to set encryption key: %1").arg(query.lastError().text()));
        return false;
    }
    
    // Test encryption by creating a test table
    if (!query.exec("CREATE TABLE IF NOT EXISTS encryption_test (id INTEGER)")) {
        ErrorHandler::getInstance().logError("DatabaseManager", "Database encryption test failed");
        return false;
    }
    
    if (!query.exec("DROP TABLE encryption_test")) {
        ErrorHandler::getInstance().logError("DatabaseManager", "Failed to clean up encryption test");
    }
    
    // Enable secure delete
    if (!query.exec("PRAGMA secure_delete = ON")) {
        ErrorHandler::getInstance().logError("DatabaseManager", 
            QString("Failed to enable secure delete: %1").arg(query.lastError().text()));
        return false;
    }
    
    // Enable foreign key constraints
    if (!query.exec("PRAGMA foreign_keys = ON")) {
        ErrorHandler::getInstance().logError("DatabaseManager", 
            QString("Failed to enable foreign keys: %1").arg(query.lastError().text()));
        return false;
    }
    
    // Set secure journal mode
    if (!query.exec("PRAGMA journal_mode = WAL")) {
        ErrorHandler::getInstance().logError("DatabaseManager", 
            QString("Failed to set journal mode: %1").arg(query.lastError().text()));
        return false;
    }
    
    return true;
}

bool DatabaseManager::initializeTables()
{
    if (!db.transaction()) {
        ErrorHandler::getInstance().logError("DatabaseManager", "Failed to start transaction for table creation");
        return false;
    }
    
    try {
        QSqlQuery query(db);
        
        // Create version table
        if (!query.exec("CREATE TABLE IF NOT EXISTS version ("
                       "id INTEGER PRIMARY KEY,"
                       "version INTEGER NOT NULL,"
                       "updated_at DATETIME NOT NULL)")) {
            throw std::runtime_error(QString("Failed to create version table: %1").arg(query.lastError().text()).toStdString());
        }
        
        // Initialize version if empty
        query.prepare("SELECT COUNT(*) FROM version");
        if (query.exec() && query.next() && query.value(0).toInt() == 0) {
            query.prepare("INSERT INTO version (version, updated_at) VALUES (?, ?)");
            query.addBindValue(CURRENT_SCHEMA_VERSION);
            query.addBindValue(QDateTime::currentDateTime());
            if (!query.exec()) {
                throw std::runtime_error("Failed to initialize version table");
            }
        }
        
        if (!createSecurityTables()) {
            throw std::runtime_error("Failed to create security tables");
        }
        
        if (!createVisitorTables()) {
            throw std::runtime_error("Failed to create visitor tables");
        }
        
        if (!createReportTables()) {
            throw std::runtime_error("Failed to create report tables");
        }
        
        if (!db.commit()) {
            throw std::runtime_error("Failed to commit table creation transaction");
        }
        
    } catch (const std::exception& e) {
        db.rollback();
        ErrorHandler::getInstance().logError("DatabaseManager", e.what());
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

QSqlDatabase DatabaseManager::getDatabase()
{
    return db;
}

bool DatabaseManager::setEncryptionKey(const QString& key)
{
    QSqlQuery query(db);
    QString pragmaKey = QString("PRAGMA rekey = '%1'").arg(key);
    
    if (!query.exec(pragmaKey)) {
        ErrorHandler::getInstance().logError("DatabaseManager", 
            QString("Failed to change encryption key: %1").arg(query.lastError().text()));
        return false;
    }
    
    storeEncryptionKey(key);
    return true;
}

QString DatabaseManager::generateDatabaseKey()
{
    // Generate a cryptographically secure 256-bit key
    unsigned char keyBytes[32];
    if (RAND_bytes(keyBytes, sizeof(keyBytes)) != 1) {
        ErrorHandler::getInstance().logError("DatabaseManager", "Failed to generate secure database key");
        // Fallback to Qt's random generator
        QByteArray fallbackKey;
        for (int i = 0; i < 32; ++i) {
            fallbackKey.append(static_cast<char>(QRandomGenerator::global()->bounded(256)));
        }
        return fallbackKey.toHex();
    }
    
    return QByteArray(reinterpret_cast<const char*>(keyBytes), sizeof(keyBytes)).toHex();
}

QString DatabaseManager::getStoredEncryptionKey()
{
    QSettings settings;
    return settings.value("database/encryption_key").toString();
}

void DatabaseManager::storeEncryptionKey(const QString& key)
{
    QSettings settings;
    settings.setValue("database/encryption_key", key);
}

bool DatabaseManager::createSecurityTables()
{
    QSqlQuery query(db);
    
    // Users table for authentication
    if (!query.exec("CREATE TABLE IF NOT EXISTS users ("
                   "user_id TEXT PRIMARY KEY,"
                   "username TEXT UNIQUE NOT NULL,"
                   "password_hash TEXT NOT NULL,"
                   "salt TEXT NOT NULL,"
                   "role INTEGER NOT NULL,"
                   "is_active BOOLEAN NOT NULL DEFAULT 1,"
                   "last_login DATETIME,"
                   "created_at DATETIME NOT NULL,"
                   "failed_login_attempts INTEGER NOT NULL DEFAULT 0,"
                   "lockout_until DATETIME"
                   ")")) {
        ErrorHandler::getInstance().logError("DatabaseManager", 
            QString("Failed to create users table: %1").arg(query.lastError().text()));
        return false;
    }
    
    // Security events table for audit logging
    if (!query.exec("CREATE TABLE IF NOT EXISTS security_events ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                   "event_type TEXT NOT NULL,"
                   "details TEXT NOT NULL,"
                   "timestamp DATETIME NOT NULL,"
                   "user_id TEXT,"
                   "ip_address TEXT,"
                   "FOREIGN KEY(user_id) REFERENCES users(user_id)"
                   ")")) {
        ErrorHandler::getInstance().logError("DatabaseManager", 
            QString("Failed to create security_events table: %1").arg(query.lastError().text()));
        return false;
    }
    
    // Sessions table for session management
    if (!query.exec("CREATE TABLE IF NOT EXISTS sessions ("
                   "session_id TEXT PRIMARY KEY,"
                   "user_id TEXT NOT NULL,"
                   "created_at DATETIME NOT NULL,"
                   "last_activity DATETIME NOT NULL,"
                   "ip_address TEXT NOT NULL,"
                   "user_agent TEXT,"
                   "is_active BOOLEAN NOT NULL DEFAULT 1,"
                   "FOREIGN KEY(user_id) REFERENCES users(user_id)"
                   ")")) {
        ErrorHandler::getInstance().logError("DatabaseManager", 
            QString("Failed to create sessions table: %1").arg(query.lastError().text()));
        return false;
    }
    
    return true;
}

bool DatabaseManager::createVisitorTables()
{
    QSqlQuery query(db);
    
    // Visitors table
    if (!query.exec("CREATE TABLE IF NOT EXISTS visitors ("
                   "visitor_id TEXT PRIMARY KEY,"
                   "name TEXT NOT NULL,"
                   "email TEXT,"
                   "phone TEXT,"
                   "company TEXT,"
                   "id_number TEXT,"
                   "photo BLOB,"
                   "id_scan BLOB,"
                   "signature BLOB,"
                   "purpose TEXT,"
                   "host_name TEXT,"
                   "host_email TEXT,"
                   "host_phone TEXT,"
                   "consent_photo BOOLEAN NOT NULL DEFAULT 0,"
                   "consent_data_processing BOOLEAN NOT NULL DEFAULT 0,"
                   "consent_marketing BOOLEAN NOT NULL DEFAULT 0,"
                   "is_blacklisted BOOLEAN NOT NULL DEFAULT 0,"
                   "blacklist_reason TEXT,"
                   "created_at DATETIME NOT NULL,"
                   "updated_at DATETIME NOT NULL,"
                   "retention_period INTEGER NOT NULL DEFAULT 2555" // 7 years in days
                   ")")) {
        ErrorHandler::getInstance().logError("DatabaseManager", 
            QString("Failed to create visitors table: %1").arg(query.lastError().text()));
        return false;
    }
    
    // Visitor check-ins table
    if (!query.exec("CREATE TABLE IF NOT EXISTS visitor_checkins ("
                   "checkin_id TEXT PRIMARY KEY,"
                   "visitor_id TEXT NOT NULL,"
                   "checkin_time DATETIME NOT NULL,"
                   "checkout_time DATETIME,"
                   "location TEXT,"
                   "purpose TEXT,"
                   "badge_number TEXT,"
                   "escort_required BOOLEAN NOT NULL DEFAULT 0,"
                   "escort_name TEXT,"
                   "status TEXT NOT NULL DEFAULT 'checked_in'," // checked_in, checked_out, overstay
                   "created_by TEXT,"
                   "notes TEXT,"
                   "FOREIGN KEY(visitor_id) REFERENCES visitors(visitor_id),"
                   "FOREIGN KEY(created_by) REFERENCES users(user_id)"
                   ")")) {
        ErrorHandler::getInstance().logError("DatabaseManager", 
            QString("Failed to create visitor_checkins table: %1").arg(query.lastError().text()));
        return false;
    }
    
    // Consent records table
    if (!query.exec("CREATE TABLE IF NOT EXISTS consent_records ("
                   "consent_id TEXT PRIMARY KEY,"
                   "visitor_id TEXT NOT NULL,"
                   "purpose TEXT NOT NULL,"
                   "granted BOOLEAN NOT NULL,"
                   "granted_at DATETIME NOT NULL,"
                   "expires_at DATETIME,"
                   "withdrawn_at DATETIME,"
                   "ip_address TEXT,"
                   "user_agent TEXT,"
                   "FOREIGN KEY(visitor_id) REFERENCES visitors(visitor_id)"
                   ")")) {
        ErrorHandler::getInstance().logError("DatabaseManager", 
            QString("Failed to create consent_records table: %1").arg(query.lastError().text()));
        return false;
    }
    
    // Blacklist table
    if (!query.exec("CREATE TABLE IF NOT EXISTS blacklist ("
                   "blacklist_id TEXT PRIMARY KEY,"
                   "visitor_id TEXT,"
                   "name TEXT,"
                   "email TEXT,"
                   "phone TEXT,"
                   "id_number TEXT,"
                   "reason TEXT NOT NULL,"
                   "added_by TEXT NOT NULL,"
                   "added_at DATETIME NOT NULL,"
                   "expires_at DATETIME,"
                   "is_active BOOLEAN NOT NULL DEFAULT 1,"
                   "FOREIGN KEY(visitor_id) REFERENCES visitors(visitor_id),"
                   "FOREIGN KEY(added_by) REFERENCES users(user_id)"
                   ")")) {
        ErrorHandler::getInstance().logError("DatabaseManager", 
            QString("Failed to create blacklist table: %1").arg(query.lastError().text()));
        return false;
    }
    
    return true;
}

bool DatabaseManager::createReportTables()
{
    QSqlQuery query(db);
    
    // Reports table for generated reports
    if (!query.exec("CREATE TABLE IF NOT EXISTS reports ("
                   "report_id TEXT PRIMARY KEY,"
                   "report_type TEXT NOT NULL,"
                   "title TEXT NOT NULL,"
                   "description TEXT,"
                   "generated_by TEXT NOT NULL,"
                   "generated_at DATETIME NOT NULL,"
                   "parameters TEXT," // JSON parameters
                   "file_path TEXT,"
                   "file_format TEXT,"
                   "file_size INTEGER,"
                   "status TEXT NOT NULL DEFAULT 'pending'," // pending, completed, failed
                   "FOREIGN KEY(generated_by) REFERENCES users(user_id)"
                   ")")) {
        ErrorHandler::getInstance().logError("DatabaseManager", 
            QString("Failed to create reports table: %1").arg(query.lastError().text()));
        return false;
    }
    
    // Data access log for PDPA compliance
    if (!query.exec("CREATE TABLE IF NOT EXISTS data_access_log ("
                   "access_id TEXT PRIMARY KEY,"
                   "accessed_table TEXT NOT NULL,"
                   "accessed_record_id TEXT NOT NULL,"
                   "access_type TEXT NOT NULL," // read, write, delete
                   "accessed_by TEXT NOT NULL,"
                   "accessed_at DATETIME NOT NULL,"
                   "ip_address TEXT,"
                   "purpose TEXT,"
                   "FOREIGN KEY(accessed_by) REFERENCES users(user_id)"
                   ")")) {
        ErrorHandler::getInstance().logError("DatabaseManager", 
            QString("Failed to create data_access_log table: %1").arg(query.lastError().text()));
        return false;
    }
    
    // Create indexes for performance
    QStringList indexes = {
        "CREATE INDEX IF NOT EXISTS idx_visitors_email ON visitors(email)",
        "CREATE INDEX IF NOT EXISTS idx_visitors_phone ON visitors(phone)",
        "CREATE INDEX IF NOT EXISTS idx_visitors_id_number ON visitors(id_number)",
        "CREATE INDEX IF NOT EXISTS idx_visitors_created_at ON visitors(created_at)",
        "CREATE INDEX IF NOT EXISTS idx_checkins_visitor_id ON visitor_checkins(visitor_id)",
        "CREATE INDEX IF NOT EXISTS idx_checkins_time ON visitor_checkins(checkin_time)",
        "CREATE INDEX IF NOT EXISTS idx_checkins_status ON visitor_checkins(status)",
        "CREATE INDEX IF NOT EXISTS idx_security_events_timestamp ON security_events(timestamp)",
        "CREATE INDEX IF NOT EXISTS idx_security_events_user_id ON security_events(user_id)",
        "CREATE INDEX IF NOT EXISTS idx_sessions_user_id ON sessions(user_id)",
        "CREATE INDEX IF NOT EXISTS idx_sessions_last_activity ON sessions(last_activity)",
        "CREATE INDEX IF NOT EXISTS idx_blacklist_visitor_id ON blacklist(visitor_id)",
        "CREATE INDEX IF NOT EXISTS idx_blacklist_active ON blacklist(is_active)",
        "CREATE INDEX IF NOT EXISTS idx_consent_visitor_id ON consent_records(visitor_id)",
        "CREATE INDEX IF NOT EXISTS idx_data_access_timestamp ON data_access_log(accessed_at)"
    };
    
    for (const QString& indexSql : indexes) {
        if (!query.exec(indexSql)) {
            ErrorHandler::getInstance().logWarning("DatabaseManager", 
                QString("Failed to create index: %1").arg(query.lastError().text()));
            // Continue with other indexes even if one fails
        }
    }
    
    return true;
} 
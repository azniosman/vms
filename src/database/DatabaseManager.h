#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QMutex>
#include <memory>

class DatabaseManager : public QObject {
    Q_OBJECT

public:
    static DatabaseManager& getInstance();
    
    bool initialize();
    void close();
    
    // Database operations
    bool createTables();
    bool upgradeSchema(int fromVersion, int toVersion);
    bool backup(const QString& backupPath);
    
    // PDPA compliance methods
    bool purgeExpiredData();
    bool exportUserData(const QString& userId, const QString& format);
    bool updateUserConsent(const QString& userId, const QString& purpose, bool consent);
    
    // Connection management
    QSqlDatabase getConnection();
    void releaseConnection(const QString& connectionName);

private:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager();
    
    // Prevent copying
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    bool initializeTables();
    bool initializeEncryption();
    bool validateSchema();
    
    static DatabaseManager* instance;
    static QMutex instanceMutex;
    
    QSqlDatabase db;
    QString dbPath;
    QMutex connectionMutex;
    
    // Database version
    static const int CURRENT_SCHEMA_VERSION = 1;
};

#endif // DATABASEMANAGER_H 
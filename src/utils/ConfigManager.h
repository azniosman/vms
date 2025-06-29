#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QObject>
#include <QSettings>
#include <QMutex>
#include <QVariant>
#include <QJsonObject>
#include <QJsonDocument>
#include <QCryptographicHash>
#include <QTimer>

class ConfigManager : public QObject
{
    Q_OBJECT

public:
    static ConfigManager& getInstance();
    
    // Configuration management
    bool initialize();
    bool loadConfiguration(const QString& configFile = QString());
    bool saveConfiguration();
    bool resetToDefaults();
    
    // Secure value access
    QVariant getValue(const QString& key, const QVariant& defaultValue = QVariant()) const;
    bool setValue(const QString& key, const QVariant& value);
    bool removeValue(const QString& key);
    bool hasKey(const QString& key) const;
    
    // Encrypted values
    QVariant getSecureValue(const QString& key, const QVariant& defaultValue = QVariant()) const;
    bool setSecureValue(const QString& key, const QVariant& value);
    
    // Configuration validation
    bool validateConfiguration() const;
    bool isConfigurationSecure() const;
    QStringList getConfigurationErrors() const;
    
    // Security settings
    QString getDatabaseEncryptionKey() const;
    bool setDatabaseEncryptionKey(const QString& key);
    int getSessionTimeout() const;
    int getMaxLoginAttempts() const;
    int getLockoutDuration() const;
    QString getLogLevel() const;
    bool isAuditLoggingEnabled() const;
    QStringList getAllowedIpAddresses() const;
    
    // PDPA compliance settings
    int getDataRetentionPeriod() const;
    bool isDataMinimizationEnabled() const;
    bool isConsentRequired() const;
    QString getPrivacyPolicyVersion() const;
    
    // Application settings
    QString getApplicationMode() const; // production, development, testing
    QString getApplicationVersion() const;
    QString getDatabasePath() const;
    QString getLogDirectory() const;
    int getMaxLogFileSize() const;
    int getMaxLogFiles() const;
    
    // Backup and restore
    bool backupConfiguration(const QString& backupPath);
    bool restoreConfiguration(const QString& backupPath);
    
    // Configuration monitoring
    void enableAutoSave(bool enabled = true);
    void enableConfigurationWatcher(bool enabled = true);

signals:
    void configurationChanged(const QString& key, const QVariant& value);
    void configurationError(const QString& error);
    void configurationReloaded();

private slots:
    void onAutoSave();
    void onConfigurationFileChanged();

private:
    explicit ConfigManager(QObject *parent = nullptr);
    ~ConfigManager();
    
    // Prevent copying
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    
    // Internal methods
    void setupDefaults();
    QString encryptValue(const QString& plaintext) const;
    QString decryptValue(const QString& ciphertext) const;
    QString generateConfigurationHash() const;
    bool verifyConfigurationIntegrity() const;
    void createSecureConfigDirectory();
    
    // Validation methods
    bool validateSecuritySettings() const;
    bool validateDatabaseSettings() const;
    bool validateLoggingSettings() const;
    bool validateNetworkSettings() const;
    
    // File operations
    bool isConfigurationFileSecure(const QString& filePath) const;
    bool setConfigurationFilePermissions(const QString& filePath) const;
    
    static ConfigManager* instance;
    static QMutex instanceMutex;
    
    // Configuration storage
    mutable QMutex configMutex;
    QJsonObject configData;
    QJsonObject secureConfigData;
    QString configFilePath;
    QString secureConfigFilePath;
    QString encryptionKey;
    QString configurationHash;
    
    // Auto-save and monitoring
    QTimer* autoSaveTimer;
    QTimer* watcherTimer;
    bool autoSaveEnabled;
    bool configWatcherEnabled;
    bool configChanged;
    
    // Default values
    static const QJsonObject defaultConfiguration;
    static const int AUTO_SAVE_INTERVAL = 30000; // 30 seconds
    static const int WATCHER_INTERVAL = 5000;    // 5 seconds
    
    // Security constants
    static const int MIN_PASSWORD_LENGTH = 12;
    static const int MIN_SESSION_TIMEOUT = 300;   // 5 minutes
    static const int MAX_SESSION_TIMEOUT = 28800; // 8 hours
    static const int MIN_LOCKOUT_DURATION = 300;  // 5 minutes
    static const int MAX_LOGIN_ATTEMPTS = 5;
};

#endif // CONFIGMANAGER_H
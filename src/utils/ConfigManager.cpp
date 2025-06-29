#include "ConfigManager.h"
#include "ErrorHandler.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QCoreApplication>
#include <QRandomGenerator>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/aes.h>

ConfigManager* ConfigManager::instance = nullptr;
QMutex ConfigManager::instanceMutex;

// Default configuration template
const QJsonObject ConfigManager::defaultConfiguration = QJsonObject{
    {"application", QJsonObject{
        {"name", "VMS - Visitor Management System"},
        {"version", "1.0.0"},
        {"mode", "production"},
        {"debug", false}
    }},
    {"security", QJsonObject{
        {"session_timeout", 1800},
        {"max_login_attempts", 3},
        {"lockout_duration", 900},
        {"password_min_length", 12},
        {"require_strong_passwords", true},
        {"enable_2fa", false},
        {"allowed_ip_addresses", QJsonArray()}
    }},
    {"database", QJsonObject{
        {"type", "sqlite"},
        {"path", "vms.db"},
        {"encryption_enabled", true},
        {"backup_enabled", true},
        {"backup_interval", 24}
    }},
    {"logging", QJsonObject{
        {"level", "info"},
        {"enable_file_logging", true},
        {"enable_database_logging", true},
        {"enable_audit_logging", true},
        {"max_log_file_size", 10485760},
        {"max_log_files", 10},
        {"log_directory", "logs"}
    }},
    {"pdpa", QJsonObject{
        {"data_retention_period", 2555},
        {"enable_data_minimization", true},
        {"require_consent", true},
        {"privacy_policy_version", "1.0"},
        {"enable_data_portability", true},
        {"enable_right_to_be_forgotten", true}
    }},
    {"ui", QJsonObject{
        {"theme", "light"},
        {"language", "en"},
        {"enable_tooltips", true},
        {"auto_refresh_interval", 30}
    }}
};

ConfigManager& ConfigManager::getInstance()
{
    if (instance == nullptr) {
        QMutexLocker locker(&instanceMutex);
        if (instance == nullptr) {
            instance = new ConfigManager();
        }
    }
    return *instance;
}

ConfigManager::ConfigManager(QObject *parent)
    : QObject(parent)
    , autoSaveTimer(new QTimer(this))
    , watcherTimer(new QTimer(this))
    , autoSaveEnabled(false)
    , configWatcherEnabled(false)
    , configurationChanged(false)
{
    // Setup timers
    autoSaveTimer->setSingleShot(false);
    autoSaveTimer->setInterval(AUTO_SAVE_INTERVAL);
    connect(autoSaveTimer, &QTimer::timeout, this, &ConfigManager::onAutoSave);
    
    watcherTimer->setSingleShot(false);
    watcherTimer->setInterval(WATCHER_INTERVAL);
    connect(watcherTimer, &QTimer::timeout, this, &ConfigManager::onConfigurationFileChanged);
}

ConfigManager::~ConfigManager()
{
    if (autoSaveEnabled && configurationChanged) {
        saveConfiguration();
    }
}

bool ConfigManager::initialize()
{
    QMutexLocker locker(&configMutex);
    
    try {
        // Create secure configuration directory
        createSecureConfigDirectory();
        
        // Set configuration file paths
        QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
        configFilePath = appDataPath + "/config.json";
        secureConfigFilePath = appDataPath + "/secure_config.dat";
        
        // Generate or load encryption key
        QString keyPath = appDataPath + "/config.key";
        QFile keyFile(keyPath);
        
        if (keyFile.exists()) {
            if (keyFile.open(QIODevice::ReadOnly)) {
                encryptionKey = keyFile.readAll();
                keyFile.close();
            } else {
                throw std::runtime_error("Failed to read encryption key file");
            }
        } else {
            // Generate new encryption key
            unsigned char keyBytes[32];
            if (RAND_bytes(keyBytes, sizeof(keyBytes)) != 1) {
                throw std::runtime_error("Failed to generate encryption key");
            }
            
            encryptionKey = QByteArray(reinterpret_cast<const char*>(keyBytes), sizeof(keyBytes)).toBase64();
            
            if (keyFile.open(QIODevice::WriteOnly)) {
                keyFile.write(encryptionKey.toUtf8());
                keyFile.close();
                setConfigurationFilePermissions(keyPath);
            } else {
                throw std::runtime_error("Failed to save encryption key file");
            }
        }
        
        // Load existing configuration or create default
        if (!loadConfiguration()) {
            LOG_WARNING("ConfigManager", "Failed to load configuration, using defaults");
            setupDefaults();
        }
        
        // Validate configuration
        if (!validateConfiguration()) {
            LOG_ERROR("ConfigManager", "Configuration validation failed");
            return false;
        }
        
        // Generate configuration hash for integrity checking
        configurationHash = generateConfigurationHash();
        
        LOG_INFO("ConfigManager", "Configuration manager initialized successfully");
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("ConfigManager", QString("Initialization failed: %1").arg(e.what()));
        return false;
    }
}

bool ConfigManager::loadConfiguration(const QString& configFile)
{
    QMutexLocker locker(&configMutex);
    
    QString filePath = configFile.isEmpty() ? configFilePath : configFile;
    
    if (!QFile::exists(filePath)) {
        LOG_INFO("ConfigManager", "Configuration file does not exist, will create default");
        setupDefaults();
        return true;
    }
    
    if (!isConfigurationFileSecure(filePath)) {
        LOG_ERROR("ConfigManager", "Configuration file has insecure permissions");
        return false;
    }
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        LOG_ERROR("ConfigManager", QString("Failed to open configuration file: %1").arg(file.errorString()));
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        LOG_ERROR("ConfigManager", QString("Failed to parse configuration: %1").arg(error.errorString()));
        return false;
    }
    
    configData = doc.object();
    
    // Load secure configuration
    if (QFile::exists(secureConfigFilePath)) {
        QFile secureFile(secureConfigFilePath);
        if (secureFile.open(QIODevice::ReadOnly)) {
            QByteArray encryptedData = secureFile.readAll();
            secureFile.close();
            
            // Decrypt secure configuration
            QString decryptedJson = decryptValue(QString::fromUtf8(encryptedData));
            if (!decryptedJson.isEmpty()) {
                QJsonDocument secureDoc = QJsonDocument::fromJson(decryptedJson.toUtf8());
                if (!secureDoc.isNull()) {
                    secureConfigData = secureDoc.object();
                }
            }
        }
    }
    
    emit configurationReloaded();
    LOG_INFO("ConfigManager", "Configuration loaded successfully");
    return true;
}

bool ConfigManager::saveConfiguration()
{
    QMutexLocker locker(&configMutex);
    
    try {
        // Save regular configuration
        QJsonDocument doc(configData);
        QFile file(configFilePath);
        
        if (!file.open(QIODevice::WriteOnly)) {
            throw std::runtime_error(QString("Failed to open config file for writing: %1").arg(file.errorString()).toStdString());
        }
        
        file.write(doc.toJson());
        file.close();
        
        setConfigurationFilePermissions(configFilePath);
        
        // Save secure configuration
        if (!secureConfigData.isEmpty()) {
            QJsonDocument secureDoc(secureConfigData);
            QString encryptedData = encryptValue(secureDoc.toJson());
            
            QFile secureFile(secureConfigFilePath);
            if (secureFile.open(QIODevice::WriteOnly)) {
                secureFile.write(encryptedData.toUtf8());
                secureFile.close();
                setConfigurationFilePermissions(secureConfigFilePath);
            }
        }
        
        // Update configuration hash
        configurationHash = generateConfigurationHash();
        configurationChanged = false;
        
        LOG_INFO("ConfigManager", "Configuration saved successfully");
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("ConfigManager", QString("Failed to save configuration: %1").arg(e.what()));
        return false;
    }
}

QVariant ConfigManager::getValue(const QString& key, const QVariant& defaultValue) const
{
    QMutexLocker locker(&configMutex);
    
    QStringList keyParts = key.split('.');
    QJsonValue value = configData;
    
    for (const QString& part : keyParts) {
        if (value.isObject()) {
            value = value.toObject().value(part);
        } else {
            return defaultValue;
        }
    }
    
    return value.isUndefined() ? defaultValue : value.toVariant();
}

bool ConfigManager::setValue(const QString& key, const QVariant& value)
{
    QMutexLocker locker(&configMutex);
    
    QStringList keyParts = key.split('.');
    QJsonObject* current = &configData;
    
    for (int i = 0; i < keyParts.size() - 1; ++i) {
        const QString& part = keyParts[i];
        if (!current->contains(part)) {
            current->insert(part, QJsonObject());
        }
        
        QJsonValue& val = (*current)[part];
        if (!val.isObject()) {
            val = QJsonObject();
        }
        current = &val.toObject();
    }
    
    current->insert(keyParts.last(), QJsonValue::fromVariant(value));
    configurationChanged = true;
    
    emit configurationChanged(key, value);
    return true;
}

QVariant ConfigManager::getSecureValue(const QString& key, const QVariant& defaultValue) const
{
    QMutexLocker locker(&configMutex);
    
    QStringList keyParts = key.split('.');
    QJsonValue value = secureConfigData;
    
    for (const QString& part : keyParts) {
        if (value.isObject()) {
            value = value.toObject().value(part);
        } else {
            return defaultValue;
        }
    }
    
    return value.isUndefined() ? defaultValue : value.toVariant();
}

bool ConfigManager::setSecureValue(const QString& key, const QVariant& value)
{
    QMutexLocker locker(&configMutex);
    
    QStringList keyParts = key.split('.');
    QJsonObject* current = &secureConfigData;
    
    for (int i = 0; i < keyParts.size() - 1; ++i) {
        const QString& part = keyParts[i];
        if (!current->contains(part)) {
            current->insert(part, QJsonObject());
        }
        
        QJsonValue& val = (*current)[part];
        if (!val.isObject()) {
            val = QJsonObject();
        }
        current = &val.toObject();
    }
    
    current->insert(keyParts.last(), QJsonValue::fromVariant(value));
    configurationChanged = true;
    
    emit configurationChanged(key, value);
    return true;
}

void ConfigManager::setupDefaults()
{
    configData = defaultConfiguration;
    configurationChanged = true;
}

QString ConfigManager::encryptValue(const QString& plaintext) const
{
    // Simplified encryption for demo - in production use proper AES encryption
    QByteArray key = QCryptographicHash::hash(encryptionKey.toUtf8(), QCryptographicHash::Sha256);
    QByteArray data = plaintext.toUtf8();
    
    // XOR encryption (replace with AES in production)
    for (int i = 0; i < data.size(); ++i) {
        data[i] = data[i] ^ key[i % key.size()];
    }
    
    return data.toBase64();
}

QString ConfigManager::decryptValue(const QString& ciphertext) const
{
    QByteArray key = QCryptographicHash::hash(encryptionKey.toUtf8(), QCryptographicHash::Sha256);
    QByteArray data = QByteArray::fromBase64(ciphertext.toUtf8());
    
    // XOR decryption (replace with AES in production)
    for (int i = 0; i < data.size(); ++i) {
        data[i] = data[i] ^ key[i % key.size()];
    }
    
    return QString::fromUtf8(data);
}

void ConfigManager::createSecureConfigDirectory()
{
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir dir;
    
    if (!dir.exists(appDataPath)) {
        if (!dir.mkpath(appDataPath)) {
            throw std::runtime_error("Failed to create configuration directory");
        }
    }
    
    // Set directory permissions (Unix-like systems)
#ifdef Q_OS_UNIX
    QFile::setPermissions(appDataPath, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
#endif
}

bool ConfigManager::validateConfiguration() const
{
    return validateSecuritySettings() &&
           validateDatabaseSettings() &&
           validateLoggingSettings() &&
           validateNetworkSettings();
}

bool ConfigManager::validateSecuritySettings() const
{
    int sessionTimeout = getValue("security.session_timeout", 1800).toInt();
    int maxLoginAttempts = getValue("security.max_login_attempts", 3).toInt();
    int lockoutDuration = getValue("security.lockout_duration", 900).toInt();
    int passwordMinLength = getValue("security.password_min_length", 12).toInt();
    
    if (sessionTimeout < MIN_SESSION_TIMEOUT || sessionTimeout > MAX_SESSION_TIMEOUT) {
        LOG_ERROR("ConfigManager", QString("Invalid session timeout: %1").arg(sessionTimeout));
        return false;
    }
    
    if (maxLoginAttempts < 1 || maxLoginAttempts > MAX_LOGIN_ATTEMPTS) {
        LOG_ERROR("ConfigManager", QString("Invalid max login attempts: %1").arg(maxLoginAttempts));
        return false;
    }
    
    if (lockoutDuration < MIN_LOCKOUT_DURATION) {
        LOG_ERROR("ConfigManager", QString("Invalid lockout duration: %1").arg(lockoutDuration));
        return false;
    }
    
    if (passwordMinLength < MIN_PASSWORD_LENGTH) {
        LOG_ERROR("ConfigManager", QString("Invalid password minimum length: %1").arg(passwordMinLength));
        return false;
    }
    
    return true;
}

bool ConfigManager::validateDatabaseSettings() const
{
    QString dbType = getValue("database.type", "sqlite").toString();
    QString dbPath = getValue("database.path", "vms.db").toString();
    
    if (dbType.isEmpty() || dbPath.isEmpty()) {
        LOG_ERROR("ConfigManager", "Database type or path is empty");
        return false;
    }
    
    return true;
}

bool ConfigManager::validateLoggingSettings() const
{
    QString logLevel = getValue("logging.level", "info").toString();
    int maxLogFileSize = getValue("logging.max_log_file_size", 10485760).toInt();
    int maxLogFiles = getValue("logging.max_log_files", 10).toInt();
    
    QStringList validLogLevels = {"debug", "info", "warning", "error", "critical"};
    if (!validLogLevels.contains(logLevel.toLower())) {
        LOG_ERROR("ConfigManager", QString("Invalid log level: %1").arg(logLevel));
        return false;
    }
    
    if (maxLogFileSize <= 0 || maxLogFiles <= 0) {
        LOG_ERROR("ConfigManager", "Invalid log file size or count");
        return false;
    }
    
    return true;
}

bool ConfigManager::validateNetworkSettings() const
{
    // Add network validation as needed
    return true;
}

bool ConfigManager::isConfigurationFileSecure(const QString& filePath) const
{
    QFileInfo fileInfo(filePath);
    
    if (!fileInfo.exists()) {
        return true; // Non-existent file is considered secure
    }
    
    // Check file permissions
    QFile::Permissions perms = fileInfo.permissions();
    
    // File should only be readable/writable by owner
    if (perms & (QFile::ReadGroup | QFile::WriteGroup | QFile::ReadOther | QFile::WriteOther)) {
        return false;
    }
    
    return true;
}

bool ConfigManager::setConfigurationFilePermissions(const QString& filePath) const
{
#ifdef Q_OS_UNIX
    return QFile::setPermissions(filePath, QFile::ReadOwner | QFile::WriteOwner);
#else
    Q_UNUSED(filePath)
    return true;
#endif
}

QString ConfigManager::generateConfigurationHash() const
{
    QJsonDocument doc(configData);
    QJsonDocument secureDoc(secureConfigData);
    
    QByteArray combinedData = doc.toJson() + secureDoc.toJson();
    return QCryptographicHash::hash(combinedData, QCryptographicHash::Sha256).toHex();
}

void ConfigManager::enableAutoSave(bool enabled)
{
    autoSaveEnabled = enabled;
    if (enabled) {
        autoSaveTimer->start();
    } else {
        autoSaveTimer->stop();
    }
}

void ConfigManager::onAutoSave()
{
    if (configurationChanged) {
        saveConfiguration();
    }
}

void ConfigManager::onConfigurationFileChanged()
{
    // Check if configuration file has been modified externally
    QString currentHash = generateConfigurationHash();
    if (currentHash != configurationHash) {
        LOG_WARNING("ConfigManager", "Configuration file modified externally");
        loadConfiguration();
    }
}

// Getter methods for common configuration values
int ConfigManager::getSessionTimeout() const
{
    return getValue("security.session_timeout", 1800).toInt();
}

int ConfigManager::getMaxLoginAttempts() const
{
    return getValue("security.max_login_attempts", 3).toInt();
}

int ConfigManager::getLockoutDuration() const
{
    return getValue("security.lockout_duration", 900).toInt();
}

QString ConfigManager::getLogLevel() const
{
    return getValue("logging.level", "info").toString();
}

bool ConfigManager::isAuditLoggingEnabled() const
{
    return getValue("logging.enable_audit_logging", true).toBool();
}

int ConfigManager::getDataRetentionPeriod() const
{
    return getValue("pdpa.data_retention_period", 2555).toInt();
}

QString ConfigManager::getDatabaseEncryptionKey() const
{
    return getSecureValue("database.encryption_key").toString();
}

bool ConfigManager::setDatabaseEncryptionKey(const QString& key)
{
    return setSecureValue("database.encryption_key", key);
}
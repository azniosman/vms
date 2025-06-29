#include "SecurityManager.h"
#include <QCryptographicHash>
#include <QDateTime>
#include <QUuid>
#include <QDebug>
#include <QSettings>
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <openssl/kdf.h>
#include <openssl/err.h>
#include "../database/DatabaseManager.h"
#include "../utils/ErrorHandler.h"

SecurityManager* SecurityManager::instance = nullptr;
QMutex SecurityManager::instanceMutex;

SecurityManager& SecurityManager::getInstance()
{
    if (instance == nullptr) {
        QMutexLocker locker(&instanceMutex);
        if (instance == nullptr) {
            instance = new SecurityManager();
        }
    }
    return *instance;
}

SecurityManager::SecurityManager(QObject *parent)
    : QObject(parent)
{
    // Initialize session cleanup timer
    sessionCleanupTimer = std::make_unique<QTimer>(this);
    connect(sessionCleanupTimer.get(), &QTimer::timeout, this, &SecurityManager::cleanExpiredSessions);
    sessionCleanupTimer->start(60000); // Clean every minute
    
    // Generate encryption key if not exists
    QSettings settings;
    encryptionKey = settings.value("security/encryption_key").toString();
    if (encryptionKey.isEmpty()) {
        encryptionKey = generateSecureKey();
        settings.setValue("security/encryption_key", encryptionKey);
    }
}

SecurityManager::~SecurityManager()
{
}

bool SecurityManager::initialize()
{
    // Initialize OpenSSL
    OpenSSL_add_all_algorithms();
    
    // Generate random seed
    unsigned char seed[32];
    if (RAND_bytes(seed, sizeof(seed)) != 1) {
        ErrorHandler::getInstance().logError("SecurityManager", "Failed to generate random seed");
        return false;
    }
    
    RAND_seed(seed, sizeof(seed));
    
    // Create default admin user if none exists
    if (getUserByUsername("admin").username.isEmpty()) {
        QString defaultPassword = "TempAdmin123!@#";
        if (!createUser("admin", defaultPassword, UserRole::SuperAdmin)) {
            ErrorHandler::getInstance().logError("SecurityManager", "Failed to create default admin user");
            return false;
        }
        
        qWarning() << "Default admin user created with password:" << defaultPassword;
        qWarning() << "CHANGE THIS PASSWORD IMMEDIATELY!";
    }
    
    logSecurityEvent("SYSTEM_INIT", "Security Manager initialized");
    return true;
}

QString SecurityManager::authenticate(const QString& username, const QString& password, const QString& ipAddress)
{
    // Check if user is locked
    if (isUserLocked(username)) {
        logSecurityEvent("AUTH_FAILED", QString("User %1 is locked out from IP %2").arg(username, ipAddress));
        return QString();
    }
    
    // Get user from database
    User user = getUserByUsername(username);
    if (user.username.isEmpty()) {
        logSecurityEvent("AUTH_FAILED", QString("User %1 not found from IP %2").arg(username, ipAddress));
        return QString();
    }
    
    // Check if user is active
    if (!user.isActive) {
        logSecurityEvent("AUTH_FAILED", QString("User %1 is inactive from IP %2").arg(username, ipAddress));
        return QString();
    }
    
    // Verify password
    if (!verifyPassword(password, user.passwordHash, user.salt)) {
        incrementFailedAttempts(username);
        logSecurityEvent("AUTH_FAILED", QString("Invalid password for user %1 from IP %2").arg(username, ipAddress));
        return QString();
    }
    
    // Reset failed attempts on successful login
    resetFailedAttempts(username);
    
    // Create session
    QString sessionId = generateSessionId();
    QDateTime now = QDateTime::currentDateTime();
    
    UserSession session;
    session.userId = user.userId;
    session.sessionId = sessionId;
    session.role = user.role;
    session.lastActivity = now;
    session.loginTime = now;
    session.ipAddress = ipAddress;
    session.failedAttempts = 0;
    
    QMutexLocker locker(&sessionMutex);
    activeSessions.insert(sessionId, session);
    
    // Update user's last login
    user.lastLogin = now;
    saveUser(user);
    
    logSecurityEvent("AUTH_SUCCESS", QString("User %1 logged in from IP %2").arg(username, ipAddress));
    return sessionId;
}

bool SecurityManager::logout(const QString& sessionId)
{
    QMutexLocker locker(&sessionMutex);
    return activeSessions.remove(sessionId) > 0;
}

bool SecurityManager::validateSession(const QString& sessionId)
{
    QMutexLocker locker(&sessionMutex);
    
    if (!activeSessions.contains(sessionId)) {
        return false;
    }
    
    UserSession& session = activeSessions[sessionId];
    QDateTime now = QDateTime::currentDateTime();
    
    if (session.lastActivity.addSecs(SESSION_TIMEOUT_MINUTES * 60) < now) {
        activeSessions.remove(sessionId);
        return false;
    }
    
    session.lastActivity = now;
    return true;
}

bool SecurityManager::changePassword(const QString& userId, const QString& oldPassword, 
                                  const QString& newPassword)
{
    // TODO: Implement password change logic
    return false;
}

bool SecurityManager::hasPermission(const QString& sessionId, const QString& resource, 
                                  const QString& action)
{
    QMutexLocker locker(&sessionMutex);
    
    if (!activeSessions.contains(sessionId)) {
        return false;
    }
    
    const UserSession& session = activeSessions[sessionId];
    
    // TODO: Implement proper role-based access control
    // This is a basic implementation
    switch (session.role) {
        case UserRole::SuperAdmin:
            return true;
            
        case UserRole::Administrator:
            return resource != "system_config";
            
        case UserRole::Receptionist:
            return resource == "visitor" && 
                   (action == "register" || action == "checkin" || action == "checkout");
            
        case UserRole::SecurityGuard:
            return resource == "visitor" && action == "view";
            
        default:
            return false;
    }
}

UserRole SecurityManager::getUserRole(const QString& sessionId)
{
    QMutexLocker locker(&sessionMutex);
    
    if (!activeSessions.contains(sessionId)) {
        throw std::runtime_error("Invalid session");
    }
    
    return activeSessions[sessionId].role;
}

void SecurityManager::updateLastActivity(const QString& sessionId)
{
    QMutexLocker locker(&sessionMutex);
    
    if (activeSessions.contains(sessionId)) {
        activeSessions[sessionId].lastActivity = QDateTime::currentDateTime();
    }
}

void SecurityManager::cleanExpiredSessions()
{
    QMutexLocker locker(&sessionMutex);
    QDateTime now = QDateTime::currentDateTime();
    
    QHash<QString, UserSession>::iterator it = activeSessions.begin();
    while (it != activeSessions.end()) {
        if (it.value().lastActivity.addSecs(SESSION_TIMEOUT_MINUTES * 60) < now) {
            it = activeSessions.erase(it);
        } else {
            ++it;
        }
    }
}

QString SecurityManager::hashPassword(const QString& password, const QString& salt)
{
    QString actualSalt = salt.isEmpty() ? generateSalt() : salt;
    
    // Use PBKDF2 as a more secure alternative to plain SHA-256
    // In production, consider using Argon2id
    QByteArray saltBytes = QByteArray::fromBase64(actualSalt.toLatin1());
    QByteArray passwordBytes = password.toUtf8();
    
    // PBKDF2 with 100,000 iterations
    const int iterations = 100000;
    const int keyLength = 32;
    
    unsigned char hash[keyLength];
    
    int result = PKCS5_PBKDF2_HMAC(
        passwordBytes.constData(), passwordBytes.length(),
        reinterpret_cast<const unsigned char*>(saltBytes.constData()), saltBytes.length(),
        iterations,
        EVP_sha256(),
        keyLength,
        hash
    );
    
    if (result != 1) {
        ErrorHandler::getInstance().logError("SecurityManager", "Password hashing failed");
        return QString();
    }
    
    QByteArray hashBytes(reinterpret_cast<const char*>(hash), keyLength);
    return hashBytes.toBase64();
}

bool SecurityManager::verifyPassword(const QString& password, const QString& hash, const QString& salt)
{
    QString computedHash = hashPassword(password, salt);
    return computedHash == hash;
}

QString SecurityManager::encrypt(const QString& data, const QString& key)
{
    QString actualKey = key.isEmpty() ? encryptionKey : key;
    
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        ErrorHandler::getInstance().logError("SecurityManager", "Failed to create encryption context");
        return QString();
    }
    
    // Generate random IV
    unsigned char iv[16];
    if (RAND_bytes(iv, sizeof(iv)) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        ErrorHandler::getInstance().logError("SecurityManager", "Failed to generate IV");
        return QString();
    }
    
    // Initialize encryption
    QByteArray keyBytes = QByteArray::fromBase64(actualKey.toLatin1());
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, 
                          reinterpret_cast<const unsigned char*>(keyBytes.constData()), iv) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        ErrorHandler::getInstance().logError("SecurityManager", "Failed to initialize encryption");
        return QString();
    }
    
    QByteArray plaintext = data.toUtf8();
    QByteArray ciphertext;
    ciphertext.resize(plaintext.length() + EVP_CIPHER_block_size(EVP_aes_256_cbc()));
    
    int len;
    if (EVP_EncryptUpdate(ctx, reinterpret_cast<unsigned char*>(ciphertext.data()), &len,
                         reinterpret_cast<const unsigned char*>(plaintext.constData()), plaintext.length()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        ErrorHandler::getInstance().logError("SecurityManager", "Encryption failed");
        return QString();
    }
    
    int ciphertext_len = len;
    
    if (EVP_EncryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(ciphertext.data()) + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        ErrorHandler::getInstance().logError("SecurityManager", "Encryption finalization failed");
        return QString();
    }
    
    ciphertext_len += len;
    ciphertext.resize(ciphertext_len);
    
    EVP_CIPHER_CTX_free(ctx);
    
    // Prepend IV to ciphertext
    QByteArray result(reinterpret_cast<const char*>(iv), sizeof(iv));
    result.append(ciphertext);
    
    return result.toBase64();
}

QString SecurityManager::decrypt(const QString& encryptedData, const QString& key)
{
    QString actualKey = key.isEmpty() ? encryptionKey : key;
    
    QByteArray data = QByteArray::fromBase64(encryptedData.toLatin1());
    if (data.length() < 16) {
        ErrorHandler::getInstance().logError("SecurityManager", "Invalid encrypted data");
        return QString();
    }
    
    // Extract IV and ciphertext
    QByteArray iv = data.left(16);
    QByteArray ciphertext = data.mid(16);
    
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        ErrorHandler::getInstance().logError("SecurityManager", "Failed to create decryption context");
        return QString();
    }
    
    // Initialize decryption
    QByteArray keyBytes = QByteArray::fromBase64(actualKey.toLatin1());
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr,
                          reinterpret_cast<const unsigned char*>(keyBytes.constData()),
                          reinterpret_cast<const unsigned char*>(iv.constData())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        ErrorHandler::getInstance().logError("SecurityManager", "Failed to initialize decryption");
        return QString();
    }
    
    QByteArray plaintext;
    plaintext.resize(ciphertext.length() + EVP_CIPHER_block_size(EVP_aes_256_cbc()));
    
    int len;
    if (EVP_DecryptUpdate(ctx, reinterpret_cast<unsigned char*>(plaintext.data()), &len,
                         reinterpret_cast<const unsigned char*>(ciphertext.constData()), ciphertext.length()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        ErrorHandler::getInstance().logError("SecurityManager", "Decryption failed");
        return QString();
    }
    
    int plaintext_len = len;
    
    if (EVP_DecryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(plaintext.data()) + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        ErrorHandler::getInstance().logError("SecurityManager", "Decryption finalization failed");
        return QString();
    }
    
    plaintext_len += len;
    plaintext.resize(plaintext_len);
    
    EVP_CIPHER_CTX_free(ctx);
    
    return QString::fromUtf8(plaintext);
}

bool SecurityManager::isIpWhitelisted(const QString& ipAddress)
{
    QSettings settings;
    QStringList whitelist = settings.value("security/ip_whitelist").toStringList();
    return whitelist.contains(ipAddress);
}

bool SecurityManager::addIpToWhitelist(const QString& ipAddress)
{
    QSettings settings;
    QStringList whitelist = settings.value("security/ip_whitelist").toStringList();
    
    if (!whitelist.contains(ipAddress)) {
        whitelist.append(ipAddress);
        settings.setValue("security/ip_whitelist", whitelist);
    }
    
    return true;
}

bool SecurityManager::removeIpFromWhitelist(const QString& ipAddress)
{
    QSettings settings;
    QStringList whitelist = settings.value("security/ip_whitelist").toStringList();
    
    if (whitelist.removeOne(ipAddress)) {
        settings.setValue("security/ip_whitelist", whitelist);
        return true;
    }
    
    return false;
}

bool SecurityManager::createUser(const QString& username, const QString& password, UserRole role)
{
    if (!validatePasswordStrength(password)) {
        ErrorHandler::getInstance().logError("SecurityManager", "Password does not meet strength requirements");
        return false;
    }
    
    // Check if user already exists
    if (!getUserByUsername(username).username.isEmpty()) {
        ErrorHandler::getInstance().logError("SecurityManager", QString("User %1 already exists").arg(username));
        return false;
    }
    
    QString salt = generateSalt();
    QString hash = hashPassword(password, salt);
    
    if (hash.isEmpty()) {
        return false;
    }
    
    User user;
    user.userId = QUuid::createUuid().toString();
    user.username = username;
    user.passwordHash = hash;
    user.salt = salt;
    user.role = role;
    user.isActive = true;
    user.createdAt = QDateTime::currentDateTime();
    user.failedLoginAttempts = 0;
    
    bool success = saveUser(user);
    if (success) {
        logSecurityEvent("USER_CREATED", QString("User %1 created with role %2").arg(username).arg(static_cast<int>(role)));
    }
    
    return success;
}

bool SecurityManager::isUserLocked(const QString& username)
{
    User user = getUserByUsername(username);
    if (user.username.isEmpty()) {
        return false;
    }
    
    if (user.failedLoginAttempts >= MAX_LOGIN_ATTEMPTS) {
        if (user.lockoutUntil.isValid() && user.lockoutUntil > QDateTime::currentDateTime()) {
            return true;
        }
    }
    
    return false;
}

void SecurityManager::incrementFailedAttempts(const QString& username)
{
    User user = getUserByUsername(username);
    if (user.username.isEmpty()) {
        return;
    }
    
    user.failedLoginAttempts++;
    if (user.failedLoginAttempts >= MAX_LOGIN_ATTEMPTS) {
        user.lockoutUntil = QDateTime::currentDateTime().addSecs(LOGIN_LOCKOUT_MINUTES * 60);
    }
    
    saveUser(user);
}

void SecurityManager::resetFailedAttempts(const QString& username)
{
    User user = getUserByUsername(username);
    if (user.username.isEmpty()) {
        return;
    }
    
    user.failedLoginAttempts = 0;
    user.lockoutUntil = QDateTime();
    saveUser(user);
}

QString SecurityManager::generateSalt()
{
    unsigned char salt[SALT_LENGTH];
    if (RAND_bytes(salt, sizeof(salt)) != 1) {
        ErrorHandler::getInstance().logError("SecurityManager", "Failed to generate salt");
        return QString();
    }
    
    return QByteArray(reinterpret_cast<const char*>(salt), sizeof(salt)).toBase64();
}

QString SecurityManager::generateSecureKey()
{
    unsigned char key[32]; // 256 bits
    if (RAND_bytes(key, sizeof(key)) != 1) {
        ErrorHandler::getInstance().logError("SecurityManager", "Failed to generate secure key");
        return QString();
    }
    
    return QByteArray(reinterpret_cast<const char*>(key), sizeof(key)).toBase64();
}

bool SecurityManager::validatePasswordStrength(const QString& password)
{
    if (password.length() < MIN_PASSWORD_LENGTH) {
        return false;
    }
    
    bool hasUpper = false, hasLower = false, hasDigit = false, hasSpecial = false;
    
    for (const QChar& ch : password) {
        if (ch.isUpper()) hasUpper = true;
        else if (ch.isLower()) hasLower = true;
        else if (ch.isDigit()) hasDigit = true;
        else if (!ch.isLetterOrNumber()) hasSpecial = true;
    }
    
    return hasUpper && hasLower && hasDigit && hasSpecial;
}

User SecurityManager::getUserByUsername(const QString& username)
{
    QMutexLocker locker(&userMutex);
    
    // Check cache first
    for (const User& user : userCache) {
        if (user.username == username) {
            return user;
        }
    }
    
    // Query database
    DatabaseManager& db = DatabaseManager::getInstance();
    QSqlQuery query(db.getDatabase());
    
    query.prepare("SELECT user_id, username, password_hash, salt, role, is_active, "
                  "last_login, created_at, failed_login_attempts, lockout_until "
                  "FROM users WHERE username = ?");
    query.addBindValue(username);
    
    if (!query.exec()) {
        ErrorHandler::getInstance().logError("SecurityManager", 
            QString("Failed to query user: %1").arg(query.lastError().text()));
        return User();
    }
    
    if (query.next()) {
        User user;
        user.userId = query.value(0).toString();
        user.username = query.value(1).toString();
        user.passwordHash = query.value(2).toString();
        user.salt = query.value(3).toString();
        user.role = static_cast<UserRole>(query.value(4).toInt());
        user.isActive = query.value(5).toBool();
        user.lastLogin = query.value(6).toDateTime();
        user.createdAt = query.value(7).toDateTime();
        user.failedLoginAttempts = query.value(8).toInt();
        user.lockoutUntil = query.value(9).toDateTime();
        
        // Cache the user
        userCache[username] = user;
        
        return user;
    }
    
    return User();
}

bool SecurityManager::saveUser(const User& user)
{
    DatabaseManager& db = DatabaseManager::getInstance();
    QSqlQuery query(db.getDatabase());
    
    // Check if user exists
    query.prepare("SELECT COUNT(*) FROM users WHERE user_id = ?");
    query.addBindValue(user.userId);
    
    if (!query.exec()) {
        ErrorHandler::getInstance().logError("SecurityManager", 
            QString("Failed to check user existence: %1").arg(query.lastError().text()));
        return false;
    }
    
    bool userExists = false;
    if (query.next()) {
        userExists = query.value(0).toInt() > 0;
    }
    
    if (userExists) {
        // Update existing user
        query.prepare("UPDATE users SET username = ?, password_hash = ?, salt = ?, "
                      "role = ?, is_active = ?, last_login = ?, failed_login_attempts = ?, "
                      "lockout_until = ? WHERE user_id = ?");
        query.addBindValue(user.username);
        query.addBindValue(user.passwordHash);
        query.addBindValue(user.salt);
        query.addBindValue(static_cast<int>(user.role));
        query.addBindValue(user.isActive);
        query.addBindValue(user.lastLogin);
        query.addBindValue(user.failedLoginAttempts);
        query.addBindValue(user.lockoutUntil);
        query.addBindValue(user.userId);
    } else {
        // Insert new user
        query.prepare("INSERT INTO users (user_id, username, password_hash, salt, role, "
                      "is_active, last_login, created_at, failed_login_attempts, lockout_until) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
        query.addBindValue(user.userId);
        query.addBindValue(user.username);
        query.addBindValue(user.passwordHash);
        query.addBindValue(user.salt);
        query.addBindValue(static_cast<int>(user.role));
        query.addBindValue(user.isActive);
        query.addBindValue(user.lastLogin);
        query.addBindValue(user.createdAt);
        query.addBindValue(user.failedLoginAttempts);
        query.addBindValue(user.lockoutUntil);
    }
    
    if (!query.exec()) {
        ErrorHandler::getInstance().logError("SecurityManager", 
            QString("Failed to save user: %1").arg(query.lastError().text()));
        return false;
    }
    
    // Update cache
    QMutexLocker locker(&userMutex);
    userCache[user.username] = user;
    
    return true;
}

QString SecurityManager::generateSessionId()
{
    // Generate a cryptographically secure session ID
    unsigned char sessionBytes[32];
    if (RAND_bytes(sessionBytes, sizeof(sessionBytes)) != 1) {
        ErrorHandler::getInstance().logError("SecurityManager", "Failed to generate session ID");
        return QUuid::createUuid().toString(); // Fallback
    }
    
    return QByteArray(reinterpret_cast<const char*>(sessionBytes), sizeof(sessionBytes)).toBase64();
}

void SecurityManager::logSecurityEvent(const QString& event, const QString& details)
{
    QDateTime now = QDateTime::currentDateTime();
    QString logEntry = QString("[%1] %2: %3")
                       .arg(now.toString(Qt::ISODate))
                       .arg(event)
                       .arg(details);
    
    ErrorHandler::getInstance().logInfo("SecurityManager", logEntry);
    
    // Store in database for audit trail
    DatabaseManager& db = DatabaseManager::getInstance();
    QSqlQuery query(db.getDatabase());
    
    query.prepare("INSERT INTO security_events (event_type, details, timestamp) VALUES (?, ?, ?)");
    query.addBindValue(event);
    query.addBindValue(details);
    query.addBindValue(now);
    
    if (!query.exec()) {
        ErrorHandler::getInstance().logError("SecurityManager", 
            QString("Failed to log security event: %1").arg(query.lastError().text()));
    }
} 
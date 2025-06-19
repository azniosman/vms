#include "SecurityManager.h"
#include <QCryptographicHash>
#include <QDateTime>
#include <QUuid>
#include <QDebug>
#include <QSettings>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/rand.h>

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
        qCritical() << "Failed to generate random seed";
        return false;
    }
    
    RAND_seed(seed, sizeof(seed));
    
    return true;
}

bool SecurityManager::authenticate(const QString& username, const QString& password)
{
    // TODO: Implement actual authentication against database
    // This is a placeholder implementation
    if (username == "admin" && password == "admin") {
        QString sessionId = QUuid::createUuid().toString();
        
        UserSession session;
        session.userId = "1";
        session.sessionId = sessionId;
        session.role = UserRole::Administrator;
        session.lastActivity = QDateTime::currentDateTime();
        session.ipAddress = "127.0.0.1"; // TODO: Get actual IP
        
        QMutexLocker locker(&sessionMutex);
        activeSessions.insert(sessionId, session);
        
        return true;
    }
    
    return false;
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

QString SecurityManager::hashPassword(const QString& password)
{
    // Use Argon2 or bcrypt in production
    // This is a basic implementation using SHA-256 for demonstration
    QByteArray salt = QUuid::createUuid().toByteArray();
    QByteArray passwordData = password.toUtf8();
    
    QByteArray salted = salt + passwordData;
    QByteArray hash = QCryptographicHash::hash(salted, QCryptographicHash::Sha256);
    
    return salt.toBase64() + ":" + hash.toBase64();
}

bool SecurityManager::verifyPassword(const QString& password, const QString& hash)
{
    QStringList parts = hash.split(':');
    if (parts.size() != 2) {
        return false;
    }
    
    QByteArray salt = QByteArray::fromBase64(parts[0].toLatin1());
    QByteArray storedHash = QByteArray::fromBase64(parts[1].toLatin1());
    
    QByteArray passwordData = password.toUtf8();
    QByteArray salted = salt + passwordData;
    QByteArray computedHash = QCryptographicHash::hash(salted, QCryptographicHash::Sha256);
    
    return storedHash == computedHash;
}

QString SecurityManager::encrypt(const QString& data)
{
    // TODO: Implement AES-256 encryption
    // This is a placeholder
    return data;
}

QString SecurityManager::decrypt(const QString& encryptedData)
{
    // TODO: Implement AES-256 decryption
    // This is a placeholder
    return encryptedData;
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
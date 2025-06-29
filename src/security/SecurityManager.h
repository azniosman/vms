#ifndef SECURITYMANAGER_H
#define SECURITYMANAGER_H

#include <QObject>
#include <QMutex>
#include <QHash>
#include <QString>
#include <QDateTime>
#include <QTimer>
#include <memory>

enum class UserRole {
    SuperAdmin,
    Administrator,
    Receptionist,
    SecurityGuard
};

struct UserSession {
    QString userId;
    QString sessionId;
    UserRole role;
    QDateTime lastActivity;
    QString ipAddress;
    QDateTime loginTime;
    int failedAttempts;
};

struct User {
    QString userId;
    QString username;
    QString passwordHash;
    QString salt;
    UserRole role;
    bool isActive;
    QDateTime lastLogin;
    QDateTime createdAt;
    int failedLoginAttempts;
    QDateTime lockoutUntil;
};

class SecurityManager : public QObject {
    Q_OBJECT

public:
    static SecurityManager& getInstance();
    
    bool initialize();
    
    // Authentication
    QString authenticate(const QString& username, const QString& password, const QString& ipAddress);
    bool logout(const QString& sessionId);
    bool validateSession(const QString& sessionId);
    bool changePassword(const QString& userId, const QString& oldPassword, const QString& newPassword);
    bool createUser(const QString& username, const QString& password, UserRole role);
    bool isUserLocked(const QString& username);
    void incrementFailedAttempts(const QString& username);
    void resetFailedAttempts(const QString& username);
    
    // Access Control
    bool hasPermission(const QString& sessionId, const QString& resource, const QString& action);
    UserRole getUserRole(const QString& sessionId);
    
    // Session Management
    void updateLastActivity(const QString& sessionId);
    void cleanExpiredSessions();
    
    // Security Features
    QString hashPassword(const QString& password, const QString& salt = QString());
    bool verifyPassword(const QString& password, const QString& hash, const QString& salt);
    QString encrypt(const QString& data, const QString& key = QString());
    QString decrypt(const QString& encryptedData, const QString& key = QString());
    QString generateSalt();
    QString generateSecureKey();
    bool validatePasswordStrength(const QString& password);
    
    // IP Whitelisting
    bool isIpWhitelisted(const QString& ipAddress);
    bool addIpToWhitelist(const QString& ipAddress);
    bool removeIpFromWhitelist(const QString& ipAddress);

private:
    explicit SecurityManager(QObject *parent = nullptr);
    ~SecurityManager();
    
    // Prevent copying
    SecurityManager(const SecurityManager&) = delete;
    SecurityManager& operator=(const SecurityManager&) = delete;
    
    // Private methods
    User getUserByUsername(const QString& username);
    bool saveUser(const User& user);
    QString generateSessionId();
    void logSecurityEvent(const QString& event, const QString& details);
    
    static SecurityManager* instance;
    static QMutex instanceMutex;
    
    QHash<QString, UserSession> activeSessions;
    QHash<QString, User> userCache;
    QMutex sessionMutex;
    QMutex userMutex;
    
    std::unique_ptr<QTimer> sessionCleanupTimer;
    
    // Security configuration
    QString encryptionKey;
    
    // Constants
    static const int SESSION_TIMEOUT_MINUTES = 30;
    static const int MAX_LOGIN_ATTEMPTS = 5;
    static const int LOGIN_LOCKOUT_MINUTES = 15;
    static const int MIN_PASSWORD_LENGTH = 12;
    static const int ARGON2_TIME_COST = 3;
    static const int ARGON2_MEMORY_COST = 65536;
    static const int ARGON2_PARALLELISM = 4;
    static const int ARGON2_TAG_LENGTH = 32;
    static const int SALT_LENGTH = 16;
};

#endif // SECURITYMANAGER_H 
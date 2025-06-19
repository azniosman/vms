#ifndef SECURITYMANAGER_H
#define SECURITYMANAGER_H

#include <QObject>
#include <QMutex>
#include <QHash>
#include <QString>
#include <QDateTime>

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
};

class SecurityManager : public QObject {
    Q_OBJECT

public:
    static SecurityManager& getInstance();
    
    bool initialize();
    
    // Authentication
    bool authenticate(const QString& username, const QString& password);
    bool logout(const QString& sessionId);
    bool validateSession(const QString& sessionId);
    bool changePassword(const QString& userId, const QString& oldPassword, const QString& newPassword);
    
    // Access Control
    bool hasPermission(const QString& sessionId, const QString& resource, const QString& action);
    UserRole getUserRole(const QString& sessionId);
    
    // Session Management
    void updateLastActivity(const QString& sessionId);
    void cleanExpiredSessions();
    
    // Security Features
    QString hashPassword(const QString& password);
    bool verifyPassword(const QString& password, const QString& hash);
    QString encrypt(const QString& data);
    QString decrypt(const QString& encryptedData);
    
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
    
    static SecurityManager* instance;
    static QMutex instanceMutex;
    
    QHash<QString, UserSession> activeSessions;
    QMutex sessionMutex;
    
    // Constants
    static const int SESSION_TIMEOUT_MINUTES = 30;
    static const int MAX_LOGIN_ATTEMPTS = 5;
    static const int LOGIN_LOCKOUT_MINUTES = 15;
};

#endif // SECURITYMANAGER_H 
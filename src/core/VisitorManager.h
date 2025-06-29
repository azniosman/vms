#ifndef VISITORMANAGER_H
#define VISITORMANAGER_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QList>
#include <QMutex>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QBuffer>
#include <QTextStream>
#include <QCoreApplication>
#include <QDebug>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QSqlError>
#include <QUuid>
#include <QTime>
#include <QDate>
#include <QRegularExpression>
#include <QValidator>
#include "Visitor.h"

class VisitorManager : public QObject {
    Q_OBJECT

public:
    static VisitorManager& getInstance();

    // Visitor registration
    bool registerVisitor(const Visitor& visitor);
    bool updateVisitor(const Visitor& visitor);
    bool deleteVisitor(const QString& visitorId);
    Visitor getVisitor(const QString& visitorId);
    QList<Visitor> getAllVisitors();
    QList<Visitor> searchVisitors(const QString& searchTerm);

    // Check-in/out operations
    bool checkInVisitor(const QString& visitorId, const QString& hostId = QString());
    bool checkOutVisitor(const QString& visitorId);
    bool isVisitorCheckedIn(const QString& visitorId);
    QList<Visitor> getCheckedInVisitors();
    QDateTime getCheckInTime(const QString& visitorId);
    QDateTime getCheckOutTime(const QString& visitorId);

    // Blacklist operations
    bool addToBlacklist(const QString& visitorId, const QString& reason);
    bool removeFromBlacklist(const QString& visitorId);
    bool isBlacklisted(const QString& visitorId);
    QList<QPair<QString, QString>> getBlacklist();

    // Consent operations
    bool recordConsent(const QString& visitorId, const QString& consentType, bool granted);
    bool hasValidConsent(const QString& visitorId, const QString& consentType);

    // Badge operations
    bool generateQRCode(const QString& visitorId);
    bool printVisitorBadge(const QString& visitorId);

    // Audit logging
    void logActivity(const QString& action, const QString& details, const QString& userId = QString());

    // Data retention
    void purgeExpiredRecords();
    bool exportVisitorData(const QString& visitorId, const QString& format);

    // Statistics
    int getTotalVisitorsToday();
    int getCurrentVisitorCount();
    QList<QPair<QDateTime, int>> getVisitorStatistics(const QDateTime& start, const QDateTime& end);

signals:
    void visitorRegistered(const QString& visitorId);
    void visitorUpdated(const QString& visitorId);
    void visitorDeleted(const QString& visitorId);
    void visitorCheckedIn(const QString& visitorId);
    void visitorCheckedOut(const QString& visitorId);
    void blacklistUpdated();
    void consentUpdated(const QString& visitorId, bool consent);

private:
    explicit VisitorManager(QObject *parent = nullptr);
    ~VisitorManager();

    // Prevent copying
    VisitorManager(const VisitorManager&) = delete;
    VisitorManager& operator=(const VisitorManager&) = delete;

    static VisitorManager* instance;
    static QMutex instanceMutex;

    // Helper methods
    bool validateVisitorData(const Visitor& visitor);
    void logVisitorActivity(const QString& visitorId, const QString& action);
    bool notifyHost(const QString& hostId, const QString& message);
    Visitor createVisitorFromQuery(const QSqlQuery& query);
    
    // Input validation methods
    bool validateEmail(const QString& email);
    bool validatePhoneNumber(const QString& phone);
    bool validateName(const QString& name);
    bool validateCompany(const QString& company);
    bool validateIdNumber(const QString& idNumber);
    bool validatePurpose(const QString& purpose);
    QString sanitizeInput(const QString& input);
    QString sanitizeHtml(const QString& input);
    bool isValidImageData(const QByteArray& data);
    bool isSecureFileName(const QString& fileName);
    
    // Security validation
    bool checkRateLimit(const QString& identifier);
    bool validateDataSize(const QByteArray& data, int maxSize);
    bool containsSqlInjection(const QString& input);
    bool containsXssAttempt(const QString& input);
    
    // Rate limiting
    QHash<QString, QDateTime> rateLimitTracker;
    QMutex rateLimitMutex;
    static const int MAX_REQUESTS_PER_MINUTE = 60;
    static const int MAX_NAME_LENGTH = 100;
    static const int MAX_COMPANY_LENGTH = 200;
    static const int MAX_PURPOSE_LENGTH = 500;
    static const int MAX_PHONE_LENGTH = 20;
    static const int MAX_EMAIL_LENGTH = 254;
    static const int MAX_ID_NUMBER_LENGTH = 50;
    static const int MAX_IMAGE_SIZE = 5 * 1024 * 1024; // 5MB
};

#endif // VISITORMANAGER_H 
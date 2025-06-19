#ifndef VISITORMANAGER_H
#define VISITORMANAGER_H

#include <QObject>
#include <QList>
#include <QDateTime>
#include "Visitor.h"

class VisitorManager : public QObject {
    Q_OBJECT

public:
    static VisitorManager& getInstance();

    // Visitor registration
    bool registerVisitor(Visitor& visitor);
    bool updateVisitor(const Visitor& visitor);
    bool deleteVisitor(const QString& visitorId);
    Visitor* getVisitor(const QString& visitorId);
    QList<Visitor*> searchVisitors(const QString& query);

    // Check-in/out operations
    bool checkIn(const QString& visitorId, const QString& hostId);
    bool checkOut(const QString& visitorId);
    QList<Visitor*> getCurrentVisitors();
    QDateTime getCheckInTime(const QString& visitorId);
    QDateTime getCheckOutTime(const QString& visitorId);

    // Blacklist operations
    bool addToBlacklist(const QString& visitorId, const QString& reason);
    bool removeFromBlacklist(const QString& visitorId);
    bool isBlacklisted(const QString& visitorId);
    QList<Visitor*> getBlacklistedVisitors();

    // Consent management
    bool updateConsent(const QString& visitorId, bool consent);
    bool hasValidConsent(const QString& visitorId);

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
    QString generateQRCode(const QString& visitorId);
    bool printVisitorBadge(const QString& visitorId);
};

#endif // VISITORMANAGER_H 
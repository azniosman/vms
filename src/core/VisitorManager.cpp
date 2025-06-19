#include "VisitorManager.h"
#include "../database/DatabaseManager.h"
#include "../security/SecurityManager.h"
#include "../utils/QRCodeGenerator.h"
#include "../utils/ErrorHandler.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QBuffer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>
#include <QDebug>

VisitorManager* VisitorManager::instance = nullptr;
QMutex VisitorManager::instanceMutex;

VisitorManager& VisitorManager::getInstance()
{
    if (instance == nullptr) {
        QMutexLocker locker(&instanceMutex);
        if (instance == nullptr) {
            instance = new VisitorManager();
        }
    }
    return *instance;
}

VisitorManager::VisitorManager(QObject *parent)
    : QObject(parent)
{
}

VisitorManager::~VisitorManager()
{
}

bool VisitorManager::registerVisitor(Visitor& visitor)
{
    if (!validateVisitorData(visitor)) {
        LOG_ERROR("Invalid visitor data", ErrorCategory::UserInput);
        return false;
    }

    QSqlDatabase db = DatabaseManager::getInstance().getConnection();
    QSqlQuery query(db);

    db.transaction();

    try {
        // Insert visitor data
        query.prepare("INSERT INTO visitors ("
                     "id, name, email, phone, company, identification_number, "
                     "type, photo, id_scan, signature, host_id, purpose, "
                     "created_at, updated_at, consent, retention_period) "
                     "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");

        QString visitorId = QUuid::createUuid().toString();
        visitor.setId(visitorId);

        QByteArray photoData;
        QBuffer photoBuffer(&photoData);
        photoBuffer.open(QIODevice::WriteOnly);
        visitor.getPhoto().save(&photoBuffer, "PNG");

        QByteArray idScanData;
        QBuffer idScanBuffer(&idScanData);
        idScanBuffer.open(QIODevice::WriteOnly);
        visitor.getIdScan().save(&idScanBuffer, "PNG");

        query.addBindValue(visitorId);
        query.addBindValue(visitor.getName());
        query.addBindValue(visitor.getEmail());
        query.addBindValue(visitor.getPhone());
        query.addBindValue(visitor.getCompany());
        query.addBindValue(visitor.getIdentificationNumber());
        query.addBindValue(static_cast<int>(visitor.getType()));
        query.addBindValue(photoData);
        query.addBindValue(idScanData);
        query.addBindValue(visitor.getSignature());
        query.addBindValue(visitor.getHostId());
        query.addBindValue(visitor.getPurpose());
        query.addBindValue(QDateTime::currentDateTime());
        query.addBindValue(QDateTime::currentDateTime());
        query.addBindValue(visitor.hasConsent());
        query.addBindValue(visitor.getRetentionPeriod());

        if (!query.exec()) {
            throw std::runtime_error(query.lastError().text().toStdString());
        }

        // Log the registration
        logVisitorActivity(visitorId, "REGISTER");

        // Generate QR code and print badge
        generateQRCode(visitorId);
        printVisitorBadge(visitorId);

        if (!db.commit()) {
            throw std::runtime_error("Failed to commit transaction");
        }

        emit visitorRegistered(visitorId);
        LOG_INFO(QString("Visitor registered successfully: %1").arg(visitorId), ErrorCategory::Database);
        return true;
    }
    catch (const std::exception& e) {
        db.rollback();
        LOG_ERROR(QString("Failed to register visitor: %1").arg(e.what()), ErrorCategory::Database);
        return false;
    }
}

bool VisitorManager::updateVisitor(const Visitor& visitor)
{
    if (!validateVisitorData(visitor)) {
        LOG_ERROR("Invalid visitor data for update", ErrorCategory::UserInput);
        return false;
    }

    QSqlDatabase db = DatabaseManager::getInstance().getConnection();
    QSqlQuery query(db);

    db.transaction();

    try {
        query.prepare("UPDATE visitors SET "
                     "name = ?, email = ?, phone = ?, company = ?, "
                     "identification_number = ?, type = ?, photo = ?, "
                     "id_scan = ?, signature = ?, host_id = ?, purpose = ?, "
                     "updated_at = ?, consent = ?, retention_period = ? "
                     "WHERE id = ?");

        QByteArray photoData;
        QBuffer photoBuffer(&photoData);
        photoBuffer.open(QIODevice::WriteOnly);
        visitor.getPhoto().save(&photoBuffer, "PNG");

        QByteArray idScanData;
        QBuffer idScanBuffer(&idScanData);
        idScanBuffer.open(QIODevice::WriteOnly);
        visitor.getIdScan().save(&idScanBuffer, "PNG");

        query.addBindValue(visitor.getName());
        query.addBindValue(visitor.getEmail());
        query.addBindValue(visitor.getPhone());
        query.addBindValue(visitor.getCompany());
        query.addBindValue(visitor.getIdentificationNumber());
        query.addBindValue(static_cast<int>(visitor.getType()));
        query.addBindValue(photoData);
        query.addBindValue(idScanData);
        query.addBindValue(visitor.getSignature());
        query.addBindValue(visitor.getHostId());
        query.addBindValue(visitor.getPurpose());
        query.addBindValue(QDateTime::currentDateTime());
        query.addBindValue(visitor.hasConsent());
        query.addBindValue(visitor.getRetentionPeriod());
        query.addBindValue(visitor.getId());

        if (!query.exec()) {
            throw std::runtime_error(query.lastError().text().toStdString());
        }

        logVisitorActivity(visitor.getId(), "UPDATE");

        if (!db.commit()) {
            throw std::runtime_error("Failed to commit transaction");
        }

        emit visitorUpdated(visitor.getId());
        LOG_INFO(QString("Visitor updated successfully: %1").arg(visitor.getId()), ErrorCategory::Database);
        return true;
    }
    catch (const std::exception& e) {
        db.rollback();
        LOG_ERROR(QString("Failed to update visitor: %1").arg(e.what()), ErrorCategory::Database);
        return false;
    }
}

bool VisitorManager::checkIn(const QString& visitorId, const QString& hostId)
{
    QSqlDatabase db = DatabaseManager::getInstance().getConnection();
    QSqlQuery query(db);

    db.transaction();

    try {
        // Check if visitor is blacklisted
        if (isBlacklisted(visitorId)) {
            throw std::runtime_error("Visitor is blacklisted");
        }

        // Insert check-in record
        query.prepare("INSERT INTO visits (id, visitor_id, host_id, check_in_time) "
                     "VALUES (?, ?, ?, ?)");

        QString visitId = QUuid::createUuid().toString();
        query.addBindValue(visitId);
        query.addBindValue(visitorId);
        query.addBindValue(hostId);
        query.addBindValue(QDateTime::currentDateTime());

        if (!query.exec()) {
            throw std::runtime_error(query.lastError().text().toStdString());
        }

        // Notify host
        notifyHost(hostId, "Your visitor has arrived");

        logVisitorActivity(visitorId, "CHECK_IN");

        if (!db.commit()) {
            throw std::runtime_error("Failed to commit transaction");
        }

        emit visitorCheckedIn(visitorId);
        LOG_INFO(QString("Visitor checked in successfully: %1").arg(visitorId), ErrorCategory::Database);
        return true;
    }
    catch (const std::exception& e) {
        db.rollback();
        LOG_ERROR(QString("Failed to check in visitor: %1").arg(e.what()), ErrorCategory::Database);
        return false;
    }
}

bool VisitorManager::checkOut(const QString& visitorId)
{
    QSqlDatabase db = DatabaseManager::getInstance().getConnection();
    QSqlQuery query(db);

    db.transaction();

    try {
        query.prepare("UPDATE visits SET check_out_time = ? "
                     "WHERE visitor_id = ? AND check_out_time IS NULL");

        query.addBindValue(QDateTime::currentDateTime());
        query.addBindValue(visitorId);

        if (!query.exec()) {
            throw std::runtime_error(query.lastError().text().toStdString());
        }

        logVisitorActivity(visitorId, "CHECK_OUT");

        if (!db.commit()) {
            throw std::runtime_error("Failed to commit transaction");
        }

        emit visitorCheckedOut(visitorId);
        LOG_INFO(QString("Visitor checked out successfully: %1").arg(visitorId), ErrorCategory::Database);
        return true;
    }
    catch (const std::exception& e) {
        db.rollback();
        LOG_ERROR(QString("Failed to check out visitor: %1").arg(e.what()), ErrorCategory::Database);
        return false;
    }
}

bool VisitorManager::addToBlacklist(const QString& visitorId, const QString& reason)
{
    QSqlDatabase db = DatabaseManager::getInstance().getConnection();
    QSqlQuery query(db);

    query.prepare("INSERT INTO blacklist (visitor_id, reason, created_at) "
                 "VALUES (?, ?, ?)");

    query.addBindValue(visitorId);
    query.addBindValue(reason);
    query.addBindValue(QDateTime::currentDateTime());

    if (!query.exec()) {
        LOG_ERROR(QString("Failed to add visitor to blacklist: %1").arg(query.lastError().text()), 
                 ErrorCategory::Database);
        return false;
    }

    logVisitorActivity(visitorId, "BLACKLIST_ADD");
    emit blacklistUpdated();
    LOG_INFO(QString("Visitor added to blacklist: %1").arg(visitorId), ErrorCategory::Security);
    return true;
}

bool VisitorManager::updateConsent(const QString& visitorId, bool consent)
{
    QSqlDatabase db = DatabaseManager::getInstance().getConnection();
    QSqlQuery query(db);

    query.prepare("UPDATE visitors SET consent = ?, updated_at = ? WHERE id = ?");

    query.addBindValue(consent);
    query.addBindValue(QDateTime::currentDateTime());
    query.addBindValue(visitorId);

    if (!query.exec()) {
        LOG_ERROR(QString("Failed to update visitor consent: %1").arg(query.lastError().text()), 
                 ErrorCategory::Database);
        return false;
    }

    logVisitorActivity(visitorId, consent ? "CONSENT_GRANTED" : "CONSENT_REVOKED");
    emit consentUpdated(visitorId, consent);
    LOG_INFO(QString("Visitor consent updated: %1 - %2").arg(visitorId).arg(consent ? "granted" : "revoked"), 
             ErrorCategory::Database);
    return true;
}

void VisitorManager::purgeExpiredRecords()
{
    QSqlDatabase db = DatabaseManager::getInstance().getConnection();
    QSqlQuery query(db);

    db.transaction();

    try {
        // Delete visitors with expired retention period
        query.prepare("DELETE FROM visitors WHERE "
                     "datetime(created_at, '+' || retention_period || ' days') < datetime('now')");

        if (!query.exec()) {
            throw std::runtime_error(query.lastError().text().toStdString());
        }

        // Delete related records
        query.prepare("DELETE FROM visits WHERE visitor_id NOT IN (SELECT id FROM visitors)");
        if (!query.exec()) {
            throw std::runtime_error(query.lastError().text().toStdString());
        }

        if (!db.commit()) {
            throw std::runtime_error("Failed to commit data purge");
        }
        
        LOG_INFO("Expired records purged successfully", ErrorCategory::Database);
    }
    catch (const std::exception& e) {
        db.rollback();
        LOG_ERROR(QString("Data purge failed: %1").arg(e.what()), ErrorCategory::Database);
    }
}

bool VisitorManager::validateVisitorData(const Visitor& visitor)
{
    if (visitor.getName().isEmpty() || visitor.getEmail().isEmpty()) {
        LOG_WARNING("Visitor validation failed: missing required fields", ErrorCategory::UserInput);
        return false;
    }

    if (!visitor.hasConsent()) {
        LOG_WARNING("Visitor validation failed: consent not provided", ErrorCategory::UserInput);
        return false;
    }

    if (visitor.getRetentionPeriod() <= 0) {
        LOG_WARNING("Visitor validation failed: invalid retention period", ErrorCategory::UserInput);
        return false;
    }

    return true;
}

void VisitorManager::logVisitorActivity(const QString& visitorId, const QString& action)
{
    QSqlDatabase db = DatabaseManager::getInstance().getConnection();
    QSqlQuery query(db);

    query.prepare("INSERT INTO audit_log (id, action, entity_type, entity_id, created_at) "
                 "VALUES (?, ?, 'VISITOR', ?, ?)");

    query.addBindValue(QUuid::createUuid().toString());
    query.addBindValue(action);
    query.addBindValue(visitorId);
    query.addBindValue(QDateTime::currentDateTime());

    if (!query.exec()) {
        LOG_ERROR(QString("Failed to log visitor activity: %1").arg(query.lastError().text()), 
                 ErrorCategory::Database);
    }
}

bool VisitorManager::notifyHost(const QString& hostId, const QString& message)
{
    // TODO: Implement host notification (email, SMS, etc.)
    LOG_INFO(QString("Host notification sent: %1 - %2").arg(hostId).arg(message), ErrorCategory::Network);
    return true;
}

QString VisitorManager::generateQRCode(const QString& visitorId)
{
    try {
        // Get visitor information for QR code
        Visitor* visitor = getVisitor(visitorId);
        if (!visitor) {
            throw std::runtime_error("Visitor not found");
        }

        QImage qrCode = QRCodeGenerator::getInstance().generateVisitorQRCode(visitorId, visitor->getName());
        
        if (qrCode.isNull()) {
            throw std::runtime_error("Failed to generate QR code");
        }

        // Save QR code to file
        QString qrCodePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) 
                           + "/qrcodes/" + visitorId + ".png";
        QRCodeGenerator::getInstance().saveQRCode(qrCode, qrCodePath);

        LOG_INFO(QString("QR code generated for visitor: %1").arg(visitorId), ErrorCategory::System);
        return qrCodePath;
        
    } catch (const std::exception& e) {
        LOG_ERROR(QString("Failed to generate QR code: %1").arg(e.what()), ErrorCategory::System);
        return QString();
    }
}

bool VisitorManager::printVisitorBadge(const QString& visitorId)
{
    try {
        // TODO: Implement actual badge printing
        // This could use QPrinter or a dedicated printing library
        LOG_INFO(QString("Visitor badge printed: %1").arg(visitorId), ErrorCategory::System);
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR(QString("Failed to print visitor badge: %1").arg(e.what()), ErrorCategory::System);
        return false;
    }
}

// Placeholder implementations for other methods
Visitor* VisitorManager::getVisitor(const QString& visitorId)
{
    // TODO: Implement visitor retrieval
    return nullptr;
}

QList<Visitor*> VisitorManager::searchVisitors(const QString& query)
{
    // TODO: Implement visitor search
    return QList<Visitor*>();
}

QList<Visitor*> VisitorManager::getCurrentVisitors()
{
    // TODO: Implement current visitors retrieval
    return QList<Visitor*>();
}

QDateTime VisitorManager::getCheckInTime(const QString& visitorId)
{
    // TODO: Implement check-in time retrieval
    return QDateTime();
}

QDateTime VisitorManager::getCheckOutTime(const QString& visitorId)
{
    // TODO: Implement check-out time retrieval
    return QDateTime();
}

bool VisitorManager::isBlacklisted(const QString& visitorId)
{
    // TODO: Implement blacklist checking
    return false;
}

QList<Visitor*> VisitorManager::getBlacklistedVisitors()
{
    // TODO: Implement blacklisted visitors retrieval
    return QList<Visitor*>();
}

bool VisitorManager::hasValidConsent(const QString& visitorId)
{
    // TODO: Implement consent validation
    return true;
}

bool VisitorManager::exportVisitorData(const QString& visitorId, const QString& format)
{
    // TODO: Implement visitor data export
    return false;
}

int VisitorManager::getTotalVisitorsToday()
{
    // TODO: Implement today's visitor count
    return 0;
}

int VisitorManager::getCurrentVisitorCount()
{
    // TODO: Implement current visitor count
    return 0;
}

QList<QPair<QDateTime, int>> VisitorManager::getVisitorStatistics(const QDateTime& start, const QDateTime& end)
{
    // TODO: Implement visitor statistics
    return QList<QPair<QDateTime, int>>();
} 
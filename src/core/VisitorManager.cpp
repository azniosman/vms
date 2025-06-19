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

bool VisitorManager::registerVisitor(const Visitor& visitor)
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
        // Create a copy to modify the ID
        Visitor visitorCopy = visitor;
        visitorCopy.setId(visitorId);

        QByteArray photoData;
        QBuffer photoBuffer(&photoData);
        photoBuffer.open(QIODevice::WriteOnly);
        visitorCopy.getPhoto().save(&photoBuffer, "PNG");

        QByteArray idScanData;
        QBuffer idScanBuffer(&idScanData);
        idScanBuffer.open(QIODevice::WriteOnly);
        visitorCopy.getIdScan().save(&idScanBuffer, "PNG");

        query.addBindValue(visitorId);
        query.addBindValue(visitorCopy.getName());
        query.addBindValue(visitorCopy.getEmail());
        query.addBindValue(visitorCopy.getPhone());
        query.addBindValue(visitorCopy.getCompany());
        query.addBindValue(visitorCopy.getIdentificationNumber());
        query.addBindValue(static_cast<int>(visitorCopy.getType()));
        query.addBindValue(photoData);
        query.addBindValue(idScanData);
        query.addBindValue(visitorCopy.getSignature());
        query.addBindValue(visitorCopy.getHostId());
        query.addBindValue(visitorCopy.getPurpose());
        query.addBindValue(QDateTime::currentDateTime());
        query.addBindValue(QDateTime::currentDateTime());
        query.addBindValue(visitorCopy.hasConsent());
        query.addBindValue(visitorCopy.getRetentionPeriod());

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

bool VisitorManager::checkInVisitor(const QString& visitorId, const QString& hostId)
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

        // Update visitor status
        query.prepare("UPDATE visitors SET status = 'checked_in', updated_at = ? WHERE id = ?");
        query.addBindValue(QDateTime::currentDateTime());
        query.addBindValue(visitorId);

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

bool VisitorManager::checkOutVisitor(const QString& visitorId)
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

        // Update visitor status
        query.prepare("UPDATE visitors SET status = 'checked_out', updated_at = ? WHERE id = ?");
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
    // Removed duplicate updateConsent method - using recordConsent instead
    return false;
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

bool VisitorManager::generateQRCode(const QString& visitorId)
{
    try {
        Visitor visitor = getVisitor(visitorId);
        if (visitor.getId().isEmpty()) {
            LOG_ERROR(QString("Visitor not found for QR code generation: %1").arg(visitorId), ErrorCategory::UserInput);
            return false;
        }

        // Create QR code data
        QJsonObject qrData;
        qrData["visitor_id"] = visitorId;
        qrData["name"] = visitor.getName();
        qrData["company"] = visitor.getCompany();
        qrData["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

        QJsonDocument doc(qrData);
        QString qrCodeData = doc.toJson(QJsonDocument::Compact);

        // Generate QR code image
        QImage qrCodeImage = QRCodeGenerator::getInstance().generateQRCode(qrCodeData);
        if (qrCodeImage.isNull()) {
            LOG_ERROR(QString("Failed to generate QR code for visitor: %1").arg(visitorId), ErrorCategory::System);
            return false;
        }

        // Convert QImage to QPixmap
        QPixmap qrCodePixmap = QPixmap::fromImage(qrCodeImage);

        // Save QR code to file
        QString qrCodePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) 
                           + "/vms/qr_codes/";
        QDir().mkpath(qrCodePath);
        
        QString fileName = QString("qr_%1.png").arg(visitorId);
        QString filePath = qrCodePath + fileName;

        if (!qrCodePixmap.save(filePath, "PNG")) {
            LOG_ERROR(QString("Failed to save QR code to file: %1").arg(filePath), ErrorCategory::FileSystem);
            return false;
        }

        LOG_INFO(QString("QR code generated successfully for visitor: %1").arg(visitorId), ErrorCategory::System);
        return true;
    }
    catch (const std::exception& e) {
        LOG_ERROR(QString("Exception in QR code generation: %1").arg(e.what()), ErrorCategory::System);
        return false;
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

Visitor VisitorManager::getVisitor(const QString& visitorId)
{
    QSqlDatabase db = DatabaseManager::getInstance().getConnection();
    QSqlQuery query(db);

    query.prepare("SELECT * FROM visitors WHERE id = ?");
    query.addBindValue(visitorId);

    if (!query.exec()) {
        LOG_ERROR(QString("Failed to get visitor: %1").arg(query.lastError().text()), ErrorCategory::Database);
        return Visitor();
    }

    if (query.next()) {
        return createVisitorFromQuery(query);
    }

    return Visitor();
}

QList<Visitor> VisitorManager::getAllVisitors()
{
    QSqlDatabase db = DatabaseManager::getInstance().getConnection();
    QSqlQuery query(db);

    query.prepare("SELECT * FROM visitors ORDER BY created_at DESC");

    if (!query.exec()) {
        LOG_ERROR(QString("Failed to get all visitors: %1").arg(query.lastError().text()), ErrorCategory::Database);
        return QList<Visitor>();
    }

    QList<Visitor> visitors;
    while (query.next()) {
        visitors.append(createVisitorFromQuery(query));
    }

    return visitors;
}

QList<Visitor> VisitorManager::searchVisitors(const QString& searchTerm)
{
    QSqlDatabase db = DatabaseManager::getInstance().getConnection();
    QSqlQuery query(db);

    query.prepare("SELECT * FROM visitors WHERE "
                 "name LIKE ? OR email LIKE ? OR phone LIKE ? OR company LIKE ? "
                 "ORDER BY created_at DESC");
    
    QString likePattern = "%" + searchTerm + "%";
    query.addBindValue(likePattern);
    query.addBindValue(likePattern);
    query.addBindValue(likePattern);
    query.addBindValue(likePattern);

    if (!query.exec()) {
        LOG_ERROR(QString("Failed to search visitors: %1").arg(query.lastError().text()), ErrorCategory::Database);
        return QList<Visitor>();
    }

    QList<Visitor> visitors;
    while (query.next()) {
        visitors.append(createVisitorFromQuery(query));
    }

    return visitors;
}

QList<Visitor> VisitorManager::getCheckedInVisitors()
{
    QSqlDatabase db = DatabaseManager::getInstance().getConnection();
    QSqlQuery query(db);

    query.prepare("SELECT v.* FROM visitors v "
                 "INNER JOIN visits vs ON v.id = vs.visitor_id "
                 "WHERE vs.check_out_time IS NULL "
                 "ORDER BY vs.check_in_time DESC");

    if (!query.exec()) {
        LOG_ERROR(QString("Failed to get checked in visitors: %1").arg(query.lastError().text()), ErrorCategory::Database);
        return QList<Visitor>();
    }

    QList<Visitor> visitors;
    while (query.next()) {
        visitors.append(createVisitorFromQuery(query));
    }

    return visitors;
}

bool VisitorManager::isVisitorCheckedIn(const QString& visitorId)
{
    QSqlDatabase db = DatabaseManager::getInstance().getConnection();
    QSqlQuery query(db);

    query.prepare("SELECT COUNT(*) FROM visits "
                 "WHERE visitor_id = ? AND check_out_time IS NULL");
    query.addBindValue(visitorId);

    if (!query.exec() || !query.next()) {
        LOG_ERROR(QString("Failed to check visitor status: %1").arg(query.lastError().text()), ErrorCategory::Database);
        return false;
    }

    return query.value(0).toInt() > 0;
}

QList<QPair<QString, QString>> VisitorManager::getBlacklist()
{
    QSqlDatabase db = DatabaseManager::getInstance().getConnection();
    QSqlQuery query(db);

    query.prepare("SELECT visitor_id, reason FROM blacklist ORDER BY created_at DESC");

    if (!query.exec()) {
        LOG_ERROR(QString("Failed to get blacklist: %1").arg(query.lastError().text()), ErrorCategory::Database);
        return QList<QPair<QString, QString>>();
    }

    QList<QPair<QString, QString>> blacklist;
    while (query.next()) {
        blacklist.append(qMakePair(
            query.value("visitor_id").toString(),
            query.value("reason").toString()
        ));
    }

    return blacklist;
}

bool VisitorManager::isBlacklisted(const QString& visitorId)
{
    QSqlDatabase db = DatabaseManager::getInstance().getConnection();
    QSqlQuery query(db);

    query.prepare("SELECT COUNT(*) FROM blacklist WHERE visitor_id = ?");
    query.addBindValue(visitorId);

    if (!query.exec() || !query.next()) {
        LOG_ERROR(QString("Failed to check blacklist: %1").arg(query.lastError().text()), ErrorCategory::Database);
        return false;
    }

    return query.value(0).toInt() > 0;
}

void VisitorManager::logActivity(const QString& action, const QString& details, const QString& userId)
{
    QSqlDatabase db = DatabaseManager::getInstance().getConnection();
    QSqlQuery query(db);

    query.prepare("INSERT INTO audit_log (id, action, details, user_id, created_at) "
                 "VALUES (?, ?, ?, ?, ?)");

    query.addBindValue(QUuid::createUuid().toString());
    query.addBindValue(action);
    query.addBindValue(details);
    query.addBindValue(userId);
    query.addBindValue(QDateTime::currentDateTime());

    if (!query.exec()) {
        LOG_ERROR(QString("Failed to log activity: %1").arg(query.lastError().text()), ErrorCategory::Database);
    }
}

// Helper method to create Visitor object from database query
Visitor VisitorManager::createVisitorFromQuery(const QSqlQuery& query)
{
    Visitor visitor;
    visitor.setId(query.value("id").toString());
    visitor.setName(query.value("name").toString());
    visitor.setEmail(query.value("email").toString());
    visitor.setPhone(query.value("phone").toString());
    visitor.setCompany(query.value("company").toString());
    visitor.setIdentificationNumber(query.value("identification_number").toString());
    visitor.setType(static_cast<Visitor::VisitorType>(query.value("type").toInt()));
    visitor.setHostId(query.value("host_id").toString());
    visitor.setPurpose(query.value("purpose").toString());
    visitor.setConsent(query.value("consent").toBool());
    visitor.setRetentionPeriod(query.value("retention_period").toInt());

    // Load photo and ID scan from BLOB data
    QByteArray photoData = query.value("photo").toByteArray();
    if (!photoData.isEmpty()) {
        QPixmap photo;
        photo.loadFromData(photoData);
        visitor.setPhoto(photo.toImage());
    }

    QByteArray idScanData = query.value("id_scan").toByteArray();
    if (!idScanData.isEmpty()) {
        QPixmap idScan;
        idScan.loadFromData(idScanData);
        visitor.setIdScan(idScan.toImage());
    }

    return visitor;
}

QDateTime VisitorManager::getCheckInTime(const QString& visitorId)
{
    QSqlDatabase db = DatabaseManager::getInstance().getConnection();
    QSqlQuery query(db);

    query.prepare("SELECT check_in_time FROM visits "
                 "WHERE visitor_id = ? AND check_out_time IS NULL "
                 "ORDER BY check_in_time DESC LIMIT 1");
    query.addBindValue(visitorId);

    if (!query.exec() || !query.next()) {
        LOG_ERROR(QString("Failed to get check-in time: %1").arg(query.lastError().text()), ErrorCategory::Database);
        return QDateTime();
    }

    return query.value("check_in_time").toDateTime();
}

QDateTime VisitorManager::getCheckOutTime(const QString& visitorId)
{
    QSqlDatabase db = DatabaseManager::getInstance().getConnection();
    QSqlQuery query(db);

    query.prepare("SELECT check_out_time FROM visits "
                 "WHERE visitor_id = ? AND check_out_time IS NOT NULL "
                 "ORDER BY check_out_time DESC LIMIT 1");
    query.addBindValue(visitorId);

    if (!query.exec() || !query.next()) {
        LOG_ERROR(QString("Failed to get check-out time: %1").arg(query.lastError().text()), ErrorCategory::Database);
        return QDateTime();
    }

    return query.value("check_out_time").toDateTime();
}

bool VisitorManager::recordConsent(const QString& visitorId, const QString& consentType, bool granted)
{
    QSqlDatabase db = DatabaseManager::getInstance().getConnection();
    QSqlQuery query(db);

    db.transaction();

    try {
        query.prepare("INSERT OR REPLACE INTO consent_records "
                     "(visitor_id, consent_type, granted, recorded_at) "
                     "VALUES (?, ?, ?, ?)");
        query.addBindValue(visitorId);
        query.addBindValue(consentType);
        query.addBindValue(granted);
        query.addBindValue(QDateTime::currentDateTime());

        if (!query.exec()) {
            throw std::runtime_error(query.lastError().text().toStdString());
        }

        if (!db.commit()) {
            throw std::runtime_error("Failed to commit transaction");
        }

        emit consentUpdated(visitorId, granted);
        LOG_INFO(QString("Consent recorded for visitor: %1, type: %2, granted: %3")
                .arg(visitorId).arg(consentType).arg(granted), ErrorCategory::Database);
        return true;
    }
    catch (const std::exception& e) {
        db.rollback();
        LOG_ERROR(QString("Failed to record consent: %1").arg(e.what()), ErrorCategory::Database);
        return false;
    }
}

bool VisitorManager::hasValidConsent(const QString& visitorId, const QString& consentType)
{
    QSqlDatabase db = DatabaseManager::getInstance().getConnection();
    QSqlQuery query(db);

    query.prepare("SELECT granted, recorded_at FROM consent_records "
                 "WHERE visitor_id = ? AND consent_type = ? "
                 "ORDER BY recorded_at DESC LIMIT 1");
    query.addBindValue(visitorId);
    query.addBindValue(consentType);

    if (!query.exec()) {
        LOG_ERROR(QString("Failed to check consent: %1").arg(query.lastError().text()), ErrorCategory::Database);
        return false;
    }

    if (query.next()) {
        bool granted = query.value("granted").toBool();
        QDateTime recordedAt = query.value("recorded_at").toDateTime();
        
        // Check if consent is still valid (within retention period)
        QDateTime expiryDate = recordedAt.addDays(365); // Default 1 year
        return granted && QDateTime::currentDateTime() < expiryDate;
    }

    return false;
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
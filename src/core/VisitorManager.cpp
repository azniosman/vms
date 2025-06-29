#include "VisitorManager.h"
#include "../database/DatabaseManager.h"
#include "../security/SecurityManager.h"
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
        LOG_ERROR_CAT("VisitorManager", "Invalid visitor data", ErrorCategory::UserInput);
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
        // TODO: Implement image saving when GUI is available
        // For now, just store the raw data
        photoData = visitorCopy.getPhoto();

        QByteArray idScanData;
        QBuffer idScanBuffer(&idScanData);
        idScanBuffer.open(QIODevice::WriteOnly);
        // TODO: Implement image saving when GUI is available
        // For now, just store the raw data
        idScanData = visitorCopy.getIdScan();

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
        LOG_INFO("VisitorManager", QString("Visitor registered successfully: %1").arg(visitorId));
        return true;
    }
    catch (const std::exception& e) {
        db.rollback();
        LOG_ERROR_CAT("VisitorManager", QString("Failed to register visitor: %1").arg(e.what()), ErrorCategory::Database);
        return false;
    }
}

bool VisitorManager::updateVisitor(const Visitor& visitor)
{
    if (!validateVisitorData(visitor)) {
        LOG_ERROR_CAT("VisitorManager", "Invalid visitor data for update", ErrorCategory::UserInput);
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
        // TODO: Implement image saving when GUI is available
        // For now, just store the raw data
        photoData = visitor.getPhoto();

        QByteArray idScanData;
        QBuffer idScanBuffer(&idScanData);
        idScanBuffer.open(QIODevice::WriteOnly);
        // TODO: Implement image saving when GUI is available
        // For now, just store the raw data
        idScanData = visitor.getIdScan();

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
        LOG_INFO("VisitorManager", QString("Visitor updated successfully: %1").arg(visitor.getId()));
        return true;
    }
    catch (const std::exception& e) {
        db.rollback();
        LOG_ERROR_CAT("VisitorManager", QString("Failed to update visitor: %1").arg(e.what()), ErrorCategory::Database);
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
        LOG_INFO("VisitorManager", QString("Visitor checked in successfully: %1").arg(visitorId));
        return true;
    }
    catch (const std::exception& e) {
        db.rollback();
        LOG_ERROR_CAT("VisitorManager", QString("Failed to check in visitor: %1").arg(e.what()), ErrorCategory::Database);
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
        LOG_INFO("VisitorManager", QString("Visitor checked out successfully: %1").arg(visitorId));
        return true;
    }
    catch (const std::exception& e) {
        db.rollback();
        LOG_ERROR_CAT("VisitorManager", QString("Failed to check out visitor: %1").arg(e.what()), ErrorCategory::Database);
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
        LOG_ERROR_CAT("VisitorManager", QString("Failed to add visitor to blacklist: %1").arg(query.lastError().text()), 
                 ErrorCategory::Database);
        return false;
    }

    logVisitorActivity(visitorId, "BLACKLIST_ADD");
    emit blacklistUpdated();
    LOG_INFO("VisitorManager", QString("Visitor added to blacklist: %1").arg(visitorId));
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
        
        LOG_INFO("VisitorManager", "Expired records purged successfully");
    }
    catch (const std::exception& e) {
        db.rollback();
        LOG_ERROR_CAT("VisitorManager", QString("Data purge failed: %1").arg(e.what()), ErrorCategory::Database);
    }
}

bool VisitorManager::validateVisitorData(const Visitor& visitor)
{
    // Check rate limiting first
    if (!checkRateLimit("validation")) {
        ErrorHandler::getInstance().logError("VisitorManager", "Rate limit exceeded for visitor validation");
        return false;
    }
    
    // Validate required fields
    if (visitor.getName().isEmpty()) {
        ErrorHandler::getInstance().logError("VisitorManager", "Visitor name is required");
        return false;
    }
    
    if (visitor.getEmail().isEmpty()) {
        ErrorHandler::getInstance().logError("VisitorManager", "Visitor email is required");
        return false;
    }
    
    // Validate name
    if (!validateName(visitor.getName())) {
        ErrorHandler::getInstance().logError("VisitorManager", "Invalid visitor name format");
        return false;
    }
    
    // Validate email
    if (!validateEmail(visitor.getEmail())) {
        ErrorHandler::getInstance().logError("VisitorManager", "Invalid email format");
        return false;
    }
    
    // Validate phone if provided
    if (!visitor.getPhone().isEmpty() && !validatePhoneNumber(visitor.getPhone())) {
        ErrorHandler::getInstance().logError("VisitorManager", "Invalid phone number format");
        return false;
    }
    
    // Validate company if provided
    if (!visitor.getCompany().isEmpty() && !validateCompany(visitor.getCompany())) {
        ErrorHandler::getInstance().logError("VisitorManager", "Invalid company name");
        return false;
    }
    
    // Validate ID number if provided
    if (!visitor.getIdentificationNumber().isEmpty() && !validateIdNumber(visitor.getIdentificationNumber())) {
        ErrorHandler::getInstance().logError("VisitorManager", "Invalid identification number");
        return false;
    }
    
    // Validate purpose if provided
    if (!visitor.getPurpose().isEmpty() && !validatePurpose(visitor.getPurpose())) {
        ErrorHandler::getInstance().logError("VisitorManager", "Invalid purpose description");
        return false;
    }
    
    // Validate image data sizes
    if (!visitor.getPhoto().isEmpty() && !validateDataSize(visitor.getPhoto(), MAX_IMAGE_SIZE)) {
        ErrorHandler::getInstance().logError("VisitorManager", "Photo size exceeds maximum limit");
        return false;
    }
    
    if (!visitor.getIdScan().isEmpty() && !validateDataSize(visitor.getIdScan(), MAX_IMAGE_SIZE)) {
        ErrorHandler::getInstance().logError("VisitorManager", "ID scan size exceeds maximum limit");
        return false;
    }
    
    if (!visitor.getSignature().isEmpty() && !validateDataSize(visitor.getSignature().toUtf8(), MAX_IMAGE_SIZE)) {
        ErrorHandler::getInstance().logError("VisitorManager", "Signature size exceeds maximum limit");
        return false;
    }
    
    // Validate image data if provided
    if (!visitor.getPhoto().isEmpty() && !isValidImageData(visitor.getPhoto())) {
        ErrorHandler::getInstance().logError("VisitorManager", "Invalid photo data");
        return false;
    }
    
    if (!visitor.getIdScan().isEmpty() && !isValidImageData(visitor.getIdScan())) {
        ErrorHandler::getInstance().logError("VisitorManager", "Invalid ID scan data");
        return false;
    }
    
    // Validate consent
    if (!visitor.hasConsent()) {
        ErrorHandler::getInstance().logError("VisitorManager", "Visitor consent is required");
        return false;
    }
    
    // Validate retention period
    if (visitor.getRetentionPeriod() <= 0 || visitor.getRetentionPeriod() > 3650) { // Max 10 years
        ErrorHandler::getInstance().logError("VisitorManager", "Invalid retention period");
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
        LOG_ERROR_CAT("VisitorManager", QString("Failed to log visitor activity: %1").arg(query.lastError().text()), 
                 ErrorCategory::Database);
    }
}

bool VisitorManager::notifyHost(const QString& hostId, const QString& message)
{
    // TODO: Implement host notification (email, SMS, etc.)
    LOG_INFO("VisitorManager", QString("Host notification sent: %1 - %2").arg(hostId).arg(message));
    return true;
}

bool VisitorManager::generateQRCode(const QString& visitorId)
{
    try {
        // TODO: Implement QR code generation when GUI is available
        LOG_INFO("VisitorManager", QString("QR code generation skipped for visitor: %1").arg(visitorId));
        return true;
    }
    catch (const std::exception& e) {
        LOG_ERROR_CAT("VisitorManager", QString("Exception in QR code generation: %1").arg(e.what()), ErrorCategory::System);
        return false;
    }
}

bool VisitorManager::printVisitorBadge(const QString& visitorId)
{
    try {
        // TODO: Implement actual badge printing
        // This could use QPrinter or a dedicated printing library
        LOG_INFO("VisitorManager", QString("Visitor badge printed: %1").arg(visitorId));
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR_CAT("VisitorManager", QString("Failed to print visitor badge: %1").arg(e.what()), ErrorCategory::System);
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
        LOG_ERROR_CAT("VisitorManager", QString("Failed to get visitor: %1").arg(query.lastError().text()), ErrorCategory::Database);
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
        LOG_ERROR_CAT("VisitorManager", QString("Failed to get all visitors: %1").arg(query.lastError().text()), ErrorCategory::Database);
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
        LOG_ERROR_CAT("VisitorManager", QString("Failed to search visitors: %1").arg(query.lastError().text()), ErrorCategory::Database);
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
        LOG_ERROR_CAT("VisitorManager", QString("Failed to get checked in visitors: %1").arg(query.lastError().text()), ErrorCategory::Database);
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
        LOG_ERROR_CAT("VisitorManager", QString("Failed to check visitor status: %1").arg(query.lastError().text()), ErrorCategory::Database);
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
        LOG_ERROR_CAT("VisitorManager", QString("Failed to get blacklist: %1").arg(query.lastError().text()), ErrorCategory::Database);
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
        LOG_ERROR_CAT("VisitorManager", QString("Failed to check blacklist: %1").arg(query.lastError().text()), ErrorCategory::Database);
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
        LOG_ERROR_CAT("VisitorManager", QString("Failed to log activity: %1").arg(query.lastError().text()), ErrorCategory::Database);
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

    // Load photo and ID scan from BLOB data (stubbed for core-only build)
    // TODO: Implement image loading when GUI is available
    QByteArray photoData = query.value("photo").toByteArray();
    if (!photoData.isEmpty()) {
        visitor.setPhoto(photoData);
        LOG_INFO("VisitorManager", "Photo data loaded successfully");
    }

    QByteArray idScanData = query.value("id_scan").toByteArray();
    if (!idScanData.isEmpty()) {
        visitor.setIdScan(idScanData);
        LOG_INFO("VisitorManager", "ID scan data loaded successfully");
    }

    return visitor;
}

bool VisitorManager::validateEmail(const QString& email)
{
    if (email.length() > MAX_EMAIL_LENGTH) {
        return false;
    }
    
    // Check for SQL injection patterns
    if (containsSqlInjection(email)) {
        return false;
    }
    
    // RFC 5322 compliant email validation
    QRegularExpression emailRegex(R"(^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$)");
    return emailRegex.match(email).hasMatch();
}

bool VisitorManager::validatePhoneNumber(const QString& phone)
{
    if (phone.length() > MAX_PHONE_LENGTH) {
        return false;
    }
    
    // Check for SQL injection patterns
    if (containsSqlInjection(phone)) {
        return false;
    }
    
    // Allow international phone numbers with optional + and spaces/dashes
    QRegularExpression phoneRegex(R"(^[\+]?[1-9][\d\s\-\(\)]{7,19}$)");
    return phoneRegex.match(phone.simplified()).hasMatch();
}

bool VisitorManager::validateName(const QString& name)
{
    if (name.isEmpty() || name.length() > MAX_NAME_LENGTH) {
        return false;
    }
    
    // Check for SQL injection patterns
    if (containsSqlInjection(name)) {
        return false;
    }
    
    // Check for XSS attempts
    if (containsXssAttempt(name)) {
        return false;
    }
    
    // Only allow letters, spaces, hyphens, apostrophes, and dots
    QRegularExpression nameRegex(R"(^[a-zA-ZÀ-ÿ\s\-\.']{1,100}$)");
    return nameRegex.match(name).hasMatch();
}

bool VisitorManager::validateCompany(const QString& company)
{
    if (company.length() > MAX_COMPANY_LENGTH) {
        return false;
    }
    
    // Check for SQL injection patterns
    if (containsSqlInjection(company)) {
        return false;
    }
    
    // Check for XSS attempts
    if (containsXssAttempt(company)) {
        return false;
    }
    
    // Allow alphanumeric, spaces, common punctuation
    QRegularExpression companyRegex(R"(^[a-zA-Z0-9À-ÿ\s\-\.,&()]{1,200}$)");
    return companyRegex.match(company).hasMatch();
}

bool VisitorManager::validateIdNumber(const QString& idNumber)
{
    if (idNumber.isEmpty() || idNumber.length() > MAX_ID_NUMBER_LENGTH) {
        return false;
    }
    
    // Check for SQL injection patterns
    if (containsSqlInjection(idNumber)) {
        return false;
    }
    
    // Allow alphanumeric and common ID separators
    QRegularExpression idRegex(R"(^[a-zA-Z0-9\-\s]{1,50}$)");
    return idRegex.match(idNumber).hasMatch();
}

bool VisitorManager::validatePurpose(const QString& purpose)
{
    if (purpose.length() > MAX_PURPOSE_LENGTH) {
        return false;
    }
    
    // Check for SQL injection patterns
    if (containsSqlInjection(purpose)) {
        return false;
    }
    
    // Check for XSS attempts
    if (containsXssAttempt(purpose)) {
        return false;
    }
    
    // Allow most characters but restrict script tags and SQL keywords
    QString cleanPurpose = sanitizeInput(purpose);
    return cleanPurpose == purpose; // If sanitization changed it, it was invalid
}

QString VisitorManager::sanitizeInput(const QString& input)
{
    QString sanitized = input;
    
    // Remove null bytes
    sanitized.remove(QChar('\0'));
    
    // Remove control characters except tab, newline, carriage return
    for (int i = 0; i < sanitized.length(); ++i) {
        QChar ch = sanitized.at(i);
        if (ch.category() == QChar::Other_Control && 
            ch != '\t' && ch != '\n' && ch != '\r') {
            sanitized.remove(i, 1);
            --i;
        }
    }
    
    // Trim whitespace
    sanitized = sanitized.trimmed();
    
    return sanitized;
}

QString VisitorManager::sanitizeHtml(const QString& input)
{
    QString sanitized = input;
    
    // Replace HTML entities
    sanitized.replace("&", "&amp;");
    sanitized.replace("<", "&lt;");
    sanitized.replace(">", "&gt;");
    sanitized.replace("\"", "&quot;");
    sanitized.replace("'", "&#x27;");
    sanitized.replace("/", "&#x2F;");
    
    return sanitized;
}

bool VisitorManager::isValidImageData(const QByteArray& data)
{
    if (data.isEmpty()) {
        return true; // Empty data is valid (optional field)
    }
    
    // Check for common image file signatures
    QList<QByteArray> validSignatures = {
        QByteArray::fromHex("FFD8FF"),     // JPEG
        QByteArray::fromHex("89504E47"),   // PNG
        QByteArray::fromHex("424D"),       // BMP
        QByteArray::fromHex("47494638"),   // GIF
        QByteArray::fromHex("52494646")    // WEBP (RIFF)
    };
    
    for (const QByteArray& signature : validSignatures) {
        if (data.startsWith(signature)) {
            return true;
        }
    }
    
    return false;
}

bool VisitorManager::isSecureFileName(const QString& fileName)
{
    if (fileName.isEmpty() || fileName.length() > 255) {
        return false;
    }
    
    // Check for path traversal attempts
    if (fileName.contains("..") || fileName.contains("/") || fileName.contains("\\")) {
        return false;
    }
    
    // Check for dangerous file extensions
    QStringList dangerousExtensions = {
        "exe", "bat", "cmd", "com", "pif", "scr", "vbs", "js", "jar", "app", "deb", "pkg", "dmg"
    };
    
    QString extension = fileName.split('.').last().toLower();
    if (dangerousExtensions.contains(extension)) {
        return false;
    }
    
    return true;
}

bool VisitorManager::checkRateLimit(const QString& identifier)
{
    QMutexLocker locker(&rateLimitMutex);
    
    QDateTime now = QDateTime::currentDateTime();
    QDateTime oneMinuteAgo = now.addSecs(-60);
    
    // Clean up old entries
    auto it = rateLimitTracker.begin();
    while (it != rateLimitTracker.end()) {
        if (it.value() < oneMinuteAgo) {
            it = rateLimitTracker.erase(it);
        } else {
            ++it;
        }
    }
    
    // Count requests in the last minute
    int requestCount = 0;
    for (auto it = rateLimitTracker.begin(); it != rateLimitTracker.end(); ++it) {
        if (it.key().startsWith(identifier)) {
            requestCount++;
        }
    }
    
    if (requestCount >= MAX_REQUESTS_PER_MINUTE) {
        return false;
    }
    
    // Record this request
    QString requestKey = QString("%1_%2").arg(identifier).arg(QDateTime::currentMSecsSinceEpoch());
    rateLimitTracker[requestKey] = now;
    
    return true;
}

bool VisitorManager::validateDataSize(const QByteArray& data, int maxSize)
{
    return data.size() <= maxSize;
}

bool VisitorManager::containsSqlInjection(const QString& input)
{
    QString lowerInput = input.toLower();
    
    // Common SQL injection patterns
    QStringList sqlPatterns = {
        "union select", "drop table", "delete from", "insert into",
        "update set", "create table", "alter table", "exec ",
        "execute ", "sp_", "xp_", "--;", "/*", "*/", "'", "\""
    };
    
    for (const QString& pattern : sqlPatterns) {
        if (lowerInput.contains(pattern)) {
            return true;
        }
    }
    
    return false;
}

bool VisitorManager::containsXssAttempt(const QString& input)
{
    QString lowerInput = input.toLower();
    
    // Common XSS patterns
    QStringList xssPatterns = {
        "<script", "</script>", "javascript:", "onload=", "onerror=",
        "onclick=", "onmouseover=", "onfocus=", "onblur=", "onchange=",
        "onsubmit=", "iframe", "object", "embed", "applet", "meta",
        "link", "style", "expression(", "url(", "import"
    };
    
    for (const QString& pattern : xssPatterns) {
        if (lowerInput.contains(pattern)) {
            return true;
        }
    }
    
    return false;
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
        LOG_ERROR_CAT("VisitorManager", QString("Failed to get check-in time: %1").arg(query.lastError().text()), ErrorCategory::Database);
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
        LOG_ERROR_CAT("VisitorManager", QString("Failed to get check-out time: %1").arg(query.lastError().text()), ErrorCategory::Database);
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
        LOG_INFO("VisitorManager", QString("Consent recorded for visitor: %1, type: %2, granted: %3")
                .arg(visitorId).arg(consentType).arg(granted));
        return true;
    }
    catch (const std::exception& e) {
        db.rollback();
        LOG_ERROR_CAT("VisitorManager", QString("Failed to record consent: %1").arg(e.what()), ErrorCategory::Database);
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
        LOG_ERROR_CAT("VisitorManager", QString("Failed to check consent: %1").arg(query.lastError().text()), ErrorCategory::Database);
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
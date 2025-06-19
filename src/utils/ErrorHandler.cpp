#include "ErrorHandler.h"
#include "../database/DatabaseManager.h"
#include "../security/SecurityManager.h"
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSqlQuery>
#include <QSqlError>
#include <QMutexLocker>

ErrorHandler* ErrorHandler::instance = nullptr;
QMutex ErrorHandler::instanceMutex;

ErrorHandler& ErrorHandler::getInstance()
{
    if (instance == nullptr) {
        QMutexLocker locker(&instanceMutex);
        if (instance == nullptr) {
            instance = new ErrorHandler();
        }
    }
    return *instance;
}

ErrorHandler::ErrorHandler(QObject *parent)
    : QObject(parent)
{
}

ErrorHandler::~ErrorHandler()
{
}

void ErrorHandler::logError(const QString& message, 
                           ErrorSeverity severity,
                           ErrorCategory category,
                           const QString& details)
{
    ErrorInfo error;
    error.message = message;
    error.details = details;
    error.severity = severity;
    error.category = category;
    error.source = QObject::tr("VMS System");
    error.timestamp = QDateTime::currentDateTime();
    error.stackTrace = getStackTrace();
    
    // Get current user and session info
    // TODO: Implement user session tracking
    error.userId = "unknown";
    error.sessionId = "unknown";

    try {
        // Save to database
        saveErrorToDatabase(error);
        
        // Save to file
        saveErrorToFile(error);
        
        // Emit signal
        emit errorLogged(error);
        
        // Handle critical errors
        if (severity == ErrorSeverity::Critical) {
            emit criticalError(error);
            notifyAdministrator(error);
        }
        
        // Log to console for debugging
        qDebug() << formatErrorMessage(error);
        
    } catch (const std::exception& e) {
        qCritical() << "Failed to log error:" << e.what();
    }
}

void ErrorHandler::logError(const QString& message, 
                           const QString& details,
                           ErrorSeverity severity,
                           ErrorCategory category)
{
    logError(message, severity, category, details);
}

void ErrorHandler::handleException(const std::exception& e, ErrorCategory category)
{
    logError(QString("Exception: %1").arg(e.what()), 
             ErrorSeverity::Error, 
             category,
             QString("Exception type: %1").arg(typeid(e).name()));
}

void ErrorHandler::handleSystemError(int errorCode, 
                                   const QString& operation,
                                   ErrorCategory category)
{
    logError(QString("System error in operation: %1").arg(operation),
             ErrorSeverity::Error,
             category,
             QString("Error code: %1").arg(errorCode));
}

QList<ErrorInfo> ErrorHandler::getErrors(const QDateTime& from,
                                        const QDateTime& to,
                                        ErrorSeverity minSeverity)
{
    QList<ErrorInfo> errors;
    
    try {
        QSqlDatabase db = DatabaseManager::getInstance().getConnection();
        QSqlQuery query(db);
        
        QString sql = "SELECT message, details, severity, category, source, "
                     "timestamp, stack_trace, user_id, session_id "
                     "FROM error_log WHERE 1=1";
        
        QList<QVariant> params;
        
        if (from.isValid()) {
            sql += " AND timestamp >= ?";
            params << from;
        }
        
        if (to.isValid()) {
            sql += " AND timestamp <= ?";
            params << to;
        }
        
        sql += " AND severity >= ?";
        params << static_cast<int>(minSeverity);
        
        sql += " ORDER BY timestamp DESC";
        
        query.prepare(sql);
        for (const QVariant& param : params) {
            query.addBindValue(param);
        }
        
        if (query.exec()) {
            while (query.next()) {
                ErrorInfo error;
                error.message = query.value("message").toString();
                error.details = query.value("details").toString();
                error.severity = static_cast<ErrorSeverity>(query.value("severity").toInt());
                error.category = static_cast<ErrorCategory>(query.value("category").toInt());
                error.source = query.value("source").toString();
                error.timestamp = query.value("timestamp").toDateTime();
                error.stackTrace = query.value("stack_trace").toString();
                error.userId = query.value("user_id").toString();
                error.sessionId = query.value("session_id").toString();
                
                errors.append(error);
            }
        } else {
            qCritical() << "Failed to retrieve errors:" << query.lastError().text();
        }
        
    } catch (const std::exception& e) {
        qCritical() << "Exception while retrieving errors:" << e.what();
    }
    
    return errors;
}

QList<ErrorInfo> ErrorHandler::getErrorsByCategory(ErrorCategory category,
                                                  const QDateTime& from,
                                                  const QDateTime& to)
{
    QList<ErrorInfo> errors;
    
    try {
        QSqlDatabase db = DatabaseManager::getInstance().getConnection();
        QSqlQuery query(db);
        
        QString sql = "SELECT message, details, severity, category, source, "
                     "timestamp, stack_trace, user_id, session_id "
                     "FROM error_log WHERE category = ?";
        
        QList<QVariant> params;
        params << static_cast<int>(category);
        
        if (from.isValid()) {
            sql += " AND timestamp >= ?";
            params << from;
        }
        
        if (to.isValid()) {
            sql += " AND timestamp <= ?";
            params << to;
        }
        
        sql += " ORDER BY timestamp DESC";
        
        query.prepare(sql);
        for (const QVariant& param : params) {
            query.addBindValue(param);
        }
        
        if (query.exec()) {
            while (query.next()) {
                ErrorInfo error;
                error.message = query.value("message").toString();
                error.details = query.value("details").toString();
                error.severity = static_cast<ErrorSeverity>(query.value("severity").toInt());
                error.category = static_cast<ErrorCategory>(query.value("category").toInt());
                error.source = query.value("source").toString();
                error.timestamp = query.value("timestamp").toDateTime();
                error.stackTrace = query.value("stack_trace").toString();
                error.userId = query.value("user_id").toString();
                error.sessionId = query.value("session_id").toString();
                
                errors.append(error);
            }
        } else {
            qCritical() << "Failed to retrieve errors by category:" << query.lastError().text();
        }
        
    } catch (const std::exception& e) {
        qCritical() << "Exception while retrieving errors by category:" << e.what();
    }
    
    return errors;
}

int ErrorHandler::getErrorCount(ErrorSeverity severity,
                               const QDateTime& from,
                               const QDateTime& to)
{
    try {
        QSqlDatabase db = DatabaseManager::getInstance().getConnection();
        QSqlQuery query(db);
        
        QString sql = "SELECT COUNT(*) FROM error_log WHERE severity = ?";
        QList<QVariant> params;
        params << static_cast<int>(severity);
        
        if (from.isValid()) {
            sql += " AND timestamp >= ?";
            params << from;
        }
        
        if (to.isValid()) {
            sql += " AND timestamp <= ?";
            params << to;
        }
        
        query.prepare(sql);
        for (const QVariant& param : params) {
            query.addBindValue(param);
        }
        
        if (query.exec() && query.next()) {
            return query.value(0).toInt();
        }
        
    } catch (const std::exception& e) {
        qCritical() << "Exception while counting errors:" << e.what();
    }
    
    return 0;
}

QList<QPair<ErrorCategory, int>> ErrorHandler::getErrorStatistics(const QDateTime& from,
                                                                 const QDateTime& to)
{
    QList<QPair<ErrorCategory, int>> statistics;
    
    try {
        QSqlDatabase db = DatabaseManager::getInstance().getConnection();
        QSqlQuery query(db);
        
        QString sql = "SELECT category, COUNT(*) FROM error_log WHERE 1=1";
        QList<QVariant> params;
        
        if (from.isValid()) {
            sql += " AND timestamp >= ?";
            params << from;
        }
        
        if (to.isValid()) {
            sql += " AND timestamp <= ?";
            params << to;
        }
        
        sql += " GROUP BY category ORDER BY COUNT(*) DESC";
        
        query.prepare(sql);
        for (const QVariant& param : params) {
            query.addBindValue(param);
        }
        
        if (query.exec()) {
            while (query.next()) {
                ErrorCategory category = static_cast<ErrorCategory>(query.value(0).toInt());
                int count = query.value(1).toInt();
                statistics.append(qMakePair(category, count));
            }
        } else {
            qCritical() << "Failed to retrieve error statistics:" << query.lastError().text();
        }
        
    } catch (const std::exception& e) {
        qCritical() << "Exception while retrieving error statistics:" << e.what();
    }
    
    return statistics;
}

void ErrorHandler::clearOldErrors(int daysToKeep)
{
    try {
        QSqlDatabase db = DatabaseManager::getInstance().getConnection();
        QSqlQuery query(db);
        
        QDateTime cutoff = QDateTime::currentDateTime().addDays(-daysToKeep);
        
        query.prepare("DELETE FROM error_log WHERE timestamp < ?");
        query.addBindValue(cutoff);
        
        if (query.exec()) {
            qInfo() << "Cleared" << query.numRowsAffected() << "old error records";
        } else {
            qCritical() << "Failed to clear old errors:" << query.lastError().text();
        }
        
    } catch (const std::exception& e) {
        qCritical() << "Exception while clearing old errors:" << e.what();
    }
}

void ErrorHandler::exportErrors(const QString& filePath,
                               const QDateTime& from,
                               const QDateTime& to)
{
    try {
        QList<ErrorInfo> errors = getErrors(from, to);
        
        QJsonArray errorArray;
        for (const ErrorInfo& error : errors) {
            QJsonObject errorObj;
            errorObj["message"] = error.message;
            errorObj["details"] = error.details;
            errorObj["severity"] = static_cast<int>(error.severity);
            errorObj["category"] = static_cast<int>(error.category);
            errorObj["source"] = error.source;
            errorObj["timestamp"] = error.timestamp.toString(Qt::ISODate);
            errorObj["stackTrace"] = error.stackTrace;
            errorObj["userId"] = error.userId;
            errorObj["sessionId"] = error.sessionId;
            
            errorArray.append(errorObj);
        }
        
        QJsonDocument doc(errorArray);
        
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(doc.toJson());
            qInfo() << "Exported" << errors.size() << "errors to" << filePath;
        } else {
            throw std::runtime_error("Failed to open file for writing");
        }
        
    } catch (const std::exception& e) {
        qCritical() << "Failed to export errors:" << e.what();
    }
}

void ErrorHandler::saveErrorToDatabase(const ErrorInfo& error)
{
    try {
        QSqlDatabase db = DatabaseManager::getInstance().getConnection();
        QSqlQuery query(db);
        
        query.prepare("INSERT INTO error_log ("
                     "message, details, severity, category, source, "
                     "timestamp, stack_trace, user_id, session_id) "
                     "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)");
        
        query.addBindValue(error.message);
        query.addBindValue(error.details);
        query.addBindValue(static_cast<int>(error.severity));
        query.addBindValue(static_cast<int>(error.category));
        query.addBindValue(error.source);
        query.addBindValue(error.timestamp);
        query.addBindValue(error.stackTrace);
        query.addBindValue(error.userId);
        query.addBindValue(error.sessionId);
        
        if (!query.exec()) {
            throw std::runtime_error(query.lastError().text().toStdString());
        }
        
    } catch (const std::exception& e) {
        qCritical() << "Failed to save error to database:" << e.what();
    }
}

void ErrorHandler::saveErrorToFile(const ErrorInfo& error)
{
    try {
        QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/logs";
        QDir().mkpath(logDir);
        
        QString logFile = logDir + "/error.log";
        QFile file(logFile);
        
        if (file.open(QIODevice::Append | QIODevice::Text)) {
            QTextStream stream(&file);
            stream << formatErrorMessage(error) << "\n";
        }
        
    } catch (const std::exception& e) {
        qCritical() << "Failed to save error to file:" << e.what();
    }
}

QString ErrorHandler::getStackTrace()
{
    // TODO: Implement proper stack trace
    // This is a simplified implementation
    return QString("Stack trace not available");
}

void ErrorHandler::notifyAdministrator(const ErrorInfo& error)
{
    // TODO: Implement administrator notification
    // This could be via email, SMS, or system notification
    qWarning() << "Critical error occurred - administrator should be notified";
}

QString ErrorHandler::formatErrorMessage(const ErrorInfo& error)
{
    QString severityStr;
    switch (error.severity) {
        case ErrorSeverity::Info: severityStr = "INFO"; break;
        case ErrorSeverity::Warning: severityStr = "WARNING"; break;
        case ErrorSeverity::Error: severityStr = "ERROR"; break;
        case ErrorSeverity::Critical: severityStr = "CRITICAL"; break;
    }
    
    QString categoryStr;
    switch (error.category) {
        case ErrorCategory::Database: categoryStr = "DATABASE"; break;
        case ErrorCategory::Security: categoryStr = "SECURITY"; break;
        case ErrorCategory::Network: categoryStr = "NETWORK"; break;
        case ErrorCategory::FileSystem: categoryStr = "FILESYSTEM"; break;
        case ErrorCategory::UserInput: categoryStr = "USERINPUT"; break;
        case ErrorCategory::System: categoryStr = "SYSTEM"; break;
        case ErrorCategory::Unknown: categoryStr = "UNKNOWN"; break;
    }
    
    return QString("[%1] [%2] [%3] %4 - %5")
           .arg(error.timestamp.toString("yyyy-MM-dd hh:mm:ss"))
           .arg(severityStr)
           .arg(categoryStr)
           .arg(error.message)
           .arg(error.details);
} 
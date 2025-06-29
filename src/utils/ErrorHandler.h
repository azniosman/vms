#ifndef ERRORHANDLER_H
#define ERRORHANDLER_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QList>
#include <QMutex>

enum class ErrorSeverity {
    Info,
    Warning,
    Error,
    Critical
};

enum class ErrorCategory {
    Database,
    Security,
    Network,
    FileSystem,
    UserInput,
    System,
    Unknown
};

struct ErrorInfo {
    QString message;
    QString details;
    ErrorSeverity severity;
    ErrorCategory category;
    QString source;
    QDateTime timestamp;
    QString stackTrace;
    QString userId;
    QString sessionId;
};

class ErrorHandler : public QObject {
    Q_OBJECT

public:
    static ErrorHandler& getInstance();

    // Error logging
    void logError(const QString& source, const QString& message, 
                  ErrorSeverity severity = ErrorSeverity::Error,
                  ErrorCategory category = ErrorCategory::Unknown,
                  const QString& details = QString());
    
    void logInfo(const QString& source, const QString& message,
                 const QString& details = QString());
    
    void logWarning(const QString& source, const QString& message,
                    const QString& details = QString());
    
    void logCritical(const QString& source, const QString& message,
                     const QString& details = QString());
    
    // Security-specific logging
    void logSecurityEvent(const QString& eventType, const QString& details,
                         const QString& userId = QString(),
                         const QString& sessionId = QString(),
                         const QString& ipAddress = QString());
    
    void logDataAccess(const QString& table, const QString& recordId,
                      const QString& accessType, const QString& userId,
                      const QString& purpose = QString());
    
    void logFailedAuthentication(const QString& username, const QString& ipAddress,
                                const QString& reason = QString());
    
    void logPrivilegeEscalation(const QString& userId, const QString& attemptedAction,
                               const QString& sessionId = QString());

    // Error handling
    void handleException(const std::exception& e, 
                        ErrorCategory category = ErrorCategory::Unknown);
    void handleSystemError(int errorCode, 
                          const QString& operation,
                          ErrorCategory category = ErrorCategory::System);

    // Error reporting
    QList<ErrorInfo> getErrors(const QDateTime& from = QDateTime(),
                              const QDateTime& to = QDateTime(),
                              ErrorSeverity minSeverity = ErrorSeverity::Info);
    
    QList<ErrorInfo> getErrorsByCategory(ErrorCategory category,
                                        const QDateTime& from = QDateTime(),
                                        const QDateTime& to = QDateTime());

    // Error statistics
    int getErrorCount(ErrorSeverity severity, 
                     const QDateTime& from = QDateTime(),
                     const QDateTime& to = QDateTime());
    
    QList<QPair<ErrorCategory, int>> getErrorStatistics(const QDateTime& from = QDateTime(),
                                                        const QDateTime& to = QDateTime());

    // Error cleanup
    void clearOldErrors(int daysToKeep = 30);
    void exportErrors(const QString& filePath, 
                     const QDateTime& from = QDateTime(),
                     const QDateTime& to = QDateTime());

signals:
    void errorLogged(const ErrorInfo& error);
    void criticalError(const ErrorInfo& error);

private:
    explicit ErrorHandler(QObject *parent = nullptr);
    ~ErrorHandler();

    // Prevent copying
    ErrorHandler(const ErrorHandler&) = delete;
    ErrorHandler& operator=(const ErrorHandler&) = delete;

    static ErrorHandler* instance;
    static QMutex instanceMutex;

    void saveErrorToDatabase(const ErrorInfo& error);
    void saveErrorToFile(const ErrorInfo& error);
    QString getStackTrace();
    void notifyAdministrator(const ErrorInfo& error);
    QString formatErrorMessage(const ErrorInfo& error);
    
    // Security monitoring
    void detectAnomalies();
    void checkSecurityThresholds();
    bool isRateLimited(const QString& source);
    void sanitizeLogData(QString& data);
    
    // File rotation
    void rotateLogFiles();
    void archiveOldLogs();
    
    QMutex logMutex;
    QHash<QString, QDateTime> rateLimitMap;
    
    // Configuration
    int maxLogFileSize;
    int maxLogFiles;
    QString logDirectory;
    bool enableFileLogging;
    bool enableDatabaseLogging;
    bool enableSyslog;
    
    // Security thresholds
    int maxFailedLoginsPerMinute;
    int maxErrorsPerMinute;
    int maxDataAccessPerHour;
};

// Convenience macros
#define LOG_ERROR(source, msg) ErrorHandler::getInstance().logError(source, msg)
#define LOG_WARNING(source, msg) ErrorHandler::getInstance().logWarning(source, msg)
#define LOG_INFO(source, msg) ErrorHandler::getInstance().logInfo(source, msg)
#define LOG_CRITICAL(source, msg) ErrorHandler::getInstance().logCritical(source, msg)

#define LOG_SECURITY_EVENT(event, details) ErrorHandler::getInstance().logSecurityEvent(event, details)
#define LOG_DATA_ACCESS(table, id, type, user) ErrorHandler::getInstance().logDataAccess(table, id, type, user)
#define LOG_FAILED_AUTH(user, ip, reason) ErrorHandler::getInstance().logFailedAuthentication(user, ip, reason)

#define HANDLE_EXCEPTION(e, cat) ErrorHandler::getInstance().handleException(e, cat)
#define HANDLE_SYSTEM_ERROR(code, op, cat) ErrorHandler::getInstance().handleSystemError(code, op, cat)

#endif // ERRORHANDLER_H 
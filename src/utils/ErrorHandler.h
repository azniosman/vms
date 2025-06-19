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
    void logError(const QString& message, 
                  ErrorSeverity severity = ErrorSeverity::Error,
                  ErrorCategory category = ErrorCategory::Unknown,
                  const QString& details = QString());
    
    void logError(const QString& message, 
                  const QString& details,
                  ErrorSeverity severity = ErrorSeverity::Error,
                  ErrorCategory category = ErrorCategory::Unknown);

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
};

// Convenience macros
#define LOG_ERROR(msg, cat) ErrorHandler::getInstance().logError(msg, ErrorSeverity::Error, cat)
#define LOG_WARNING(msg, cat) ErrorHandler::getInstance().logError(msg, ErrorSeverity::Warning, cat)
#define LOG_INFO(msg, cat) ErrorHandler::getInstance().logError(msg, ErrorSeverity::Info, cat)
#define LOG_CRITICAL(msg, cat) ErrorHandler::getInstance().logError(msg, ErrorSeverity::Critical, cat)

#define HANDLE_EXCEPTION(e, cat) ErrorHandler::getInstance().handleException(e, cat)
#define HANDLE_SYSTEM_ERROR(code, op, cat) ErrorHandler::getInstance().handleSystemError(code, op, cat)

#endif // ERRORHANDLER_H 
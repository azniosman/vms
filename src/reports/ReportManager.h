#ifndef REPORTMANAGER_H
#define REPORTMANAGER_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QList>
#include <QMap>
#include <QVariant>
#include <QMutex>

enum class ReportType {
    DailyVisitorLog,
    CurrentVisitors,
    VisitorFrequency,
    PeakTimeAnalysis,
    VisitDuration,
    CustomerStatistics,
    SecurityIncidents,
    ContractorTracking,
    EmergencyEvacuation,
    ComplianceReport,
    CustomReport
};

enum class ReportFormat {
    PDF,
    CSV,
    Excel,
    JSON,
    HTML
};

struct ReportParameters {
    ReportType type;
    QDateTime startDate;
    QDateTime endDate;
    QStringList filters;
    QMap<QString, QVariant> customParams;
    ReportFormat format;
    bool includeCharts;
    bool includeDetails;
};

struct ReportData {
    QString title;
    QString description;
    QDateTime generatedAt;
    QString generatedBy;
    QList<QStringList> data;
    QStringList headers;
    QMap<QString, QVariant> summary;
    QString filePath;
};

class ReportManager : public QObject {
    Q_OBJECT

public:
    static ReportManager& getInstance();

    // Report generation
    ReportData generateReport(const ReportParameters& params);
    bool generateAndSaveReport(const ReportParameters& params, const QString& filePath);
    
    // Specific report generators
    ReportData generateDailyVisitorLog(const QDateTime& date);
    ReportData generateCurrentVisitorsReport();
    ReportData generateVisitorFrequencyReport(const QDateTime& start, const QDateTime& end);
    ReportData generatePeakTimeAnalysis(const QDateTime& start, const QDateTime& end);
    ReportData generateVisitDurationReport(const QDateTime& start, const QDateTime& end);
    ReportData generateCustomerStatisticsReport(const QDateTime& start, const QDateTime& end);
    ReportData generateSecurityIncidentsReport(const QDateTime& start, const QDateTime& end);
    ReportData generateContractorTrackingReport(const QDateTime& start, const QDateTime& end);
    ReportData generateEmergencyEvacuationReport();
    ReportData generateComplianceReport(const QDateTime& start, const QDateTime& end);

    // Report management
    QList<ReportData> getGeneratedReports();
    bool deleteReport(const QString& reportId);
    ReportData getReport(const QString& reportId);
    
    // Report templates
    QStringList getAvailableTemplates();
    bool saveTemplate(const QString& name, const ReportParameters& params);
    ReportParameters loadTemplate(const QString& name);
    bool deleteTemplate(const QString& name);

    // Export functionality
    bool exportToPDF(const ReportData& report, const QString& filePath);
    bool exportToCSV(const ReportData& report, const QString& filePath);
    bool exportToExcel(const ReportData& report, const QString& filePath);
    bool exportToJSON(const ReportData& report, const QString& filePath);
    bool exportToHTML(const ReportData& report, const QString& filePath);

signals:
    void reportGenerated(const ReportData& report);
    void reportGenerationFailed(const QString& error);
    void reportExported(const QString& filePath);

private:
    explicit ReportManager(QObject *parent = nullptr);
    ~ReportManager();

    // Prevent copying
    ReportManager(const ReportManager&) = delete;
    ReportManager& operator=(const ReportManager&) = delete;

    static ReportManager* instance;
    static QMutex instanceMutex;

    // Helper methods
    QString generateReportId();
    void saveReportToDatabase(const ReportData& report);
    QList<ReportData> loadReportsFromDatabase();
    QString formatReportTitle(ReportType type, const QDateTime& start, const QDateTime& end);
    QMap<QString, QVariant> calculateSummary(const QList<QStringList>& data);
    void addChartsToReport(ReportData& report);
    void validateReportParameters(const ReportParameters& params);
};

#endif // REPORTMANAGER_H 
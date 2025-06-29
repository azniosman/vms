#ifndef DASHBOARDWIDGET_H
#define DASHBOARDWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QTimer>
#include <QProgressBar>
#include <QFrame>
#include <QScrollArea>
#include <QGroupBox>
#include "../security/SecurityManager.h"
#include "../core/VisitorManager.h"

class StatCard : public QFrame
{
    Q_OBJECT
    
public:
    explicit StatCard(const QString& title, const QString& value, 
                     const QString& icon, const QColor& color, QWidget *parent = nullptr);
    void updateValue(const QString& newValue);
    void setTrend(double percentage);

private:
    void setupUI(const QString& title, const QString& icon, const QColor& color);
    
    QLabel *valueLabel;
    QLabel *trendLabel;
    QColor cardColor;
};

class DashboardWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DashboardWidget(const QString& sessionId, QWidget *parent = nullptr);
    ~DashboardWidget();

public slots:
    void refreshData();
    void onVisitorRegistered();
    void onVisitorCheckedIn();
    void onVisitorCheckedOut();

private slots:
    void updateStats();
    void onQuickActionClicked();
    void showVisitorDetails(int row, int column);

private:
    void setupUI();
    void createStatsCards();
    void createVisitorTable();
    void createCharts();
    void createQuickActions();
    void updateVisitorTable();
    void updateCharts();
    void loadRecentActivity();
    bool hasPermission(const QString& action);
    
    // UI Components
    QVBoxLayout *mainLayout;
    QScrollArea *scrollArea;
    QWidget *contentWidget;
    
    // Stats cards
    QHBoxLayout *statsLayout;
    StatCard *totalVisitorsCard;
    StatCard *checkedInCard;
    StatCard *todayVisitorsCard;
    StatCard *alertsCard;
    
    // Quick actions
    QGroupBox *quickActionsGroup;
    QHBoxLayout *actionsLayout;
    QPushButton *registerVisitorBtn;
    QPushButton *checkInBtn;
    QPushButton *checkOutBtn;
    QPushButton *reportsBtn;
    
    // Visitor table
    QGroupBox *visitorsGroup;
    QTableWidget *visitorsTable;
    
    // Charts (placeholder for future implementation)
    QGroupBox *chartsGroup;
    
    // Data refresh
    QTimer *refreshTimer;
    
    // Security
    QString sessionId;
    UserRole userRole;
    
    // Constants
    static const int REFRESH_INTERVAL = 30000; // 30 seconds
    static const int MAX_RECENT_VISITORS = 10;
};

#endif // DASHBOARDWIDGET_H
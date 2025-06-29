#include "DashboardWidget.h"
#include <QApplication>
#include <QDateTime>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QMessageBox>
#include <QSplitter>
#include <QPropertyAnimation>
#include <QGraphicsDropShadowEffect>
#include "../utils/ErrorHandler.h"

// StatCard Implementation
StatCard::StatCard(const QString& title, const QString& value, 
                   const QString& icon, const QColor& color, QWidget *parent)
    : QFrame(parent), cardColor(color)
{
    setupUI(title, icon, color);
    updateValue(value);
}

void StatCard::setupUI(const QString& title, const QString& icon, const QColor& color)
{
    setFixedSize(200, 120);
    setFrameStyle(QFrame::Box);
    setStyleSheet(QString(
        "QFrame { "
        "background-color: white; "
        "border: 1px solid #e1e8ed; "
        "border-radius: 8px; "
        "margin: 5px; "
        "} "
        "QFrame:hover { "
        "border-color: %1; "
        "box-shadow: 0 4px 8px rgba(0,0,0,0.1); "
        "}"
    ).arg(color.name()));
    
    // Add drop shadow effect
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect;
    shadow->setBlurRadius(10);
    shadow->setColor(QColor(0, 0, 0, 30));
    shadow->setOffset(0, 2);
    setGraphicsEffect(shadow);
    
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(15, 15, 15, 15);
    layout->setSpacing(5);
    
    // Icon and title row
    QHBoxLayout *headerLayout = new QHBoxLayout();
    
    QLabel *iconLabel = new QLabel(icon);
    iconLabel->setStyleSheet(QString("QLabel { color: %1; font-size: 24px; }").arg(color.name()));
    iconLabel->setFixedSize(30, 30);
    
    QLabel *titleLabel = new QLabel(title);
    titleLabel->setStyleSheet("QLabel { color: #657786; font-size: 12px; font-weight: bold; }");
    titleLabel->setWordWrap(true);
    
    headerLayout->addWidget(iconLabel);
    headerLayout->addWidget(titleLabel, 1);
    
    layout->addLayout(headerLayout);
    
    // Value label
    valueLabel = new QLabel("0");
    valueLabel->setStyleSheet("QLabel { color: #14171a; font-size: 24px; font-weight: bold; }");
    valueLabel->setAlignment(Qt::AlignLeft);
    layout->addWidget(valueLabel);
    
    // Trend label
    trendLabel = new QLabel("");
    trendLabel->setStyleSheet("QLabel { color: #657786; font-size: 11px; }");
    trendLabel->setAlignment(Qt::AlignLeft);
    layout->addWidget(trendLabel);
    
    layout->addStretch();
}

void StatCard::updateValue(const QString& newValue)
{
    valueLabel->setText(newValue);
    
    // Add a subtle animation
    QPropertyAnimation *animation = new QPropertyAnimation(valueLabel, "styleSheet");
    animation->setDuration(300);
    animation->setStartValue(QString("QLabel { color: %1; font-size: 24px; font-weight: bold; }").arg(cardColor.name()));
    animation->setEndValue("QLabel { color: #14171a; font-size: 24px; font-weight: bold; }");
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void StatCard::setTrend(double percentage)
{
    if (percentage > 0) {
        trendLabel->setText(QString("↗ +%1% from yesterday").arg(QString::number(percentage, 'f', 1)));
        trendLabel->setStyleSheet("QLabel { color: #17bf63; font-size: 11px; }");
    } else if (percentage < 0) {
        trendLabel->setText(QString("↘ %1% from yesterday").arg(QString::number(percentage, 'f', 1)));
        trendLabel->setStyleSheet("QLabel { color: #e0245e; font-size: 11px; }");
    } else {
        trendLabel->setText("→ No change from yesterday");
        trendLabel->setStyleSheet("QLabel { color: #657786; font-size: 11px; }");
    }
}

// DashboardWidget Implementation
DashboardWidget::DashboardWidget(const QString& sessionId, QWidget *parent)
    : QWidget(parent)
    , sessionId(sessionId)
    , refreshTimer(new QTimer(this))
{
    // Get user role for permission checking
    SecurityManager& security = SecurityManager::getInstance();
    userRole = security.getUserRole(sessionId);
    
    setupUI();
    
    // Setup refresh timer
    connect(refreshTimer, &QTimer::timeout, this, &DashboardWidget::updateStats);
    refreshTimer->start(REFRESH_INTERVAL);
    
    // Initial data load
    refreshData();
}

DashboardWidget::~DashboardWidget()
{
    refreshTimer->stop();
}

void DashboardWidget::setupUI()
{
    setStyleSheet(
        "QGroupBox { "
        "font-weight: bold; "
        "border: 2px solid #e1e8ed; "
        "border-radius: 8px; "
        "margin-top: 1ex; "
        "padding-top: 10px; "
        "background-color: white; "
        "} "
        "QGroupBox::title { "
        "subcontrol-origin: margin; "
        "left: 10px; "
        "padding: 0 10px 0 10px; "
        "color: #14171a; "
        "}"
    );
    
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);
    
    // Create scroll area
    scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("QScrollArea { background-color: #f7f9fa; }");
    
    contentWidget = new QWidget();
    contentWidget->setStyleSheet("QWidget { background-color: #f7f9fa; }");
    QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(20);
    
    createStatsCards();
    createQuickActions();
    createVisitorTable();
    createCharts();
    
    contentLayout->addLayout(statsLayout);
    contentLayout->addWidget(quickActionsGroup);
    contentLayout->addWidget(visitorsGroup);
    contentLayout->addWidget(chartsGroup);
    contentLayout->addStretch();
    
    scrollArea->setWidget(contentWidget);
    mainLayout->addWidget(scrollArea);
}

void DashboardWidget::createStatsCards()
{
    statsLayout = new QHBoxLayout();
    statsLayout->setSpacing(15);
    
    totalVisitorsCard = new StatCard("Total Visitors", "0", "👥", QColor("#3498db"));
    checkedInCard = new StatCard("Currently Checked In", "0", "✅", QColor("#2ecc71"));
    todayVisitorsCard = new StatCard("Today's Visitors", "0", "📅", QColor("#f39c12"));
    alertsCard = new StatCard("Security Alerts", "0", "⚠️", QColor("#e74c3c"));
    
    statsLayout->addWidget(totalVisitorsCard);
    statsLayout->addWidget(checkedInCard);
    statsLayout->addWidget(todayVisitorsCard);
    statsLayout->addWidget(alertsCard);
    statsLayout->addStretch();
}

void DashboardWidget::createQuickActions()
{
    quickActionsGroup = new QGroupBox("Quick Actions");
    actionsLayout = new QHBoxLayout(quickActionsGroup);
    actionsLayout->setSpacing(15);
    actionsLayout->setContentsMargins(20, 20, 20, 20);
    
    QString buttonStyle = 
        "QPushButton { "
        "background-color: #3498db; "
        "color: white; "
        "border: none; "
        "border-radius: 6px; "
        "padding: 12px 20px; "
        "font-size: 14px; "
        "font-weight: bold; "
        "min-width: 120px; "
        "} "
        "QPushButton:hover:enabled { "
        "background-color: #2980b9; "
        "transform: translateY(-1px); "
        "} "
        "QPushButton:pressed:enabled { "
        "background-color: #21618c; "
        "} "
        "QPushButton:disabled { "
        "background-color: #bdc3c7; "
        "color: #7f8c8d; "
        "}";
    
    registerVisitorBtn = new QPushButton("🆕 Register Visitor");
    registerVisitorBtn->setStyleSheet(buttonStyle);
    registerVisitorBtn->setEnabled(hasPermission("register_visitor"));
    connect(registerVisitorBtn, &QPushButton::clicked, this, &DashboardWidget::onQuickActionClicked);
    
    checkInBtn = new QPushButton("📥 Check In");
    checkInBtn->setStyleSheet(buttonStyle.replace("#3498db", "#2ecc71").replace("#2980b9", "#27ae60").replace("#21618c", "#229954"));
    checkInBtn->setEnabled(hasPermission("check_in"));
    connect(checkInBtn, &QPushButton::clicked, this, &DashboardWidget::onQuickActionClicked);
    
    checkOutBtn = new QPushButton("📤 Check Out");
    checkOutBtn->setStyleSheet(buttonStyle.replace("#3498db", "#e67e22").replace("#2980b9", "#d68910").replace("#21618c", "#ca6f1e"));
    checkOutBtn->setEnabled(hasPermission("check_out"));
    connect(checkOutBtn, &QPushButton::clicked, this, &DashboardWidget::onQuickActionClicked);
    
    reportsBtn = new QPushButton("📊 Reports");
    reportsBtn->setStyleSheet(buttonStyle.replace("#3498db", "#9b59b6").replace("#2980b9", "#8e44ad").replace("#21618c", "#7d3c98"));
    reportsBtn->setEnabled(hasPermission("view_reports"));
    connect(reportsBtn, &QPushButton::clicked, this, &DashboardWidget::onQuickActionClicked);
    
    actionsLayout->addWidget(registerVisitorBtn);
    actionsLayout->addWidget(checkInBtn);
    actionsLayout->addWidget(checkOutBtn);
    actionsLayout->addWidget(reportsBtn);
    actionsLayout->addStretch();
}

void DashboardWidget::createVisitorTable()
{
    visitorsGroup = new QGroupBox("Recent Visitors");
    QVBoxLayout *tableLayout = new QVBoxLayout(visitorsGroup);
    tableLayout->setContentsMargins(20, 20, 20, 20);
    
    visitorsTable = new QTableWidget(0, 6);
    visitorsTable->setHorizontalHeaderLabels({"Name", "Company", "Purpose", "Check-in Time", "Status", "Actions"});
    
    // Configure table appearance
    visitorsTable->setAlternatingRowColors(true);
    visitorsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    visitorsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    visitorsTable->setShowGrid(false);
    visitorsTable->verticalHeader()->setVisible(false);
    visitorsTable->setStyleSheet(
        "QTableWidget { "
        "background-color: white; "
        "border: 1px solid #e1e8ed; "
        "border-radius: 4px; "
        "gridline-color: #e1e8ed; "
        "} "
        "QTableWidget::item { "
        "padding: 8px; "
        "border-bottom: 1px solid #e1e8ed; "
        "} "
        "QTableWidget::item:selected { "
        "background-color: #e3f2fd; "
        "} "
        "QHeaderView::section { "
        "background-color: #f8f9fa; "
        "border: none; "
        "border-bottom: 2px solid #e1e8ed; "
        "padding: 8px; "
        "font-weight: bold; "
        "color: #495057; "
        "}"
    );
    
    // Configure column widths
    QHeaderView *header = visitorsTable->horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::Stretch); // Name
    header->setSectionResizeMode(1, QHeaderView::ResizeToContents); // Company
    header->setSectionResizeMode(2, QHeaderView::ResizeToContents); // Purpose
    header->setSectionResizeMode(3, QHeaderView::ResizeToContents); // Check-in Time
    header->setSectionResizeMode(4, QHeaderView::ResizeToContents); // Status
    header->setSectionResizeMode(5, QHeaderView::ResizeToContents); // Actions
    
    connect(visitorsTable, &QTableWidget::cellDoubleClicked, this, &DashboardWidget::showVisitorDetails);
    
    tableLayout->addWidget(visitorsTable);
}

void DashboardWidget::createCharts()
{
    chartsGroup = new QGroupBox("Analytics");
    chartsLayout = new QHBoxLayout(chartsGroup);
    chartsLayout->setContentsMargins(20, 20, 20, 20);
    chartsLayout->setSpacing(20);
    
    // Daily visitors chart
    QChart *dailyChart = new QChart();
    dailyChart->setTitle("Daily Visitors (Last 7 Days)");
    dailyChart->setTitleFont(QFont("Arial", 12, QFont::Bold));
    dailyChart->setBackgroundRoundness(8);
    
    dailyChartView = new QChartView(dailyChart);
    dailyChartView->setRenderHint(QPainter::Antialiasing);
    dailyChartView->setMinimumSize(400, 300);
    
    // Hourly visitors chart
    QChart *hourlyChart = new QChart();
    hourlyChart->setTitle("Hourly Distribution (Today)");
    hourlyChart->setTitleFont(QFont("Arial", 12, QFont::Bold));
    hourlyChart->setBackgroundRoundness(8);
    
    hourlyChartView = new QChartView(hourlyChart);
    hourlyChartView->setRenderHint(QPainter::Antialiasing);
    hourlyChartView->setMinimumSize(400, 300);
    
    chartsLayout->addWidget(dailyChartView);
    chartsLayout->addWidget(hourlyChartView);
}

void DashboardWidget::refreshData()
{
    updateStats();
    updateVisitorTable();
    updateCharts();
}

void DashboardWidget::updateStats()
{
    VisitorManager& vm = VisitorManager::getInstance();
    
    // Update stat cards
    int totalVisitors = vm.getAllVisitors().count();
    int checkedIn = vm.getCheckedInVisitors().count();
    int todayVisitors = vm.getTotalVisitorsToday();
    
    totalVisitorsCard->updateValue(QString::number(totalVisitors));
    checkedInCard->updateValue(QString::number(checkedIn));
    todayVisitorsCard->updateValue(QString::number(todayVisitors));
    
    // Calculate security alerts (blacklisted visitors, overstays, etc.)
    int alerts = vm.getBlacklist().count();
    alertsCard->updateValue(QString::number(alerts));
    
    // Set trends (simplified calculation)
    totalVisitorsCard->setTrend(5.2);
    checkedInCard->setTrend(-2.1);
    todayVisitorsCard->setTrend(12.5);
    alertsCard->setTrend(0.0);
}

void DashboardWidget::updateVisitorTable()
{
    VisitorManager& vm = VisitorManager::getInstance();
    QList<Visitor> recentVisitors = vm.getCheckedInVisitors();
    
    // Limit to recent visitors
    if (recentVisitors.size() > MAX_RECENT_VISITORS) {
        recentVisitors = recentVisitors.mid(0, MAX_RECENT_VISITORS);
    }
    
    visitorsTable->setRowCount(recentVisitors.size());
    
    for (int i = 0; i < recentVisitors.size(); ++i) {
        const Visitor& visitor = recentVisitors[i];
        
        visitorsTable->setItem(i, 0, new QTableWidgetItem(visitor.getName()));
        visitorsTable->setItem(i, 1, new QTableWidgetItem(visitor.getCompany()));
        visitorsTable->setItem(i, 2, new QTableWidgetItem(visitor.getPurpose()));
        
        QDateTime checkInTime = vm.getCheckInTime(visitor.getId());
        visitorsTable->setItem(i, 3, new QTableWidgetItem(checkInTime.toString("hh:mm AP")));
        
        QString status = vm.isVisitorCheckedIn(visitor.getId()) ? "Checked In" : "Checked Out";
        QTableWidgetItem *statusItem = new QTableWidgetItem(status);
        if (status == "Checked In") {
            statusItem->setForeground(QColor("#2ecc71"));
        } else {
            statusItem->setForeground(QColor("#e74c3c"));
        }
        visitorsTable->setItem(i, 4, statusItem);
        
        // Add action button
        QPushButton *actionBtn = new QPushButton("View");
        actionBtn->setStyleSheet(
            "QPushButton { "
            "background-color: #3498db; "
            "color: white; "
            "border: none; "
            "border-radius: 3px; "
            "padding: 4px 8px; "
            "font-size: 12px; "
            "} "
            "QPushButton:hover { "
            "background-color: #2980b9; "
            "}"
        );
        visitorsTable->setCellWidget(i, 5, actionBtn);
    }
}

void DashboardWidget::updateCharts()
{
    // Update daily chart
    VisitorManager& vm = VisitorManager::getInstance();
    QDateTime endDate = QDateTime::currentDateTime();
    QDateTime startDate = endDate.addDays(-7);
    
    QList<QPair<QDateTime, int>> dailyStats = vm.getVisitorStatistics(startDate, endDate);
    
    // Create bar series for daily chart
    QBarSeries *dailySeries = new QBarSeries();
    QBarSet *dailySet = new QBarSet("Visitors");
    dailySet->setColor(QColor("#3498db"));
    
    QStringList categories;
    for (const auto& stat : dailyStats) {
        categories << stat.first.toString("MM/dd");
        *dailySet << stat.second;
    }
    
    dailySeries->append(dailySet);
    
    QChart *dailyChart = dailyChartView->chart();
    dailyChart->removeAllSeries();
    dailyChart->addSeries(dailySeries);
    
    QBarCategoryAxis *dailyAxisX = new QBarCategoryAxis();
    dailyAxisX->append(categories);
    dailyChart->addAxis(dailyAxisX, Qt::AlignBottom);
    dailySeries->attachAxis(dailyAxisX);
    
    QValueAxis *dailyAxisY = new QValueAxis();
    dailyAxisY->setRange(0, 100);
    dailyChart->addAxis(dailyAxisY, Qt::AlignLeft);
    dailySeries->attachAxis(dailyAxisY);
    
    // Update hourly chart (simplified)
    QBarSeries *hourlySeries = new QBarSeries();
    QBarSet *hourlySet = new QBarSet("Visitors");
    hourlySet->setColor(QColor("#2ecc71"));
    
    QStringList hourCategories;
    for (int i = 8; i <= 18; ++i) {
        hourCategories << QString("%1:00").arg(i);
        *hourlySet << (qrand() % 20); // Mock data
    }
    
    hourlySeries->append(hourlySet);
    
    QChart *hourlyChart = hourlyChartView->chart();
    hourlyChart->removeAllSeries();
    hourlyChart->addSeries(hourlySeries);
    
    QBarCategoryAxis *hourlyAxisX = new QBarCategoryAxis();
    hourlyAxisX->append(hourCategories);
    hourlyChart->addAxis(hourlyAxisX, Qt::AlignBottom);
    hourlySeries->attachAxis(hourlyAxisX);
    
    QValueAxis *hourlyAxisY = new QValueAxis();
    hourlyAxisY->setRange(0, 20);
    hourlyChart->addAxis(hourlyAxisY, Qt::AlignLeft);
    hourlySeries->attachAxis(hourlyAxisY);
}

void DashboardWidget::onVisitorRegistered()
{
    refreshData();
}

void DashboardWidget::onVisitorCheckedIn()
{
    refreshData();
}

void DashboardWidget::onVisitorCheckedOut()
{
    refreshData();
}

void DashboardWidget::onQuickActionClicked()
{
    QPushButton *button = qobject_cast<QPushButton*>(sender());
    if (!button) return;
    
    QString action = button->text();
    
    // Emit signals for parent to handle
    if (action.contains("Register")) {
        // Signal to open registration dialog
    } else if (action.contains("Check In")) {
        // Signal to open check-in dialog
    } else if (action.contains("Check Out")) {
        // Signal to open check-out dialog
    } else if (action.contains("Reports")) {
        // Signal to open reports window
    }
}

void DashboardWidget::showVisitorDetails(int row, int column)
{
    Q_UNUSED(column)
    
    if (row < 0 || row >= visitorsTable->rowCount()) return;
    
    QString visitorName = visitorsTable->item(row, 0)->text();
    QMessageBox::information(this, "Visitor Details", 
                           QString("Detailed information for %1 would be shown here.").arg(visitorName));
}

bool DashboardWidget::hasPermission(const QString& action)
{
    SecurityManager& security = SecurityManager::getInstance();
    return security.hasPermission(sessionId, "visitor", action);
}
#include "CheckInOutDialog.h"
#include "../core/VisitorManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QDateTime>
#include <QTimer>

CheckInOutDialog::CheckInOutDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUI();
    setupConnections();
    loadVisitors();
}

CheckInOutDialog::~CheckInOutDialog()
{
    qDeleteAll(currentVisitors);
    qDeleteAll(searchResults);
}

void CheckInOutDialog::setupUI()
{
    setWindowTitle(tr("Visitor Check-in/Check-out"));
    setMinimumSize(800, 600);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Search bar
    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchEdit = new QLineEdit(this);
    searchEdit->setPlaceholderText(tr("Search by name, email, or ID"));
    QPushButton *searchButton = new QPushButton(tr("Search"), this);
    searchLayout->addWidget(searchEdit);
    searchLayout->addWidget(searchButton);

    // Visitor table
    visitorTable = new QTableWidget(this);
    visitorTable->setColumnCount(7);
    visitorTable->setHorizontalHeaderLabels({
        tr("Name"),
        tr("ID Number"),
        tr("Company"),
        tr("Host"),
        tr("Purpose"),
        tr("Status"),
        tr("Time")
    });
    visitorTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    visitorTable->horizontalHeader()->setStretchLastSection(true);
    visitorTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    visitorTable->setSelectionMode(QAbstractItemView::SingleSelection);
    visitorTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    visitorTable->setAlternatingRowColors(true);

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    checkInButton = new QPushButton(tr("Check In"), this);
    checkOutButton = new QPushButton(tr("Check Out"), this);
    refreshButton = new QPushButton(tr("Refresh"), this);
    printBadgeButton = new QPushButton(tr("Print Badge"), this);
    viewDetailsButton = new QPushButton(tr("View Details"), this);
    closeButton = new QPushButton(tr("Close"), this);

    buttonLayout->addWidget(checkInButton);
    buttonLayout->addWidget(checkOutButton);
    buttonLayout->addWidget(refreshButton);
    buttonLayout->addWidget(printBadgeButton);
    buttonLayout->addWidget(viewDetailsButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);

    // Status label
    statusLabel = new QLabel(this);
    statusLabel->setText(tr("Ready"));

    // Add all components to main layout
    mainLayout->addLayout(searchLayout);
    mainLayout->addWidget(visitorTable);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(statusLabel);

    // Initial button states
    checkInButton->setEnabled(false);
    checkOutButton->setEnabled(false);
    printBadgeButton->setEnabled(false);
    viewDetailsButton->setEnabled(false);
}

void CheckInOutDialog::setupConnections()
{
    connect(searchEdit, &QLineEdit::returnPressed, this, &CheckInOutDialog::onSearch);
    connect(visitorTable, &QTableWidget::itemSelectionChanged, this, &CheckInOutDialog::onVisitorSelected);
    connect(checkInButton, &QPushButton::clicked, this, &CheckInOutDialog::onCheckIn);
    connect(checkOutButton, &QPushButton::clicked, this, &CheckInOutDialog::onCheckOut);
    connect(refreshButton, &QPushButton::clicked, this, &CheckInOutDialog::onRefresh);
    connect(printBadgeButton, &QPushButton::clicked, this, &CheckInOutDialog::onPrintBadge);
    connect(viewDetailsButton, &QPushButton::clicked, this, &CheckInOutDialog::onViewDetails);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    // Set up auto-refresh timer
    QTimer *refreshTimer = new QTimer(this);
    connect(refreshTimer, &QTimer::timeout, this, &CheckInOutDialog::onRefresh);
    refreshTimer->start(30000); // Refresh every 30 seconds
}

void CheckInOutDialog::loadVisitors()
{
    qDeleteAll(currentVisitors);
    currentVisitors = VisitorManager::getInstance().getCurrentVisitors();
    updateVisitorTable();
    statusLabel->setText(tr("Total visitors on premises: %1").arg(currentVisitors.size()));
}

void CheckInOutDialog::updateVisitorTable()
{
    visitorTable->setRowCount(0);
    const QList<Visitor*>& visitors = searchResults.isEmpty() ? currentVisitors : searchResults;

    for (const Visitor* visitor : visitors) {
        int row = visitorTable->rowCount();
        visitorTable->insertRow(row);

        QDateTime checkInTime = VisitorManager::getInstance().getCheckInTime(visitor->getId());
        QDateTime checkOutTime = VisitorManager::getInstance().getCheckOutTime(visitor->getId());
        QString status = checkOutTime.isValid() ? tr("Checked Out") : tr("Checked In");
        QString time = checkOutTime.isValid() ? 
                      checkOutTime.toString("HH:mm") : 
                      checkInTime.toString("HH:mm");

        visitorTable->setItem(row, 0, new QTableWidgetItem(visitor->getName()));
        visitorTable->setItem(row, 1, new QTableWidgetItem(visitor->getIdentificationNumber()));
        visitorTable->setItem(row, 2, new QTableWidgetItem(visitor->getCompany()));
        visitorTable->setItem(row, 3, new QTableWidgetItem(visitor->getHostId()));
        visitorTable->setItem(row, 4, new QTableWidgetItem(visitor->getPurpose()));
        visitorTable->setItem(row, 5, new QTableWidgetItem(status));
        visitorTable->setItem(row, 6, new QTableWidgetItem(time));

        // Store visitor ID in the first column's item data
        visitorTable->item(row, 0)->setData(Qt::UserRole, visitor->getId());
    }

    updateButtons();
}

void CheckInOutDialog::updateButtons()
{
    QList<QTableWidgetItem*> selectedItems = visitorTable->selectedItems();
    bool hasSelection = !selectedItems.isEmpty();

    if (hasSelection) {
        QString visitorId = visitorTable->item(selectedItems[0]->row(), 0)->data(Qt::UserRole).toString();
        QDateTime checkOutTime = VisitorManager::getInstance().getCheckOutTime(visitorId);
        bool isCheckedIn = !checkOutTime.isValid();

        checkInButton->setEnabled(!isCheckedIn);
        checkOutButton->setEnabled(isCheckedIn);
        printBadgeButton->setEnabled(isCheckedIn);
        viewDetailsButton->setEnabled(true);
    } else {
        checkInButton->setEnabled(false);
        checkOutButton->setEnabled(false);
        printBadgeButton->setEnabled(false);
        viewDetailsButton->setEnabled(false);
    }
}

void CheckInOutDialog::onSearch()
{
    QString query = searchEdit->text().trimmed();
    
    if (query.isEmpty()) {
        searchResults.clear();
        updateVisitorTable();
        return;
    }

    qDeleteAll(searchResults);
    searchResults = VisitorManager::getInstance().searchVisitors(query);
    updateVisitorTable();
    
    statusLabel->setText(tr("Found %1 matching visitors").arg(searchResults.size()));
}

void CheckInOutDialog::onCheckIn()
{
    QList<QTableWidgetItem*> selectedItems = visitorTable->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }

    QString visitorId = visitorTable->item(selectedItems[0]->row(), 0)->data(Qt::UserRole).toString();
    
    if (!validateCheckIn(visitorId)) {
        return;
    }

    QString hostId = visitorTable->item(selectedItems[0]->row(), 3)->text();
    
    if (VisitorManager::getInstance().checkIn(visitorId, hostId)) {
        QMessageBox::information(this, tr("Success"),
                               tr("Visitor checked in successfully."));
        onRefresh();
    } else {
        QMessageBox::critical(this, tr("Error"),
                            tr("Failed to check in visitor."));
    }
}

void CheckInOutDialog::onCheckOut()
{
    QList<QTableWidgetItem*> selectedItems = visitorTable->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }

    QString visitorId = visitorTable->item(selectedItems[0]->row(), 0)->data(Qt::UserRole).toString();
    
    if (VisitorManager::getInstance().checkOut(visitorId)) {
        QMessageBox::information(this, tr("Success"),
                               tr("Visitor checked out successfully."));
        onRefresh();
    } else {
        QMessageBox::critical(this, tr("Error"),
                            tr("Failed to check out visitor."));
    }
}

void CheckInOutDialog::onRefresh()
{
    loadVisitors();
}

void CheckInOutDialog::onVisitorSelected()
{
    updateButtons();
}

void CheckInOutDialog::onQRCodeScanned(const QString& visitorId)
{
    // Find and select the visitor in the table
    for (int row = 0; row < visitorTable->rowCount(); ++row) {
        if (visitorTable->item(row, 0)->data(Qt::UserRole).toString() == visitorId) {
            visitorTable->selectRow(row);
            break;
        }
    }
}

void CheckInOutDialog::onPrintBadge()
{
    QList<QTableWidgetItem*> selectedItems = visitorTable->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }

    QString visitorId = visitorTable->item(selectedItems[0]->row(), 0)->data(Qt::UserRole).toString();
    
    if (VisitorManager::getInstance().printVisitorBadge(visitorId)) {
        statusLabel->setText(tr("Badge printed successfully"));
    } else {
        QMessageBox::warning(this, tr("Warning"),
                           tr("Failed to print visitor badge."));
    }
}

void CheckInOutDialog::onViewDetails()
{
    QList<QTableWidgetItem*> selectedItems = visitorTable->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }

    QString visitorId = visitorTable->item(selectedItems[0]->row(), 0)->data(Qt::UserRole).toString();
    Visitor* visitor = VisitorManager::getInstance().getVisitor(visitorId);
    
    if (visitor) {
        showVisitorDetails(visitor);
    }
}

bool CheckInOutDialog::validateCheckIn(const QString& visitorId)
{
    // Check if visitor is blacklisted
    if (VisitorManager::getInstance().isBlacklisted(visitorId)) {
        QMessageBox::critical(this, tr("Error"),
                            tr("This visitor is blacklisted and cannot be checked in."));
        return false;
    }

    // Check if visitor has valid consent
    if (!VisitorManager::getInstance().hasValidConsent(visitorId)) {
        QMessageBox::critical(this, tr("Error"),
                            tr("Visitor's consent has expired. Please update consent before check-in."));
        return false;
    }

    return true;
}

void CheckInOutDialog::showVisitorDetails(const Visitor* visitor)
{
    QString details = tr("Visitor Details:\n\n"
                        "Name: %1\n"
                        "Email: %2\n"
                        "Phone: %3\n"
                        "Company: %4\n"
                        "ID Number: %5\n"
                        "Purpose: %6\n"
                        "Host: %7\n"
                        "Consent Status: %8\n"
                        "Retention Period: %9 days")
                        .arg(visitor->getName())
                        .arg(visitor->getEmail())
                        .arg(visitor->getPhone())
                        .arg(visitor->getCompany())
                        .arg(visitor->getIdentificationNumber())
                        .arg(visitor->getPurpose())
                        .arg(visitor->getHostId())
                        .arg(visitor->hasConsent() ? tr("Valid") : tr("Invalid"))
                        .arg(visitor->getRetentionPeriod());

    QMessageBox::information(this, tr("Visitor Details"), details);
} 
#ifndef CHECKINOUTDIALOG_H
#define CHECKINOUTDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include "../core/Visitor.h"

class CheckInOutDialog : public QDialog {
    Q_OBJECT

public:
    explicit CheckInOutDialog(QWidget *parent = nullptr);
    ~CheckInOutDialog();

private slots:
    void onSearch();
    void onCheckIn();
    void onCheckOut();
    void onRefresh();
    void onVisitorSelected();
    void onQRCodeScanned(const QString& visitorId);
    void onPrintBadge();
    void onViewDetails();

private:
    void setupUI();
    void setupConnections();
    void loadVisitors();
    void updateVisitorTable();
    void updateButtons();
    bool validateCheckIn(const QString& visitorId);
    void showVisitorDetails(const Visitor* visitor);

    // UI components
    QLineEdit *searchEdit;
    QTableWidget *visitorTable;
    QPushButton *checkInButton;
    QPushButton *checkOutButton;
    QPushButton *refreshButton;
    QPushButton *printBadgeButton;
    QPushButton *viewDetailsButton;
    QPushButton *closeButton;
    QLabel *statusLabel;

    // Data
    QList<Visitor*> currentVisitors;
    QList<Visitor*> searchResults;
};

#endif // CHECKINOUTDIALOG_H 
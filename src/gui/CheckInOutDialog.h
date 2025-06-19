#ifndef CHECKINOUTDIALOG_H
#define CHECKINOUTDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
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
    void onValidateConsent();

private:
    void setupUI();
    void setupConnections();
    void loadCurrentVisitors();
    void updateVisitorList();
    void updateButtons();
    bool validateCheckIn(const QString& visitorId);
    void showVisitorDetails(const Visitor& visitor);
    void searchVisitors();
    void checkInVisitor();
    void checkOutVisitor();
    void printBadge();
    void validateConsent();
    void getVisitorDetails();
    void clearForm();

    // UI components
    QLineEdit *searchLineEdit;
    QListWidget *visitorListWidget;
    QPushButton *checkInButton;
    QPushButton *checkOutButton;
    QPushButton *refreshButton;
    QPushButton *printBadgeButton;
    QPushButton *viewDetailsButton;
    QPushButton *validateConsentButton;
    QPushButton *closeButton;
    QLabel *statusLabel;
    QLineEdit *hostIdLineEdit;

    // Data
    QList<Visitor> currentVisitors;
    QList<Visitor> searchResults;
    QString selectedVisitorId;
};

#endif // CHECKINOUTDIALOG_H 
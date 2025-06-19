#ifndef VISITORREGISTRATIONDIALOG_H
#define VISITORREGISTRATIONDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QCheckBox>
#include <QDateEdit>
#include <QTimeEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QBuffer>
#include <QCoreApplication>
#include <QDebug>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include "../core/Visitor.h"

class VisitorRegistrationDialog : public QDialog {
    Q_OBJECT

public:
    explicit VisitorRegistrationDialog(QWidget *parent = nullptr);
    ~VisitorRegistrationDialog();

    Visitor getVisitor() const;

private slots:
    void onRegisterClicked();
    void onCancelClicked();
    void onPhotoCaptureClicked();
    void onIdScanCaptureClicked();
    void onSignatureCaptureClicked();
    void onConsentChanged(bool checked);

private:
    void setupUI();
    void setupConnections();
    bool validateForm();
    void clearForm();
    void capturePhoto();
    void captureIdScan();
    void captureSignature();

    // Form fields
    QLineEdit *nameEdit;
    QLineEdit *emailEdit;
    QLineEdit *phoneEdit;
    QLineEdit *companyEdit;
    QLineEdit *idNumberEdit;
    QComboBox *visitorTypeCombo;
    QLineEdit *hostIdEdit;
    QLineEdit *purposeEdit;
    QSpinBox *retentionPeriodSpin;
    QCheckBox *consentCheckBox;
    
    // Photo and document capture
    QLabel *photoLabel;
    QLabel *idScanLabel;
    QLabel *signatureLabel;
    QPushButton *photoCaptureButton;
    QPushButton *idScanCaptureButton;
    QPushButton *signatureCaptureButton;
    
    // Buttons
    QPushButton *registerButton;
    QPushButton *cancelButton;
    
    // Data
    QPixmap capturedPhoto;
    QPixmap capturedIdScan;
    QString capturedSignature;
    bool isCapturingPhoto;
    bool isCapturingId;
    bool isCapturingSignature;
};

#endif // VISITORREGISTRATIONDIALOG_H 
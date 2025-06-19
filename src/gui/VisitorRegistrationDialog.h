#ifndef VISITORREGISTRATIONDIALOG_H
#define VISITORREGISTRATIONDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QCamera>
#include <QCameraImageCapture>
#include "../core/Visitor.h"

class VisitorRegistrationDialog : public QDialog {
    Q_OBJECT

public:
    explicit VisitorRegistrationDialog(QWidget *parent = nullptr);
    ~VisitorRegistrationDialog();

private slots:
    void onCapturePhoto();
    void onScanId();
    void onCaptureSignature();
    void onRegister();
    void onCancel();
    void onConsentToggled(bool checked);
    void onImageCaptured(int id, const QImage &preview);
    void onCameraError(QCamera::Error error);

private:
    void setupUI();
    void setupCamera();
    void validateForm();
    bool saveVisitor();
    void clearForm();

    // Form fields
    QLineEdit *nameEdit;
    QLineEdit *emailEdit;
    QLineEdit *phoneEdit;
    QLineEdit *companyEdit;
    QLineEdit *idNumberEdit;
    QComboBox *visitorTypeCombo;
    QLabel *photoLabel;
    QLabel *idScanLabel;
    QLabel *signatureLabel;
    QLineEdit *hostIdEdit;
    QLineEdit *purposeEdit;
    QSpinBox *retentionPeriodSpin;
    QCheckBox *consentCheck;

    // Buttons
    QPushButton *photoButton;
    QPushButton *scanIdButton;
    QPushButton *signatureButton;
    QPushButton *registerButton;
    QPushButton *cancelButton;

    // Camera components
    QCamera *camera;
    QCameraImageCapture *imageCapture;
    bool isCapturingPhoto;
    bool isCapturingId;

    // Captured data
    QImage photoImage;
    QImage idScanImage;
    QString signatureData;
};

#endif // VISITORREGISTRATIONDIALOG_H 
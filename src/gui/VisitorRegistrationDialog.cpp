#include "VisitorRegistrationDialog.h"
#include "../core/VisitorManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QFileDialog>
#include <QImageWriter>
#include <QBuffer>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

VisitorRegistrationDialog::VisitorRegistrationDialog(QWidget *parent)
    : QDialog(parent)
    , camera(nullptr)
    , imageCapture(nullptr)
    , isCapturingPhoto(false)
    , isCapturingId(false)
{
    setupUI();
    setupCamera();
}

VisitorRegistrationDialog::~VisitorRegistrationDialog()
{
    delete camera;
    delete imageCapture;
}

void VisitorRegistrationDialog::setupUI()
{
    setWindowTitle(tr("Register New Visitor"));
    setMinimumWidth(600);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QFormLayout *formLayout = new QFormLayout();

    // Create form fields
    nameEdit = new QLineEdit(this);
    nameEdit->setPlaceholderText(tr("Enter full name"));

    emailEdit = new QLineEdit(this);
    emailEdit->setPlaceholderText(tr("Enter email address"));
    QRegularExpression emailRegex("\\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,}\\b");
    emailEdit->setValidator(new QRegularExpressionValidator(emailRegex, this));

    phoneEdit = new QLineEdit(this);
    phoneEdit->setPlaceholderText(tr("Enter phone number"));
    phoneEdit->setValidator(new QRegularExpressionValidator(QRegularExpression("\\+?\\d+"), this));

    companyEdit = new QLineEdit(this);
    companyEdit->setPlaceholderText(tr("Enter company name"));

    idNumberEdit = new QLineEdit(this);
    idNumberEdit->setPlaceholderText(tr("Enter ID/Passport number"));

    visitorTypeCombo = new QComboBox(this);
    visitorTypeCombo->addItem(tr("Guest"), static_cast<int>(Visitor::VisitorType::Guest));
    visitorTypeCombo->addItem(tr("Contractor"), static_cast<int>(Visitor::VisitorType::Contractor));
    visitorTypeCombo->addItem(tr("Delivery"), static_cast<int>(Visitor::VisitorType::Delivery));
    visitorTypeCombo->addItem(tr("Interview"), static_cast<int>(Visitor::VisitorType::Interview));
    visitorTypeCombo->addItem(tr("Other"), static_cast<int>(Visitor::VisitorType::Other));

    hostIdEdit = new QLineEdit(this);
    hostIdEdit->setPlaceholderText(tr("Enter host ID or name"));

    purposeEdit = new QLineEdit(this);
    purposeEdit->setPlaceholderText(tr("Enter purpose of visit"));

    retentionPeriodSpin = new QSpinBox(this);
    retentionPeriodSpin->setMinimum(1);
    retentionPeriodSpin->setMaximum(365);
    retentionPeriodSpin->setValue(30);
    retentionPeriodSpin->setSuffix(tr(" days"));

    // Photo capture
    photoLabel = new QLabel(this);
    photoLabel->setFixedSize(120, 160);
    photoLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    photoLabel->setAlignment(Qt::AlignCenter);
    photoLabel->setText(tr("No Photo"));

    photoButton = new QPushButton(tr("Capture Photo"), this);
    connect(photoButton, &QPushButton::clicked, this, &VisitorRegistrationDialog::onCapturePhoto);

    // ID scan
    idScanLabel = new QLabel(this);
    idScanLabel->setFixedSize(320, 200);
    idScanLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    idScanLabel->setAlignment(Qt::AlignCenter);
    idScanLabel->setText(tr("No ID Scan"));

    scanIdButton = new QPushButton(tr("Scan ID"), this);
    connect(scanIdButton, &QPushButton::clicked, this, &VisitorRegistrationDialog::onScanId);

    // Signature
    signatureLabel = new QLabel(this);
    signatureLabel->setFixedSize(320, 100);
    signatureLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    signatureLabel->setAlignment(Qt::AlignCenter);
    signatureLabel->setText(tr("No Signature"));

    signatureButton = new QPushButton(tr("Capture Signature"), this);
    connect(signatureButton, &QPushButton::clicked, this, &VisitorRegistrationDialog::onCaptureSignature);

    // Consent checkbox
    consentCheck = new QCheckBox(tr("I consent to the collection and processing of my personal data"), this);
    connect(consentCheck, &QCheckBox::toggled, this, &VisitorRegistrationDialog::onConsentToggled);

    // Add fields to form layout
    formLayout->addRow(tr("Name:"), nameEdit);
    formLayout->addRow(tr("Email:"), emailEdit);
    formLayout->addRow(tr("Phone:"), phoneEdit);
    formLayout->addRow(tr("Company:"), companyEdit);
    formLayout->addRow(tr("ID Number:"), idNumberEdit);
    formLayout->addRow(tr("Visitor Type:"), visitorTypeCombo);
    formLayout->addRow(tr("Host:"), hostIdEdit);
    formLayout->addRow(tr("Purpose:"), purposeEdit);
    formLayout->addRow(tr("Retention Period:"), retentionPeriodSpin);

    // Create photo layout
    QVBoxLayout *photoLayout = new QVBoxLayout();
    photoLayout->addWidget(photoLabel);
    photoLayout->addWidget(photoButton);

    // Create ID scan layout
    QVBoxLayout *idScanLayout = new QVBoxLayout();
    idScanLayout->addWidget(idScanLabel);
    idScanLayout->addWidget(scanIdButton);

    // Create signature layout
    QVBoxLayout *signatureLayout = new QVBoxLayout();
    signatureLayout->addWidget(signatureLabel);
    signatureLayout->addWidget(signatureButton);

    // Create media layout
    QHBoxLayout *mediaLayout = new QHBoxLayout();
    mediaLayout->addLayout(photoLayout);
    mediaLayout->addLayout(idScanLayout);
    mediaLayout->addLayout(signatureLayout);

    // Create buttons
    registerButton = new QPushButton(tr("Register"), this);
    registerButton->setEnabled(false);
    connect(registerButton, &QPushButton::clicked, this, &VisitorRegistrationDialog::onRegister);

    cancelButton = new QPushButton(tr("Cancel"), this);
    connect(cancelButton, &QPushButton::clicked, this, &VisitorRegistrationDialog::onCancel);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(registerButton);
    buttonLayout->addWidget(cancelButton);

    // Add layouts to main layout
    mainLayout->addLayout(formLayout);
    mainLayout->addLayout(mediaLayout);
    mainLayout->addWidget(consentCheck);
    mainLayout->addLayout(buttonLayout);
}

void VisitorRegistrationDialog::setupCamera()
{
    camera = new QCamera(this);
    imageCapture = new QCameraImageCapture(camera, this);

    connect(imageCapture, &QCameraImageCapture::imageCaptured,
            this, &VisitorRegistrationDialog::onImageCaptured);
    connect(camera, QOverload<QCamera::Error>::of(&QCamera::error),
            this, &VisitorRegistrationDialog::onCameraError);
}

void VisitorRegistrationDialog::onCapturePhoto()
{
    isCapturingPhoto = true;
    isCapturingId = false;
    camera->start();
    imageCapture->capture();
}

void VisitorRegistrationDialog::onScanId()
{
    isCapturingPhoto = false;
    isCapturingId = true;
    camera->start();
    imageCapture->capture();
}

void VisitorRegistrationDialog::onCaptureSignature()
{
    // TODO: Implement signature capture
    // This could be implemented using a QWidget for drawing or a signature pad device
}

void VisitorRegistrationDialog::onRegister()
{
    if (!validateForm()) {
        return;
    }

    if (saveVisitor()) {
        QMessageBox::information(this, tr("Success"),
                               tr("Visitor registered successfully."));
        accept();
    } else {
        QMessageBox::critical(this, tr("Error"),
                            tr("Failed to register visitor."));
    }
}

void VisitorRegistrationDialog::onCancel()
{
    if (QMessageBox::question(this, tr("Confirm Cancel"),
                            tr("Are you sure you want to cancel registration?"),
                            QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        reject();
    }
}

void VisitorRegistrationDialog::onConsentToggled(bool checked)
{
    registerButton->setEnabled(checked && !nameEdit->text().isEmpty() &&
                             !emailEdit->text().isEmpty());
}

void VisitorRegistrationDialog::onImageCaptured(int id, const QImage &preview)
{
    Q_UNUSED(id);

    if (isCapturingPhoto) {
        photoImage = preview;
        QPixmap pixmap = QPixmap::fromImage(preview).scaled(photoLabel->size(),
                                                          Qt::KeepAspectRatio,
                                                          Qt::SmoothTransformation);
        photoLabel->setPixmap(pixmap);
    } else if (isCapturingId) {
        idScanImage = preview;
        QPixmap pixmap = QPixmap::fromImage(preview).scaled(idScanLabel->size(),
                                                          Qt::KeepAspectRatio,
                                                          Qt::SmoothTransformation);
        idScanLabel->setPixmap(pixmap);
    }

    camera->stop();
}

void VisitorRegistrationDialog::onCameraError(QCamera::Error error)
{
    QString errorMessage;
    switch (error) {
        case QCamera::NoError:
            return;
        case QCamera::CameraError:
            errorMessage = tr("Camera error occurred");
            break;
        case QCamera::InvalidRequestError:
            errorMessage = tr("Invalid camera request");
            break;
        case QCamera::NotSupportedFeatureError:
            errorMessage = tr("Camera feature not supported");
            break;
        default:
            errorMessage = tr("Unknown camera error");
    }

    QMessageBox::warning(this, tr("Camera Error"), errorMessage);
}

void VisitorRegistrationDialog::validateForm()
{
    bool isValid = true;
    QString errorMessage;

    if (nameEdit->text().isEmpty()) {
        errorMessage += tr("- Name is required\n");
        isValid = false;
    }

    if (emailEdit->text().isEmpty()) {
        errorMessage += tr("- Email is required\n");
        isValid = false;
    }

    if (idNumberEdit->text().isEmpty()) {
        errorMessage += tr("- ID number is required\n");
        isValid = false;
    }

    if (hostIdEdit->text().isEmpty()) {
        errorMessage += tr("- Host information is required\n");
        isValid = false;
    }

    if (purposeEdit->text().isEmpty()) {
        errorMessage += tr("- Purpose of visit is required\n");
        isValid = false;
    }

    if (!consentCheck->isChecked()) {
        errorMessage += tr("- Consent is required\n");
        isValid = false;
    }

    if (!isValid) {
        QMessageBox::warning(this, tr("Validation Error"),
                           tr("Please correct the following errors:\n\n") + errorMessage);
    }
}

bool VisitorRegistrationDialog::saveVisitor()
{
    Visitor visitor;
    visitor.setName(nameEdit->text());
    visitor.setEmail(emailEdit->text());
    visitor.setPhone(phoneEdit->text());
    visitor.setCompany(companyEdit->text());
    visitor.setIdentificationNumber(idNumberEdit->text());
    visitor.setType(static_cast<Visitor::VisitorType>(
                       visitorTypeCombo->currentData().toInt()));
    visitor.setPhoto(photoImage);
    visitor.setIdScan(idScanImage);
    visitor.setSignature(signatureData);
    visitor.setHostId(hostIdEdit->text());
    visitor.setPurpose(purposeEdit->text());
    visitor.setRetentionPeriod(retentionPeriodSpin->value());
    visitor.setConsent(consentCheck->isChecked());

    return VisitorManager::getInstance().registerVisitor(visitor);
}

void VisitorRegistrationDialog::clearForm()
{
    nameEdit->clear();
    emailEdit->clear();
    phoneEdit->clear();
    companyEdit->clear();
    idNumberEdit->clear();
    visitorTypeCombo->setCurrentIndex(0);
    hostIdEdit->clear();
    purposeEdit->clear();
    retentionPeriodSpin->setValue(30);
    consentCheck->setChecked(false);

    photoLabel->setText(tr("No Photo"));
    photoLabel->setPixmap(QPixmap());
    photoImage = QImage();

    idScanLabel->setText(tr("No ID Scan"));
    idScanLabel->setPixmap(QPixmap());
    idScanImage = QImage();

    signatureLabel->setText(tr("No Signature"));
    signatureLabel->setPixmap(QPixmap());
    signatureData.clear();
} 
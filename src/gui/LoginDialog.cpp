#include "LoginDialog.h"
#include <QApplication>
#include <QKeyEvent>
#include <QCloseEvent>
#include <QMessageBox>
#include <QSettings>
#include <QScreen>
#include <QTimer>
#include <QStyle>
#include <QDesktopServices>
#include <QUrl>
#include "../utils/ErrorHandler.h"

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent, Qt::WindowCloseButtonHint | Qt::WindowTitleHint)
    , sessionId("")
    , userRole(UserRole::SecurityGuard)
    , lockoutTimer(new QTimer(this))
    , inactivityTimer(new QTimer(this))
    , failedAttempts(0)
    , lockoutTime(0)
    , isLocked(false)
{
    setupUI();
    setupSecurity();
    
    // Load saved username if remember option was checked
    QSettings settings;
    if (settings.value("login/remember_username", false).toBool()) {
        usernameEdit->setText(settings.value("login/username").toString());
        rememberUsernameCheckbox->setChecked(true);
        passwordEdit->setFocus();
    } else {
        usernameEdit->setFocus();
    }
}

LoginDialog::~LoginDialog()
{
    secureMemoryCleanup();
}

void LoginDialog::setupUI()
{
    setWindowTitle("VMS - Secure Login");
    setWindowIcon(QIcon(":/icons/lock.png"));
    setFixedSize(400, 350);
    setModal(true);
    
    // Center the dialog
    if (QWidget *parent = parentWidget()) {
        move(parent->geometry().center() - rect().center());
    } else {
        QScreen *screen = QApplication::primaryScreen();
        move(screen->geometry().center() - rect().center());
    }
    
    // Main layout
    mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    
    // Title
    titleLabel = new QLabel("Visitor Management System");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("QLabel { font-size: 18px; font-weight: bold; color: #2c3e50; margin-bottom: 10px; }");
    mainLayout->addWidget(titleLabel);
    
    // Status label
    statusLabel = new QLabel("Please enter your credentials");
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet("QLabel { color: #7f8c8d; margin-bottom: 20px; }");
    mainLayout->addWidget(statusLabel);
    
    // Form layout
    formLayout = new QFormLayout();
    formLayout->setSpacing(12);
    
    // Username field
    usernameEdit = new QLineEdit();
    usernameEdit->setPlaceholderText("Enter username");
    usernameEdit->setMaxLength(50);
    usernameEdit->setStyleSheet(
        "QLineEdit { "
        "padding: 8px; "
        "border: 2px solid #bdc3c7; "
        "border-radius: 4px; "
        "font-size: 14px; "
        "} "
        "QLineEdit:focus { "
        "border-color: #3498db; "
        "}"
    );
    usernameEdit->installEventFilter(this);
    connect(usernameEdit, &QLineEdit::textChanged, this, &LoginDialog::onUsernameChanged);
    formLayout->addRow("Username:", usernameEdit);
    
    // Password field
    passwordEdit = new QLineEdit();
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->setPlaceholderText("Enter password");
    passwordEdit->setMaxLength(128);
    passwordEdit->setStyleSheet(usernameEdit->styleSheet());
    passwordEdit->installEventFilter(this);
    connect(passwordEdit, &QLineEdit::textChanged, this, &LoginDialog::onPasswordChanged);
    connect(passwordEdit, &QLineEdit::returnPressed, this, &LoginDialog::onLoginClicked);
    formLayout->addRow("Password:", passwordEdit);
    
    mainLayout->addLayout(formLayout);
    
    // Show password checkbox
    showPasswordCheckbox = new QCheckBox("Show password");
    showPasswordCheckbox->setStyleSheet("QCheckBox { color: #7f8c8d; }");
    connect(showPasswordCheckbox, &QCheckBox::toggled, this, &LoginDialog::onShowPasswordToggled);
    mainLayout->addWidget(showPasswordCheckbox);
    
    // Remember username checkbox
    rememberUsernameCheckbox = new QCheckBox("Remember username");
    rememberUsernameCheckbox->setStyleSheet("QCheckBox { color: #7f8c8d; }");
    mainLayout->addWidget(rememberUsernameCheckbox);
    
    // Progress bar (hidden by default)
    progressBar = new QProgressBar();
    progressBar->setRange(0, 0);
    progressBar->setVisible(false);
    progressBar->setStyleSheet(
        "QProgressBar { "
        "border: 1px solid #bdc3c7; "
        "border-radius: 4px; "
        "text-align: center; "
        "} "
        "QProgressBar::chunk { "
        "background-color: #3498db; "
        "border-radius: 3px; "
        "}"
    );
    mainLayout->addWidget(progressBar);
    
    // Lockout label (hidden by default)
    lockoutLabel = new QLabel();
    lockoutLabel->setAlignment(Qt::AlignCenter);
    lockoutLabel->setStyleSheet("QLabel { color: #e74c3c; font-weight: bold; }");
    lockoutLabel->setVisible(false);
    mainLayout->addWidget(lockoutLabel);
    
    // Button layout
    buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);
    
    cancelButton = new QPushButton("Cancel");
    cancelButton->setMinimumSize(80, 35);
    cancelButton->setStyleSheet(
        "QPushButton { "
        "background-color: #95a5a6; "
        "color: white; "
        "border: none; "
        "border-radius: 4px; "
        "padding: 8px 16px; "
        "font-size: 14px; "
        "} "
        "QPushButton:hover { "
        "background-color: #7f8c8d; "
        "} "
        "QPushButton:pressed { "
        "background-color: #6c7b7d; "
        "}"
    );
    connect(cancelButton, &QPushButton::clicked, this, &LoginDialog::onCancelClicked);
    
    loginButton = new QPushButton("Login");
    loginButton->setMinimumSize(80, 35);
    loginButton->setDefault(true);
    loginButton->setEnabled(false);
    loginButton->setStyleSheet(
        "QPushButton { "
        "background-color: #3498db; "
        "color: white; "
        "border: none; "
        "border-radius: 4px; "
        "padding: 8px 16px; "
        "font-size: 14px; "
        "font-weight: bold; "
        "} "
        "QPushButton:hover:enabled { "
        "background-color: #2980b9; "
        "} "
        "QPushButton:pressed:enabled { "
        "background-color: #21618c; "
        "} "
        "QPushButton:disabled { "
        "background-color: #bdc3c7; "
        "color: #7f8c8d; "
        "}"
    );
    connect(loginButton, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    
    buttonLayout->addStretch();
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(loginButton);
    
    mainLayout->addLayout(buttonLayout);
}

void LoginDialog::setupSecurity()
{
    // Setup lockout timer
    connect(lockoutTimer, &QTimer::timeout, this, &LoginDialog::onLockoutTimerTick);
    
    // Setup inactivity timer
    inactivityTimer->setSingleShot(true);
    inactivityTimer->setInterval(INACTIVITY_TIMEOUT * 1000);
    connect(inactivityTimer, &QTimer::timeout, this, &LoginDialog::clearSensitiveData);
    
    // Start inactivity timer
    inactivityTimer->start();
}

void LoginDialog::onLoginClicked()
{
    if (!isValidInput()) {
        return;
    }
    
    if (isLocked) {
        QMessageBox::warning(this, "Account Locked", 
                           QString("Account is locked. Please wait %1 seconds.").arg(lockoutTime));
        return;
    }
    
    enableLoginButton(false);
    progressBar->setVisible(true);
    statusLabel->setText("Authenticating...");
    statusLabel->setStyleSheet("QLabel { color: #3498db; }");
    
    // Reset inactivity timer
    inactivityTimer->start();
    
    QString username = usernameEdit->text().trimmed();
    QString password = passwordEdit->text();
    
    // Get client IP (in real implementation, this would be the actual client IP)
    QString clientIp = "127.0.0.1";
    
    // Authenticate with SecurityManager
    SecurityManager& security = SecurityManager::getInstance();
    sessionId = security.authenticate(username, password, clientIp);
    
    progressBar->setVisible(false);
    
    if (!sessionId.isEmpty()) {
        // Successful login
        userRole = security.getUserRole(sessionId);
        
        // Save username if remember option is checked
        QSettings settings;
        if (rememberUsernameCheckbox->isChecked()) {
            settings.setValue("login/username", username);
            settings.setValue("login/remember_username", true);
        } else {
            settings.remove("login/username");
            settings.setValue("login/remember_username", false);
        }
        
        logSecurityEvent("LOGIN_SUCCESS", true);
        accept();
    } else {
        // Failed login
        failedAttempts++;
        logSecurityEvent("LOGIN_FAILED", false);
        
        if (failedAttempts >= MAX_FAILED_ATTEMPTS) {
            isLocked = true;
            lockoutTime = LOCKOUT_DURATION;
            lockoutTimer->start(1000);
            showLockoutMessage(lockoutTime);
            
            // Disable all input
            usernameEdit->setEnabled(false);
            passwordEdit->setEnabled(false);
            showPasswordCheckbox->setEnabled(false);
            rememberUsernameCheckbox->setEnabled(false);
        } else {
            statusLabel->setText(QString("Invalid credentials. %1 attempts remaining.")
                               .arg(MAX_FAILED_ATTEMPTS - failedAttempts));
            statusLabel->setStyleSheet("QLabel { color: #e74c3c; }");
            passwordEdit->clear();
            passwordEdit->setFocus();
        }
    }
    
    enableLoginButton(true);
}

void LoginDialog::onCancelClicked()
{
    logSecurityEvent("LOGIN_CANCELLED", false);
    reject();
}

void LoginDialog::onUsernameChanged()
{
    validateInput();
    inactivityTimer->start(); // Reset inactivity timer
}

void LoginDialog::onPasswordChanged()
{
    validateInput();
    inactivityTimer->start(); // Reset inactivity timer
}

void LoginDialog::onShowPasswordToggled(bool show)
{
    passwordEdit->setEchoMode(show ? QLineEdit::Normal : QLineEdit::Password);
    inactivityTimer->start(); // Reset inactivity timer
}

void LoginDialog::onLockoutTimerTick()
{
    lockoutTime--;
    if (lockoutTime <= 0) {
        lockoutTimer->stop();
        isLocked = false;
        failedAttempts = 0;
        
        // Re-enable inputs
        usernameEdit->setEnabled(true);
        passwordEdit->setEnabled(true);
        showPasswordCheckbox->setEnabled(true);
        rememberUsernameCheckbox->setEnabled(true);
        
        lockoutLabel->setVisible(false);
        statusLabel->setText("Please enter your credentials");
        statusLabel->setStyleSheet("QLabel { color: #7f8c8d; }");
        
        validateInput();
    } else {
        showLockoutMessage(lockoutTime);
    }
}

void LoginDialog::clearSensitiveData()
{
    // Clear password field for security
    passwordEdit->clear();
    validateInput();
    
    statusLabel->setText("Session timed out. Please re-enter your credentials.");
    statusLabel->setStyleSheet("QLabel { color: #e67e22; }");
}

void LoginDialog::validateInput()
{
    bool valid = isValidInput();
    enableLoginButton(valid && !isLocked);
}

void LoginDialog::showLockoutMessage(int remainingSeconds)
{
    lockoutLabel->setText(QString("Account locked. Try again in %1 seconds.").arg(remainingSeconds));
    lockoutLabel->setVisible(true);
}

void LoginDialog::enableLoginButton(bool enabled)
{
    loginButton->setEnabled(enabled);
}

void LoginDialog::secureMemoryCleanup()
{
    // Securely clear sensitive data
    if (passwordEdit) {
        passwordEdit->clear();
    }
}

bool LoginDialog::isValidInput() const
{
    QString username = usernameEdit->text().trimmed();
    QString password = passwordEdit->text();
    
    return !username.isEmpty() && 
           password.length() >= MIN_PASSWORD_LENGTH &&
           username.length() <= 50;
}

void LoginDialog::logSecurityEvent(const QString& event, bool success)
{
    QString details = QString("User: %1, Success: %2")
                     .arg(usernameEdit->text().trimmed())
                     .arg(success ? "Yes" : "No");
    
    ErrorHandler::getInstance().logInfo("LoginDialog", QString("%1 - %2").arg(event, details));
}

void LoginDialog::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        onCancelClicked();
    } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (loginButton->isEnabled()) {
            onLoginClicked();
        }
    } else {
        QDialog::keyPressEvent(event);
    }
    
    // Reset inactivity timer on any key press
    inactivityTimer->start();
}

void LoginDialog::closeEvent(QCloseEvent *event)
{
    logSecurityEvent("LOGIN_WINDOW_CLOSED", false);
    event->accept();
}

bool LoginDialog::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress || 
        event->type() == QEvent::MouseButtonPress ||
        event->type() == QEvent::FocusIn) {
        inactivityTimer->start(); // Reset inactivity timer
    }
    
    return QDialog::eventFilter(obj, event);
}
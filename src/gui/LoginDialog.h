#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QCheckBox>
#include <QProgressBar>
#include <QTimer>
#include <QCryptographicHash>
#include <QSystemTrayIcon>
#include <QIcon>
#include "../security/SecurityManager.h"

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog();
    
    QString getSessionId() const { return sessionId; }
    UserRole getUserRole() const { return userRole; }

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onLoginClicked();
    void onCancelClicked();
    void onUsernameChanged();
    void onPasswordChanged();
    void onShowPasswordToggled(bool show);
    void onLockoutTimerTick();
    void clearSensitiveData();

private:
    void setupUI();
    void setupSecurity();
    void validateInput();
    void showLockoutMessage(int remainingSeconds);
    void enableLoginButton(bool enabled);
    void secureMemoryCleanup();
    bool isValidInput() const;
    void logSecurityEvent(const QString& event, bool success = false);
    
    // UI Components
    QVBoxLayout *mainLayout;
    QFormLayout *formLayout;
    QHBoxLayout *buttonLayout;
    
    QLabel *titleLabel;
    QLabel *statusLabel;
    QLabel *lockoutLabel;
    QLineEdit *usernameEdit;
    QLineEdit *passwordEdit;
    QCheckBox *showPasswordCheckbox;
    QCheckBox *rememberUsernameCheckbox;
    QPushButton *loginButton;
    QPushButton *cancelButton;
    QProgressBar *progressBar;
    
    // Security features
    QString sessionId;
    UserRole userRole;
    QTimer *lockoutTimer;
    QTimer *inactivityTimer;
    int failedAttempts;
    int lockoutTime;
    bool isLocked;
    
    // Constants
    static const int MAX_FAILED_ATTEMPTS = 3;
    static const int LOCKOUT_DURATION = 300; // 5 minutes
    static const int INACTIVITY_TIMEOUT = 300; // 5 minutes
    static const int MIN_PASSWORD_LENGTH = 8;
};

#endif // LOGINDIALOG_H
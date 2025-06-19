#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QToolBar>
#include <QMenuBar>
#include <QStatusBar>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onVisitorRegistration();
    void onVisitorCheckIn();
    void onVisitorCheckOut();
    void onReports();
    void onSettings();
    void onLogout();

private:
    void setupUI();
    void createActions();
    void createMenus();
    void createToolbar();
    void setupStatusBar();
    void loadSettings();
    void saveSettings();

    // UI Components
    QStackedWidget *centralStack;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;
    
    // Actions
    QAction *visitorRegistrationAction;
    QAction *visitorCheckInAction;
    QAction *visitorCheckOutAction;
    QAction *reportsAction;
    QAction *settingsAction;
    QAction *logoutAction;
};

#endif // MAINWINDOW_H 
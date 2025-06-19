#include "MainWindow.h"
#include <QApplication>
#include <QMessageBox>
#include <QSettings>
#include <QStyle>
#include <QMenu>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUI();
    createActions();
    createMenus();
    createToolbar();
    setupStatusBar();
    loadSettings();

    setWindowTitle(tr("VMS - Visitor Management System"));
    setMinimumSize(1024, 768);
}

MainWindow::~MainWindow()
{
    saveSettings();
}

void MainWindow::setupUI()
{
    centralStack = new QStackedWidget(this);
    setCentralWidget(centralStack);
}

void MainWindow::createActions()
{
    visitorRegistrationAction = new QAction(tr("&Register Visitor"), this);
    visitorRegistrationAction->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
    visitorRegistrationAction->setStatusTip(tr("Register a new visitor"));
    connect(visitorRegistrationAction, &QAction::triggered, this, &MainWindow::onVisitorRegistration);

    visitorCheckInAction = new QAction(tr("Check &In"), this);
    visitorCheckInAction->setIcon(style()->standardIcon(QStyle::SP_ArrowRight));
    visitorCheckInAction->setStatusTip(tr("Check in a visitor"));
    connect(visitorCheckInAction, &QAction::triggered, this, &MainWindow::onVisitorCheckIn);

    visitorCheckOutAction = new QAction(tr("Check &Out"), this);
    visitorCheckOutAction->setIcon(style()->standardIcon(QStyle::SP_ArrowLeft));
    visitorCheckOutAction->setStatusTip(tr("Check out a visitor"));
    connect(visitorCheckOutAction, &QAction::triggered, this, &MainWindow::onVisitorCheckOut);

    reportsAction = new QAction(tr("&Reports"), this);
    reportsAction->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    reportsAction->setStatusTip(tr("Generate reports"));
    connect(reportsAction, &QAction::triggered, this, &MainWindow::onReports);

    settingsAction = new QAction(tr("&Settings"), this);
    settingsAction->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    settingsAction->setStatusTip(tr("Configure system settings"));
    connect(settingsAction, &QAction::triggered, this, &MainWindow::onSettings);

    logoutAction = new QAction(tr("&Logout"), this);
    logoutAction->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
    logoutAction->setStatusTip(tr("Logout from the system"));
    connect(logoutAction, &QAction::triggered, this, &MainWindow::onLogout);
}

void MainWindow::createMenus()
{
    QMenu *visitorMenu = menuBar()->addMenu(tr("&Visitor"));
    visitorMenu->addAction(visitorRegistrationAction);
    visitorMenu->addAction(visitorCheckInAction);
    visitorMenu->addAction(visitorCheckOutAction);

    QMenu *adminMenu = menuBar()->addMenu(tr("&Admin"));
    adminMenu->addAction(reportsAction);
    adminMenu->addAction(settingsAction);

    menuBar()->addAction(logoutAction);
}

void MainWindow::createToolbar()
{
    mainToolBar = addToolBar(tr("Main"));
    mainToolBar->addAction(visitorRegistrationAction);
    mainToolBar->addAction(visitorCheckInAction);
    mainToolBar->addAction(visitorCheckOutAction);
    mainToolBar->addSeparator();
    mainToolBar->addAction(reportsAction);
    mainToolBar->addAction(settingsAction);
}

void MainWindow::setupStatusBar()
{
    statusBar = new QStatusBar(this);
    setStatusBar(statusBar);
    statusBar->showMessage(tr("Ready"));
}

void MainWindow::loadSettings()
{
    QSettings settings;
    restoreGeometry(settings.value("mainWindow/geometry").toByteArray());
    restoreState(settings.value("mainWindow/windowState").toByteArray());
}

void MainWindow::saveSettings()
{
    QSettings settings;
    settings.setValue("mainWindow/geometry", saveGeometry());
    settings.setValue("mainWindow/windowState", saveState());
}

// Slot implementations
void MainWindow::onVisitorRegistration()
{
    // TODO: Implement visitor registration
    statusBar->showMessage(tr("Opening visitor registration..."), 2000);
}

void MainWindow::onVisitorCheckIn()
{
    // TODO: Implement visitor check-in
    statusBar->showMessage(tr("Opening visitor check-in..."), 2000);
}

void MainWindow::onVisitorCheckOut()
{
    // TODO: Implement visitor check-out
    statusBar->showMessage(tr("Opening visitor check-out..."), 2000);
}

void MainWindow::onReports()
{
    // TODO: Implement reports
    statusBar->showMessage(tr("Opening reports..."), 2000);
}

void MainWindow::onSettings()
{
    // TODO: Implement settings
    statusBar->showMessage(tr("Opening settings..."), 2000);
}

void MainWindow::onLogout()
{
    if (QMessageBox::question(this, tr("Confirm Logout"),
                            tr("Are you sure you want to logout?"),
                            QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        // TODO: Implement proper logout
        qApp->quit();
    }
} 
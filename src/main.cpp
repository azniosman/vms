#include <QApplication>
#include <QMessageBox>
#include "gui/MainWindow.h"
#include "database/DatabaseManager.h"
#include "security/SecurityManager.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    // Set application metadata
    QApplication::setApplicationName("VMS");
    QApplication::setApplicationVersion("1.0.0");
    QApplication::setOrganizationName("PDPA Compliant Systems");
    
    try {
        // Initialize database
        if (!DatabaseManager::getInstance().initialize()) {
            QMessageBox::critical(nullptr, "Error", "Failed to initialize database");
            return 1;
        }
        
        // Initialize security manager
        if (!SecurityManager::getInstance().initialize()) {
            QMessageBox::critical(nullptr, "Error", "Failed to initialize security system");
            return 1;
        }
        
        // Create and show main window
        MainWindow mainWindow;
        mainWindow.show();
        
        return app.exec();
    } catch (const std::exception& e) {
        QMessageBox::critical(nullptr, "Fatal Error", 
            QString("Application failed to start: %1").arg(e.what()));
        return 1;
    }
} 
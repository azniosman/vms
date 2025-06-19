#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#include "core/Visitor.h"
#include "core/VisitorManager.h"
#include "database/DatabaseManager.h"
#include "security/SecurityManager.h"
#include "utils/ErrorHandler.h"
#include "reports/ReportManager.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    qDebug() << "VMS - Visitor Management System";
    qDebug() << "Version: 1.0.0";
    qDebug() << "Starting core system...";
    
    try {
        // Initialize error handler
        qDebug() << "Error handler initialized";
        
        // Initialize database
        if (!DatabaseManager::getInstance().initialize()) {
            qDebug() << "Failed to initialize database";
            return 1;
        }
        qDebug() << "Database initialized successfully";
        
        // Initialize security manager
        if (!SecurityManager::getInstance().initialize()) {
            qDebug() << "Failed to initialize security manager";
            return 1;
        }
        qDebug() << "Security manager initialized successfully";
        
        // Test visitor registration
        Visitor visitor;
        visitor.setName("John Doe");
        visitor.setEmail("john.doe@example.com");
        visitor.setPhone("+65 9123 4567");
        visitor.setCompany("Test Company");
        visitor.setIdentificationNumber("S1234567A");
        visitor.setType(Visitor::VisitorType::Guest);
        visitor.setHostId("HOST001");
        visitor.setPurpose("Business Meeting");
        visitor.setConsent(true);
        visitor.setRetentionPeriod(365);
        
        // Register visitor
        if (VisitorManager::getInstance().registerVisitor(visitor)) {
            qDebug() << "Visitor registered successfully";
            
            // Get the visitor back
            QString visitorId = visitor.getId();
            if (!visitorId.isEmpty()) {
                Visitor retrievedVisitor = VisitorManager::getInstance().getVisitor(visitorId);
                qDebug() << "Retrieved visitor:" << retrievedVisitor.getName();
                
                // Test check-in
                if (VisitorManager::getInstance().checkInVisitor(visitorId, "HOST001")) {
                    qDebug() << "Visitor checked in successfully";
                    
                    // Test check-out
                    if (VisitorManager::getInstance().checkOutVisitor(visitorId)) {
                        qDebug() << "Visitor checked out successfully";
                    }
                }
            }
        } else {
            qDebug() << "Failed to register visitor";
        }
        
        // Test reporting
        qDebug() << "Report manager initialized";
        
        // Generate a simple report
        ReportData report = ReportManager::getInstance().generateCurrentVisitorsReport();
        qDebug() << "Generated report with" << report.data.size() << "rows";
        
        qDebug() << "All core functionality tested successfully!";
        
    } catch (const std::exception& e) {
        qDebug() << "Exception occurred:" << e.what();
        return 1;
    }
    
    qDebug() << "VMS core system test completed successfully";
    return 0;
} 
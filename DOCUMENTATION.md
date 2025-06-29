# VMS - Visitor Management System Documentation

## Overview

The Visitor Management System (VMS) is a comprehensive, PDPA-compliant solution built with C++ and Qt for managing visitor registration, check-in/check-out, and reporting. The system is designed to meet Singapore's Personal Data Protection Act (PDPA) requirements while providing a modern, user-friendly interface.

## Architecture

### Core Components

1. **MainWindow** (`src/gui/MainWindow.h/cpp`)
   - Main application window
   - Menu and toolbar management
   - Central widget stack for different views

2. **VisitorManager** (`src/core/VisitorManager.h/cpp`)
   - Core business logic for visitor operations
   - Registration, check-in/out, blacklist management
   - PDPA compliance features

3. **DatabaseManager** (`src/database/DatabaseManager.h/cpp`)
   - SQLite database management with encryption
   - Schema management and migrations
   - Data retention and purging

4. **SecurityManager** (`src/security/SecurityManager.h/cpp`)
   - Authentication and authorization
   - Role-based access control
   - Session management and security features

5. **ErrorHandler** (`src/utils/ErrorHandler.h/cpp`)
   - Comprehensive error logging and handling
   - Error categorization and reporting
   - Audit trail for system errors

6. **QRCodeGenerator** (`src/utils/QRCodeGenerator.h/cpp`)
   - QR code generation for visitor badges
   - Custom visitor QR codes with embedded data
   - Image export and management

7. **ReportManager** (`src/reports/ReportManager.h/cpp`)
   - Comprehensive reporting system
   - Multiple report types and formats
   - Export functionality (CSV, JSON, HTML)

### GUI Components

1. **VisitorRegistrationDialog** (`src/gui/VisitorRegistrationDialog.h/cpp`)
   - Visitor registration form
   - Photo and ID capture
   - Consent management
   - Data validation

2. **CheckInOutDialog** (`src/gui/CheckInOutDialog.h/cpp`)
   - Visitor check-in/out interface
   - Real-time visitor status
   - Search and filtering
   - Badge printing

## PDPA Compliance Features

### Data Protection
- **Encryption**: All personal data encrypted at rest using AES-256
- **Data Minimization**: Only necessary information is collected
- **Purpose Limitation**: Clear consent forms and purpose statements
- **Data Retention**: Automatic data purging after specified periods

### User Rights
- **Right to Access**: Users can request their personal data
- **Right to Correction**: Data can be updated with version history
- **Right to Withdraw Consent**: Consent can be revoked at any time
- **Right to Data Portability**: Data can be exported in standard formats

### Security Measures
- **Access Control**: Role-based permissions with audit logging
- **Session Management**: Secure session handling with timeouts
- **Audit Trail**: Complete logging of all data access and modifications
- **IP Whitelisting**: Restricted access to authorized IP addresses

## User Roles and Permissions

### Super Administrator
- Full system access and configuration
- User management and role assignment
- System audit log access
- Data export and import capabilities

### Administrator
- Visitor management and reporting
- Customer management
- Bulk operations
- View audit logs

### Receptionist
- Visitor registration and check-in/out
- Basic visitor search
- Badge printing
- View today's visitors

### Security Guard
- View checked-in visitors
- Emergency evacuation lists
- Blacklist checking
- Limited search capabilities

## Database Schema

### Core Tables

1. **visitors**
   ```sql
   CREATE TABLE visitors (
       id TEXT PRIMARY KEY,
       name TEXT NOT NULL,
       email TEXT NOT NULL,
       phone TEXT,
       company TEXT,
       identification_number TEXT,
       type INTEGER,
       photo BLOB,
       id_scan BLOB,
       signature TEXT,
       host_id TEXT,
       purpose TEXT,
       created_at DATETIME NOT NULL,
       updated_at DATETIME NOT NULL,
       consent BOOLEAN NOT NULL,
       retention_period INTEGER NOT NULL
   );
   ```

2. **visits**
   ```sql
   CREATE TABLE visits (
       id TEXT PRIMARY KEY,
       visitor_id TEXT NOT NULL,
       host_id TEXT NOT NULL,
       check_in_time DATETIME NOT NULL,
       check_out_time DATETIME,
       FOREIGN KEY(visitor_id) REFERENCES visitors(id)
   );
   ```

3. **audit_log**
   ```sql
   CREATE TABLE audit_log (
       id TEXT PRIMARY KEY,
       action TEXT NOT NULL,
       entity_type TEXT NOT NULL,
       entity_id TEXT NOT NULL,
       user_id TEXT,
       details TEXT,
       created_at DATETIME NOT NULL
   );
   ```

4. **error_log**
   ```sql
   CREATE TABLE error_log (
       id TEXT PRIMARY KEY,
       message TEXT NOT NULL,
       details TEXT,
       severity INTEGER NOT NULL,
       category INTEGER NOT NULL,
       source TEXT,
       timestamp DATETIME NOT NULL,
       stack_trace TEXT,
       user_id TEXT,
       session_id TEXT
   );
   ```

## Error Handling

### Error Categories
- **Database**: Database connection and query errors
- **Security**: Authentication and authorization failures
- **Network**: Network communication issues
- **FileSystem**: File I/O operations
- **UserInput**: Invalid user input validation
- **System**: System-level errors

### Error Severity Levels
- **Info**: Informational messages
- **Warning**: Non-critical issues
- **Error**: Errors that affect functionality
- **Critical**: System-threatening errors

### Error Logging
- Database storage for structured querying
- File-based logging for backup
- Console output for debugging
- Administrator notifications for critical errors

## Reporting System

### Available Reports

1. **Daily Visitor Log**
   - Complete visitor activity for a specific date
   - Check-in/out times and durations
   - Host and purpose information

2. **Current Visitors**
   - Real-time list of visitors on premises
   - Duration of current visits
   - Emergency contact information

3. **Visitor Frequency Analysis**
   - Visitor patterns over time
   - Peak usage periods
   - Unique vs. total visitors

4. **Peak Time Analysis**
   - Hourly visitor distribution
   - Busiest periods identification
   - Resource planning data

5. **Visit Duration Report**
   - Average visit lengths
   - Duration distribution analysis
   - Capacity planning insights

6. **Customer Statistics**
   - Company-wise visitor analysis
   - Most frequent visitors
   - Business relationship insights

7. **Security Incidents**
   - Security-related events
   - Incident tracking and reporting
   - Compliance monitoring

8. **Contractor Tracking**
   - Contractor time tracking
   - Work hour analysis
   - Project-based reporting

9. **Emergency Evacuation**
   - Current visitor list for emergencies
   - Contact information
   - Location tracking

10. **Compliance Report**
    - PDPA compliance status
    - Consent management
    - Data retention compliance

### Export Formats
- **CSV**: Comma-separated values for spreadsheet import
- **JSON**: Structured data for API integration
- **HTML**: Web-ready reports with formatting
- **PDF**: Professional document format (planned)
- **Excel**: Native Excel format (planned)

## QR Code System

### QR Code Features
- **Visitor-specific codes**: Unique QR codes for each visitor
- **Embedded data**: Visitor ID, name, and timestamp
- **Badge integration**: QR codes embedded in visitor badges
- **Scan validation**: Real-time QR code validation

### QR Code Format
```
VMS:visitor_id:visitor_name:timestamp
```

### Usage
- **Check-in**: Scan QR code for quick check-in
- **Badge printing**: QR codes printed on visitor badges
- **Access control**: QR codes for door access (planned)
- **Tracking**: QR codes for visitor movement tracking

## Security Features

### Authentication
- **Password hashing**: Secure password storage with salt
- **Session management**: Secure session handling
- **Login attempts**: Failed login attempt tracking
- **Account lockout**: Automatic account lockout after failed attempts

### Authorization
- **Role-based access**: Granular permissions based on user roles
- **Resource protection**: Protected access to sensitive data
- **Action logging**: All actions logged for audit purposes

### Data Protection
- **Encryption at rest**: Database encryption using AES-256
- **Secure deletion**: Secure data deletion when retention expires
- **Access logging**: All data access logged and monitored

## Configuration

### System Settings
- **Database encryption key**: Configured in DatabaseManager
- **Session timeout**: Configurable session duration
- **Data retention**: Configurable retention periods
- **IP whitelist**: Restricted access IP addresses

### User Preferences
- **Language settings**: Multi-language support
- **Theme preferences**: UI theme customization
- **Notification settings**: Email/SMS notification preferences

## Building and Deployment

### Prerequisites
- C++17 compatible compiler
- Qt 6.x development libraries
- OpenSSL development libraries
- CMake 3.16 or later

### Build Process
```bash
# Clone the repository
git clone https://github.com/azniosman/vms.git
cd vms

# Build the project
./build.sh

# Or manually
mkdir build
cd build
cmake ..
make
```

### Installation
```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get install build-essential cmake qtbase6-dev libqt6sql6-sqlite libssl-dev

# Install dependencies (macOS)
brew install qt6 cmake openssl

# Run the application
./build/vms
```

## Troubleshooting

### Common Issues

1. **Database Connection Errors**
   - Check database file permissions
   - Verify encryption key configuration
   - Ensure sufficient disk space

2. **Camera Access Issues**
   - Check camera permissions
   - Verify camera driver installation
   - Test camera with other applications

3. **Printing Problems**
   - Check printer driver installation
   - Verify printer permissions
   - Test with system print dialog

4. **Performance Issues**
   - Monitor database size and performance
   - Check system resources
   - Review error logs for bottlenecks

### Error Logs
- **Application logs**: Located in `~/.local/share/VMS/logs/`
- **Database logs**: Embedded in SQLite database
- **System logs**: Check system log files

### Support
For technical support and issues:
- Check the error logs for detailed information
- Review the troubleshooting section
- Contact system administrator
- Submit bug reports with error details

## Future Enhancements

### Planned Features
1. **Facial Recognition**: Automated visitor identification
2. **Mobile App**: Companion mobile application
3. **API Integration**: RESTful API for third-party integration
4. **Advanced Analytics**: Machine learning-based insights
5. **Multi-site Support**: Multi-location management
6. **Cloud Integration**: Cloud-based data synchronization

### Technology Upgrades
1. **Qt 7**: Migration to latest Qt version
2. **C++20**: Modern C++ features
3. **Advanced Encryption**: Post-quantum cryptography
4. **Real-time Updates**: WebSocket-based real-time features

## License and Legal

### License
This project is licensed under the MIT License - see the LICENSE file for details.

### Compliance
- **PDPA**: Fully compliant with Singapore's Personal Data Protection Act
- **GDPR**: Compatible with EU General Data Protection Regulation
- **ISO 27001**: Information security management standards

### Support
For support and inquiries:
- Email: az@azni.me
- Documentation: See this file and README.md 
# PDPA-Compliant Visitor Management System

A comprehensive Visitor Management System (VMS) built with C++ and Qt, fully compliant with Singapore's Personal Data Protection Act (PDPA).

## Features

### Core Functionality
- Visitor registration and management
- Check-in/check-out processing
- QR code generation for visitor badges
- Photo capture and ID scanning
- Digital signature capture
- Host notification system
- Blacklist checking
- Health declaration forms

### PDPA Compliance
- Data encryption (AES-256)
- Secure data storage
- Automated data retention policies
- Consent management
- Data export capabilities
- Audit logging
- Access control

### Security Features
- Role-based access control
- Session management
- Password hashing with salt
- IP whitelisting
- Failed login attempt tracking
- SSL/TLS encryption

### Reporting
- Visitor analytics
- Audit trail reports
- Compliance reports
- Custom report generation

## Requirements

### System Requirements
- C++17 or later
- Qt 6.x
- OpenSSL
- SQLite 3.x

### Build Dependencies
- CMake 3.16 or later
- Qt development tools
- OpenSSL development libraries
- C++ compiler with C++17 support

## Building the Project

1. Install dependencies:
   ```bash
   # Ubuntu/Debian
   sudo apt-get install build-essential cmake qtbase6-dev libqt6sql6-sqlite libssl-dev

   # macOS
   brew install qt6 cmake openssl
   ```

2. Clone the repository:
   ```bash
   git clone https://github.com/azniosman/vms.git
   cd vms
   ```

3. Create build directory:
   ```bash
   mkdir build
   cd build
   ```

4. Configure and build:
   ```bash
   cmake ..
   make
   ```

## Configuration

### Database Setup
The system uses SQLite with encryption for data storage. The database file is automatically created at:
- Linux: `~/.local/share/VMS/vms.db`
- macOS: `~/Library/Application Support/VMS/vms.db`
- Windows: `%APPDATA%\VMS\vms.db`

### Security Configuration
1. Set up IP whitelist in the settings
2. Configure password policies
3. Set up data retention periods
4. Configure SSL/TLS certificates

## Usage

### User Roles

#### Super Administrator
- Full system access
- User management
- System configuration
- Audit log access

#### Administrator
- Visitor management
- Report generation
- Customer management
- View audit logs

#### Receptionist
- Visitor check-in/check-out
- Basic visitor search
- Print badges

#### Security Guard
- View checked-in visitors
- Emergency evacuation lists
- Blacklist checking

### Basic Operations

1. Login with your credentials
2. Register new visitors
3. Process check-ins/check-outs
4. Generate reports
5. Manage user data and consent

## Security Best Practices

1. Use strong passwords
2. Regularly update the IP whitelist
3. Monitor audit logs
4. Regularly backup the database
5. Keep the system updated
6. Review user permissions regularly

## PDPA Compliance

### Data Protection
- Personal data is encrypted at rest
- Data is collected only with consent
- Data is retained only for the necessary period
- Access to data is strictly controlled

### User Rights
- Right to access personal data
- Right to correct personal data
- Right to withdraw consent
- Right to data portability

## Development

### Project Structure
```
/src
  /core           - Business logic
  /gui            - UI components
  /database       - Data access layer
  /security       - Security features
  /utils          - Helper functions
  /integration    - Third-party integrations
  /reports        - Report generation
```

### Contributing
1. Fork the repository
2. Create a feature branch
3. Commit your changes
4. Push to the branch
5. Create a Pull Request

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

- Qt Framework
- OpenSSL
- SQLite
- Other open source contributors 
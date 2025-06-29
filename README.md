# 🏢 PDPA-Compliant Visitor Management System

[![Security Status](https://img.shields.io/badge/Security-Enterprise%20Grade-green.svg)](SECURITY.md)
[![PDPA Compliant](https://img.shields.io/badge/PDPA-Compliant-blue.svg)](SECURITY.md)
[![GDPR Ready](https://img.shields.io/badge/GDPR-Ready-blue.svg)](SECURITY.md)
[![Build Status](https://img.shields.io/badge/Build-Passing-brightgreen.svg)](#building-the-project)

A **secure, enterprise-grade** Visitor Management System (VMS) built with C++ and Qt, featuring comprehensive security measures and full compliance with Singapore's Personal Data Protection Act (PDPA) and GDPR requirements.

## 🔒 Security First

This VMS implementation prioritizes security and privacy protection with:

- ✅ **Zero Critical Vulnerabilities** - Comprehensive security audit completed
- 🔐 **Enterprise Authentication** - PBKDF2 password hashing, account lockout protection
- 🗄️ **Encrypted Database** - AES-256 encryption with secure key management
- 🛡️ **Input Validation** - SQL injection and XSS prevention on all inputs
- 📊 **Audit Logging** - Complete audit trails for compliance
- ⚙️ **Secure Configuration** - Encrypted settings and integrity validation

> **For detailed security information, see [SECURITY.md](SECURITY.md)**

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

### 🔐 Advanced Security Features
- **Authentication Security**: PBKDF2 password hashing (100K iterations), account lockout, secure sessions
- **Database Security**: AES-256 encryption, secure key management, SQL injection prevention
- **Input Validation**: Comprehensive validation, XSS protection, rate limiting
- **Access Control**: Role-based permissions (4 user roles), session management, IP whitelisting
- **Audit & Monitoring**: Complete audit trails, security event logging, anomaly detection
- **Data Protection**: Encrypted biometric data, consent management, data retention policies

### Reporting
- Visitor analytics
- Audit trail reports
- Compliance reports
- Custom report generation

## 📋 Requirements

### System Requirements
- **C++17** or later (modern C++ features)
- **Qt 6.x** (Core, Widgets, SQL, Network, PrintSupport)
- **OpenSSL** (cryptographic operations)
- **SQLite 3.x** with encryption support
- **Operating System**: Windows 10+, macOS 10.15+, Ubuntu 18.04+

### Build Dependencies
- **CMake 3.16** or later
- **Qt 6 development tools** (qmake, moc, uic, rcc)
- **OpenSSL development libraries**
- **Modern C++ compiler** with C++17 support (GCC 7+, Clang 5+, MSVC 2019+)
- **Git** for version control

## 🚀 Quick Start

### 1. Install Dependencies

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install build-essential cmake qtbase6-dev libqt6widgets6-dev \
                     libqt6sql6-sqlite libqt6network6-dev libssl-dev git

# macOS (with Homebrew)
brew install qt6 cmake openssl git

# Windows (with vcpkg)
vcpkg install qt6[core,widgets,sql,network] openssl
```

### 2. Clone and Build

```bash
# Clone the repository
git clone https://github.com/azniosman/vms.git
cd vms

# Quick build using the provided script
./build.sh

# Or manual build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)  # Linux/macOS
```

### 3. Run the Application

```bash
# From project root
./build/vms

# First run will create default admin user
# Username: admin, Password: TempAdmin123!@#
# ⚠️ CHANGE THIS PASSWORD IMMEDIATELY!
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

## 🔒 Security Best Practices

### Production Deployment Checklist
- [ ] **Change default admin password** immediately after first login
- [ ] **Configure IP whitelist** to restrict access to authorized networks
- [ ] **Enable HTTPS** with valid SSL/TLS certificates
- [ ] **Set up log monitoring** and security alerting
- [ ] **Configure firewall rules** to limit network exposure
- [ ] **Enable database backups** with encryption
- [ ] **Review user permissions** and implement least privilege
- [ ] **Set up vulnerability scanning** and security monitoring

### Ongoing Security Maintenance
1. **Monitor audit logs** regularly for suspicious activity
2. **Update dependencies** and apply security patches promptly
3. **Review user access** and remove inactive accounts
4. **Test backup/restore** procedures regularly
5. **Conduct security assessments** periodically
6. **Train staff** on security procedures and threat awareness

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

## 💻 Development

### Project Architecture
```
/src
  /core           - Business logic (Visitor, VisitorManager)
  /gui            - Modern UI components (LoginDialog, DashboardWidget)
  /database       - Secure data access layer (DatabaseManager)
  /security       - Authentication & authorization (SecurityManager)
  /utils          - Utilities (ErrorHandler, ConfigManager)
  /reports        - Report generation and analytics
```

### Key Components
- **SecurityManager**: Authentication, authorization, session management
- **DatabaseManager**: Encrypted database operations, schema management
- **VisitorManager**: Business logic, input validation, PDPA compliance
- **ConfigManager**: Secure configuration management
- **ErrorHandler**: Security logging and audit trails

### Contributing
1. **Fork** the repository
2. **Create** a feature branch (`git checkout -b feature/amazing-feature`)
3. **Implement** your changes following security best practices
4. **Test** thoroughly including security validation
5. **Commit** your changes (`git commit -m 'Add amazing feature'`)
6. **Push** to the branch (`git push origin feature/amazing-feature`)
7. **Create** a Pull Request

### Security Guidelines for Contributors
- All code must pass security review
- Input validation required for all user inputs
- Follow secure coding practices (see [SECURITY.md](SECURITY.md))
- No hardcoded credentials or sensitive data
- Comprehensive logging for security events

## 📄 Documentation

- **[SECURITY.md](SECURITY.md)** - Comprehensive security implementation details
- **[CLAUDE.md](CLAUDE.md)** - Development guide for Claude Code
- **API Documentation** - Generated from source code comments
- **User Manual** - Available in `/docs` directory (when implemented)

## 🎯 Roadmap

### Current Status: v1.0.0 - Security Hardened ✅
- Enterprise-grade security implementation
- PDPA/GDPR compliance
- Modern UI components
- Comprehensive audit logging

### Planned Features
- [ ] Mobile companion app
- [ ] Advanced analytics dashboard
- [ ] Integration with badge printers
- [ ] Facial recognition system
- [ ] Multi-language support
- [ ] Cloud deployment options

## 📞 Support

For security issues, please email security@vms-project.org
For general support, please open an issue on GitHub.

## 📜 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- **Qt Framework** - Cross-platform UI framework
- **OpenSSL** - Cryptographic library for security
- **SQLite** - Embedded database engine
- **Security Community** - For best practices and guidelines
- **Privacy Advocates** - For PDPA/GDPR compliance guidance

---

**⚠️ Security Notice**: This software handles sensitive personal data. Ensure proper security measures are in place before production deployment. See [SECURITY.md](SECURITY.md) for detailed security information. 
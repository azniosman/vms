#ifndef VISITOR_H
#define VISITOR_H

#include <QString>
#include <QDateTime>
#include <QImage>

class Visitor {
public:
    enum class VisitorType {
        Guest,
        Contractor,
        Delivery,
        Interview,
        Other
    };

    Visitor();
    ~Visitor();

    // Getters
    QString getId() const { return id; }
    QString getName() const { return name; }
    QString getEmail() const { return email; }
    QString getPhone() const { return phone; }
    QString getCompany() const { return company; }
    QString getIdentificationNumber() const { return identificationNumber; }
    VisitorType getType() const { return type; }
    QImage getPhoto() const { return photo; }
    QImage getIdScan() const { return idScan; }
    QString getSignature() const { return signature; }
    QString getHostId() const { return hostId; }
    QString getPurpose() const { return purpose; }
    QDateTime getCreatedAt() const { return createdAt; }
    QDateTime getUpdatedAt() const { return updatedAt; }
    bool hasConsent() const { return consent; }
    int getRetentionPeriod() const { return retentionPeriod; }

    // Setters
    void setId(const QString& value) { id = value; }
    void setName(const QString& value) { name = value; }
    void setEmail(const QString& value) { email = value; }
    void setPhone(const QString& value) { phone = value; }
    void setCompany(const QString& value) { company = value; }
    void setIdentificationNumber(const QString& value) { identificationNumber = value; }
    void setType(VisitorType value) { type = value; }
    void setPhoto(const QImage& value) { photo = value; }
    void setIdScan(const QImage& value) { idScan = value; }
    void setSignature(const QString& value) { signature = value; }
    void setHostId(const QString& value) { hostId = value; }
    void setPurpose(const QString& value) { purpose = value; }
    void setCreatedAt(const QDateTime& value) { createdAt = value; }
    void setUpdatedAt(const QDateTime& value) { updatedAt = value; }
    void setConsent(bool value) { consent = value; }
    void setRetentionPeriod(int value) { retentionPeriod = value; }

private:
    QString id;
    QString name;
    QString email;
    QString phone;
    QString company;
    QString identificationNumber;
    VisitorType type;
    QImage photo;
    QImage idScan;
    QString signature;
    QString hostId;
    QString purpose;
    QDateTime createdAt;
    QDateTime updatedAt;
    bool consent;
    int retentionPeriod;
};

#endif // VISITOR_H 
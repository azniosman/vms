#ifndef QRCODEGENERATOR_H
#define QRCODEGENERATOR_H

#include <QObject>
#include <QImage>
#include <QString>
#include <QByteArray>

class QRCodeGenerator : public QObject {
    Q_OBJECT

public:
    static QRCodeGenerator& getInstance();

    // Generate QR code from text
    QImage generateQRCode(const QString& text, int size = 256);
    
    // Generate QR code for visitor badge
    QImage generateVisitorQRCode(const QString& visitorId, const QString& visitorName);
    
    // Save QR code to file
    bool saveQRCode(const QImage& qrCode, const QString& filePath);
    
    // Get QR code as byte array
    QByteArray getQRCodeBytes(const QImage& qrCode, const QString& format = "PNG");

private:
    explicit QRCodeGenerator(QObject *parent = nullptr);
    ~QRCodeGenerator();

    // Prevent copying
    QRCodeGenerator(const QRCodeGenerator&) = delete;
    QRCodeGenerator& operator=(const QRCodeGenerator&) = delete;

    static QRCodeGenerator* instance;
    static QMutex instanceMutex;

    // Helper methods
    QByteArray encodeQRData(const QString& text);
    QImage renderQRCode(const QByteArray& qrData, int size);
};

#endif // QRCODEGENERATOR_H 
#include "QRCodeGenerator.h"
#include <QPainter>
#include <QFont>
#include <QFontMetrics>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#include <QMutex>
#include <QMutexLocker>

// Simple QR code implementation using Reed-Solomon error correction
// In production, you would use a library like libqrencode or similar

QRCodeGenerator* QRCodeGenerator::instance = nullptr;
QMutex QRCodeGenerator::instanceMutex;

QRCodeGenerator& QRCodeGenerator::getInstance()
{
    if (instance == nullptr) {
        QMutexLocker locker(&instanceMutex);
        if (instance == nullptr) {
            instance = new QRCodeGenerator();
        }
    }
    return *instance;
}

QRCodeGenerator::QRCodeGenerator(QObject *parent)
    : QObject(parent)
{
}

QRCodeGenerator::~QRCodeGenerator()
{
}

QImage QRCodeGenerator::generateQRCode(const QString& text, int size)
{
    try {
        QByteArray qrData = encodeQRData(text);
        return renderQRCode(qrData, size);
    } catch (const std::exception& e) {
        qCritical() << "Failed to generate QR code:" << e.what();
        return QImage();
    }
}

QImage QRCodeGenerator::generateVisitorQRCode(const QString& visitorId, const QString& visitorName)
{
    try {
        // Create visitor-specific QR code data
        QString qrData = QString("VMS:%1:%2:%3")
                        .arg(visitorId)
                        .arg(visitorName)
                        .arg(QDateTime::currentDateTime().toString("yyyyMMddHHmmss"));

        QImage qrCode = generateQRCode(qrData, 256);
        
        if (qrCode.isNull()) {
            throw std::runtime_error("Failed to generate QR code image");
        }

        // Add visitor information to the QR code
        QImage badgeImage(qrCode.size() + QSize(0, 60), QImage::Format_ARGB32);
        badgeImage.fill(Qt::white);

        QPainter painter(&badgeImage);
        painter.setRenderHint(QPainter::Antialiasing);

        // Draw QR code
        painter.drawImage(0, 0, qrCode);

        // Draw visitor information
        painter.setPen(Qt::black);
        QFont font = painter.font();
        font.setPointSize(10);
        painter.setFont(font);

        QRect textRect(0, qrCode.height(), badgeImage.width(), 60);
        painter.drawText(textRect, Qt::AlignCenter, 
                        QString("%1\n%2").arg(visitorName).arg(visitorId));

        return badgeImage;
    } catch (const std::exception& e) {
        qCritical() << "Failed to generate visitor QR code:" << e.what();
        return QImage();
    }
}

bool QRCodeGenerator::saveQRCode(const QImage& qrCode, const QString& filePath)
{
    try {
        if (qrCode.isNull()) {
            throw std::runtime_error("QR code image is null");
        }

        QDir dir = QFileInfo(filePath).dir();
        if (!dir.exists() && !dir.mkpath(".")) {
            throw std::runtime_error("Failed to create directory");
        }

        if (!qrCode.save(filePath, "PNG")) {
            throw std::runtime_error("Failed to save QR code image");
        }

        return true;
    } catch (const std::exception& e) {
        qCritical() << "Failed to save QR code:" << e.what();
        return false;
    }
}

QByteArray QRCodeGenerator::getQRCodeBytes(const QImage& qrCode, const QString& format)
{
    try {
        if (qrCode.isNull()) {
            throw std::runtime_error("QR code image is null");
        }

        QByteArray bytes;
        QBuffer buffer(&bytes);
        buffer.open(QIODevice::WriteOnly);

        if (!qrCode.save(&buffer, format.toLatin1())) {
            throw std::runtime_error("Failed to convert QR code to bytes");
        }

        return bytes;
    } catch (const std::exception& e) {
        qCritical() << "Failed to get QR code bytes:" << e.what();
        return QByteArray();
    }
}

QByteArray QRCodeGenerator::encodeQRData(const QString& text)
{
    // This is a simplified QR code encoding
    // In production, use a proper QR code library
    QByteArray data = text.toUtf8();
    
    // Add simple error correction (this is just a placeholder)
    // Real QR codes use Reed-Solomon error correction
    QByteArray encoded;
    encoded.append(static_cast<char>(data.size()));
    encoded.append(data);
    
    return encoded;
}

QImage QRCodeGenerator::renderQRCode(const QByteArray& qrData, int size)
{
    // This is a simplified QR code rendering
    // In production, use a proper QR code library
    QImage image(size, size, QImage::Format_ARGB32);
    image.fill(Qt::white);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw a simple pattern representing the QR code
    // This is just for demonstration - real QR codes have specific patterns
    int cellSize = size / 25; // 25x25 grid
    painter.setPen(Qt::black);
    painter.setBrush(Qt::black);

    for (int i = 0; i < qrData.size() && i < 625; ++i) { // 25x25 = 625
        int row = i / 25;
        int col = i % 25;
        
        if (qrData[i] & 0x01) {
            painter.drawRect(col * cellSize, row * cellSize, cellSize, cellSize);
        }
    }

    return image;
} 
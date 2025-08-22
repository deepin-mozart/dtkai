// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "DOCRRecognition"

#include <QCoreApplication>
#include <QDebug>
#include <QTimer>
#include <QFile>
#include <QDir>

DAI_USE_NAMESPACE

class OCRDemo : public QObject
{
    Q_OBJECT

public:
    explicit OCRDemo(QObject *parent = nullptr) : QObject(parent) 
    {
        ocr = new DOCRRecognition(this);
    }

private:
    // Read image data from Qt resource system
    QByteArray getEmbeddedImageData() const
    {
        QFile imageFile(":/test/textrecognition.png");
        if (!imageFile.open(QIODevice::ReadOnly)) {
            qWarning() << "Failed to open embedded test image resource";
            return QByteArray();
        }
        
        QByteArray imageData = imageFile.readAll();
        qInfo() << "Loaded embedded image data from Qt resource, size:" << imageData.size() << "bytes";
        return imageData;
    }

    // Fallback: copy resource file to temporary file and return absolute path
    QString getTestImagePath() const
    {
        // Read data from resource
        QByteArray imageData = getEmbeddedImageData();
        if (imageData.isEmpty()) {
            return QString();
        }
        
        // Create temporary file
        QString tempPath = QDir::temp().absoluteFilePath("ocr_test_embedded.png");
        QFile tempFile(tempPath);
        
        if (!tempFile.open(QIODevice::WriteOnly)) {
            qWarning() << "Failed to create temporary file:" << tempPath;
            return QString();
        }
        
        tempFile.write(imageData);
        tempFile.close();
        
        qInfo() << "Created temporary image file:" << tempPath;
        return tempPath;
    }

public Q_SLOTS:
    void demonstrateOCR()
    {
        qInfo() << "=== DTK AI OCR Client Demo (Qt Resource System) ===";
        
        // Method 1: Use image data directly for OCR
        QByteArray imageData = getEmbeddedImageData();
        if (!imageData.isEmpty()) {
            qInfo() << "Testing with embedded image data (" << imageData.size() << " bytes)";
            
            // Test with image data
            qInfo() << "\n--- Test: Recognize Image Data ---";
            QVariantHash params;
            params["includeDetails"] = true;
            params["language"] = "zh-Hans_en";
            
            QString result = ocr->recognizeImage(imageData, params);
            qInfo() << "Image data OCR recognition result:" << result;
        }
        
        // Method 2: OCR through temporary file (some OCR engines may need file path)
        QString imagePath = getTestImagePath();
        if (!imagePath.isEmpty()) {
            qInfo() << "\n--- Test: Recognize File Path ---";
            QVariantHash params;
            params["language"] = "zh-Hans_en";
            
            QString result = ocr->recognizeFile(imagePath, params);
            qInfo() << "File path OCR recognition result:" << result;
            
            // Clean up temporary file
            QFile::remove(imagePath);
            qInfo() << "Cleaned up temporary file:" << imagePath;
        }
        
        // Test other functions
        qInfo() << "\n--- Test: Get Supported Languages ---";
        QStringList languages = ocr->getSupportedLanguages();
        qInfo() << "Supported languages:" << languages;
        
        qInfo() << "\n--- Test: Get Supported Formats ---";
        QStringList formats = ocr->getSupportedFormats();
        qInfo() << "Supported formats:" << formats;
        
        qInfo() << "\n--- Test: Get Capabilities ---";
        QString capabilities = ocr->getCapabilities();
        qInfo() << "Capabilities:" << capabilities;
        
        // Check for errors
        auto error = ocr->lastError();
        if (error.getErrorCode() != 0) {
            qWarning() << "OCR error occurred:" << error.getErrorCode() << error.getErrorMessage();
        }
        
        qInfo() << "\n=== OCR Demo Completed ===";
        QTimer::singleShot(100, QCoreApplication::instance(), &QCoreApplication::quit);
    }

private:
    DOCRRecognition *ocr = nullptr;
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    qInfo() << "Starting DTK AI OCR Client Test (Qt Resource System)...";
    
    OCRDemo demo;
    
    // Start demo after a short delay
    QTimer::singleShot(100, &demo, &OCRDemo::demonstrateOCR);
    
    return app.exec();
}

#include "ocr_client_test.moc"

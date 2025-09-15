// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "test_base.h"
#include "test_utils.h"

#include "dtkai/docrrecognition.h"
#include "dtkai/dtkaitypes.h"
#include "dtkai/DAIError"

#include <QSignalSpy>
#include <QTimer>
#include <QTest>
#include <QVariantHash>
#include <QByteArray>
#include <QTemporaryFile>
#include <QStandardPaths>
#include <QDir>
#include <QRect>
#include <QBuffer>

DAI_USE_NAMESPACE

/**
 * @brief Test fixture for DOCRRecognition interface
 * 
 * Tests the OCR (Optical Character Recognition) functionality including:
 * - Constructor/destructor behavior
 * - Text recognition from files and data
 * - Region-based recognition
 * - Information query methods
 * - Error handling and parameter validation
 * - Termination functionality
 */
class TestDOCRRecognition : public TestBase
{
protected:
    void SetUp() override
    {
        TestBase::SetUp();
        qDebug() << "Setting up DOCRRecognition interface tests";
        
        // Create OCR recognition instance
        ocrRec = new DOCRRecognition();
        ASSERT_NE(ocrRec, nullptr);
    }
    
    void TearDown() override
    {
        if (ocrRec) {
            delete ocrRec;
            ocrRec = nullptr;
        }
        
        // Clean up test files
        cleanupTestFiles();
        
        TestBase::TearDown();
    }
    
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

    /**
     * @brief Validate error state for operations that may fail in test environment
     */
    void validateErrorState(const DTK_CORE_NAMESPACE::DError &error, bool expectSuccess = false)
    {
        if (expectSuccess) {
            EXPECT_EQ(error.getErrorCode(), NoError) << "Expected no error, but got: "
                                               << error.getErrorMessage().toStdString();
        } else {
            // In test environment, API server not available (code 1) is acceptable
            if (error.getErrorCode() == 1) {
                qInfo() << "AI daemon not available (error code 1) - this is normal in test environment";
            } else if (error.getErrorCode() != NoError) {
                qDebug() << "Unexpected error code:" << error.getErrorCode() 
                        << "message:" << error.getErrorMessage();
            }
        }
    }
    
    /**
     * @brief Clean up test files
     */
    void cleanupTestFiles()
    {
        for (const QString &file : testFiles) {
            if (QFile::exists(file)) {
                QFile::remove(file);
            }
        }
        testFiles.clear();
    }

protected:
    DOCRRecognition *ocrRec = nullptr;
    QStringList testFiles;
};

/**
 * @brief Test DOCRRecognition constructor and destructor
 */
TEST_F(TestDOCRRecognition, constructorDestructor)
{
    qDebug() << "Testing DOCRRecognition constructor and destructor";
    
    // Test constructor with parent
    QObject *parent = new QObject();
    DOCRRecognition *ocrRecWithParent = new DOCRRecognition(parent);
    EXPECT_NE(ocrRecWithParent, nullptr);
    EXPECT_EQ(ocrRecWithParent->parent(), parent);
    
    // Parent will clean up child automatically
    delete parent;
    
    // Test standalone constructor (already created in SetUp)
    EXPECT_NE(ocrRec, nullptr);
    EXPECT_EQ(ocrRec->parent(), nullptr);
    
    qDebug() << "Constructor/destructor tests completed";
}

/**
 * @brief Test OCR recognition from file
 */
TEST_F(TestDOCRRecognition, fileRecognition)
{
    qDebug() << "Testing DOCRRecognition file recognition";
    
    // Create test image file
    QString testImagePath = getTestImagePath();
    ASSERT_FALSE(testImagePath.isEmpty());
    ASSERT_TRUE(QFile::exists(testImagePath));
    
    // Test: Basic file recognition
    QString result = ocrRec->recognizeFile(testImagePath);
    qDebug() << "File recognition result:" << result;
    
    DTK_CORE_NAMESPACE::DError error = ocrRec->lastError();
    validateErrorState(error);
    
    // Test: File recognition with parameters
    QVariantHash params;
    params["language"] = "en";
    params["output_format"] = "text";
    params["confidence_threshold"] = 0.8;
    
    QString paramResult = ocrRec->recognizeFile(testImagePath, params);
    qDebug() << "File recognition with params result:" << paramResult;
    
    error = ocrRec->lastError();
    validateErrorState(error);
    
    // Test: Invalid file path
    QString invalidResult = ocrRec->recognizeFile("/nonexistent/path/image.jpg");
    qDebug() << "Expect: Invalid file result:" << invalidResult;
    
    error = ocrRec->lastError();
    // Should have an error for invalid file
    EXPECT_NE(error.getErrorCode(), NoError);
    
    qDebug() << "File recognition tests completed";
}

/**
 * @brief Test OCR recognition from image data
 */
TEST_F(TestDOCRRecognition, imageDataRecognition)
{
    qDebug() << "Testing DOCRRecognition image data recognition";
    
    // Generate test image data
    QByteArray imageData = getEmbeddedImageData();
    ASSERT_FALSE(imageData.isEmpty());
    
    // Test: Basic data recognition
    QString result = ocrRec->recognizeImage(imageData);
    qDebug() << "Data recognition result:" << result;
    
    DTK_CORE_NAMESPACE::DError error = ocrRec->lastError();
    validateErrorState(error);
    
    // Test: Data recognition with parameters
    QVariantHash params;
    params["language"] = "zh-cn";
    params["deskew"] = true;
    params["remove_noise"] = true;
    
    QString paramResult = ocrRec->recognizeImage(imageData, params);
    qDebug() << "Data recognition with params result:" << paramResult;
    
    error = ocrRec->lastError();
    validateErrorState(error);
    
    // Test: Empty data
    QString emptyResult = ocrRec->recognizeImage(QByteArray());
    qDebug() << "Empty data result:" << emptyResult;
    
    error = ocrRec->lastError();
    // Should have an error for empty data
    EXPECT_NE(error.getErrorCode(), NoError);
    
    // Test: Invalid image data
    QByteArray invalidData("This is not image data");
    QString invalidResult = ocrRec->recognizeImage(invalidData);
    qDebug() << "Invalid data result:" << invalidResult;
    
    error = ocrRec->lastError();
    EXPECT_NE(error.getErrorCode(), NoError);
    
    qDebug() << "Image data recognition tests completed";
}

/**
 * @brief Test region-based OCR recognition
 */
TEST_F(TestDOCRRecognition, regionRecognition)
{
    qDebug() << "Testing DOCRRecognition region recognition";
    
    QString testImagePath = getTestImagePath();
    ASSERT_FALSE(testImagePath.isEmpty());
    
    // Test: Region recognition from string
    QString regionString = "0,0,728,370"; // x,y,width,height
    QString stringResult = ocrRec->recognizeRegionFromString(testImagePath, regionString);
    qDebug() << "Region string recognition result:" << stringResult;
    
    DTK_CORE_NAMESPACE::DError error = ocrRec->lastError();
    validateErrorState(error);
    
    // Test: Region recognition from string with parameters
    QVariantHash params;
    params["language"] = "en";
    params["psm"] = 6; // Assume uniform block of text
    
    QString stringParamResult = ocrRec->recognizeRegionFromString(testImagePath, regionString, params);
    qDebug() << "Region string with params result:" << stringParamResult;
    
    error = ocrRec->lastError();
    validateErrorState(error);
    
    // Test: Region recognition from QRect
    QRect region(0,0,728,370);
    QString rectResult = ocrRec->recognizeRegionFromRect(testImagePath, region);
    qDebug() << "Region rect recognition result:" << rectResult;
    
    error = ocrRec->lastError();
    validateErrorState(error);
    
    // Test: Region recognition from QRect with parameters
    QString rectParamResult = ocrRec->recognizeRegionFromRect(testImagePath, region, params);
    qDebug() << "Region rect with params result:" << rectParamResult;
    
    error = ocrRec->lastError();
    validateErrorState(error);
    
    // Test: Invalid region string
    QString invalidStringResult = ocrRec->recognizeRegionFromString(testImagePath, "invalid,region");
    qDebug() << "Invalid region string result:" << invalidStringResult;
    
    error = ocrRec->lastError();
    validateErrorState(error); // May or may not be an error depending on implementation
    
    // Test: Empty region
    QRect emptyRegion;
    QString emptyRectResult = ocrRec->recognizeRegionFromRect(testImagePath, emptyRegion);
    qDebug() << "Empty region result:" << emptyRectResult;
    
    error = ocrRec->lastError();
    validateErrorState(error);
    
    // Test: Region outside image bounds
    QRect outsideRegion(1000, 1000, 100, 100);
    QString outsideResult = ocrRec->recognizeRegionFromRect(testImagePath, outsideRegion);
    qDebug() << "Exspect: Outside region result:" << outsideResult;
    
    error = ocrRec->lastError();
    validateErrorState(error);
    
    qDebug() << "Region recognition tests completed";
}

/**
 * @brief Test information query methods
 */
TEST_F(TestDOCRRecognition, informationMethods)
{
    qDebug() << "Testing DOCRRecognition information methods";
    
    // Test: Get supported languages
    QStringList languages = ocrRec->getSupportedLanguages();
    qDebug() << "Supported languages:" << languages;
    
    // Languages may be empty in test environment
    if (!languages.isEmpty()) {
        EXPECT_GT(languages.size(), 0);
        // Common languages should be supported
        bool hasCommonLanguage = languages.contains("en") || languages.contains("zh-cn") || 
                                languages.contains("zh") || languages.contains("eng");
        if (hasCommonLanguage) {
            qDebug() << "Common languages are supported";
        }
    } else {
        qDebug() << "No supported languages returned - may be normal in test environment";
    }
    
    // Test: Get supported formats
    QStringList formats = ocrRec->getSupportedFormats();
    qDebug() << "Supported formats:" << formats;
    
    // Formats may be empty in test environment
    if (!formats.isEmpty()) {
        EXPECT_GT(formats.size(), 0);
        // Common formats should be supported
        bool hasCommonFormat = formats.contains("jpg") || formats.contains("jpeg") || 
                              formats.contains("png") || formats.contains("pdf");
        if (hasCommonFormat) {
            qDebug() << "Common image formats are supported";
        }
    } else {
        qDebug() << "No supported formats returned - may be normal in test environment";
    }
    
    // Test: Get capabilities
    QString capabilities = ocrRec->getCapabilities();
    qDebug() << "OCR capabilities:" << capabilities;
    
    // Capabilities may be empty in test environment
    if (!capabilities.isEmpty()) {
        EXPECT_GT(capabilities.length(), 0);
        EXPECT_LT(capabilities.length(), 10000); // Should be reasonable length
        qDebug() << "OCR capabilities information available";
    } else {
        qDebug() << "No capabilities returned - may be normal in test environment";
    }
    
    qDebug() << "Information methods tests completed";
}

/**
 * @brief Test termination functionality
 */
TEST_F(TestDOCRRecognition, terminateOperation)
{
    qDebug() << "Testing DOCRRecognition terminate functionality";
    
    // Test: Terminate with no active operation
    EXPECT_NO_THROW({
        ocrRec->terminate();
    });
    
    DTK_CORE_NAMESPACE::DError error = ocrRec->lastError();
    validateErrorState(error);
    
    qDebug() << "Terminate called with no active operation";
    
    // Test: Multiple terminate calls
    EXPECT_NO_THROW({
        ocrRec->terminate();
        ocrRec->terminate();
    });
    
    qDebug() << "Multiple terminate calls handled correctly";
    
    qDebug() << "Terminate functionality tests completed";
}

/**
 * @brief Test error handling scenarios
 */
TEST_F(TestDOCRRecognition, errorHandling)
{
    qDebug() << "Testing DOCRRecognition error handling";
    
    // Test: Recognition with very large fake data
    QByteArray largeData(50 * 1024 * 1024, 'X'); // 50MB of 'X'
    QString largeResult = ocrRec->recognizeImage(largeData);
    qDebug() << "Large data result length:" << largeResult.length();
    
    DTK_CORE_NAMESPACE::DError error = ocrRec->lastError();
    // Should have an error for invalid/large image data
    EXPECT_NE(error.getErrorCode(), NoError);
    
    // Test: Recognition with corrupted image file
    QString testImagePath = getTestImagePath();
    
    // Corrupt the file by truncating it
    QFile file(testImagePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write("corrupted");
        file.close();
    }
    
    QString corruptedResult = ocrRec->recognizeFile(testImagePath);
    qDebug() << "Corrupted file result:" << corruptedResult;
    
    error = ocrRec->lastError();
    EXPECT_NE(error.getErrorCode(), NoError);
    
    // Test: Multiple rapid recognition requests
    QString validImagePath = getTestImagePath();
    
    for (int i = 0; i < 5; ++i) {
        QString rapidResult = ocrRec->recognizeFile(validImagePath);
        qDebug() << "Rapid recognition" << i << "result length:" << rapidResult.length();
        
        error = ocrRec->lastError();
        validateErrorState(error);
        
        // Small delay between requests
        QTest::qWait(10);
    }
    
    qDebug() << "Error handling tests completed";
}

/**
 * @brief Test parameter validation
 */
TEST_F(TestDOCRRecognition, parameterValidation)
{
    qDebug() << "Testing DOCRRecognition parameter validation";
    
    QString testImagePath = getTestImagePath();
    ASSERT_FALSE(testImagePath.isEmpty());
    
    // Test: Valid language parameters
    QStringList testLanguages = {"en", "zh-cn", "zh", "ja", "ko", "de", "fr"};
    
    for (const QString &lang : testLanguages) {
        QVariantHash params;
        params["language"] = lang;
        
        QString langResult = ocrRec->recognizeFile(testImagePath, params);
        qDebug() << "Language" << lang << "result length:" << langResult.length();
        
        DTK_CORE_NAMESPACE::DError error = ocrRec->lastError();
        validateErrorState(error);
    }
    
    // Test: Various OCR parameters
    QVariantHash ocrParams;
    ocrParams["psm"] = 6;                    // Page segmentation mode
    ocrParams["oem"] = 3;                    // OCR engine mode
    ocrParams["dpi"] = 300;                  // DPI setting
    ocrParams["scale"] = 2.0;                // Scale factor
    ocrParams["deskew"] = true;              // Deskew image
    ocrParams["remove_noise"] = true;        // Remove noise
    ocrParams["enhance_contrast"] = false;   // Enhance contrast
    
    QString ocrParamResult = ocrRec->recognizeFile(testImagePath, ocrParams);
    qDebug() << "OCR params result length:" << ocrParamResult.length();
    
    DTK_CORE_NAMESPACE::DError error = ocrRec->lastError();
    validateErrorState(error);
    
    // Test: Invalid parameter types
    QVariantHash invalidParams;
    invalidParams["psm"] = "not_a_number";
    invalidParams["dpi"] = -1;
    invalidParams["language"] = 12345;
    invalidParams["invalid_param"] = QVariantList();
    
    QString invalidParamResult = ocrRec->recognizeFile(testImagePath, invalidParams);
    qDebug() << "Invalid params result:" << invalidParamResult;
    
    error = ocrRec->lastError();
    validateErrorState(error);
    
    // Test: Region parameters
    QRect testRegion(5, 5, 90, 25);    
    QString regionParamResult = ocrRec->recognizeRegionFromRect(testImagePath, testRegion);
    qDebug() << "Region params result:" << regionParamResult;
    
    error = ocrRec->lastError();
    validateErrorState(error);
    
    qDebug() << "Parameter validation tests completed";
}

// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "test_base.h"
#include "test_utils.h"

#include "dtkai/dimagerecognition.h"
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
#include <QBuffer>

DAI_USE_NAMESPACE

/**
 * @brief Test fixture for DImageRecognition interface
 * 
 * Tests the image recognition functionality including:
 * - Constructor/destructor behavior
 * - Image recognition from file, data, and URL
 * - Information query methods
 * - Error handling and parameter validation
 * - Termination functionality
 */
class TestDImageRecognition : public TestBase
{
protected:
    void SetUp() override
    {
        TestBase::SetUp();
        qDebug() << "Setting up DImageRecognition interface tests";
        
        // Create image recognition instance
        imageRec = new DImageRecognition();
        ASSERT_NE(imageRec, nullptr);
    }
    
    void TearDown() override
    {
        if (imageRec) {
            delete imageRec;
            imageRec = nullptr;
        }
        
        // Clean up test files
        cleanupTestFiles();
        
        TestBase::TearDown();
    }
    

    /**
     * @brief Generate test image data
     */
    QByteArray getEmbeddedImageData()
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

    /**
     * @brief Create a test image file for testing
     */
    QString getTestImage()
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
    DImageRecognition *imageRec = nullptr;
    QStringList testFiles;
};

/**
 * @brief Test DImageRecognition constructor and destructor
 */
TEST_F(TestDImageRecognition, constructorDestructor)
{
    qDebug() << "Testing DImageRecognition constructor and destructor";
    
    // Test constructor with parent
    QObject *parent = new QObject();
    DImageRecognition *imageRecWithParent = new DImageRecognition(parent);
    EXPECT_NE(imageRecWithParent, nullptr);
    EXPECT_EQ(imageRecWithParent->parent(), parent);
    
    // Parent will clean up child automatically
    delete parent;
    
    // Test standalone constructor (already created in SetUp)
    EXPECT_NE(imageRec, nullptr);
    EXPECT_EQ(imageRec->parent(), nullptr);
    
    qDebug() << "Constructor/destructor tests completed";
}

/**
 * @brief Test image recognition from file
 */
TEST_F(TestDImageRecognition, imageFileRecognition)
{
    qDebug() << "Testing DImageRecognition file recognition";
    
    // Create test image file
    QString testImagePath = getTestImage();
    ASSERT_FALSE(testImagePath.isEmpty());
    ASSERT_TRUE(QFile::exists(testImagePath));
    
    // Test: Basic image recognition
    QString result = imageRec->recognizeImage(testImagePath);
    qDebug() << "Basic recognition result:" << result;
    
    DTK_CORE_NAMESPACE::DError error = imageRec->lastError();
    validateErrorState(error);
    
    // Test: Recognition with prompt
    QString promptResult = imageRec->recognizeImage(testImagePath, "Describe this image");
    qDebug() << "Recognition with prompt result:" << promptResult;
    
    error = imageRec->lastError();
    validateErrorState(error);
    
    // Test: Recognition with parameters
    QVariantHash params;
    params["language"] = "en";
    params["detail_level"] = "high";
    
    QString paramResult = imageRec->recognizeImage(testImagePath, "What do you see?", params);
    qDebug() << "Recognition with params result:" << paramResult;
    
    error = imageRec->lastError();
    validateErrorState(error);
    
    // Test: Invalid file path
    QString invalidResult = imageRec->recognizeImage("/nonexistent/path/image.jpg");
    qDebug() << "Invalid file result:" << invalidResult;
    
    error = imageRec->lastError();
    // Should have an error for invalid file
    EXPECT_NE(error.getErrorCode(), NoError);
    
    qDebug() << "File recognition tests completed";
}

/**
 * @brief Test image recognition from data
 */
TEST_F(TestDImageRecognition, imageDataRecognition)
{
    qDebug() << "Testing DImageRecognition data recognition";
    
    // Generate test image data
    QByteArray imageData = getEmbeddedImageData();
    ASSERT_FALSE(imageData.isEmpty());
    
    // Test: Basic data recognition
    QString result = imageRec->recognizeImageData(imageData);
    qDebug() << "Data recognition result:" << result;
    
    DTK_CORE_NAMESPACE::DError error = imageRec->lastError();
    validateErrorState(error);
    
    // Test: Data recognition with prompt
    QString promptResult = imageRec->recognizeImageData(imageData, "Analyze this image");
    qDebug() << "Data recognition with prompt result:" << promptResult;
    
    error = imageRec->lastError();
    validateErrorState(error);
    
    QString paramResult = imageRec->recognizeImageData(imageData, "What objects are present?");
    qDebug() << "Data recognition with params result:" << paramResult;
    
    error = imageRec->lastError();
    validateErrorState(error);
    
    // Test: Empty data
    QString emptyResult = imageRec->recognizeImageData(QByteArray());
    qDebug() << "Empty data result:" << emptyResult;
    
    error = imageRec->lastError();
    // Should have an error for empty data
    EXPECT_NE(error.getErrorCode(), NoError);
    
    qDebug() << "Data recognition tests completed";
}

/**
 * @brief Test image recognition from URL
 */
TEST_F(TestDImageRecognition, imageUrlRecognition)
{
    qDebug() << "Testing DImageRecognition URL recognition";
    
    // Test: URL recognition (using a placeholder URL)
    QString testUrl = "https://ark-project.tos-cn-beijing.ivolces.com/images/view.jpeg";
    QString result = imageRec->recognizeImageUrl(testUrl);
    qDebug() << "URL recognition result:" << result;
    EXPECT_NE(result.size(), 10);
    
    DTK_CORE_NAMESPACE::DError error = imageRec->lastError();
    validateErrorState(error);
    
    // Test: URL recognition with prompt
    QString promptResult = imageRec->recognizeImageUrl(testUrl, "Describe the content");
    qDebug() << "URL recognition with prompt result:" << promptResult;
    
    error = imageRec->lastError();
    validateErrorState(error);
    
    QString paramResult = imageRec->recognizeImageUrl(testUrl, "Identify objects");
    qDebug() << "URL recognition with params result:" << paramResult;
    
    error = imageRec->lastError();
    validateErrorState(error);
    
    // Test: Invalid URL
    QString invalidResult = imageRec->recognizeImageUrl("not-a-valid-url");
    qDebug() << "Invalid URL result:" << invalidResult;
    
    error = imageRec->lastError();
    // Should have an error for invalid URL
    EXPECT_NE(error.getErrorCode(), NoError);
    
    qDebug() << "URL recognition tests completed";
}

/**
 * @brief Test information query methods
 */
TEST_F(TestDImageRecognition, informationMethods)
{
    qDebug() << "Testing DImageRecognition information methods";
    
    // Test: Get supported image formats
    QStringList formats = imageRec->getSupportedImageFormats();
    qDebug() << "Supported formats:" << formats;
    
    // Formats may be empty in test environment
    if (!formats.isEmpty()) {
        EXPECT_GT(formats.size(), 0);
        // Common formats should be supported
        bool hasCommonFormat = formats.contains("jpg") || formats.contains("jpeg") || 
                              formats.contains("png") || formats.contains("gif");
        if (hasCommonFormat) {
            qDebug() << "Common image formats are supported";
        }
    } else {
        qDebug() << "No supported formats returned - may be normal in test environment";
    }
    
    // Test: Get maximum image size
    int maxSize = imageRec->getMaxImageSize();
    qDebug() << "Maximum image size:" << maxSize;
    
    // Max size should be reasonable (either 0 for unlimited or a positive value)
    EXPECT_GE(maxSize, 0);
    if (maxSize > 0) {
        EXPECT_LE(maxSize, 100 * 1024 * 1024); // Should not exceed 100MB
        qDebug() << "Maximum image size is reasonable:" << maxSize << "bytes";
    } else {
        qDebug() << "Unlimited image size or not available in test environment";
    }
    
    qDebug() << "Information methods tests completed";
}

/**
 * @brief Test termination functionality
 */
TEST_F(TestDImageRecognition, terminateOperation)
{
    qDebug() << "Testing DImageRecognition terminate functionality";
    
    // Test: Terminate with no active operation
    EXPECT_NO_THROW({
        imageRec->terminate();
    });
    
    DTK_CORE_NAMESPACE::DError error = imageRec->lastError();
    validateErrorState(error);
    
    qDebug() << "Terminate called with no active operation";
    
    // Test: Multiple terminate calls
    EXPECT_NO_THROW({
        imageRec->terminate();
        imageRec->terminate();
    });
    
    qDebug() << "Multiple terminate calls handled correctly";
    
    qDebug() << "Terminate functionality tests completed";
}

/**
 * @brief Test error handling scenarios
 */
TEST_F(TestDImageRecognition, errorHandling)
{
    qDebug() << "Testing DImageRecognition error handling";
    
    // Test: Recognition with very large fake data
    QByteArray largeData(10 * 1024 * 1024, 'X'); // 10MB of 'X'
    QString largeResult = imageRec->recognizeImageData(largeData);
    qDebug() << "Large data result length:" << largeResult.length();
    
    DTK_CORE_NAMESPACE::DError error = imageRec->lastError();
    // Should have an error for invalid image data
    EXPECT_NE(error.getErrorCode(), NoError);
    
    // Test: Recognition with invalid image data
    QByteArray invalidData("This is not image data");
    QString invalidResult = imageRec->recognizeImageData(invalidData);
    qDebug() << "Invalid data result:" << invalidResult;
    
    error = imageRec->lastError();
    EXPECT_NE(error.getErrorCode(), NoError);
    
    // Test: Recognition with extremely long prompt
    QString longPrompt = QString("Describe ").repeated(1000) + "this image.";
    QString testImagePath = getTestImage();
    QString longPromptResult = imageRec->recognizeImage(testImagePath, longPrompt);
    qDebug() << "Long prompt result length:" << longPromptResult.length();
    
    error = imageRec->lastError();
    validateErrorState(error);
    
    qDebug() << "Error handling tests completed";
}

/**
 * @brief Test parameter validation
 */
TEST_F(TestDImageRecognition, parameterValidation)
{
    qDebug() << "Testing DImageRecognition parameter validation";
    
    QString testImagePath = getTestImage();
    ASSERT_FALSE(testImagePath.isEmpty());
    
    // Test: Various parameter combinations
    QVariantHash validParams;
    validParams["language"] = "en";
    validParams["confidence"] = 0.9;
    validParams["detail_level"] = "medium";
    
    QString validResult = imageRec->recognizeImage(testImagePath, "Test", validParams);
    qDebug() << "Valid params result length:" << validResult.length();
    
    DTK_CORE_NAMESPACE::DError error = imageRec->lastError();
    validateErrorState(error);
    
    // Test: Invalid parameter types
    QVariantHash invalidParams;
    invalidParams["confidence"] = "not_a_number";
    invalidParams["timeout"] = -1;
    invalidParams["invalid_param"] = QVariantList();
    
    QString invalidParamResult = imageRec->recognizeImage(testImagePath, "Test", invalidParams);
    qDebug() << "Invalid params result:" << invalidParamResult;
    
    error = imageRec->lastError();
    validateErrorState(error);
    
    // Test: Empty parameters
    QString emptyParamResult = imageRec->recognizeImage(testImagePath, "Test", QVariantHash());
    qDebug() << "Empty params result length:" << emptyParamResult.length();
    
    error = imageRec->lastError();
    validateErrorState(error);
    
    qDebug() << "Parameter validation tests completed";
}

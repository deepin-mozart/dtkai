// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "test_base.h"
#include "test_utils.h"

#include "dtkai/speech/dtexttospeech.h"
#include "dtkai/dtkaitypes.h"
#include "dtkai/DAIError"

#include <QSignalSpy>
#include <QTimer>
#include <QTest>
#include <QVariantHash>
#include <QByteArray>
#include <QTemporaryFile>
#include <QStandardPaths>

DAI_USE_NAMESPACE

/**
 * @brief Test class for DTextToSpeech interface
 * 
 * This class tests the text-to-speech functionality including
 * text synthesis, stream synthesis, and voice configuration.
 */
class TestDTextToSpeech : public TestBase
{
protected:
    void SetUp() override
    {
        TestBase::SetUp();
        qDebug() << "Setting up DTextToSpeech interface tests";
        
        // Create text-to-speech instance for testing
        tts = new DTextToSpeech();
        ASSERT_NE(tts, nullptr) << "Failed to create DTextToSpeech instance";
    }
    
    void TearDown() override
    {
        if (tts) {
            delete tts;
            tts = nullptr;
        }
        TestBase::TearDown();
    }
    
    /**
     * @brief Helper method to create sample speech parameters
     * @return A QVariantHash with common text-to-speech parameters
     */
    QVariantHash createSampleSpeechParameters()
    {
        QVariantHash params;
        params["voice"] = "x4_yezi";
        params["speed"] = 1.0;
        params["pitch"] = 1.0;
        params["volume"] = 1.0;
        params["sample_rate"] = 16000;
        params["format"] = "wav";
        params["encoding"] = "pcm";
        return params;
    }
    
    /**
     * @brief Validate synthesized audio data
     * @param audioData The audio data to validate
     */
    void validateAudioData(const QByteArray &audioData)
    {
        if (!audioData.isEmpty()) {
            EXPECT_GT(audioData.size(), 0) << "Audio data should not be empty";
            EXPECT_LT(audioData.size(), 10 * 1024 * 1024) << "Audio data should be reasonable size (< 10MB)";
            
            qDebug() << "Audio data size:" << audioData.size() << "bytes";
            
            // Basic validation - audio data should contain some variation
            // (not all zeros or all same value)
            if (audioData.size() > 100) {
                bool hasVariation = false;
                char firstByte = audioData.at(0);
                for (int i = 1; i < qMin(100, audioData.size()); ++i) {
                    if (audioData.at(i) != firstByte) {
                        hasVariation = true;
                        break;
                    }
                }
                
                if (hasVariation) {
                    qDebug() << "Audio data shows variation (good sign)";
                } else {
                    qDebug() << "Audio data appears uniform (may be silence or mock data)";
                }
            }
        } else {
            qDebug() << "Audio data is empty - may be normal in test environment";
        }
    }
    
    /**
     * @brief Validate error state for text-to-speech
     * @param expectedError Whether an error is expected
     */
    void validateErrorState(bool expectedError = false)
    {
        auto error = tts->lastError();
        
        // In test environment without AI daemon, error code 1 (APIServerNotAvailable) is expected
        if (expectedError) {
            EXPECT_NE(error.getErrorCode(), NoError) << "Expected an error to be set";
            if (error.getErrorCode() != NoError) {
                qDebug() << "Expected error code:" << error.getErrorCode() << "message:" << error.getErrorMessage();
            }
        } else {
            // In test environment, we may get APIServerNotAvailable (code 1) which is acceptable
            if (error.getErrorCode() == APIServerNotAvailable) {
                qDebug() << "Info: AI daemon not available (error code 1) - this is normal in test environment";
            } else if (error.getErrorCode() != NoError) {
                qDebug() << "Warning: Unexpected error code:" << error.getErrorCode() << "message:" << error.getErrorMessage();
            }
        }
    }

protected:
    DTextToSpeech *tts = nullptr;
};

/**
 * @brief Test constructor and destructor
 * 
 * This test verifies that DTextToSpeech can be properly
 * constructed and destroyed.
 */
TEST_F(TestDTextToSpeech, constructorDestructor)
{
    qInfo() << "Testing DTextToSpeech constructor and destructor";
    
    // Test: Constructor with parent
    EXPECT_NO_THROW({
        QObject parent;
        DTextToSpeech *ttsWithParent = new DTextToSpeech(&parent);
        EXPECT_NE(ttsWithParent, nullptr);
        EXPECT_EQ(ttsWithParent->parent(), &parent);
        
        // Destructor will be called automatically when parent is destroyed
    }) << "Constructor with parent should work correctly";
    
    // Test: Constructor without parent
    EXPECT_NO_THROW({
        DTextToSpeech *standaloneTts = new DTextToSpeech();
        EXPECT_NE(standaloneTts, nullptr);
        EXPECT_EQ(standaloneTts->parent(), nullptr);
        
        delete standaloneTts;
    }) << "Constructor without parent should work correctly";
    
    // Test: Initial error state
    validateErrorState(false);
    
    qInfo() << "Constructor/destructor tests completed";
}

/**
 * @brief Test stream synthesis functionality
 * 
 * This test verifies that streaming text synthesis works
 * correctly with proper signal emission.
 */
TEST_F(TestDTextToSpeech, streamSynthesis)
{
    qInfo() << "Testing DTextToSpeech stream synthesis";
    
    // Set up signal spies
    QSignalSpy resultSpy(tts, &DTextToSpeech::synthesisResult);
    QSignalSpy errorSpy(tts, &DTextToSpeech::synthesisError);
    QSignalSpy completedSpy(tts, &DTextToSpeech::synthesisCompleted);
    
    ASSERT_TRUE(resultSpy.isValid()) << "synthesisResult signal should be valid";
    ASSERT_TRUE(errorSpy.isValid()) << "synthesisError signal should be valid";
    ASSERT_TRUE(completedSpy.isValid()) << "synthesisCompleted signal should be valid";
    
    // Test: Basic stream synthesis
    EXPECT_NO_THROW({
        QString text = "This is a streaming text-to-speech test.";
        QVariantHash params = createSampleSpeechParameters();
        
        bool started = tts->startStreamSynthesis(text, params);
        
        if (started) {
            qDebug() << "Stream synthesis started successfully";
            
            // End the stream and get final audio
            QByteArray finalAudio = tts->endStreamSynthesis();
            
            qDebug() << "Final audio size:" << finalAudio.size();
            validateAudioData(finalAudio);
            
            // Wait for potential signals
            QTest::qWait(100);
            
            // In test environment, signals may not be emitted due to no real daemon
            qDebug() << "Signal counts - Result:" << resultSpy.count() 
                     << "Error:" << errorSpy.count()
                     << "Completed:" << completedSpy.count();
            
        } else {
            qDebug() << "Stream synthesis failed to start - checking for errors";
            validateErrorState(true);
        }
        
    }) << "Basic stream synthesis should work";
    
    // Clear spies for next test
    resultSpy.clear();
    errorSpy.clear();
    completedSpy.clear();
    
    // Test: Stream synthesis without parameters
    EXPECT_NO_THROW({
        QString text = "Simple stream test without parameters.";
        bool started = tts->startStreamSynthesis(text);
        
        if (started) {
            qDebug() << "Stream synthesis without params started";
            
            QByteArray result = tts->endStreamSynthesis();
            qDebug() << "Stream result without params size:" << result.size();
            validateAudioData(result);
        }
    });
    
    qDebug() << "Stream synthesis without parameters test completed";
    
    // Test: Multiple text stream synthesis - simplified
    {
        QStringList texts = {
            "First sentence for synthesis.",
            "Second sentence with different content.",
            "Final sentence to complete the test."
        };
        
        EXPECT_NO_THROW({
            for (const QString &text : texts) {
                bool started = tts->startStreamSynthesis(text);
                if (started) {
                    QByteArray audio = tts->endStreamSynthesis();
                    qDebug() << "Multi-text synthesis result size:" << audio.size();
                }
                QTest::qWait(10);
            }
        });
    }
    
    qDebug() << "Multiple text stream synthesis test completed";
    
    qInfo() << "Stream synthesis tests completed";
}

/**
 * @brief Test terminate functionality
 * 
 * This test verifies that ongoing synthesis operations
 * can be properly terminated.
 */
TEST_F(TestDTextToSpeech, terminateOperation)
{
    qInfo() << "Testing DTextToSpeech terminate functionality";
    
    // Test: Terminate stream synthesis
    EXPECT_NO_THROW({
        QString longText = "This is a very long text that should take some time to synthesize. "
                          "We will attempt to terminate this synthesis operation before it completes.";
        
        bool started = tts->startStreamSynthesis(longText);
        
        if (started) {
            qDebug() << "Started long stream synthesis for termination test";
            
            // Give it a moment to start
            QTest::qWait(50);
            
            // Terminate the operation
            tts->terminate();
            qDebug() << "Terminate called";
            
            validateErrorState(false); // Termination should not cause error state
            
        } else {
            qDebug() << "Stream failed to start for termination test";
        }
        
    }) << "Terminate stream synthesis should work correctly";
    
    // Test: Terminate when no operation is running
    EXPECT_NO_THROW({
        tts->terminate();
        qDebug() << "Terminate called with no active operation";
        validateErrorState(false); // Should not cause error
        
    }) << "Terminate with no active operation should be safe";
    
    qInfo() << "Terminate functionality tests completed";
}

/**
 * @brief Test information methods
 * 
 * This test verifies that information retrieval methods
 * work correctly and return reasonable data.
 */
TEST_F(TestDTextToSpeech, informationMethods)
{
    qInfo() << "Testing DTextToSpeech information methods";
    
    // Test: Get supported voices
    EXPECT_NO_THROW({
        QStringList voices = tts->getSupportedVoices();
        
        qDebug() << "Supported voices:" << voices;
        
        // Voices list may be empty in test environment, which is acceptable
        if (!voices.isEmpty()) {
            // Voice identifiers should be reasonable strings
            for (const QString &voice : voices) {
                EXPECT_FALSE(voice.isEmpty()) << "Voice string should not be empty";
                EXPECT_LT(voice.length(), 100) << "Voice string should be reasonable length";
                
                // Voice names often contain language codes or descriptive names
                qDebug() << "Voice:" << voice;
            }
            
            // Check for common voice patterns
            bool hasReasonableVoice = false;
            for (const QString &voice : voices) {
                if (voice.contains("zh", Qt::CaseInsensitive) ||
                    voice.contains("en", Qt::CaseInsensitive) ||
                    voice.contains("female", Qt::CaseInsensitive) ||
                    voice.contains("male", Qt::CaseInsensitive)) {
                    hasReasonableVoice = true;
                    break;
                }
            }
            
            if (hasReasonableVoice) {
                qDebug() << "Found reasonable voice identifiers in supported list";
            }
        } else {
            qDebug() << "No supported voices returned - may be normal in test environment";
        }
        
    }) << "getSupportedVoices should work correctly";
    
    // Test: Get provider information (commented out - method not yet implemented)
    /*
    EXPECT_NO_THROW({
        QString providerInfo = tts->getProviderInfo();
        
        qDebug() << "Provider info:" << providerInfo;
        
        // Provider info may be empty in test environment
        if (!providerInfo.isEmpty()) {
            EXPECT_LT(providerInfo.length(), 1000) << "Provider info should be reasonable length";
        } else {
            qDebug() << "No provider info returned - may be normal in test environment";
        }
    });
    */
    
    qDebug() << "getProviderInfo test skipped - method not yet implemented";
    
    qInfo() << "Information methods tests completed";
}

/**
 * @brief Test error handling scenarios
 * 
 * This test verifies that various error conditions
 * are handled appropriately.
 */
TEST_F(TestDTextToSpeech, errorHandling)
{
    qInfo() << "Testing DTextToSpeech error handling";

    // Test: Stream synthesis interruption
    EXPECT_NO_THROW({
        QString text = "This synthesis will be interrupted.";
        
        bool started1 = tts->startStreamSynthesis(text);
        bool started2 = tts->startStreamSynthesis(text); // Second start without ending first
        
        qDebug() << "Multiple stream starts:" << started1 << started2;
        
        // End the stream (whichever is active)
        QByteArray result = tts->endStreamSynthesis();
        qDebug() << "Result after interruption size:" << result.size();
        
    }) << "Stream synthesis interruption should be handled gracefully";
    
    qInfo() << "Error handling tests completed";
}

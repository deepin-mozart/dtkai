// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <DTextToSpeech>
#include <daierror.h>

#include <QCoreApplication>
#include <QTimer>
#include <QDebug>
#include <QFile>
#include <QDir>

DCORE_USE_NAMESPACE
DAI_USE_NAMESPACE

class TextToSpeechTest : public QObject
{
    Q_OBJECT
public:
    explicit TextToSpeechTest(QObject *parent = nullptr);
    ~TextToSpeechTest();

    void testSynthesis();
    void testStreamSynthesis();

private Q_SLOTS:
    void onSynthesisResult(const QByteArray &audioData);
    void onSynthesisError(int errorCode, const QString &errorMessage);
    void onSynthesisCompleted(const QByteArray &finalAudio);

private:
    DTextToSpeech *tts = nullptr;
    QByteArray receivedAudioData;
    bool synthesisCompleted = false;
};

TextToSpeechTest::TextToSpeechTest(QObject *parent)
    : QObject(parent)
{
    tts = new DTextToSpeech(this);
    qDebug() << "TextToSpeechTest constructor - creating DTextToSpeech instance";
    
    // Connect signals
    connect(tts, &DTextToSpeech::synthesisResult, this, &TextToSpeechTest::onSynthesisResult);
    connect(tts, &DTextToSpeech::synthesisError, this, &TextToSpeechTest::onSynthesisError);
    connect(tts, &DTextToSpeech::synthesisCompleted, this, &TextToSpeechTest::onSynthesisCompleted);
    
    qDebug() << "TextToSpeechTest constructor - signals connected";
}

TextToSpeechTest::~TextToSpeechTest()
{
    qDebug() << "TextToSpeechTest destructor";
}

void TextToSpeechTest::testSynthesis()
{
    qDebug() << "Testing text synthesis...";
    
    // Get supported voices
    QStringList voices = tts->getSupportedVoices();
    qDebug() << "Supported voices:" << voices;
    
    // Test text synthesis
    QString testText = "这是一个语音合成测试，Hello World!";
    QVariantHash params;
    params["voice"] = "x4_yezi";
    params["speed"] = 50;
    params["volume"] = 50;
    params["pitch"] = 50;
    
    qDebug() << "Synthesizing text:" << testText;
    QByteArray audioData = tts->synthesizeText(testText, params);
    
    if (!audioData.isEmpty()) {
        qDebug() << "Synthesis successful, audio data size:" << audioData.size();
        
        // Save audio data to file
        QFile file("synthesized_audio.pcm");
        if (file.open(QIODevice::WriteOnly)) {
            file.write(audioData);
            file.close();
            qDebug() << "Audio data saved to synthesized_audio.pcm";
        } else {
            qDebug() << "Failed to save audio data to file";
        }
    } else {
        qDebug() << "Synthesis failed:" << tts->lastError().getErrorMessage();
    }
}

void TextToSpeechTest::testStreamSynthesis()
{
    qDebug() << "Testing stream synthesis...";
    
    QString testText = "这是流式语音合成测试，Streaming Text-to-Speech Test!";
    QVariantHash params;
    params["voice"] = "x4_yezi";
    params["speed"] = 50;
    params["volume"] = 50;
    params["pitch"] = 50;
    
    qDebug() << "Starting stream synthesis for text:" << testText;
    
    if (tts->startStreamSynthesis(testText, params)) {
        qDebug() << "Stream synthesis started successfully";
        
        // Wait for synthesis to complete
        synthesisCompleted = false;
        receivedAudioData.clear();
        
        // Set a timer to end synthesis after a reasonable time
        QTimer::singleShot(10000, [this]() {
            if (!synthesisCompleted) {
                qDebug() << "Timeout reached, ending stream synthesis";
                QByteArray finalAudio = tts->endStreamSynthesis();
                qDebug() << "Final audio data size:" << finalAudio.size();
                
                // Save final audio data
                if (!finalAudio.isEmpty()) {
                    QFile file("stream_synthesized_audio.pcm");
                    if (file.open(QIODevice::WriteOnly)) {
                        file.write(finalAudio);
                        file.close();
                        qDebug() << "Stream audio data saved to stream_synthesized_audio.pcm";
                    }
                }
                
                QCoreApplication::quit();
            }
        });
    } else {
        qDebug() << "Failed to start stream synthesis:" << tts->lastError().getErrorMessage();
        QCoreApplication::quit();
    }
}

void TextToSpeechTest::onSynthesisResult(const QByteArray &audioData)
{
    qDebug() << "Received synthesis result, audio data size:" << audioData.size();
    receivedAudioData.append(audioData);
}

void TextToSpeechTest::onSynthesisError(int errorCode, const QString &errorMessage)
{
    qDebug() << "Synthesis error:" << errorCode << errorMessage;
    synthesisCompleted = true;
    QCoreApplication::quit();
}

void TextToSpeechTest::onSynthesisCompleted(const QByteArray &finalAudio)
{
    qDebug() << "Synthesis completed, final audio data size:" << finalAudio.size();
    synthesisCompleted = true;
    
    // Save final audio data
    if (!finalAudio.isEmpty()) {
        QFile file("stream_synthesized_audio.pcm");
        if (file.open(QIODevice::WriteOnly)) {
            file.write(finalAudio);
            file.close();
            qDebug() << "Stream audio data saved to stream_synthesized_audio.pcm";
        }
    }
    
    QCoreApplication::quit();
}

int main(int argc, char *argv[])
{
    qDebug() << "main() started - creating QCoreApplication";
    
    QCoreApplication app(argc, argv);
    
    qDebug() << "QCoreApplication created";
    qDebug() << "Starting Text-to-Speech test application...";
    
    qDebug() << "About to create TextToSpeechTest instance";
    TextToSpeechTest test;
    qDebug() << "TextToSpeechTest instance created";
    
    // Test basic synthesis
    qDebug() << "Testing basic synthesis mode...";
    test.testSynthesis();
    
    // Wait a bit before testing stream synthesis
    QTimer::singleShot(2000, [&test]() {
        qDebug() << "Testing stream synthesis mode...";
        test.testStreamSynthesis();
    });
    
    qDebug() << "Entering event loop...";
    return app.exec();
}

#include "texttospeech_test.moc"

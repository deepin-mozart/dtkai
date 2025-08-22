// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <DSpeechToText>

#include <QCoreApplication>
#include <QDebug>
#include <QTimer>
#include <QAudioInput>
#include <QAudioFormat>
#include <QAudioDeviceInfo>
#include <QIODevice>
#include <QBuffer>
#include <QJsonDocument>
#include <QJsonObject>


DCORE_USE_NAMESPACE
DAI_USE_NAMESPACE

class RealTimeSpeechTest : public QObject
{
    Q_OBJECT
public:
    explicit RealTimeSpeechTest(QObject *parent = nullptr)
        : QObject(parent)
    {
        speechToText = new DSpeechToText(this);
        sendTimer = new QTimer(this);
        qDebug() << "RealTimeSpeechTest constructor - creating DSpeechToText instance";
        
        // Connect speech recognition signals
        connect(speechToText, &DSpeechToText::recognitionResult, 
                this, &RealTimeSpeechTest::onRecognitionResult);
        connect(speechToText, &DSpeechToText::recognitionPartialResult, 
                this, &RealTimeSpeechTest::onRecognitionPartialResult);
        connect(speechToText, &DSpeechToText::recognitionError, 
                this, &RealTimeSpeechTest::onRecognitionError);
        connect(speechToText, &DSpeechToText::recognitionCompleted, 
                this, &RealTimeSpeechTest::onRecognitionCompleted);
        
        qDebug() << "RealTimeSpeechTest constructor - signals connected";
        
        // Setup audio format for 16kHz, 16-bit, mono PCM
        setupAudioFormat();
        
        // Setup send timer to ensure audio data is sent regularly
        connect(sendTimer, &QTimer::timeout, this, &RealTimeSpeechTest::onSendTimer);
        sendTimer->setInterval(100); // Send every 100ms to prevent session timeout
    }

    ~RealTimeSpeechTest()
    {
        stopRecording();
    }

    void setupAudioFormat()
    {
        audioFormat.setSampleRate(16000);
        audioFormat.setChannelCount(1);
        audioFormat.setSampleSize(16);
        audioFormat.setCodec("audio/pcm");
        audioFormat.setByteOrder(QAudioFormat::LittleEndian);
        audioFormat.setSampleType(QAudioFormat::SignedInt);
        
        // Check if the format is supported
        QAudioDeviceInfo inputDevice = QAudioDeviceInfo::defaultInputDevice();
        if (!inputDevice.isFormatSupported(audioFormat)) {
            qWarning() << "Default audio format not supported, trying to use nearest format";
            audioFormat = inputDevice.nearestFormat(audioFormat);
            qDebug() << "Using audio format:" << audioFormat;
        } else {
            qDebug() << "Audio format supported:" << audioFormat;
        }
        
        // Update chunk size based on actual format - use 50ms for more frequent sending
        chunkSize = (audioFormat.sampleRate() * audioFormat.channelCount() * audioFormat.sampleSize() / 8) / 20; // 50ms
        qDebug() << "Chunk size set to:" << chunkSize << "bytes (50ms intervals)";
    }

    void startRecording()
    {
        if (isRecording) {
            qDebug() << "Already recording";
            return;
        }
        
        qDebug() << "Starting real-time speech recognition...";
        
        // Test supported formats and provider info
        QStringList formats = speechToText->getSupportedFormats();
        qDebug() << "Supported formats:" << formats;
        
        // Start stream recognition
        QVariantHash params;
        params["language"] = "zh-cn";
        params["format"] = "pcm";
        params["sampleRate"] = audioFormat.sampleRate();
        params["channels"] = audioFormat.channelCount();
        params["bitsPerSample"] = audioFormat.sampleSize();
        
        qDebug() << "Stream recognition parameters:" << params;
        qDebug() << "Audio format - sampleRate:" << audioFormat.sampleRate() 
                 << "channels:" << audioFormat.channelCount() 
                 << "sampleSize:" << audioFormat.sampleSize();
        
        if (!speechToText->startStreamRecognition(params)) {
            auto error = speechToText->lastError();
            qCritical() << "Failed to start stream recognition:" << error.getErrorCode() << error.getErrorMessage();
            return;
        }
        
        qDebug() << "Stream recognition started successfully";
        
        // Setup audio input
        QAudioDeviceInfo inputDevice = QAudioDeviceInfo::defaultInputDevice();
        qDebug() << "Using audio input device:" << inputDevice.deviceName();
        
        audioInput = new QAudioInput(inputDevice, audioFormat, this);
        
        // Start recording
        audioDevice = audioInput->start();
        if (!audioDevice) {
            qCritical() << "Failed to start audio input";
            return;
        }
        
        // Connect audio data ready signal
        connect(audioDevice, &QIODevice::readyRead, this, &RealTimeSpeechTest::onAudioDataReady);
        
        isRecording = true;
        sendTimer->start(); // Start the send timer
        qDebug() << "Audio recording started. Speak into your microphone...";
        qDebug() << "Press Ctrl+C to stop recording and get final result.";
    }

    void stopRecording()
    {
        if (!isRecording) {
            return;
        }
        
        qDebug() << "Stopping recording...";
        
        isRecording = false;
        
        sendTimer->stop(); // Stop the send timer
        
        if (audioInput) {
            audioInput->stop();
            audioInput->deleteLater();
            audioInput = nullptr;
            audioDevice = nullptr;
        }
        
        // Send any remaining buffered audio data
        if (!audioBuffer.isEmpty()) {
            qDebug() << "Sending remaining" << audioBuffer.size() << "bytes of audio data";
            if (!speechToText->sendAudioData(audioBuffer)) {
                qWarning() << "Failed to send remaining audio data";
            }
            audioBuffer.clear();
        }
        
        // End stream recognition
        QString finalResult = speechToText->endStreamRecognition();
        qDebug() << "Final recognition result:" << finalResult;
        
        // Exit application after a short delay
        QTimer::singleShot(1000, QCoreApplication::instance(), &QCoreApplication::quit);
    }

private Q_SLOTS:
    void onAudioDataReady()
    {
        if (!audioDevice || !isRecording) {
            return;
        }
        
        QByteArray newAudioData = audioDevice->readAll();
        if (newAudioData.isEmpty()) {
            return;
        }
        
        // Add new data to buffer
        audioBuffer.append(newAudioData);
        
        // Send complete chunks
        while (audioBuffer.size() >= chunkSize) {
            QByteArray chunk = audioBuffer.left(chunkSize);
            audioBuffer.remove(0, chunkSize);
            
            // Send audio data to speech recognition
            if (!speechToText->sendAudioData(chunk)) {
                qWarning() << "Failed to send audio chunk of" << chunk.size() << "bytes";
            } else {
                qDebug() << "Sent audio chunk:" << chunk.size() << "bytes";
            }
        }
    }

    void onRecognitionResult(const QString &text)
    {
        qInfo() << "Recognition result:" << text;
    }

    void onRecognitionPartialResult(const QString &partialText)
    {
        qInfo() << "Partial result:" << partialText;
    }

    void onRecognitionError(int errorCode, const QString &errorMessage)
    {
        qCritical() << "Recognition error:" << errorCode << errorMessage;
        
        // Handle specific error codes
        if (errorCode == 10165) {
            qWarning() << "Session handle invalid (10165) - this may be due to session timeout or connection issues";
            qWarning() << "Consider restarting the recognition session";
        }
        
        // For now, stop recording on any error
        // In a production environment, you might want to implement retry logic
        stopRecording();
    }

    void onRecognitionCompleted(const QString &finalText)
    {
        qInfo() << "Recognition completed:" << finalText;
        // Don't stop recording immediately, let user continue speaking
        // stopRecording();
    }

    void onSendTimer()
    {
        // not send anything.
        return;
    }

private:
    DSpeechToText *speechToText = nullptr;
    QAudioInput *audioInput = nullptr;
    QIODevice *audioDevice = nullptr;
    QAudioFormat audioFormat;
    bool isRecording = false;
    QByteArray audioBuffer;
    int chunkSize = 3200; // 50ms at 16kHz, 16-bit mono for more frequent sending
    QTimer *sendTimer = nullptr;
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    qDebug() << "Starting Real-time Speech Recognition Test (Fixed Version)...";
    qDebug() << "This program will record from your default microphone and perform real-time speech recognition.";
    qDebug() << "Make sure your microphone is working and speak clearly.";
    
    RealTimeSpeechTest test;
    
    // Setup signal handler for graceful shutdown
    QTimer::singleShot(100, &test, [&test]() {
        test.startRecording();
    });
    
    return app.exec();
}

#include "speechtotext_realtime.moc"

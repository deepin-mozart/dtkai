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
#include <QAudioOutput>
#include <QAudioFormat>
#include <QAudioDeviceInfo>
#include <QBuffer>
#include <QIODevice>
#include <QDataStream>
#include <QDateTime>
#include <QQueue>
#include <QMutex>

DCORE_USE_NAMESPACE
DAI_USE_NAMESPACE

class StreamingTTSDemo : public QObject
{
    Q_OBJECT
public:
    explicit StreamingTTSDemo(QObject *parent = nullptr);
    ~StreamingTTSDemo();

    void startStreamingSynthesis(const QString &text);

private Q_SLOTS:
    void onSynthesisResult(const QByteArray &audioData);
    void onSynthesisError(int errorCode, const QString &errorMessage);
    void onSynthesisCompleted(const QByteArray &finalAudio);
    void onAudioStateChanged(QAudio::State state);
    void onAudioNotify();
    void playContinuousAudio();


private:
    void setupAudioOutput();
    void playAudioChunk(const QByteArray &audioData);
    void saveAudioToFile(const QByteArray &audioData, const QString &filename);

private:
    DTextToSpeech *tts = nullptr;
    QAudioOutput *audioOutput = nullptr;
    QIODevice *audioDevice = nullptr;
    QBuffer *audioBuffer = nullptr;
    
    // Audio format settings (matching TTS output)
    QAudioFormat audioFormat;
    
    // Streaming state
    bool isStreaming = false;
    QByteArray accumulatedAudio;
    QString currentText;

    // Audio buffering for continuous playback
    QQueue<QByteArray> audioBufferQueue;
    QMutex bufferMutex;
    QTimer *playbackTimer = nullptr;
    bool isPlaying = false;
    static const int BUFFER_THRESHOLD = 4; // Number of chunks to buffer before playing
    QByteArray continuousAudioBuffer; // Combined audio buffer for smooth playback
    
    // File output
    QFile outputFile;
    QString outputFilename;
};

StreamingTTSDemo::StreamingTTSDemo(QObject *parent)
    : QObject(parent)
{
    qInfo() << "Creating StreamingTTSDemo...";
    
    // Create TTS instance
    tts = new DTextToSpeech(this);
    
    // Connect TTS signals
    connect(tts, &DTextToSpeech::synthesisResult, 
            this, &StreamingTTSDemo::onSynthesisResult);
    connect(tts, &DTextToSpeech::synthesisError, 
            this, &StreamingTTSDemo::onSynthesisError);
    connect(tts, &DTextToSpeech::synthesisCompleted, 
            this, &StreamingTTSDemo::onSynthesisCompleted);
    
    // Setup audio format (16kHz, 16-bit, mono - matching TTS output)
    audioFormat.setSampleRate(16000);
    audioFormat.setChannelCount(1);
    audioFormat.setSampleSize(16);
    audioFormat.setCodec("audio/pcm");
    audioFormat.setByteOrder(QAudioFormat::LittleEndian);
    audioFormat.setSampleType(QAudioFormat::SignedInt);
    
    // Initialize playback timer
    playbackTimer = new QTimer(this);
    playbackTimer->setSingleShot(true);
    connect(playbackTimer, &QTimer::timeout, this, &StreamingTTSDemo::playContinuousAudio);
    
    qInfo() << "StreamingTTSDemo created successfully";
}

StreamingTTSDemo::~StreamingTTSDemo()
{
    qInfo() << "Destroying StreamingTTSDemo...";
    
    if (audioOutput) {
        audioOutput->stop();
        delete audioOutput;
    }
    
    if (audioBuffer) {
        delete audioBuffer;
    }
    
    if (outputFile.isOpen()) {
        outputFile.close();
    }
    
    qInfo() << "StreamingTTSDemo destroyed";
}

void StreamingTTSDemo::setupAudioOutput()
{
    if (audioOutput) {
        audioOutput->stop();
        delete audioOutput;
        audioOutput = nullptr;
    }
    
    if (audioBuffer) {
        delete audioBuffer;
        audioBuffer = nullptr;
    }
    
    // Create audio output
    audioOutput = new QAudioOutput(audioFormat, this);
    
    // Create buffer for audio data
    audioBuffer = new QBuffer(this);
    audioBuffer->open(QIODevice::ReadWrite);
    
    // Connect audio signals
    connect(audioOutput, &QAudioOutput::stateChanged, 
            this, &StreamingTTSDemo::onAudioStateChanged);
    connect(audioOutput, &QAudioOutput::notify, 
            this, &StreamingTTSDemo::onAudioNotify);
    
    qInfo() << "Audio output setup completed";
    qInfo() << "Audio format:" << audioFormat.sampleRate() << "Hz," 
            << audioFormat.channelCount() << "channels," 
            << audioFormat.sampleSize() << "bits";
}

void StreamingTTSDemo::startStreamingSynthesis(const QString &text)
{
    if (isStreaming) {
        qWarning() << "Already streaming, please wait for completion";
        return;
    }
    
    qInfo() << "Starting streaming synthesis for text:" << text.left(50) << "...";
    
    // Reset state
    isStreaming = true;
    accumulatedAudio.clear();
    currentText = text;
    
    // Reset audio buffer state
    QMutexLocker locker(&bufferMutex);
    audioBufferQueue.clear();
    isPlaying = false;
    if (playbackTimer) {
        playbackTimer->stop();
    }
    
    // Setup audio output
    setupAudioOutput();
    
    // Prepare output file
    outputFilename = QString("streaming_tts_output_%1.pcm")
                    .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
    outputFile.setFileName(outputFilename);
    if (!outputFile.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open output file:" << outputFilename;
    } else {
        qInfo() << "Output file opened:" << outputFilename;
    }
    
    // Start streaming synthesis
    if (!tts->startStreamSynthesis(text)) {
        qCritical() << "Failed to start stream synthesis:" << tts->lastError().getErrorMessage();
        isStreaming = false;
        return;
    }
    
    qInfo() << "Stream synthesis started successfully";
}

void StreamingTTSDemo::onSynthesisResult(const QByteArray &audioData)
{
    if (!isStreaming) return;
    
    qInfo() << "Received synthesis result, audio data size:" << audioData.size();
    
    // Accumulate audio data
    accumulatedAudio.append(audioData);
    
    // Save to file
    if (outputFile.isOpen()) {
        outputFile.write(audioData);
        outputFile.flush();
    }
    
    // Play audio chunk
    playAudioChunk(audioData);
}

void StreamingTTSDemo::onSynthesisError(int errorCode, const QString &errorMessage)
{
    qCritical() << "Synthesis error:" << errorCode << errorMessage;
    isStreaming = false;
    
    if (outputFile.isOpen()) {
        outputFile.close();
    }
    
    // Stop audio output
    if (audioOutput && audioOutput->state() != QAudio::StoppedState) {
        audioOutput->stop();
    }
}

void StreamingTTSDemo::onSynthesisCompleted(const QByteArray &finalAudio)
{
    qInfo() << "Synthesis completed, final audio data size:" << finalAudio.size();
    qInfo() << "Total accumulated audio size:" << accumulatedAudio.size();
    
    isStreaming = false;
    
    // Close output file
    if (outputFile.isOpen()) {
        outputFile.close();
        qInfo() << "Audio saved to file:" << outputFilename;
    }
    
    // Convert to WAV for easier playback
    QString wavFilename = outputFilename.replace(".pcm", ".wav");
    QFile wavFile(wavFilename);
    if (wavFile.open(QIODevice::WriteOnly)) {
        // Write WAV header
        QDataStream wavStream(&wavFile);
        wavStream.setByteOrder(QDataStream::LittleEndian);
        
        // WAV header
        wavStream.writeRawData("RIFF", 4);
        wavStream << quint32(36 + accumulatedAudio.size()); // File size - 8
        wavStream.writeRawData("WAVE", 4);
        
        // fmt chunk
        wavStream.writeRawData("fmt ", 4);
        wavStream << quint32(16); // fmt chunk size
        wavStream << quint16(1);  // PCM format
        wavStream << quint16(1);  // Mono
        wavStream << quint32(16000); // Sample rate
        wavStream << quint32(16000 * 2); // Byte rate (sample rate * channels * bits per sample / 8)
        wavStream << quint16(2);   // Block align
        wavStream << quint16(16);  // Bits per sample
        
        // data chunk
        wavStream.writeRawData("data", 4);
        wavStream << quint32(accumulatedAudio.size());
        wavStream.writeRawData(accumulatedAudio.data(), accumulatedAudio.size());
        
        wavFile.close();
        qInfo() << "WAV file created:" << wavFilename;
    }
    
    // Stop audio output after a delay to ensure current chunk is played
    QTimer::singleShot(1000, [this]() {
        if (audioOutput && audioOutput->state() != QAudio::StoppedState) {
            audioOutput->stop();
        }
    });
}

void StreamingTTSDemo::playAudioChunk(const QByteArray &audioData)
{
    bool shouldStartPlayback = false;
    
    {
        QMutexLocker locker(&bufferMutex);
        
        // Add audio data to buffer queue
        audioBufferQueue.enqueue(audioData);
        
        qDebug() << "Added audio chunk to buffer, size:" << audioData.size() 
                 << "queue size:" << audioBufferQueue.size();
        
        // Check if we should start playback
        shouldStartPlayback = (audioBufferQueue.size() >= BUFFER_THRESHOLD && !isPlaying);
    }
    
    // Start playback if needed (without holding lock)
    if (shouldStartPlayback) {
        playContinuousAudio();
    }
}

void StreamingTTSDemo::playContinuousAudio()
{
    QByteArray combinedAudio;
    
    // Combine multiple audio chunks into one continuous buffer
    {
        QMutexLocker locker(&bufferMutex);
        if (audioBufferQueue.isEmpty()) {
            isPlaying = false;
            return;
        }
        
        // Combine all buffered chunks
        while (!audioBufferQueue.isEmpty()) {
            combinedAudio.append(audioBufferQueue.dequeue());
        }
        
        isPlaying = true;
    }
    
    if (!audioOutput || !audioBuffer) return;
    
    // Write combined audio data to buffer
    audioBuffer->seek(0);
    audioBuffer->write(combinedAudio);
    audioBuffer->seek(0);
    
    // Start playing the combined audio
    audioOutput->start(audioBuffer);
    
    qDebug() << "Playing continuous audio, combined size:" << combinedAudio.size();
    
    // Calculate total duration for the combined audio
    double totalDurationMs = (combinedAudio.size() * 1000.0) / (16000.0 * 1.0 * 2.0);
    
    // Add extra delay to slow down playback (make it 2x slower)
    int delayMs = qMax(static_cast<int>(totalDurationMs), 800);
    
    qDebug() << "Combined audio duration:" << totalDurationMs << "ms, delay:" << delayMs << "ms";
    
    // Schedule next continuous playback
    playbackTimer->start(delayMs);
}

void StreamingTTSDemo::onAudioStateChanged(QAudio::State state)
{
    switch (state) {
    case QAudio::ActiveState:
        qDebug() << "Audio output: Active";
        break;
    case QAudio::SuspendedState:
        qDebug() << "Audio output: Suspended";
        break;
    case QAudio::StoppedState:
        qDebug() << "Audio output: Stopped";
        break;
    case QAudio::IdleState:
        qDebug() << "Audio output: Idle";
        break;
    case QAudio::InterruptedState:
        qDebug() << "Audio output: Interrupted";
        break;
    }
}

void StreamingTTSDemo::onAudioNotify()
{
    // This is called periodically during playback
    // Can be used for progress tracking if needed
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    qInfo() << "=== Streaming TTS Demo ===";
    qInfo() << "This demo demonstrates real-time streaming text-to-speech synthesis";
    qInfo() << "with automatic audio playback as data arrives.";
    
    StreamingTTSDemo demo;
    
    // Test text for streaming synthesis
    QString testText = "这是一个流式语音合成演示程序。"
                      "它可以实时接收音频数据并自动播放，"
                      "同时将音频保存到文件中。"
                      "This is a streaming text-to-speech demo. "
                      "It can receive audio data in real-time and play automatically, "
                      "while also saving the audio to a file.";
    
    qInfo() << "Starting streaming synthesis...";
    demo.startStreamingSynthesis(testText);
    
    // Set up a timer to check completion
    QTimer *checkTimer = new QTimer(&app);
    QObject::connect(checkTimer, &QTimer::timeout, [&app, &demo]() {
        // This timer can be used to check if synthesis is complete
        // For now, we'll just let the signals handle everything
    });
    checkTimer->start(1000); // Check every second
    
    return app.exec();
}

#include "texttospeech_streaming_demo.moc"

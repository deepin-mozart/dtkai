// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <DChatCompletions>
#include <DModelManager>

#include <QCoreApplication>
#include <QDebug>
#include <QTimer>

DCORE_USE_NAMESPACE
DAI_USE_NAMESPACE

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // Query supported capabilities
    qDebug() << "=== Supported Capabilities ===";
    auto capabilities = DModelManager::supportedCapabilities();
    qDebug() << "Supported capabilities:" << capabilities;

    // Query available models for Chat
    qDebug() << "\n=== Available Chat Models ===";
    auto textChatModels = DModelManager::availableModels("Chat");
    for (const auto &model : textChatModels) {
        qDebug() << "Model:" << model.modelName 
                 << "Provider:" << model.provider
                 << "Capability:" << model.capability;
    }

    // Check if Chat capability is available
    bool textChatAvailable = DModelManager::isCapabilityAvailable("Chat");
    if (!textChatAvailable) {
        qWarning() << "Chat capability is not available, exiting...";
        return -1;
    }

    // Create chat completions instance
    DChatCompletions chat;
    
    // Example 1: Simple chat with default model
    qDebug() << "\n=== Example 1: Default Chat ===";
    QString response1 = chat.chat("你好，请介绍一下自己");
    qDebug() << "Response:" << response1;

    // Example 2: Chat with specific model (use first available model)
    qDebug() << "\n=== Example 2: Chat with Specific Model ===";
    QVariantHash params2;
    if (!textChatModels.isEmpty()) {
        QString modelName = textChatModels.first().modelName;
        params2["model"] = modelName;
        qDebug() << "Using model:" << modelName;
        
        // Use parameters from model configuration
        auto modelInfo = DModelManager::modelInfo(modelName);
        if (!modelInfo.parameters.isEmpty()) {
            qDebug() << "Model default parameters:" << modelInfo.parameters;
            // Use model's default temperature if available, otherwise set our own
            if (!modelInfo.parameters.contains("temperature")) {
                params2["temperature"] = 0.8;
            }
            if (!modelInfo.parameters.contains("max_tokens")) {
                params2["max_tokens"] = 1000;
            }
        } else {
            params2["temperature"] = 0.8;
            params2["max_tokens"] = 1000;
        }
    }
    
    QString response2 = chat.chat("写一首关于春天的诗", {}, params2);
    qDebug() << "Response:" << response2;

    // Example 3: Chat with history
    qDebug() << "\n=== Example 3: Chat with History ===";
    QList<ChatHistory> history;
    ChatHistory userMsg;
    userMsg.role = kChatRoleUser;
    userMsg.content = "我的名字是小明";
    history.append(userMsg);
    
    ChatHistory assistantMsg;
    assistantMsg.role = kChatRoleAssistant;
    assistantMsg.content = "你好小明，很高兴认识你！";
    history.append(assistantMsg);
    
    QVariantHash params3;
    if (!textChatModels.isEmpty()) {
        params3["model"] = textChatModels.first().modelName;
        params3["temperature"] = 0.7;
    }
    
    QString response3 = chat.chat("请记住我的名字，然后告诉我今天天气如何", history, params3);
    qDebug() << "Response:" << response3;

    // Example 4: Streaming chat
    qDebug() << "\n=== Example 4: Streaming Chat ===";
    QObject::connect(&chat, &DChatCompletions::streamOutput, [](const QString &content) {
        qDebug() << "Stream:" << content;
    });
    
    QObject::connect(&chat, &DChatCompletions::streamFinished, [&app](int error) {
        qDebug() << "Stream finished with error code:" << error;
        app.quit();
    });
    
    QVariantHash params4;
    if (!textChatModels.isEmpty()) {
        params4["model"] = textChatModels.first().modelName;
        params4["temperature"] = 0.9;
        params4["stream"] = true;
    }
    
    bool success = chat.chatStream("请用流式方式讲一个有趣的故事", {}, params4);
    if (success) {
        qDebug() << "Streaming chat started...";
    } else {
        qDebug() << "Failed to start streaming chat";
        app.quit();
    }

    // Example 5: Query specific model information
    qDebug() << "\n=== Example 5: Model Information ===";
    if (!textChatModels.isEmpty()) {
        QString testModelName = textChatModels.first().modelName;
        auto modelInfo = DModelManager::modelInfo(testModelName);
        if (!modelInfo.modelName.isEmpty()) {
            qDebug() << "Model:" << modelInfo.modelName;
            qDebug() << "Provider:" << modelInfo.provider;
            qDebug() << "Description:" << modelInfo.description;
            qDebug() << "Capability:" << modelInfo.capability;
            qDebug() << "Deploy Type:" << (modelInfo.deployType == DeployType::Local ? "Local" : 
                                         modelInfo.deployType == DeployType::Cloud ? "Cloud" : "Custom");
            qDebug() << "Available:" << modelInfo.isAvailable;
            qDebug() << "Parameters:" << modelInfo.parameters;
        } else {
            qDebug() << "Model info not found for:" << testModelName;
        }
    }

    return app.exec();
}

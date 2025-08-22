// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <DModelManager>

#include <QCoreApplication>
#include <QDebug>

DAI_USE_NAMESPACE

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    qDebug() << "=== DModelManager Test ===";
    
    // Test 1: Get supported capabilities
    qDebug() << "\n1. Testing supportedCapabilities():";
    auto capabilities = DModelManager::supportedCapabilities();
    qDebug() << "Supported capabilities:" << capabilities;
    
    // Test 2: Check if Chat is available
    qDebug() << "\n2. Testing isCapabilityAvailable():";
    bool textChatAvailable = DModelManager::isCapabilityAvailable("Chat");
    qDebug() << "Chat available:" << textChatAvailable;
    
    // Test 3: Get all available models
    qDebug() << "\n3. Testing availableModels():";
    auto allModels = DModelManager::availableModels();
    qDebug() << "Total models available:" << allModels.size();
    
    for (const auto &model : allModels) {
        qDebug() << "  Model:" << model.modelName 
                 << "Provider:" << model.provider
                 << "Capability:" << model.capability
                 << "Deploy:" << (model.deployType == DeployType::Local ? "Local" : 
                               model.deployType == DeployType::Cloud ? "Cloud" : "Custom")
                 << "Available:" << model.isAvailable;
    }
    
    // Test 4: Get models for specific capability
    if (!capabilities.isEmpty()) {
        QString testCapability = capabilities.first();
        qDebug() << "\n4. Testing availableModels() for capability:" << testCapability;
        auto capabilityModels = DModelManager::availableModels(testCapability);
        qDebug() << "Models for" << testCapability << ":" << capabilityModels.size();
        
        for (const auto &model : capabilityModels) {
            qDebug() << "  Model:" << model.modelName << "Parameters:" << model.parameters.keys();
        }
    }
    
    // Test 5: Get specific model info
    if (!allModels.isEmpty()) {
        QString testModelName = allModels.first().modelName;
        qDebug() << "\n5. Testing modelInfo() for model:" << testModelName;
        auto modelInfo = DModelManager::modelInfo(testModelName);
        
        if (!modelInfo.modelName.isEmpty()) {
            qDebug() << "Model found:" << modelInfo.modelName;
            qDebug() << "Description:" << modelInfo.description;
            qDebug() << "Parameters:" << modelInfo.parameters;
        } else {
            qDebug() << "Model not found";
        }
    }
    
    qDebug() << "\n=== Test Complete ===";
    
    return 0;
} 
// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DOCRRECOGNITION_H
#define DOCRRECOGNITION_H

#include "dtkai_global.h"
#include "derror.h"

#include <QObject>
#include <QVariantHash>
#include <QRect>

DAI_BEGIN_NAMESPACE

class DOCRRecognitionPrivate;
class DOCRRecognition : public QObject
{
    Q_OBJECT
    
public:
    explicit DOCRRecognition(QObject *parent = nullptr);
    ~DOCRRecognition();
    
    // Synchronous OCR methods
    QString recognizeFile(const QString &imageFile, const QVariantHash &params = {});
    QString recognizeImage(const QByteArray &imageData, const QVariantHash &params = {});
    QString recognizeRegion(const QString &imageFile, const QString &region, const QVariantHash &params = {});
    QString recognizeRegion(const QString &imageFile, const QRect &region, const QVariantHash &params = {});
    
    // Information query methods
    QStringList getSupportedLanguages();
    QStringList getSupportedFormats();
    QString getCapabilities();
    
    // Control methods  
    void terminate();
    
    // Error handling
    DTK_CORE_NAMESPACE::DError lastError() const;
    
private:
    QScopedPointer<DOCRRecognitionPrivate> d;
};

DAI_END_NAMESPACE

#endif // DOCRRECOGNITION_H

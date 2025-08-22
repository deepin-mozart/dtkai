// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DIMAGERECOGNITION_P_H
#define DIMAGERECOGNITION_P_H

#include "dimagerecognition.h"
#include "aidaemon_apisession_imagerecognition.h"

DAI_BEGIN_NAMESPACE

class DImageRecognitionPrivate : public QObject
{
    Q_OBJECT
public:
    explicit DImageRecognitionPrivate(DImageRecognition *q);
    ~DImageRecognitionPrivate();
    bool ensureServer();
    static QString packageParams(const QVariantHash &params);
    
public Q_SLOTS:
    void onRecognitionResult(const QString &sessionId, const QString &result);
    void onRecognitionError(const QString &sessionId, int errorCode, const QString &errorMessage);
    void onRecognitionCompleted(const QString &sessionId, const QString &finalResult);
    
public:
    QMutex mtx;
    bool running = false;
    DTK_CORE_NAMESPACE::DError error;
    QScopedPointer<OrgDeepinAiDaemonSessionImageRecognitionInterface> imageIfs;
    QString sessionId;
    
public:
    DImageRecognition *q = nullptr;
};

DAI_END_NAMESPACE

#endif // DIMAGERECOGNITION_P_H


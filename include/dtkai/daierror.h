// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DTKAIERROR_H
#define DTKAIERROR_H

#include <QString>

#include "dtkai_global.h"

DAI_BEGIN_NAMESPACE

enum AIErrorCode {
    NoError = 0,
    APIServerNotAvailable = 1,
    InvalidParameter = 2,
    ParseError = 3
};

DAI_END_NAMESPACE

#endif // DTKAIERROR_H

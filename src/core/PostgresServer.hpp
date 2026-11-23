// SPDX-FileCopyrightText: 2026 Oscar Mauricio Bonil P.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>

class PostgresServer {
public:
    static QString ensureRunning(const QString& applicationDirectory,
                                 const QString& dataDirectory,
                                 const QString& binaryDirectory,
                                 const QString& port,
                                 const QString& listenAddresses,
                                 const QString& allowedCidrs);

    static bool stop(const QString& applicationDirectory,
                     const QString& dataDirectory,
                     const QString& binaryDirectory,
                     QString* errorMessage = nullptr);

    static QString findProgram(const QString& name,
                               const QString& applicationDirectory,
                               const QString& binaryDirectory);
};

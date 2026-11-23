// SPDX-FileCopyrightText: 2026 Oscar Mauricio Bonil P.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "PostgresDatabase.hpp"

#include <QString>

#include <memory>

class DatabaseManager {
public:
    static DatabaseManager& instance();

    bool initialize(const QString& applicationDirectory);
    Db::Database& getDb();
    QString lastError() const;
    bool isServerMode() const;
    bool stopLocalServer(QString* errorMessage = nullptr) const;
    bool createBackup(const QString& destination, QString* errorMessage = nullptr) const;

    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

private:
    DatabaseManager() = default;
    ~DatabaseManager() = default;

    void runSchemaFromResources();

    std::unique_ptr<Db::Database> m_db;
    QString m_applicationDirectory;
    QString m_binaryDirectory;
    QString m_host;
    QString m_port;
    QString m_databaseName;
    QString m_username;
    QString m_password;
    QString m_sslMode;
    QString m_lastError;
    bool m_serverMode = false;
};

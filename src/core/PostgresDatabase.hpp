// SPDX-FileCopyrightText: 2026 Oscar Mauricio Bonil P.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <pqxx/pqxx>

#include <cstdint>
#include <iomanip>
#include <limits>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace Db {

class Exception : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class Column {
public:
    explicit Column(std::optional<std::string> value);

    bool isNull() const;
    std::string getString() const;
    int getInt() const;
    std::int64_t getInt64() const;
    double getDouble() const;

private:
    std::optional<std::string> m_value;
};

class Database {
public:
    explicit Database(const std::string& connectionString);

    void exec(const std::string& sql);
    Column execAndGet(const std::string& sql);
    std::int64_t getLastInsertRowid();
    int getChanges() const;
    const std::string& connectionString() const;

private:
    friend class Statement;
    friend class Transaction;

    pqxx::result execute(const std::string& sql);
    std::string quote(const std::string& value) const;

    pqxx::connection m_connection;
    pqxx::transaction_base* m_activeTransaction = nullptr;
    std::string m_connectionString;
    int m_changes = 0;
};

class Transaction {
public:
    explicit Transaction(Database& database);
    ~Transaction();

    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;

    void commit();
    void rollback();

private:
    Database& m_database;
    std::unique_ptr<pqxx::work> m_transaction;
};

class Statement {
public:
    Statement(Database& database, std::string sql);

    template <typename T>
    void bind(int index, const T& value) {
        if (index < 1) {
            throw std::invalid_argument("El índice de parámetro debe ser mayor que cero.");
        }

        if (m_parameters.size() < static_cast<std::size_t>(index)) {
            m_parameters.resize(static_cast<std::size_t>(index));
        }
        m_parameters[static_cast<std::size_t>(index - 1)] = stringify(value);
        m_executed = false;
        m_hasCurrentRow = false;
    }

    void bind(int index) {
        bindNull(index);
    }

    void bindNull(int index);
    void exec();
    bool executeStep();
    void reset();
    Column getColumn(int index) const;

private:
    template <typename T>
    static std::string stringify(const T& value) {
        if constexpr (std::is_same_v<std::remove_cvref_t<T>, std::string>) {
            return value;
        } else if constexpr (std::is_convertible_v<T, const char*>) {
            return std::string(value);
        } else if constexpr (std::is_integral_v<T>) {
            return std::to_string(value);
        } else if constexpr (std::is_floating_point_v<T>) {
            std::ostringstream stream;
            stream << std::setprecision(std::numeric_limits<T>::max_digits10) << value;
            return stream.str();
        } else {
            static_assert(std::is_same_v<T, void>, "Tipo de parámetro no soportado por Db::Statement.");
        }
    }

    std::string parameterizedSql() const;

    Database& m_database;
    std::string m_sql;
    std::vector<std::optional<std::string>> m_parameters;
    pqxx::result m_result;
    std::size_t m_row = 0;
    bool m_executed = false;
    bool m_hasCurrentRow = false;
};

}  // namespace Db

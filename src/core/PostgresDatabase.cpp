// SPDX-FileCopyrightText: 2026 Oscar Mauricio Bonil P.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "PostgresDatabase.hpp"

#include <charconv>
#include <stdexcept>
#include <string_view>

namespace Db {

Column::Column(std::optional<std::string> value) : m_value(std::move(value)) {}

bool Column::isNull() const {
    return !m_value.has_value();
}

std::string Column::getString() const {
    return m_value.value_or("");
}

int Column::getInt() const {
    return m_value ? std::stoi(*m_value) : 0;
}

std::int64_t Column::getInt64() const {
    return m_value ? std::stoll(*m_value) : 0;
}

double Column::getDouble() const {
    return m_value ? std::stod(*m_value) : 0.0;
}

Database::Database(const std::string& connectionString)
    : m_connection(connectionString), m_connectionString(connectionString) {}

void Database::exec(const std::string& sql) {
    execute(sql);
}

Column Database::execAndGet(const std::string& sql) {
    const pqxx::result result = execute(sql);
    if (result.empty() || result.columns() == 0) {
        throw Exception("La consulta no devolvió ningún valor.");
    }

    const auto field = result.front().front();
    if (field.is_null()) {
        return Column(std::nullopt);
    }
    return Column(std::string(field.c_str(), field.size()));
}

std::int64_t Database::getLastInsertRowid() {
    return execAndGet("SELECT LASTVAL()").getInt64();
}

int Database::getChanges() const {
    return m_changes;
}

const std::string& Database::connectionString() const {
    return m_connectionString;
}

pqxx::result Database::execute(const std::string& sql) {
    try {
        pqxx::result result;
        if (m_activeTransaction) {
            result = m_activeTransaction->exec(sql);
        } else {
            pqxx::work transaction(m_connection);
            result = transaction.exec(sql);
            transaction.commit();
        }

        m_changes = static_cast<int>(result.affected_rows());
        return result;
    } catch (const pqxx::failure& error) {
        throw Exception(error.what());
    }
}

std::string Database::quote(const std::string& value) const {
    try {
        return m_connection.quote(value);
    } catch (const pqxx::failure& error) {
        throw Exception(error.what());
    }
}

Transaction::Transaction(Database& database) : m_database(database) {
    if (m_database.m_activeTransaction) {
        throw Exception("No se permiten transacciones anidadas.");
    }

    try {
        m_transaction = std::make_unique<pqxx::work>(m_database.m_connection);
        m_database.m_activeTransaction = m_transaction.get();
    } catch (const pqxx::failure& error) {
        throw Exception(error.what());
    }
}

Transaction::~Transaction() {
    m_database.m_activeTransaction = nullptr;
}

void Transaction::commit() {
    if (!m_transaction) {
        throw Exception("La transacción ya fue finalizada.");
    }

    try {
        m_transaction->commit();
        m_database.m_activeTransaction = nullptr;
        m_transaction.reset();
    } catch (const pqxx::failure& error) {
        throw Exception(error.what());
    }
}

void Transaction::rollback() {
    if (!m_transaction) {
        return;
    }

    try {
        m_transaction->abort();
        m_database.m_activeTransaction = nullptr;
        m_transaction.reset();
    } catch (const pqxx::failure& error) {
        throw Exception(error.what());
    }
}

Statement::Statement(Database& database, std::string sql)
    : m_database(database), m_sql(std::move(sql)) {}

void Statement::bindNull(int index) {
    if (index < 1) {
        throw std::invalid_argument("El índice de parámetro debe ser mayor que cero.");
    }

    if (m_parameters.size() < static_cast<std::size_t>(index)) {
        m_parameters.resize(static_cast<std::size_t>(index));
    }
    m_parameters[static_cast<std::size_t>(index - 1)] = std::nullopt;
    m_executed = false;
    m_hasCurrentRow = false;
}

void Statement::exec() {
    m_result = m_database.execute(parameterizedSql());
    m_row = 0;
    m_executed = true;
    m_hasCurrentRow = false;
}

bool Statement::executeStep() {
    if (!m_executed) {
        m_result = m_database.execute(parameterizedSql());
        m_row = 0;
        m_executed = true;
    } else if (m_hasCurrentRow) {
        ++m_row;
    }

    if (m_row >= m_result.size()) {
        m_hasCurrentRow = false;
        return false;
    }

    m_hasCurrentRow = true;
    return true;
}

void Statement::reset() {
    m_result = pqxx::result{};
    m_row = 0;
    m_executed = false;
    m_hasCurrentRow = false;
}

Column Statement::getColumn(int index) const {
    if (!m_hasCurrentRow || index < 0
        || static_cast<std::size_t>(index) >= m_result[m_row].size()) {
        throw std::out_of_range("La columna solicitada no está disponible.");
    }

    const auto field = m_result[m_row][static_cast<std::size_t>(index)];
    if (field.is_null()) {
        return Column(std::nullopt);
    }
    return Column(std::string(field.c_str(), field.size()));
}

std::string Statement::parameterizedSql() const {
    std::string query;
    query.reserve(m_sql.size() + m_parameters.size() * 8);

    bool inSingleQuote = false;
    bool inDoubleQuote = false;
    std::size_t parameterIndex = 0;

    for (std::size_t i = 0; i < m_sql.size(); ++i) {
        const char current = m_sql[i];

        if (current == '\'' && !inDoubleQuote) {
            query += current;
            if (inSingleQuote && i + 1 < m_sql.size() && m_sql[i + 1] == '\'') {
                query += m_sql[++i];
                continue;
            }
            inSingleQuote = !inSingleQuote;
            continue;
        }
        if (current == '"' && !inSingleQuote) {
            inDoubleQuote = !inDoubleQuote;
            query += current;
            continue;
        }

        if (current == '?' && !inSingleQuote && !inDoubleQuote) {
            if (parameterIndex >= m_parameters.size()) {
                throw Exception("Falta un parámetro para la consulta PostgreSQL.");
            }
            const auto& parameter = m_parameters[parameterIndex++];
            query += parameter ? m_database.quote(*parameter) : "NULL";
        } else {
            query += current;
        }
    }

    if (parameterIndex != m_parameters.size()) {
        throw Exception("La consulta PostgreSQL recibió parámetros sin marcador.");
    }
    return query;
}

}  // namespace Db

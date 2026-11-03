/**
 * @file MqttDatabase.cpp
 * @brief Implementation of MQTT offline message database
 * 
 * Adapted from taximeter SqlDatabase - simplified for MQTT use
 */

#include "SqlDatabase.hpp"
#include "main.hpp"  // For log_message
#include <time.h>
#include <cstring>

// ============================================================================
// Constructor / Destructor
// ============================================================================

MqttDatabase::MqttDatabase(const std::string& db_name, const std::string& table_name)
    : db_(nullptr), db_name_(db_name), table_name_(table_name)
{
    if (initDatabase() != SQLITE_OK)
    {
        log_message(LOG_ERROR, "Failed to initialize MQTT database");
        if (db_)
        {
            sqlite3_close(db_);
            db_ = nullptr;
        }
    }
    else
    {
        log_message(LOG_INFO, "MQTT database initialized: %s", db_name.c_str());
    }
}

MqttDatabase::~MqttDatabase()
{
    if (db_)
    {
        sqlite3_close(db_);
        log_message(LOG_INFO, "MQTT database closed");
    }
}

// ============================================================================
// Private Methods
// ============================================================================

int MqttDatabase::initDatabase()
{
    // Open database
    int rc = sqlite3_open(db_name_.c_str(), &db_);
    if (rc != SQLITE_OK)
    {
        log_message(LOG_ERROR, "initDatabase: Cannot open database '%s': %s", db_name_.c_str(), sqlite3_errmsg(db_));
        return rc;
    }

    // Create table if not exists
    // Simple structure: id, data (JSON), timestamp
    std::string create_table_sql =
        "CREATE TABLE IF NOT EXISTS " + table_name_ + " ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "data TEXT NOT NULL, "
        "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP);";

    char* err_msg = nullptr;
    rc = sqlite3_exec(db_, create_table_sql.c_str(), nullptr, nullptr, &err_msg);
    if (rc != SQLITE_OK)
    {
        log_message(LOG_ERROR, "initDatabase: Failed to create table '%s': %s", table_name_.c_str(), err_msg);
        sqlite3_free(err_msg);
        return rc;
    }

    return SQLITE_OK;
}

// ============================================================================
// Public Methods
// ============================================================================

bool MqttDatabase::isOpen() const
{
    return (db_ != nullptr);
}

bool MqttDatabase::saveMessage(const std::string& json_data)
{
    if (!db_)
    {
        log_message(LOG_ERROR, "saveMessage: Database not open");
        return false;
    }

    std::string insert_sql = "INSERT INTO " + table_name_ + " (data) VALUES (?);";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, insert_sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        log_message(LOG_ERROR, "saveMessage: Failed to prepare INSERT statement: %s", sqlite3_errmsg(db_));
        return false;
    }

    // Bind the JSON data
    sqlite3_bind_text(stmt, 1, json_data.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE)
    {
        log_message(LOG_ERROR, "saveMessage: Failed to insert message: %s", sqlite3_errmsg(db_));
        return false;
    }

    log_message(LOG_DEBUG, "saveMessage: Message saved to database (ID: %lld)", sqlite3_last_insert_rowid(db_));
    return true;
}

int MqttDatabase::fetchPendingMessages(int limit, std::vector<MessageRecord>& messages)
{
    if (!db_)
    {
        log_message(LOG_ERROR, "fetchPendingMessages: Database not open");
        return SQLITE_ERROR;
    }

    messages.clear();

    // FIFO: ORDER BY id ASC (oldest first)
    std::string select_sql = "SELECT id, data, timestamp FROM " + table_name_ + " ORDER BY id ASC";
    if (limit > 0)
    {
        select_sql += " LIMIT ?;";
    }
    else
    {
        select_sql += ";";
    }

    sqlite3_stmt* stmt = nullptr;
    sqlite3_busy_timeout(db_, SQL_OPERATION_TIMEOUT_MS);

    int rc = sqlite3_prepare_v2(db_, select_sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        log_message(LOG_ERROR, "fetchPendingMessages: Failed to prepare SELECT statement: %s", sqlite3_errmsg(db_));
        return rc;
    }

    // Bind limit if specified
    if (limit > 0)
    {
        sqlite3_bind_int(stmt, 1, limit);
    }

    time_t start_time = time(nullptr);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        // Check timeout
        if (difftime(time(nullptr), start_time) > (SQL_OPERATION_TIMEOUT_MS / 1000))
        {
            log_message(LOG_ERROR, "fetchPendingMessages: Query timeout reached after %d ms", SQL_OPERATION_TIMEOUT_MS);
            rc = SQLITE_ERROR;
            break;
        }

        MessageRecord record;
        record.database_id = sqlite3_column_int(stmt, 0);

        const unsigned char* data = sqlite3_column_text(stmt, 1);
        if (data)
        {
            record.json_data = reinterpret_cast<const char*>(data);
        }

        const unsigned char* timestamp = sqlite3_column_text(stmt, 2);
        if (timestamp)
        {
            record.timestamp = reinterpret_cast<const char*>(timestamp);
        }

        messages.push_back(record);
    }

    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE && rc != SQLITE_OK)
    {
        log_message(LOG_ERROR, "fetchPendingMessages: Error fetching messages: %s", sqlite3_errmsg(db_));
        return rc;
    }

    return SQLITE_OK;
}

int MqttDatabase::fetchBatchData(int limit, std::vector<int>& ids, std::vector<std::string>& json_data)
{
    std::vector<MessageRecord> messages;
    int rc = fetchPendingMessages(limit, messages);
    
    if (rc != SQLITE_OK)
    {
        return rc;
    }

    ids.clear();
    json_data.clear();

    for (const MessageRecord& msg : messages)
    {
        ids.push_back(msg.database_id);
        json_data.push_back(msg.json_data);
    }

    return SQLITE_OK;
}

int MqttDatabase::deleteSingleRow(int id)
{
    if (!db_)
    {
        log_message(LOG_ERROR, "deleteSingleRow: Database not open");
        return SQLITE_ERROR;
    }

    std::string delete_sql = "DELETE FROM " + table_name_ + " WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, delete_sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        log_message(LOG_ERROR, "deleteSingleRow: Failed to prepare DELETE statement: %s", sqlite3_errmsg(db_));
        return rc;
    }

    sqlite3_bind_int(stmt, 1, id);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE)
    {
        log_message(LOG_ERROR, "deleteSingleRow: Failed to delete row ID %d: %s", id, sqlite3_errmsg(db_));
        return rc;
    }

    return SQLITE_OK;
}

int MqttDatabase::countPendingMessages()
{
    if (!db_)
    {
        log_message(LOG_ERROR, "countPendingMessages: Database not open");
        return -1;
    }

    std::string count_sql = "SELECT COUNT(*) FROM " + table_name_ + ";";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, count_sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        log_message(LOG_ERROR, "countPendingMessages: Failed to prepare COUNT statement: %s", sqlite3_errmsg(db_));
        return -1;
    }

    int count = 0;
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        count = sqlite3_column_int(stmt, 0);
    }
    else if (rc != SQLITE_DONE)
    {
        log_message(LOG_ERROR, "countPendingMessages: Failed to execute COUNT query: %s", sqlite3_errmsg(db_));
        sqlite3_finalize(stmt);
        return -1;
    }

    sqlite3_finalize(stmt);
    return count;
}
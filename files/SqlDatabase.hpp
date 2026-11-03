/**
 * @file MqttDatabase.hpp
 * @brief SQLite database management class for offline MQTT message storage
 * 
 * Simplified version adapted from taximeter SqlDatabase
 * Stores only failed MQTT messages for later retry (FIFO)
 */

#pragma once

#include <string>
#include <vector>
#include <sqlite3.h>

#define MAX_ROWS 100 
#define SQL_OPERATION_TIMEOUT_MS 5000  // 5 second timeout

/**
 * @struct MessageRecord
 * @brief Structure to hold MQTT message from database
 */
struct MessageRecord
{
    int database_id;              // Auto-increment ID
    std::string json_data;        // JSON message payload
    std::string timestamp;        // When message was stored
};

/**
 * @class MqttDatabase
 * @brief RAII wrapper for SQLite database operations for MQTT offline storage
 * 
 * This class manages a SQLite database connection and provides methods for
 * storing failed MQTT messages and retrieving them in FIFO order.
 * The database connection is automatically managed through RAII principles.
 */
class MqttDatabase
{
private:
    sqlite3* db_;
    std::string db_name_;
    std::string table_name_;
    
    /**
     * @brief Initialize database and create table if not exists
     * @return SQLITE_OK on success, error code on failure
     */
    int initDatabase();
    
public:
    /**
     * @brief Constructor - opens database connection
     * @param db_name Database file name (default: "offline_mqtt.db")
     * @param table_name Table name (default: "mqtt_messages")
     */
    MqttDatabase(const std::string& db_name = "flespi_offline_crumps.db", 
                 const std::string& table_name = "mqtt_messages");
    
    /**
     * @brief Destructor - closes database connection
     */
    ~MqttDatabase();
    
    // Disable copy constructor and assignment operator (RAII)
    MqttDatabase(const MqttDatabase&) = delete;
    MqttDatabase& operator=(const MqttDatabase&) = delete;
    
    /**
     * @brief Check if database connection is valid
     * @return true if connected, false otherwise
     */
    bool isOpen() const;
    
    /**
     * @brief Save MQTT message to database (for offline storage)
     * @param json_data JSON message string
     * @return true on success, false on failure
     */
    bool saveMessage(const std::string& json_data);
    
    /**
     * @brief Fetch batch of pending messages from database in FIFO order
     * @param limit Maximum number of records to fetch (0 = fetch all)
     * @param messages Output vector for message records
     * @return SQLITE_OK on success, error code on failure
     */
    int fetchPendingMessages(int limit, std::vector<MessageRecord>& messages);
    
    /**
     * @brief Fetch batch of pending messages (simple version - for compatibility)
     * @param limit Maximum number of records to fetch
     * @param ids Output vector for record IDs
     * @param json_data Output vector for JSON strings
     * @return SQLITE_OK on success, error code on failure
     */
    int fetchBatchData(int limit, std::vector<int>& ids, std::vector<std::string>& json_data);
    
    /**
     * @brief Delete a single message by ID
     * @param id Record ID to delete
     * @return SQLITE_OK on success, error code on failure
     */
    int deleteSingleRow(int id);
    
    /**
     * @brief Count pending messages in database
     * @return Number of messages waiting to be sent
     */
    int countPendingMessages();
    
    /**
     * @brief Get the database name
     * @return Database file name
     */
    std::string getDbName() const { return db_name_; }
    
    /**
     * @brief Get the table name
     * @return Table name
     */
    std::string getTableName() const { return table_name_; }
};
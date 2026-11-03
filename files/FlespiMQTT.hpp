/**
 * @file FlespiMQTT.hpp
 * @brief Flespi-specific MQTT client with internal message building AND database management
 * 
 * Encapsulates both MQTT publishing and offline database management
 */

#pragma once

#include "MQTTClientBase.hpp"
#include "IOTISTIC_Messaging_Protocol.hpp"
#include "SqlDatabase.hpp"
#include <memory>
#include <atomic>   
#include <chrono> 
/**
 * @class FlespiMQTT
 * @brief MQTT client for Flespi broker with integrated offline database
 */
class FlespiMQTT : public MQTTClientBase {
public:
    /**
     * @brief Constructor
     * @param broker_address Flespi broker address
     * @param port Broker port
     * @param client_id Client ID (IMEI)
     * @param token Flespi token (used as password)
     * @param publish_topic Topic for publishing messages
     * @param db_filename Database filename (nullptr to disable offline storage)
     * @param use_short_names Use short field names (default: false)
     * @param keepalive Keep-alive interval (default: 60s)
     */
    FlespiMQTT(
        const char* broker_address,
        int port,
        const char* client_id,
        const char* token,
        const char* publish_topic,
        const char* db_filename = "offline_mqtt.db",
        bool use_short_names = false,
        int keepalive = 10,
        short int qos =0
    );

    /**
     * @brief Destructor
     */
    ~FlespiMQTT() override;

    // ========================================================================
    // Message Builders - Build messages internally
    // ========================================================================

    /**
     * @brief Build basic Flespi message (Type: B)
     * @return JSON string (must be freed by caller)
     */
    char* buildBasicMessage();

    /**
     * @brief Build running Flespi message (Type: R)
     * @return JSON string (must be freed by caller)
     */
    char* buildRunningMessage();

    /**
     * @brief Build info Flespi message (Type: I)
     * @return JSON string (must be freed by caller)
     */
    char* buildInfoMessage();

    /**
     * @brief Build event Flespi message (Type: E)
     * @param event Event type string
     * @return JSON string (must be freed by caller)
     */
    char* buildEventMessage(const char* event, cJSON* value);

    // ========================================================================
    // Smart Publish Methods - Auto-retry pending messages first
    // ========================================================================

    /**
     * @brief Smart publish - sends pending messages first, then current message
     * If publish fails and database is configured, message is saved for later retry
     * @return MQTTCLIENT_SUCCESS on success
     */
    int publishBasicMessage();

    /**
     * @brief Smart publish - sends pending messages first, then current message
     * If publish fails and database is configured, message is saved for later retry
     * @return MQTTCLIENT_SUCCESS on success
     */
    int publishRunningMessage();

    /**
     * @brief Smart publish - sends pending messages first, then current message
     * If publish fails and database is configured, message is saved for later retry
     * @return MQTTCLIENT_SUCCESS on success
     */
    int publishInfoMessage();

    /**
     * @brief Smart publish - sends pending messages first, then current message
     * If publish fails and database is configured, message is saved for later retry
     * @return MQTTCLIENT_SUCCESS on success
     */
    int publishEventMessage(const char* event, cJSON* value);

    /**
     * @brief Get number of pending messages in database
     * @return Count of pending messages
     */
    int getPendingMessageCount();

    /**
     * @brief Check if there are pending messages
     * @return true if there are pending messages, false otherwise
     */
    bool hasPendingMessages();

    /**
     * @brief Publish all pending messages from database
     * @return Number of successfully published messages
     */
    int publishPendingMessages();

    /**
     * @brief Ensure client is connected, attempt reconnection if needed
     * @return true if connected, false if offline
     */
    bool ensureConnected();

    /**
     * @brief Save message directly to database (for offline mode)
     * @param message Message to save
     * @return true if saved successfully
     */
    bool saveToDatabase(const char* message);

    // ========================================================================
    // Override Base Class Methods
    // ========================================================================

    /**
     * @brief Handle incoming MQTT message (not used for Flespi)
     * @return 1 on success
     */
    int handleMessage(char* topicName, MQTTClient_message* message) override;

private:
    // ========================================================================
    // Private Members
    // ========================================================================
    
    std::string publish_topic_;     // Topic for publishing
    IMP_attributes attributes_;     // Protocol field names
    std::unique_ptr<MqttDatabase> db_; // Database for offline storage
    std::chrono::steady_clock::time_point last_reconnect_attempt_{};

    // ========================================================================
    // Private Helper Functions
    // ========================================================================

    /**
     * @brief Create common fields shared by all message types
     * @param message_type "B", "R", "I", or "E"
     * @return cJSON object with common fields (must be deleted by caller)
     */
    cJSON* createCommonMessage(const char* message_type);

    /**
     * @brief Add CAN bus attributes to message
     * @param message cJSON message object
     * @return true on success, false on failure
     */
    bool addCANBusAttributes(cJSON* message);

    /**
     * @brief Internal publish method with database fallback
     * @param message Message to publish
     * @return MQTTCLIENT_SUCCESS on success
     */
    int publishWithRetry(const char* message);

    /**
     * @brief Try to publish message, save to DB if fails
     * @param message Message to publish
     * @return true if published successfully, false otherwise
     */
    bool tryPublishAndSaveOnFailure(const char* message);

    bool processPendingMessages();

};


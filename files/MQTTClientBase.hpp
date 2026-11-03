/**
 * @file MQTTClientBase.hpp
 * @brief Base MQTT client class with minimal constructor parameters
 * 
 * Takes only essential connection parameters, not entire config struct
 */

#pragma once

#include <MQTTClient.h>
#include <string>
#include <mutex>
#include <cjson/cJSON.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
/**
 * @class MQTTClientBase
 * @brief Abstract base class for MQTT client implementations
 * 
 * Constructor takes only essential parameters needed for connection
 */
class MQTTClientBase {
public:
    /**
     * @brief Constructor with minimal required parameters
     * @param broker_name Friendly name for logging (e.g., "Flespi", "IOTISTIC")
     * @param broker_address Broker hostname or IP
     * @param port Broker port
     * @param client_id Client ID (typically IMEI)
     * @param username Authentication username (can be nullptr)
     * @param password Authentication password (can be nullptr)
     * @param keepalive Keep-alive interval in seconds
     */
    MQTTClientBase(
        const char* broker_name,
        const char* broker_address,
        int port,
        const char* client_id,
        const char* username = nullptr,
        const char* password = nullptr,
        int keepalive = 60, 
        short int qos =0 
    );
    
    /**
     * @brief Virtual destructor
     */
    virtual ~MQTTClientBase();

    // Delete copy constructor and assignment operator
    MQTTClientBase(const MQTTClientBase&) = delete;
    MQTTClientBase& operator=(const MQTTClientBase&) = delete;

    // ========================================================================
    // Core MQTT Operations
    // ========================================================================

    /**
     * @brief Initialize and connect to MQTT broker
     * @param last_will_topic Last Will topic (nullptr = no LWT)
     * @param last_will_message Last Will message (nullptr = default)
     * @param qos Quality of Service level (default: 0)
     * @return MQTTCLIENT_SUCCESS on success, error code otherwise
     */
    virtual int connect(const char* last_will_topic = nullptr,
                       const char* last_will_message = nullptr,
                       int qos = 0);

    /**
     * @brief Publish a message to a topic
     * @param topic Topic to publish to
     * @param payload Message payload
     * @param qos Quality of Service level (default: 0)
     * @param retained Retain message flag (default: false)
     * @return MQTTCLIENT_SUCCESS on success, error code otherwise
     */
    virtual int publish(const char* topic, const char* payload, 
                       int qos = 0, bool retained = false);

    /**
     * @brief Subscribe to a topic
     * @param topic Topic to subscribe to
     * @param qos Quality of Service level (default: 0)
     * @return MQTTCLIENT_SUCCESS on success, error code otherwise
     */
    virtual int subscribe(const char* topic, int qos = 0);

    /**
     * @brief Reconnect to MQTT broker
     * @param last_will_topic Last Will topic (nullptr = no LWT)
     * @param last_will_message Last Will message (nullptr = default)
     * @param qos Quality of Service level
     * @return MQTTCLIENT_SUCCESS on success, error code otherwise
     */
    virtual int reconnect(const char* last_will_topic = nullptr,
                         const char* last_will_message = nullptr,
                         int qos = 0);

    /**
     * @brief Disconnect from MQTT broker
     */
    virtual void disconnect();

    /**
     * @brief Check if client is connected
     * @return true if connected, false otherwise
     */
    virtual bool isConnected() const;

    /**
     * @brief Get broker name
     * @return Broker name string
     */
    const char* getBrokerName() const { return broker_name_.c_str(); }

    // ========================================================================
    // Message Callback - Must be implemented by derived classes
    // ========================================================================

    /**
     * @brief Handle incoming MQTT message (pure virtual)
     * @param topicName Topic where message was received
     * @param message MQTT message
     * @return 1 on success
     */
    virtual int handleMessage(char* topicName, MQTTClient_message* message) = 0;

    /**
     * @brief Static callback wrapper for Paho MQTT
     */
    static int messageCallbackWrapper(void* context, char* topicName, 
                                      int topicLen, MQTTClient_message* message);

protected:
    // ========================================================================
    // Protected Members - Accessible by derived classes
    // ========================================================================

    MQTTClient client_;                // Paho MQTT client handle
    std::string broker_name_;          // Friendly name for logging
    std::string broker_address_;       // Broker hostname/IP
    int port_;                         // Broker port
    std::string client_id_;            // Client ID
    std::string username_;             // Username (empty if not used)
    std::string password_;             // Password (empty if not used)
    int keepalive_;                    // Keep-alive interval
    bool is_connected_;                // Connection status
    mutable std::mutex mqtt_mutex_;    // Thread safety
    short int qos_; //Quality of Service level
    /**
     * @brief Build default Last Will message (can be overridden)
     * @return Last Will message string
     */
    virtual std::string buildDefaultLastWillMessage();

    /**
     * @brief Log error with detailed message
     */
    void logError(const char* function_name, int error_code) const;

    /**
     * @brief Get human-readable error string
     */
    const char* getErrorString(int error_code) const;
};

inline bool addTruncatedNumber(cJSON* obj, const char* name, double value, int decimals) {
    if (!obj || !name) return false;

    // Format to string with exact precision
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%.*f", decimals, floor(value * pow(10.0, decimals)) / pow(10.0, decimals));

    // Parse back to number (cJSON will preserve the precision)
    double truncated = atof(buffer);
    return cJSON_AddNumberToObject(obj, name, truncated) != NULL;
}
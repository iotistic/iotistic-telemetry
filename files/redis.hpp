/**
 * @file redis_oop.hpp
 * @brief Generic OOP Redis client singleton for interacting with Redis using hiredis
 *        Reusable across different recipes - no application-specific logic
 */
#pragma once

#include <string>
#include <mutex>
#include <hiredis/hiredis.h>

class RedisClient {
public:
    /**
     * @brief Get the singleton instance
     * @return Reference to the single RedisClient instance
     */
    static RedisClient& getInstance();

    // Delete copy constructor and assignment operator
    RedisClient(const RedisClient&) = delete;
    RedisClient& operator=(const RedisClient&) = delete;

    // Connection management
    int connect(const char* hostname = "127.0.0.1", int port = 6379);
    int waitForConnection(int max_retries = 10, int delay_seconds = 2);
    void disconnect();
    bool isConnected() const;

    // String operations
    bool setString(const char* key, const char* value);
    std::string getString(const char* key, const char* default_value = "");

    // Integer operations
    bool setInt(const char* key, int value);
    int getInt(const char* key, int default_value = 0);

    // Float operations
    bool setFloat(const char* key, float value);
    float getFloat(const char* key, float default_value = 0.0f);

    // Key operations
    bool deleteKey(const char* key);
    bool exists(const char* key);

    // Pub/Sub operations
    bool subscribe(const char* channel);
    bool publish(const char* topic, const char* message);

    // Access to subscriber context for advanced pub/sub usage
    redisContext* getSubscriberContext() { return subscriber_context; }

private:
    // Private constructor for singleton
    RedisClient();
    ~RedisClient();

    redisContext* context;
    redisContext* subscriber_context;
    bool is_connected;
    mutable std::mutex redis_mutex;  // For thread safety
};


/**
 * @namespace redis_data
 * @brief Application-level data synchronization from Redis
 * 
 * This namespace provides functions to fetch and parse various
 * data types from Redis into the global m2nvr_data structure.
 */
namespace redis_data {

    /**
     * @brief Fetch all data from Redis and update m2nvr_data
     * 
     * Retrieves GPS, USB, modem, CAN, camera status, and device info.
     * This is the main entry point for data synchronization.
     */
    void get_all();
    
    // Individual parsers 
    void parse_usb_json(const char *json_str);
    void parse_modem_json(const char *json_str);
    void parse_gps_json(const char *json_str);
    void parse_can_json(const char *json_str);
    void parse_cam_status_json(const char *json_str);
    void parse_soc_data_json(const char *json_str);
}


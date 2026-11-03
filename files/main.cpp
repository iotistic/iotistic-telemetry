/**
 * @file main.cpp
 * @brief Main service file
 *
 * Version: V2.3.0
 *
 */

#include "main.hpp"
#include "config.hpp"
#include "redis.hpp"
#include "utils.hpp"

#include "FlespiMQTT.hpp"

#include <chrono>
#include <memory>
#include <stdlib.h>
#include <thread>

// Forward declarations
void redis_handle_message(const char *channel, const char *message, FlespiMQTT *flespi);
void redis_message_loop(FlespiMQTT *flespi);
void subscribeToRedisChannels();
void mqtt_handle_event(const char *event_type, cJSON *value, FlespiMQTT *flespi_client);

// Global variables
st_m2nvr_data m2nvr_data;

int main() {
  printf("############################\n");
  printf("   IOTISTIC MQTT recipe     \n");
  printf("      Version V1.0.10        \n");
  printf("############################\n\n");

  // Read configuration file
  config_read_json_configuration_file();

  // Use singleton pattern for Redis
  RedisClient &redis = RedisClient::getInstance();

  // Establish Redis connection with retry logic
  if (redis.waitForConnection(30, 5) != 0) {
    log_message(LOG_ERROR,
                "Redis connection failed after all retries - exiting");
    return EXIT_FAILURE;
  }

  // Get Redis data
  redis_data::get_all();

  // ========================================================================
  // Create MQTT Clients - Using unique_ptr
  // ========================================================================

  // Create Flespi client with offline database support
  std::unique_ptr<FlespiMQTT> flespi_client =
      std::unique_ptr<FlespiMQTT>(new FlespiMQTT(
          config.flespi.Broker, config.flespi.Port, m2nvr_data.moduel.IMEI,
          config.flespi.username, config.flespi.publish_Topic,
          "offline_crumps.db", config.flespi.flespi_short_names,
          config.flespi.keepalive,config.flespi.qos_level));


  // Try initial connection (will fail gracefully if offline)
  if (m2nvr_data.device_info.internetConnection) {
    flespi_client->connect();
  } else {
    log_message(LOG_INFO,
                "No internet connection Faild to connect to Flespi & IOTISIC ");
  }

  flespi_client->publishInfoMessage();
  setup_timers();

  // ========================================================================
  // Start Redis thread WITH dependency injection
  // ========================================================================

  std::thread redis_thread(redis_message_loop,
                           flespi_client.get());
  redis_thread.detach(); // Don't wait for thread to finish
  log_message(LOG_INFO, "Redis subscription thread started successfully");

  log_message(LOG_INFO,
              "Service initialized successfully - entering main loop");

  // ========================================================================
  // Main service loop - runs forever
  // ========================================================================
  while (1) {
    // ====================================================================
    // Timer-based message publishing
    // ====================================================================

    // Flespi running message
    if (config.timer.publish_run_message.IsActive) {
      config.timer.publish_run_message.IsActive = false;

      if (flespi_client) {
        redis_data::get_all();
        flespi_client->publishRunningMessage();
      }
    }

    // Flespi basic message
    if (config.timer.publish_basic_message.IsActive) {
      config.timer.publish_basic_message.IsActive = false;

      if (flespi_client) {
        redis_data::get_all();
        if(m2nvr_data.gps.Speed_kmh >5){
        flespi_client->publishBasicMessage();
        }
      }
    }


    // Yield for MQTT message callbacks
    if (m2nvr_data.device_info.internetConnection) {
     if(flespi_client->ensureConnected())
      {MQTTClient_yield();}
    }

    // Small sleep to prevent busy waiting
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  return 0;
}

// ============================================================================
// Helper Functions
// ============================================================================

void redis_message_loop(FlespiMQTT *flespi) {
  // Use singleton instance
  RedisClient &redis = RedisClient::getInstance();

  // Subscribe to channels
subscribeToRedisChannels();

  while (true) {
    redisReply *reply = nullptr;

    // Use getSubscriberContext() for low-level access
    int result = redisGetReply(redis.getSubscriberContext(), (void **)&reply);

    if (result == REDIS_OK && reply) {
      if (reply->type == REDIS_REPLY_ARRAY && reply->elements == 3) {
        if (reply->element[0] && reply->element[1] && reply->element[2] &&
            reply->element[0]->type == REDIS_REPLY_STRING &&
            reply->element[1]->type == REDIS_REPLY_STRING &&
            reply->element[2]->type == REDIS_REPLY_STRING) {
          const char *msg_type = reply->element[0]->str;
          const char *channel = reply->element[1]->str;
          const char *message = reply->element[2]->str;

          //log_message(LOG_DEBUG, "Redis message: type=%s, channel=%s", msg_type,   channel);

          if (strcmp(msg_type, "message") == 0) {
            redis_handle_message(channel, message, flespi);
          }
        }
      }
      freeReplyObject(reply);
    } else {
      // Connection lost
      if (reply)
        freeReplyObject(reply);

      log_message(LOG_ERROR,
                  "Redis connection lost, attempting to reconnect...");

      // Use OOP reconnection method
      if (redis.waitForConnection(5, 3) != 0) {
        log_message(LOG_ERROR, "Failed to reconnect to Redis after retries");
        std::this_thread::sleep_for(std::chrono::seconds(5));
        continue;
      }

      // Re-subscribe using OOP method
      // log_message(LOG_INFO, "Re-subscribing to Redis channels...");
     subscribeToRedisChannels();
      log_message(LOG_INFO, "Successfully re-subscribed");
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

void redis_handle_message(const char *channel, const char *message,
                           FlespiMQTT *flespi) {
  if (!flespi) {
    log_message(LOG_ERROR, "Flespi client is null in redis_handle_message");
    return;
  }

  log_message(LOG_INFO, "Redis message received on '%s'", channel);

  // Handle mqtt_event channel
  if (strcmp(channel, "mqtt_event") == 0) {
    cJSON *redis_json = cJSON_Parse(message);
    if (!redis_json) {
      log_message(LOG_ERROR, "Failed to parse JSON from mqtt_event");
      return;
    }

    cJSON *event_item = cJSON_GetObjectItem(redis_json, "event");
    cJSON *value_item = cJSON_GetObjectItem(redis_json, "value");

    if (!event_item || !cJSON_IsString(event_item) || !value_item ||
        (!cJSON_IsNumber(value_item) && !cJSON_IsString(value_item) && !cJSON_IsBool(value_item))) {
      log_message(LOG_ERROR, "Missing or invalid fields in mqtt_event");
      cJSON_Delete(redis_json);
      return;
    }

    const char *event_type = event_item->valuestring;

    log_message(LOG_DEBUG, "redis_handle_message() : Event: %s", event_type);

    // Handle the event (pass flespi client)
    mqtt_handle_event(event_type, value_item, flespi);

    cJSON_Delete(redis_json);
  }
}

void mqtt_handle_event(const char *event_type, cJSON *value,
                       FlespiMQTT *flespi_client) {
  if (!flespi_client) {
    log_message(LOG_WARNING, "Flespi client is null - event '%s' not published",
                event_type);
    return;
  }

  // Class handles internet check and connection internally
  flespi_client->publishEventMessage(event_type, value);
}

void subscribeToRedisChannels() {
  RedisClient &redis = RedisClient::getInstance();

  // log_message(LOG_INFO, "Subscribing to Redis channels...");
  redis.subscribe("mqtt_event");
}
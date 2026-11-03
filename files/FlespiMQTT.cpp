/**
 * @file FlespiMQTT.cpp
 * @brief Implementation of Flespi-specific MQTT client with integrated database
 */

#include "FlespiMQTT.hpp"
#include "main.hpp"
#include "utils.hpp"

// ============================================================================
// Constructor / Destructor
// ============================================================================

FlespiMQTT::FlespiMQTT(const char *broker_address, int port,
                       const char *client_id, const char *token,
                       const char *publish_topic, const char *db_filename,
                       bool use_short_names, int keepalive, short int qos)
    : MQTTClientBase("Flespi", broker_address, port, client_id, nullptr, token,
                     keepalive, qos), // Token is password, no username
      publish_topic_(publish_topic) {
  // Initialize protocol attributes (short or full names)
  if (use_short_names) {
    IMP_use_short_attributes(&attributes_);
  } else {
    IMP_use_full_attributes(&attributes_);
  }

  // Initialize database if filename provided
  if (db_filename && db_filename[0] != '\0') {
    db_ = std::unique_ptr<MqttDatabase>(
        new MqttDatabase(db_filename, "mqtt_messages"));
    if (db_->isOpen()) {
      int pending_count = db_->countPendingMessages();
      log_message(LOG_INFO,
                  "FlespiMQTT client created with offline database: %d pending "
                  "messages",
                  pending_count);
    } else {
      log_message(LOG_ERROR, "Failed to open database for FlespiMQTT");
      db_.reset(); // Release the failed database
    }
  } else {
    log_message(LOG_INFO,
                "FlespiMQTT client created (offline storage disabled)");
  }
}

FlespiMQTT::~FlespiMQTT() {
  log_message(LOG_INFO, "FlespiMQTT client destroyed");
}

// ============================================================================
// Private Helper Functions
// ============================================================================

cJSON *FlespiMQTT::createCommonMessage(const char *message_type) {
  cJSON *common_message = cJSON_CreateObject();
  if (!common_message) {
    log_message(
        LOG_ERROR,
        "createCommonMessage: Failed to create JSON object for type '%s'",
        message_type);
    return nullptr;
  }

  if (cJSON_AddStringToObject(common_message, attributes_.basic.message_type,
                              message_type) == NULL ||
      cJSON_AddStringToObject(common_message, attributes_.basic.identifier,
                              m2nvr_data.moduel.IMEI) == NULL ||
      cJSON_AddNumberToObject(common_message, attributes_.basic.timestamp,
                              m2nvr_data.device_info.epoic_time) == NULL ||
      !addTruncatedNumber(common_message, attributes_.basic.longitude,
                          m2nvr_data.gps.Longitude, 6) ||
      !addTruncatedNumber(common_message, attributes_.basic.latitude,
                          m2nvr_data.gps.Latitude, 6) ||
      !addTruncatedNumber(common_message, attributes_.basic.direction,
                          m2nvr_data.gps.direction, 2) ||
      !addTruncatedNumber(common_message, attributes_.basic.speed,
                          m2nvr_data.gps.Speed_kmh, 2) ||
      !addTruncatedNumber(common_message, attributes_.basic.hdop,
                          m2nvr_data.gps.Horizontal_Precision, 2) ||
      cJSON_AddNumberToObject(common_message, attributes_.basic.satellites,
                              m2nvr_data.gps.Number_Of_Satellites) == NULL) {
    log_message(
        LOG_ERROR,
        "createCommonMessage: Failed to add basic attributes for type '%s'",
        message_type);
    cJSON_Delete(common_message);
    return nullptr;
  }

  return common_message;
}

bool FlespiMQTT::addCANBusAttributes(cJSON *message) {
  // Add CAN bus attributes
  if (cJSON_AddNumberToObject(message, "can.vehicle.speed",
                              m2nvr_data.canbus.speed_kmh) == NULL ||
      cJSON_AddNumberToObject(message, "can.coolant_temp",
                              m2nvr_data.canbus.coolant_temp) == NULL ||
      cJSON_AddNumberToObject(message, "can.engine_rpm",
                              m2nvr_data.canbus.engine_rpm) == NULL ||
      cJSON_AddNumberToObject(message, "can.engine_load",
                              m2nvr_data.canbus.engine_load) == NULL) {
    log_message(LOG_ERROR,
                "addCANBusAttributes: Failed to add CAN bus data to message");
    return false;
  }

  return true;
}

bool FlespiMQTT::tryPublishAndSaveOnFailure(const char *message) {
  if (!message) {
    log_message(LOG_ERROR, "tryPublishAndSaveOnFailure: NULL message provided");
    return false;
  }

  int rc = publish(publish_topic_.c_str(), message, 0, false);

  if (rc == MQTTCLIENT_SUCCESS) {
    return true;
  } else {
    // Save to database if publish failed
    if (db_ && db_->isOpen()) {
      log_message(LOG_WARNING,
                  "Publish failed - saving message to offline database");
      db_->saveMessage(std::string(message));
    }
    return false;
  }
}

int FlespiMQTT::publishWithRetry(const char *message) {
  if (!message) {
    log_message(LOG_ERROR, "publishWithRetry: NULL message");
    return MQTTCLIENT_FAILURE;
  }

  // ========================================================================
  // Step 1: Check internet
  // ========================================================================
  if (!m2nvr_data.device_info.internetConnection) {
    log_message(LOG_DEBUG, "No internet - saving to DB");

    if (db_ && db_->isOpen()) {
      db_->saveMessage(std::string(message));
      return MQTTCLIENT_SUCCESS;
    }

    log_message(LOG_ERROR, "No internet, no DB - message lost");
    return MQTTCLIENT_FAILURE;
  }

  // ========================================================================
  // Step 2: Ensure connected
  // ========================================================================
  if (!ensureConnected()) {
    log_message(LOG_WARNING, "Connection failed - saving to DB");

    if (db_ && db_->isOpen()) {
      db_->saveMessage(std::string(message));
      return MQTTCLIENT_SUCCESS;
    }

    log_message(LOG_ERROR, "Connection failed, no DB - message lost");
    return MQTTCLIENT_FAILURE;
  }

  // ========================================================================
  // Step 3: Process pending messages if any exist (BLOCKING)
  // ========================================================================
  if (hasPendingMessages()) {
    log_message(LOG_INFO, "Found %d pending messages - processing first...",
                getPendingMessageCount());

    bool allSent = processPendingMessages();

    if (!allSent) {
      // Some failed - save current message to maintain FIFO
      log_message(LOG_WARNING,
                  "Pending messages incomplete - saving current to DB");

      if (db_ && db_->isOpen()) {
        db_->saveMessage(std::string(message));
        return MQTTCLIENT_SUCCESS;
      }

      log_message(LOG_ERROR, "Cannot save - DB unavailable");
      return MQTTCLIENT_FAILURE;
    }

    log_message(LOG_INFO, "✓ All pending sent - sending current message");
  }

  // ========================================================================
  // Step 4: Publish current message
  // ========================================================================
  int rc = publish(publish_topic_.c_str(), message, qos_, false);

  if (rc == MQTTCLIENT_SUCCESS) {
    // log_message(LOG_DEBUG, "Message published");
    return MQTTCLIENT_SUCCESS;
  }

  // ========================================================================
  // Step 5: Failed - save to DB
  // ========================================================================
  log_message(LOG_WARNING, "Publish failed (rc=%d) - saving to DB", rc);

  if (db_ && db_->isOpen()) {
    db_->saveMessage(std::string(message));
    return MQTTCLIENT_SUCCESS;
  }

  log_message(LOG_ERROR, "Publish failed, no DB - message lost");
  return MQTTCLIENT_FAILURE;
}

// ============================================================================
// Message Builders - Internal Implementation
// ============================================================================

char *FlespiMQTT::buildBasicMessage() {
  cJSON *basic_message = createCommonMessage("B");
  if (!basic_message) {
    log_message(LOG_ERROR,
                "buildBasicMessage: Failed to create common message");
    return nullptr;
  }

  if (cJSON_AddNumberToObject(basic_message, attributes_.run.signal_level,
                              -99) == NULL) {
    log_message(LOG_ERROR,
                "buildBasicMessage: Failed to add signal_level attribute");
    cJSON_Delete(basic_message);
    return nullptr;
  }

  char *string = cJSON_PrintUnformatted(basic_message);
  cJSON_Delete(basic_message);
  return string;
}

char *FlespiMQTT::buildRunningMessage() {
  cJSON *running_message = createCommonMessage("R");
  if (!running_message) {
    log_message(LOG_ERROR,
                "buildRunningMessage: Failed to create common message");
    return nullptr;
  }

  if (!addTruncatedNumber(running_message, attributes_.run.x_axis_angle,
                          m2nvr_data.imu.gyro_x, 2) ||
      !addTruncatedNumber(running_message, attributes_.run.y_axis_angle,
                          m2nvr_data.imu.gyro_y, 2) ||
      !addTruncatedNumber(running_message, attributes_.run.z_axis_angle,
                          m2nvr_data.imu.gyro_z, 2) ||
      !addTruncatedNumber(running_message, attributes_.run.x_acceleration,
                          m2nvr_data.imu.accel_x, 2) ||
      !addTruncatedNumber(running_message, attributes_.run.y_acceleration,
                          m2nvr_data.imu.accel_y, 2) ||
      !addTruncatedNumber(running_message, attributes_.run.z_acceleration,
                          m2nvr_data.imu.accel_z, 2) ||
      !addTruncatedNumber(running_message, attributes_.run.device_temp,
                          m2nvr_data.imu.gyro_temp, 2) ||
      !addTruncatedNumber(running_message, attributes_.run.cpu_temp,
                          m2nvr_data.device_info.cpu_temp, 2) ||
      cJSON_AddStringToObject(running_message, attributes_.run.access_tech,
                              m2nvr_data.simcard.access_technology) == NULL ||
      !addTruncatedNumber(running_message, attributes_.run.cpu_usage,
                          m2nvr_data.device_info.cpu_usage, 2) ||
      cJSON_AddNumberToObject(running_message,
                              attributes_.event.ignition_status,
                              m2nvr_data.io.ignition) == NULL ||
      cJSON_AddNumberToObject(running_message, "light.sensor",
                              m2nvr_data.io.LDR_status) == NULL ||
      cJSON_AddNumberToObject(running_message, "din", input_bitmask()) ==
          NULL ||
      cJSON_AddNumberToObject(running_message, "dout", output_bitmask()) ==
          NULL ||
      cJSON_AddNumberToObject(running_message, "internal.voltage",
                              m2nvr_data.power.soc_input_voltage) == NULL ||
      cJSON_AddNumberToObject(running_message, "data.usage.rx",
                              m2nvr_data.device_info.rx_bytes) == NULL ||

      cJSON_AddNumberToObject(running_message, "data.usage.tx",
                              m2nvr_data.device_info.tx_bytes) == NULL ||

      !addTruncatedNumber(running_message, "ain.2", m2nvr_data.io.analog2, 2) ||
      // cJSON_AddNumberToObject(running_message, "taxi.total.hired.distance",
      //                         m2nvr_data.taximeter.totalizer_distance_hired) == NULL ||

      cJSON_AddNumberToObject(running_message, "taxi.total.distance",
                              m2nvr_data.taximeter.totalizer_distance) == NULL ||

      cJSON_AddStringToObject(running_message, attributes_.run.device_uptime,
                              m2nvr_data.device_info.os_uptime.c_str()) ==
          NULL ||

      cJSON_AddNumberToObject(running_message, "gsm.signal.level",
                              m2nvr_data.simcard.rssi) == NULL ||

      cJSON_AddStringToObject(
          running_message, "gsm.signal.quality",
          get_signal_quality(m2nvr_data.simcard.rssi).c_str()) == NULL ||

      cJSON_AddNumberToObject(running_message, "motherboard.off.delay.timer",
                              m2nvr_data.device_info.shutdown_timer) == NULL ||

      cJSON_AddNumberToObject(
          running_message, "motherboard.off.delay.timer.countdown",
          m2nvr_data.device_info.shutdown_counter) == NULL ||

      !addTruncatedNumber(running_message, attributes_.run.ext_power_voltage,
                          m2nvr_data.power.bat_voltage, 2)) {
    log_message(LOG_ERROR,
                "buildRunningMessage: Failed to add running attributes");
    cJSON_Delete(running_message);
    return nullptr;
  }

  if (!addCANBusAttributes(running_message)) {
    log_message(LOG_ERROR,
                "buildRunningMessage: Failed to add CAN bus attributes");
    cJSON_Delete(running_message);
    return nullptr;
  }

  char *string = cJSON_PrintUnformatted(running_message);
  cJSON_Delete(running_message);
  return string;
}

char *FlespiMQTT::buildInfoMessage() {
  cJSON *info_message = createCommonMessage("I");
  if (!info_message) {
    log_message(LOG_ERROR, "buildInfoMessage: Failed to create common message");
    return nullptr;
  }

  if (cJSON_AddStringToObject(info_message, attributes_.info.imsi,
                              m2nvr_data.moduel.IMSI) == NULL ||
      cJSON_AddStringToObject(info_message, attributes_.info.iccid,
                              m2nvr_data.simcard.ICCID) == NULL) {
    log_message(LOG_ERROR,
                "buildInfoMessage: Failed to add info attributes (IMSI/ICCID)");
    cJSON_Delete(info_message);
    return nullptr;
  }

  char *string = cJSON_PrintUnformatted(info_message);
  cJSON_Delete(info_message);
  return string;
}

char *FlespiMQTT::buildEventMessage(const char *event, cJSON *value) {
  cJSON *event_message = createCommonMessage("E");
  if (!event_message) {
    log_message(
        LOG_ERROR,
        "buildEventMessage: Failed to create common message for event '%s'",
        event ? event : "NULL");
    return nullptr;
  }

  cJSON *value_copy = cJSON_Duplicate(value, 0);
  if (!value_copy || !cJSON_AddItemToObject(event_message, event, value_copy)) {
    log_message(LOG_ERROR,
                "buildEventMessage: Failed to add event '%s' to message",
                event ? event : "NULL");
    cJSON_Delete(value_copy);
    cJSON_Delete(event_message);
    return nullptr;
  }

  char *string = cJSON_PrintUnformatted(event_message);
  cJSON_Delete(event_message);
  return string;
}

// ============================================================================
// Smart Publish Methods - Auto-retry pending messages first
// ============================================================================

int FlespiMQTT::publishBasicMessage() {
  redis_data::get_all();
  char *message = buildBasicMessage();
  if (!message) {
    log_message(LOG_ERROR, "Failed to build basic message");
    return MQTTCLIENT_FAILURE;
  }

  int result = publishWithRetry(message);
  (result == MQTTCLIENT_SUCCESS)
      ? log_message(LOG_INFO, "[%s] Basic message published", getBrokerName())
      : (void)0;

  free(message);
  return result;
}

int FlespiMQTT::publishRunningMessage() {
  redis_data::get_all();
  char *message = buildRunningMessage();
  if (!message) {
    log_message(LOG_ERROR, "Failed to build running message");
    return MQTTCLIENT_FAILURE;
  }

  int result = publishWithRetry(message);
  (result == MQTTCLIENT_SUCCESS)
      ? log_message(LOG_INFO, "[%s] Runnig message published", getBrokerName())
      : (void)0;

  free(message);
  return result;
}

int FlespiMQTT::publishInfoMessage() {
  char *message = buildInfoMessage();
  if (!message) {
    log_message(LOG_ERROR, "Failed to build info message");
    return MQTTCLIENT_FAILURE;
  }

  int result = publishWithRetry(message);
  if (result == MQTTCLIENT_SUCCESS)
    log_message(LOG_INFO, "[%s] info message published", getBrokerName());

  free(message);
  return result;
}

int FlespiMQTT::publishEventMessage(const char *event, cJSON *value) {
  char *message = buildEventMessage(event, value);
  if (!message) {
    log_message(LOG_ERROR, "Failed to build event message: %s", event);
    return MQTTCLIENT_FAILURE;
  }

  int result = publishWithRetry(message);
  if (result == MQTTCLIENT_SUCCESS)
    log_message(LOG_INFO, "[%s] event message published: %s", getBrokerName(),
                event);

  free(message);
  return result;
}

// ============================================================================
// Flush Helper - WITH INTERNET CHECK
// ============================================================================
bool FlespiMQTT::processPendingMessages() {
  if (!db_ || !db_->isOpen()) {
    log_message(LOG_ERROR, "processPendingMessages(): no database opened");
    return true; // No database, nothing to process
  }

  if (!m2nvr_data.device_info.internetConnection || !isConnected()) {
    log_message(LOG_ERROR, "processPendingMessages(): no Internet connection");
    return false; // Can't send without connection
  }

  int total_count = getPendingMessageCount();
  if (total_count == 0) {
    return true; // Nothing to send
  }

  log_message(LOG_INFO, "Processing %d offline messages...", total_count);

  int sent_count = 0;
  auto start_time = std::chrono::steady_clock::now();

  // ========================================================================
  // Send ALL pending messages (blocking)
  // ========================================================================
  while (hasPendingMessages()) {
    // Check connection status
    if (!m2nvr_data.device_info.internetConnection || !isConnected()) {
      log_message(LOG_WARNING, "Connection lost after %d messages - stopping",
                  sent_count);
      return false;
    }

    // Fetch one batch
    std::vector<int> ids;
    std::vector<std::string> messages;

    int rc = db_->fetchBatchData(10, ids, messages);
    if (rc != SQLITE_OK || messages.empty()) {
      break;
    }

    // Send each message in FIFO order
    for (size_t i = 0; i < messages.size(); i++) {
      int pub_rc =
          publish(publish_topic_.c_str(), messages[i].c_str(), qos_, false);

      if (pub_rc == MQTTCLIENT_SUCCESS) {
        // Delete successfully sent message
        db_->deleteSingleRow(ids[i]);
        sent_count++;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
      } else {
        // Failed - stop to maintain FIFO order
        log_message(LOG_ERROR,
                    "⚠ Send failed at message ID:%d - stopping (FIFO)", ids[i]);
        return false;
      }
    }

    // Progress log every 100 messages
    if (sent_count % 100 == 0) {
      auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                         std::chrono::steady_clock::now() - start_time)
                         .count();

      log_message(LOG_INFO, "Progress: %d sent, %d remain, %lds elapsed",
                  sent_count, getPendingMessageCount(), elapsed);

      MQTTClient_yield();
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
  }

  auto duration = std::chrono::duration_cast<std::chrono::seconds>(
                      std::chrono::steady_clock::now() - start_time)
                      .count();

  log_message(LOG_INFO, "✓ Processed %d messages in %lds", sent_count,
              duration);
  return true;
}

int FlespiMQTT::getPendingMessageCount() {
  if (!db_ || !db_->isOpen()) {
    log_message(LOG_DEBUG, "getPendingMessageCount: Database not available");
    return 0;
  }

  return db_->countPendingMessages();
}

bool FlespiMQTT::hasPendingMessages() {
  if (!db_ || !db_->isOpen()) {
    log_message(LOG_DEBUG, "hasPendingMessages: Database not available");
    return false;
  }

  return (db_->countPendingMessages() > 0);
}

int FlespiMQTT::publishPendingMessages() {
  if (!db_ || !db_->isOpen()) {
    log_message(LOG_DEBUG, "publishPendingMessages: Database not available");
    return 0;
  }

  std::vector<int> ids;
  std::vector<std::string> messages;

  int rc = db_->fetchBatchData(MAX_ROWS, ids, messages);
  if (rc != SQLITE_OK || messages.empty()) {
    if (rc != SQLITE_OK) {
      log_message(LOG_ERROR,
                  "publishPendingMessages: Failed to fetch batch data (rc=%d)",
                  rc);
    }
    return 0;
  }

  int published_count = 0;

  for (size_t i = 0; i < messages.size(); i++) {
    int return_code =
        publish(publish_topic_.c_str(), messages[i].c_str(), 0, false);

    if (return_code == MQTTCLIENT_SUCCESS) {
      // Delete successfully published message
      if (db_->deleteSingleRow(ids[i]) == SQLITE_OK) {
        published_count++;
      }
    } else {
      // Stop on first failure
      log_message(LOG_ERROR, "Failed to publish pending message ID %d, rc=%d",
                  ids[i], return_code);
      break;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  return published_count;
}

bool FlespiMQTT::ensureConnected() {
  // Check if already connected
  if (isConnected()) {
    return true;
  }

  // Throttle reconnection attempts to avoid hammering the broker
  auto now = std::chrono::steady_clock::now();
  if (std::chrono::duration_cast<std::chrono::seconds>(now -
                                                       last_reconnect_attempt_)
          .count() < 5) {
    return false;
  }
  last_reconnect_attempt_ = now;

  // Not connected - try to reconnect
  log_message(LOG_INFO, "Flespi disconnected - attempting reconnection...");

  int rc = reconnect();

  if (rc == MQTTCLIENT_SUCCESS) {
    log_message(LOG_INFO, "Flespi reconnected successfully");
    return true;
  } else {
    log_message(LOG_WARNING,
                "Flespi reconnection failed - operating in offline mode");
    return false;
  }
}

bool FlespiMQTT::saveToDatabase(const char *message) {
  if (!message) {
    log_message(LOG_ERROR, "Cannot save NULL message to database");
    return false;
  }

  if (!db_ || !db_->isOpen()) {
    log_message(LOG_ERROR, "Database not available - message lost");
    return false;
  }

  db_->saveMessage(std::string(message));
  return true;
}

// ============================================================================
// Message Handler (not used for Flespi - no incoming messages)
// ============================================================================

int FlespiMQTT::handleMessage(char *topicName, MQTTClient_message *message) {
  // Flespi is publish-only, no incoming messages expected
  log_message(LOG_WARNING, "Unexpected message received on Flespi topic: %s",
              topicName);

  MQTTClient_freeMessage(&message);
  MQTTClient_free(topicName);

  return 1;
}

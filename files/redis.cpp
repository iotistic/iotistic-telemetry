/**
 * @file redis_oop.cpp
 * @brief Implementation of RedisClient singleton class
 */
#include "redis.hpp"
#include "FlespiMQTT.hpp"
#include "main.hpp"
#include "utils.hpp"
#include <chrono>
#include <cjson/cJSON.h>
#include <cmath> // Add at top of file
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
// Singleton instance
RedisClient &RedisClient::getInstance() {
  static RedisClient instance;
  return instance;
}

// Constructor
RedisClient::RedisClient()
    : context(nullptr), subscriber_context(nullptr), is_connected(false) {}

// Destructor
RedisClient::~RedisClient() { disconnect(); }

// Connection management
int RedisClient::connect(const char *hostname, int port) {
  std::lock_guard<std::mutex> lock(redis_mutex);

  // Clean up any existing connections
  if (context) {
    redisFree(context);
    context = nullptr;
  }
  if (subscriber_context) {
    redisFree(subscriber_context);
    subscriber_context = nullptr;
  }

  context = redisConnect(hostname, port);
  subscriber_context = redisConnect(hostname, port);

  if (!context || context->err || !subscriber_context ||
      subscriber_context->err) {
    log_message(LOG_ERROR, "Redis connection failed: %s",
                context ? context->errstr : "Unknown error");
    is_connected = false;
    return -1;
  }

  is_connected = true;
  //  log_message(LOG_INFO, "Connected to Redis at %s:%d", hostname, port);
  return 0;
}

int RedisClient::waitForConnection(int max_retries, int delay_seconds) {
  int retries = 0;

  while (retries < max_retries) {
    if (connect() == 0) {
      log_message(LOG_INFO, "Redis connection established successfully");
      return 0;
    }

    retries++;
    log_message(
        LOG_ERROR,
        "Redis connection attempt %d/%d failed, retrying in %d seconds...",
        retries, max_retries, delay_seconds);

    if (retries < max_retries) {
      std::this_thread::sleep_for(std::chrono::seconds(delay_seconds));
    }
  }

  log_message(LOG_ERROR, "Failed to connect to Redis after %d attempts",
              max_retries);
  return -1;
}

void RedisClient::disconnect() {
  std::lock_guard<std::mutex> lock(redis_mutex);

  if (context) {
    redisFree(context);
    context = nullptr;
  }

  if (subscriber_context) {
    redisFree(subscriber_context);
    subscriber_context = nullptr;
  }

  if (is_connected) {
    is_connected = false;
    log_message(LOG_INFO, "Redis connection closed");
  }
}

bool RedisClient::isConnected() const {
  std::lock_guard<std::mutex> lock(redis_mutex);
  return is_connected && context && !context->err;
}

// String operations
bool RedisClient::setString(const char *key, const char *value) {
  std::lock_guard<std::mutex> lock(redis_mutex);

  if (!is_connected || !context) {
    log_message(LOG_ERROR, "setString: Redis not connected");
    return false;
  }

  redisReply *reply =
      (redisReply *)redisCommand(context, "SET %s %s", key, value);
  if (!reply) {
    log_message(LOG_ERROR, "setString: Redis SET command failed for key '%s'", key);
    return false;
  }
  freeReplyObject(reply);
  return true;
}

std::string RedisClient::getString(const char *key, const char *default_value) {
  std::lock_guard<std::mutex> lock(redis_mutex);

  if (!is_connected || !context || context->err) {
    log_message(LOG_ERROR, "getString: Redis not connected");
    return std::string(default_value);
  }

  redisReply *reply = (redisReply *)redisCommand(context, "GET %s", key);
  if (!reply) {
    log_message(LOG_ERROR, "getString: Redis GET failed for key '%s'", key);
    return std::string(default_value);
  }

  std::string value;
  if (reply->type == REDIS_REPLY_STRING) {
    value = reply->str;
  } else if (reply->type == REDIS_REPLY_NIL) {
    log_message(LOG_DEBUG, "getString: Key '%s' does not exist, using default", key);
    value = default_value;
  } else {
    log_message(LOG_ERROR, "getString: Unexpected reply type %d for key '%s'", reply->type, key);
    value = default_value;
  }
  freeReplyObject(reply);
  return value;
}

// Integer operations
bool RedisClient::setInt(const char *key, int value) {
  std::lock_guard<std::mutex> lock(redis_mutex);

  if (!is_connected || !context) {
    log_message(LOG_ERROR, "setInt: Redis not connected");
    return false;
  }

  redisReply *reply =
      (redisReply *)redisCommand(context, "SET %s %d", key, value);
  if (!reply) {
    log_message(LOG_ERROR, "setInt: Redis SET command failed for key '%s'", key);
    return false;
  }
  freeReplyObject(reply);
  return true;
}

int RedisClient::getInt(const char *key, int default_value) {
  std::lock_guard<std::mutex> lock(redis_mutex);

  if (!is_connected || !context) {
    log_message(LOG_ERROR, "getInt: Redis not connected");
    return default_value;
  }

  redisReply *reply = (redisReply *)redisCommand(context, "GET %s", key);
  if (!reply) {
    log_message(LOG_ERROR, "getInt: Redis GET command failed for key '%s'", key);
    return default_value;
  }

  if (reply->type == REDIS_REPLY_NIL) {
    log_message(LOG_DEBUG, "getInt: Key '%s' does not exist, using default %d", key, default_value);
    freeReplyObject(reply);
    return default_value;
  }

  if (reply->type != REDIS_REPLY_STRING) {
    log_message(LOG_ERROR, "getInt: Unexpected reply type %d for key '%s'", reply->type, key);
    freeReplyObject(reply);
    return default_value;
  }

  int val = atoi(reply->str);
  freeReplyObject(reply);
  return val;
}

// Float operations
bool RedisClient::setFloat(const char *key, float value) {
  std::lock_guard<std::mutex> lock(redis_mutex);

  if (!is_connected || !context) {
    log_message(LOG_ERROR, "setFloat: Redis not connected");
    return false;
  }

  redisReply *reply =
      (redisReply *)redisCommand(context, "SET %s %f", key, value);
  if (!reply) {
    log_message(LOG_ERROR, "setFloat: Redis SET command failed for key '%s'", key);
    return false;
  }
  freeReplyObject(reply);
  return true;
}

float RedisClient::getFloat(const char *key, float default_value) {
  std::lock_guard<std::mutex> lock(redis_mutex);

  if (!is_connected || !context || context->err) {
    log_message(LOG_ERROR, "getFloat: Redis not connected");
    return default_value;
  }

  redisReply *reply = (redisReply *)redisCommand(context, "GET %s", key);
  if (!reply) {
    log_message(LOG_ERROR, "getFloat: Redis GET failed for key '%s'", key);
    return default_value;
  }

  float value = default_value;
  if (reply->type == REDIS_REPLY_STRING) {
    value = strtof(reply->str, nullptr);
  } else if (reply->type == REDIS_REPLY_NIL) {
    log_message(LOG_DEBUG, "getFloat: Key '%s' does not exist, using default", key);
  } else {
    log_message(LOG_ERROR, "getFloat: Unexpected reply type %d for key '%s'", reply->type, key);
  }
  freeReplyObject(reply);
  return value;
}

// Key operations
bool RedisClient::deleteKey(const char *key) {
  std::lock_guard<std::mutex> lock(redis_mutex);

  if (!is_connected || !context) {
    log_message(LOG_ERROR, "deleteKey: Redis not connected");
    return false;
  }

  redisReply *reply = (redisReply *)redisCommand(context, "DEL %s", key);
  if (!reply) {
    log_message(LOG_ERROR, "deleteKey: Redis DEL command failed for key '%s'", key);
    return false;
  }
  bool deleted = (reply->integer > 0);
  freeReplyObject(reply);
  return deleted;
}

bool RedisClient::exists(const char *key) {
  std::lock_guard<std::mutex> lock(redis_mutex);

  if (!is_connected || !context) {
    log_message(LOG_ERROR, "exists: Redis not connected");
    return false;
  }

  redisReply *reply = (redisReply *)redisCommand(context, "EXISTS %s", key);
  if (!reply) {
    log_message(LOG_ERROR, "exists: Redis EXISTS command failed for key '%s'", key);
    return false;
  }
  bool key_exists = (reply->integer > 0);
  freeReplyObject(reply);
  return key_exists;
}

// Pub/Sub operations
bool RedisClient::subscribe(const char *channel) {
  std::lock_guard<std::mutex> lock(redis_mutex);

  if (!is_connected || !subscriber_context) {
    log_message(LOG_ERROR, "subscribe: Redis not connected");
    return false;
  }

  redisReply *reply =
      (redisReply *)redisCommand(subscriber_context, "SUBSCRIBE %s", channel);
  if (!reply) {
    log_message(LOG_ERROR, "subscribe: Redis SUBSCRIBE command failed for channel '%s'", channel);
    return false;
  }
  // log_message(LOG_INFO, "Subscribed to channel '%s'", channel);
  freeReplyObject(reply);
  return true;
}

bool RedisClient::publish(const char *topic, const char *message) {
  std::lock_guard<std::mutex> lock(redis_mutex);

  if (!is_connected || !context || context->err) {
    log_message(LOG_ERROR, "publish: Redis not connected");
    return false;
  }

  redisReply *reply =
      (redisReply *)redisCommand(context, "PUBLISH %s %s", topic, message);
  if (!reply) {
    log_message(LOG_ERROR, "publish: Redis PUBLISH command failed for topic '%s'",
                topic);
    return false;
  }

  bool success = (reply->type == REDIS_REPLY_INTEGER);
  if (!success) {
    log_message(LOG_ERROR, "publish: Unexpected reply type %d for topic '%s'", reply->type, topic);
  }
  freeReplyObject(reply);
  return success;
}

//=======================================

void redis_data::parse_usb_json(const char *json_str) {
  cJSON *root = cJSON_Parse(json_str);
  if (!root) {
    log_message(LOG_ERROR, "parse_usb_json: Failed to parse JSON");
    return;
  }

  cJSON *item;

  // IMU Data
  item = cJSON_GetObjectItem(root, "Ax");
  if (item && cJSON_IsString(item))
    m2nvr_data.imu.accel_x = atof(item->valuestring);

  item = cJSON_GetObjectItem(root, "Ay");
  if (item && cJSON_IsString(item))
    m2nvr_data.imu.accel_y = atof(item->valuestring);

  item = cJSON_GetObjectItem(root, "Az");
  if (item && cJSON_IsString(item))
    m2nvr_data.imu.accel_z = atof(item->valuestring);

  item = cJSON_GetObjectItem(root, "Gx");
  if (item && cJSON_IsString(item))
    m2nvr_data.imu.gyro_x = atof(item->valuestring);

  item = cJSON_GetObjectItem(root, "Gy");
  if (item && cJSON_IsString(item))
    m2nvr_data.imu.gyro_y = atof(item->valuestring);

  item = cJSON_GetObjectItem(root, "Gz");
  if (item && cJSON_IsString(item))
    m2nvr_data.imu.gyro_z = atof(item->valuestring);

  item = cJSON_GetObjectItem(root, "gyro_temp");
  if (item && cJSON_IsString(item))
    m2nvr_data.imu.gyro_temp = atof(item->valuestring);

  // IMU Config
  item = cJSON_GetObjectItem(root, "gyro_range");
  if (item)
    m2nvr_data.imu_config.gyro_range = item->valueint;

  item = cJSON_GetObjectItem(root, "gyro_sampling_freq");
  if (item && cJSON_IsString(item))
    m2nvr_data.imu_config.gyro_sampling_freq =
        atof(item->valuestring);

  item = cJSON_GetObjectItem(root, "accelerometer_range");
  if (item)
    m2nvr_data.imu_config.accelerometer_range = item->valueint;

  item = cJSON_GetObjectItem(root, "accelerometer_sampling_freq");
  if (item && cJSON_IsString(item))
    m2nvr_data.imu_config.accelerometer_sampling_freq =
        atof(item->valuestring);

  item = cJSON_GetObjectItem(root, "gyroscope_temp_bandwidth");
  if (item)
    m2nvr_data.imu_config.gyroscope_temp_bandwidth = item->valueint;

  item = cJSON_GetObjectItem(root, "gyroscope_bandwidth");
  if (item)
    m2nvr_data.imu_config.gyroscope_bandwidth = item->valueint;

  item = cJSON_GetObjectItem(root, "accelerometer_bandwidth");
  if (item)
    m2nvr_data.imu_config.accelerometer_bandwidth = item->valueint;

  // Power Data
  item = cJSON_GetObjectItem(root, "bat_in_sense");
  if (item && cJSON_IsString(item))
    m2nvr_data.power.bat_voltage = atof(item->valuestring);

  item = cJSON_GetObjectItem(root, "soc_input_voltage");
  if (item && cJSON_IsString(item))
    m2nvr_data.power.soc_input_voltage = atof(item->valuestring);

  item = cJSON_GetObjectItem(root, "soc_output_voltage");
  if (item && cJSON_IsString(item))
    m2nvr_data.power.soc_output_voltage = atof(item->valuestring);

  item = cJSON_GetObjectItem(root, "controller_temperature_sense");
  if (item && cJSON_IsString(item))
    m2nvr_data.power.controller_temp = atof(item->valuestring);

  item = cJSON_GetObjectItem(root, "usb_voltage");
  if (item && cJSON_IsString(item))
    m2nvr_data.power.usb_voltage = atof(item->valuestring);

  // IO Controls
  item = cJSON_GetObjectItem(root, "quectel_power_ctrl");
  if (item)
    m2nvr_data.io.quectel_power = item->valueint;

  item = cJSON_GetObjectItem(root, "main_power_ctrl");
  if (item)
    m2nvr_data.io.main_power = item->valueint;

  item = cJSON_GetObjectItem(root, "out1_ctrl");
  if (item)
    m2nvr_data.io.out1 = item->valueint;

  item = cJSON_GetObjectItem(root, "out2_ctrl");
  if (item)
    m2nvr_data.io.out2 = item->valueint;

  item = cJSON_GetObjectItem(root, "ignition_sense");
  if (item)
    m2nvr_data.io.ignition = item->valueint;

  item = cJSON_GetObjectItem(root, "input1");
  if (item)
    m2nvr_data.io.input1 = item->valueint;

  item = cJSON_GetObjectItem(root, "input2");
  if (item)
    m2nvr_data.io.input2 = item->valueint;

  // Tariffs (day)
  item = cJSON_GetObjectItem(root, "day_initial_fee");
  if (item && cJSON_IsString(item))
    m2nvr_data.taximeter.day_tariff.initial_fee = atof(item->valuestring);

  item = cJSON_GetObjectItem(root, "day_initial_distance");
  if (item && cJSON_IsString(item))
    m2nvr_data.taximeter.day_tariff.initial_distance = atof(item->valuestring);

  item = cJSON_GetObjectItem(root, "day_initial_time");
  if (item)
    m2nvr_data.taximeter.day_tariff.initial_time = item->valueint;

  item = cJSON_GetObjectItem(root, "day_tariff_per_km");
  if (item && cJSON_IsString(item))
    m2nvr_data.taximeter.day_tariff.tariff_per_km = atof(item->valuestring);

  item = cJSON_GetObjectItem(root, "day_tariff_per_hour");
  if (item && cJSON_IsString(item))
    m2nvr_data.taximeter.day_tariff.tariff_per_hour = atof(item->valuestring);

  // Tariffs (night)
  item = cJSON_GetObjectItem(root, "night_initial_fee");
  if (item && cJSON_IsString(item))
    m2nvr_data.taximeter.night_tariff.initial_fee = atof(item->valuestring);

  item = cJSON_GetObjectItem(root, "night_initial_distance");
  if (item && cJSON_IsString(item))
    m2nvr_data.taximeter.night_tariff.initial_distance =
        atof(item->valuestring);

  item = cJSON_GetObjectItem(root, "night_initial_time");
  if (item)
    m2nvr_data.taximeter.night_tariff.initial_time = item->valueint;

  item = cJSON_GetObjectItem(root, "night_tariff_per_km");
  if (item && cJSON_IsString(item))
    m2nvr_data.taximeter.night_tariff.tariff_per_km = atof(item->valuestring);

  item = cJSON_GetObjectItem(root, "night_tariff_per_hour");
  if (item && cJSON_IsString(item))
    m2nvr_data.taximeter.night_tariff.tariff_per_hour = atof(item->valuestring);

  // Trip Info
  item = cJSON_GetObjectItem(root, "trip_time");
  if (item && cJSON_IsString(item))
    m2nvr_data.taximeter.trip.time = atof(item->valuestring);

  item = cJSON_GetObjectItem(root, "trip_distance");
  if (item && cJSON_IsString(item))
    m2nvr_data.taximeter.trip.distance = atof(item->valuestring);

  item = cJSON_GetObjectItem(root, "total_distance");
  if (item && cJSON_IsString(item))
    m2nvr_data.taximeter.trip.total_distance = atof(item->valuestring);

  item = cJSON_GetObjectItem(root, "fare");
  if (item && cJSON_IsString(item))
    m2nvr_data.taximeter.trip.fare = atof(item->valuestring);

  item = cJSON_GetObjectItem(root, "fare_trip_time_pulses");
  if (item)
    m2nvr_data.taximeter.trip.time_pulses = item->valueint;

  item = cJSON_GetObjectItem(root, "fare_trip_distance_pulses");
  if (item)
    m2nvr_data.taximeter.trip.distance_pulses = item->valueint;

  item = cJSON_GetObjectItem(root, "trip_status");
  if (item && cJSON_IsString(item))
    strncpy(m2nvr_data.taximeter.trip.status, item->valuestring,
            sizeof(m2nvr_data.taximeter.trip.status));

  // Day/Night
  item = cJSON_GetObjectItem(root, "night_status");
  if (item && cJSON_IsString(item))
    strncpy(m2nvr_data.taximeter.daynight.current, item->valuestring,
            sizeof(m2nvr_data.taximeter.daynight.current));

  item = cJSON_GetObjectItem(root, "start_night_time");
  if (item && cJSON_IsString(item))
    strncpy(m2nvr_data.taximeter.daynight.start_time, item->valuestring,
            sizeof(m2nvr_data.taximeter.daynight.start_time));

  item = cJSON_GetObjectItem(root, "end_night_time");
  if (item && cJSON_IsString(item))
    strncpy(m2nvr_data.taximeter.daynight.end_time, item->valuestring,
            sizeof(m2nvr_data.taximeter.daynight.end_time));

  item = cJSON_GetObjectItem(root, "cross_over_speed");
  if (item && cJSON_IsString(item))
    m2nvr_data.taximeter.daynight.cross_over_speed = atof(item->valuestring);

  // RTC
  item = cJSON_GetObjectItem(root, "rtc_time");
  if (item && cJSON_IsString(item))
    strncpy(m2nvr_data.RTC.time, item->valuestring,
            sizeof(m2nvr_data.RTC.time));

  item = cJSON_GetObjectItem(root, "rtc_date");
  if (item && cJSON_IsString(item))
    strncpy(m2nvr_data.RTC.date, item->valuestring,
            sizeof(m2nvr_data.RTC.date));

  item = cJSON_GetObjectItem(root, "rtc_day");
  if (item)
    m2nvr_data.RTC.day = item->valueint;

  // Other
  item = cJSON_GetObjectItem(root, "shutdown_counter");
  if (item)
    m2nvr_data.device_info.shutdown_timer = item->valueint;

  item = cJSON_GetObjectItem(root, "time_stopped_delay_counter");
  if (item)
    m2nvr_data.device_info.shutdown_counter = item->valueint;

  item = cJSON_GetObjectItem(root, "flash_counter");
  if (item)
    m2nvr_data.taximeter.flash_counter = item->valueint;

  item = cJSON_GetObjectItem(root, "k_constant");
  if (item)
    m2nvr_data.taximeter.k_constant = item->valueint;

  item = cJSON_GetObjectItem(root, "calc_flag");
  if (item)
    m2nvr_data.taximeter.calc_flag = item->valueint;

  item = cJSON_GetObjectItem(root, "mode");
  if (item && cJSON_IsString(item))
    strncpy(m2nvr_data.taximeter.mode, item->valuestring,
            sizeof(m2nvr_data.taximeter.mode));

  item = cJSON_GetObjectItem(root, "calculation_method");
  if (item && cJSON_IsString(item))
    strncpy(m2nvr_data.taximeter.calculation_method, item->valuestring,
            sizeof(m2nvr_data.taximeter.calculation_method));

  item = cJSON_GetObjectItem(root, "speed");
  if (item && cJSON_IsString(item))
    m2nvr_data.taximeter.speed = atof(item->valuestring);

  // new data
  item = cJSON_GetObjectItem(root, "taxi_meter_totalizer_fare");
  if (item && cJSON_IsNumber(item))
    m2nvr_data.taximeter.totalizer_fare = item->valuedouble;

  item = cJSON_GetObjectItem(root, "taxi_meter_totalizer_distance_hired");
  if (item && cJSON_IsNumber(item))
    m2nvr_data.taximeter.totalizer_distance_hired = item->valuedouble;

  item = cJSON_GetObjectItem(root, "taxi_meter_totalizer_distance");
  if (item && cJSON_IsNumber(item))
    m2nvr_data.taximeter.totalizer_distance = item->valuedouble;

  item = cJSON_GetObjectItem(root, "taxi_meter_totalizer_number_of_journeys");
  if (item && cJSON_IsNumber(item))
    m2nvr_data.taximeter.totalizer_number_of_journeys = item->valueint;

  item = cJSON_GetObjectItem(root, "time_block_status");
  if (item && cJSON_IsString(item))
    strncpy(m2nvr_data.taximeter.time_block_status, item->valuestring,
            sizeof(m2nvr_data.taximeter.time_block_status));

  item = cJSON_GetObjectItem(root, "time_block_output");
  if (item && cJSON_IsString(item))
    strncpy(m2nvr_data.taximeter.time_block_output, item->valuestring,
            sizeof(m2nvr_data.taximeter.time_block_output));

  item = cJSON_GetObjectItem(root, "signal_error_flag");
  if (item && cJSON_IsNumber(item))
    m2nvr_data.taximeter.signal_error_flag = item->valueint;

  item = cJSON_GetObjectItem(root, "voltage_error_flag");
  if (item && cJSON_IsNumber(item))
    m2nvr_data.taximeter.voltage_error_flag = item->valueint;

  item = cJSON_GetObjectItem(root, "rtc_error_flag");
  if (item && cJSON_IsNumber(item))
    m2nvr_data.taximeter.rtc_error_flag = item->valueint;

  cJSON_Delete(root);
}

void redis_data::parse_cam_status_json(const char *json_str) {
  cJSON *root = cJSON_Parse(json_str);
  if (!root) {
    log_message(LOG_ERROR, "parse_cam_status_json: Failed to parse JSON");
    return;
  }

  // Clear previous camera statuses to avoid carrying over old data
  // Assuming m2nvr_data.cams is an array or a similar structure
  // that can be dynamically sized or has a max capacity.
  // For this example, we'll assume a max size and clear all entries.
  // For a more robust solution, you might need a different data structure.
  for (int i = 0; i < MAX_CAMS; i++) {
    m2nvr_data.cams_status[i] = 0; // Initialize to offline
  }

  cJSON *cam_item;
  cJSON_ArrayForEach(cam_item, root) {
    char *cam_name = cam_item->string;
    if (cam_name && cJSON_IsString(cam_item)) {
      // Extract the camera number from the key, e.g., "cam1 " ->
      int cam_id;
      if (sscanf(cam_name, "cam%d", &cam_id) == 1) {
        if (cam_id > 0 && cam_id <= MAX_CAMS) // Use a defined max camera number
        {
          if (strcmp(cam_item->valuestring, "ON") == 0) {
            m2nvr_data.cams_status[cam_id - 1] = 1; // Mark as online
          }
          // No need for an 'OFF' check, as status is 0 by default
        }
      }
    }
  }

  cJSON_Delete(root);
}

void redis_data::parse_modem_json(const char *json_str) {

  if (!json_str) {
    log_message(LOG_ERROR, "parse_modem_json: NULL JSON string provided");
    return;
  }

  cJSON *modem_obj = cJSON_Parse(json_str);
  if (!modem_obj) {
    log_message(LOG_ERROR, "parse_modem_json: Failed to parse JSON");
    return;
  }

  cJSON *item;

  // IMEI
  item = cJSON_GetObjectItem(modem_obj, "imei");
  if (cJSON_IsString(item) && item->valuestring)
    strncpy(m2nvr_data.moduel.IMEI, item->valuestring,
            sizeof(m2nvr_data.moduel.IMEI) - 1);
  else
    strncpy(m2nvr_data.moduel.IMEI, "N/A", sizeof(m2nvr_data.moduel.IMEI) - 1);
  m2nvr_data.moduel.IMEI[sizeof(m2nvr_data.moduel.IMEI) - 1] = '\0';

  // IMSI
  item = cJSON_GetObjectItem(modem_obj, "imsi");
  if (cJSON_IsString(item) && item->valuestring)
    strncpy(m2nvr_data.moduel.IMSI, item->valuestring,
            sizeof(m2nvr_data.moduel.IMSI) - 1);
  else
    strncpy(m2nvr_data.moduel.IMSI, "N/A", sizeof(m2nvr_data.moduel.IMSI) - 1);
  m2nvr_data.moduel.IMSI[sizeof(m2nvr_data.moduel.IMSI) - 1] = '\0';

  // ICCID
  item = cJSON_GetObjectItem(modem_obj, "iccid");
  if (cJSON_IsString(item) && item->valuestring)
    strncpy(m2nvr_data.simcard.ICCID, item->valuestring,
            sizeof(m2nvr_data.simcard.ICCID) - 1);
  else
    strncpy(m2nvr_data.simcard.ICCID, "N/A",
            sizeof(m2nvr_data.simcard.ICCID) - 1);
  m2nvr_data.simcard.ICCID[sizeof(m2nvr_data.simcard.ICCID) - 1] = '\0';

  item = cJSON_GetObjectItem(modem_obj, "internet_status");
  if (cJSON_IsString(item) && item->valuestring) {
    m2nvr_data.device_info.internetConnection =
        (strcmp(item->valuestring, "Connected") == 0);
  }

  item =cJSON_GetObjectItem(modem_obj,"network_parameters");
  if(cJSON_IsString(item)&& item->valuestring)
  {
    std::string network_parameters=item->valuestring;
    m2nvr_data.simcard.rssi=extract_value_from_brackets(network_parameters.c_str(),"signalStrength"); 
  }


  cJSON_Delete(modem_obj);

}

void redis_data::parse_gps_json(const char *json_str) {
  if (!json_str) {
    log_message(LOG_ERROR, "parse_gps_json: NULL JSON string provided");
    return;
  }

  cJSON *gps_data_obj = cJSON_Parse(json_str);
  if (!gps_data_obj) {
    log_message(LOG_ERROR, "parse_gps_json: Failed to parse JSON");
    return;
  }

  cJSON *item;

  // Store raw values - truncation happens in addTruncatedNumber()
  item = cJSON_GetObjectItem(gps_data_obj, "longitude");
  m2nvr_data.gps.Longitude =
      (cJSON_IsNumber(item)) ? static_cast<float>(item->valuedouble) : -1.0f;

  item = cJSON_GetObjectItem(gps_data_obj, "latitude");
  m2nvr_data.gps.Latitude =
      (cJSON_IsNumber(item)) ? static_cast<float>(item->valuedouble) : -1.0f;

  item = cJSON_GetObjectItem(gps_data_obj, "altitude");
  m2nvr_data.gps.Altitude =
      (cJSON_IsNumber(item)) ? static_cast<float>(item->valuedouble) : -1.0f;

  item = cJSON_GetObjectItem(gps_data_obj, "speed");
  m2nvr_data.gps.Speed_kmh =
      (cJSON_IsNumber(item)) ? static_cast<float>(item->valuedouble) : -1.0f;

  item = cJSON_GetObjectItem(gps_data_obj, "course");
  m2nvr_data.gps.direction =
      (cJSON_IsNumber(item)) ? static_cast<float>(item->valuedouble) : -1.0f;

  item = cJSON_GetObjectItem(gps_data_obj, "nsat");
  m2nvr_data.gps.Number_Of_Satellites =
      (cJSON_IsNumber(item)) ? item->valueint : -1;

  item = cJSON_GetObjectItem(gps_data_obj, "hdop");
  m2nvr_data.gps.Horizontal_Precision =
      (cJSON_IsNumber(item)) ? static_cast<float>(item->valuedouble) : -1.0f;

  item = cJSON_GetObjectItem(gps_data_obj, "timestamp");
  long long gps_epoch =
      (cJSON_IsNumber(item)) ? static_cast<long long>(item->valuedouble) : -1;

  snprintf(m2nvr_data.gps.gps_utc, sizeof(m2nvr_data.gps.gps_utc), "%lld",
           gps_epoch);
  strncpy(m2nvr_data.gps.gps_date, "000000", sizeof(m2nvr_data.gps.gps_date));

  cJSON_Delete(gps_data_obj);
}

void redis_data::parse_can_json(const char *json_str) {
  if (!json_str) {
    log_message(LOG_ERROR, "parse_can_json: NULL JSON string provided");
    return;
  }

  cJSON *can_data_obj = cJSON_Parse(json_str);
  if (!can_data_obj) {
    log_message(LOG_ERROR, "parse_can_json: Failed to parse JSON");
    return;
  }

  cJSON *item;
  item = cJSON_GetObjectItem(can_data_obj, "speed_kmh");
  m2nvr_data.canbus.speed_kmh = (cJSON_IsNumber(item)) ? item->valueint : -1;

  item = cJSON_GetObjectItem(can_data_obj, "coolant_temp");
  m2nvr_data.canbus.coolant_temp = (cJSON_IsNumber(item)) ? item->valueint : -1;

  item = cJSON_GetObjectItem(can_data_obj, "engine_rpm");
  m2nvr_data.canbus.engine_rpm = (cJSON_IsNumber(item)) ? item->valueint : -1;

  item = cJSON_GetObjectItem(can_data_obj, "engine_load");
  m2nvr_data.canbus.engine_load = (cJSON_IsNumber(item)) ? item->valueint : -1;

  cJSON_Delete(can_data_obj);
}

void redis_data::parse_soc_data_json(const char *json_str) {
  if (!json_str) {
    log_message(LOG_ERROR, "parse_soc_data_json: NULL JSON string provided");
    return;
  }

  cJSON *soc_data_obj = cJSON_Parse(json_str);
  if (!soc_data_obj) {
    log_message(LOG_ERROR, "parse_soc_data_json: Failed to parse JSON");
    return;
  }

  cJSON *item;

  // Epoch time - convert number to string
  item = cJSON_GetObjectItem(soc_data_obj, "epoch_time");
  if (cJSON_IsNumber(item))
    m2nvr_data.device_info.epoic_time = static_cast<int64_t>(item->valuedouble);

  // Uptime seconds
  item = cJSON_GetObjectItem(soc_data_obj, "uptime_sec");
  m2nvr_data.device_info.os_uptime_seconds =
      (cJSON_IsNumber(item)) ? static_cast<int>(item->valuedouble) : 1;

  // CPU usage
  item = cJSON_GetObjectItem(soc_data_obj, "cpu_usage");
  m2nvr_data.device_info.cpu_usage =
      (cJSON_IsNumber(item)) ? static_cast<float>(item->valuedouble) : -1.0f;

  // CPU temperature average
  item = cJSON_GetObjectItem(soc_data_obj, "cpu_temp_avg");
  m2nvr_data.device_info.cpu_temp =
      (cJSON_IsNumber(item)) ? static_cast<float>(item->valuedouble) : -99.0f;

  // LDR status
  item = cJSON_GetObjectItem(soc_data_obj, "ldr_status");
  m2nvr_data.io.LDR_status =
      (cJSON_IsNumber(item)) ? item->valueint : 0;

  // rx bytes
  item = cJSON_GetObjectItem(soc_data_obj, "rx_bytes");
  m2nvr_data.device_info.rx_bytes = (cJSON_IsNumber(item)) ? item->valueint : 0;

  // tx bytes
  item = cJSON_GetObjectItem(soc_data_obj, "tx_bytes");
  m2nvr_data.device_info.tx_bytes = (cJSON_IsNumber(item)) ? item->valueint : 0;

  // Analge 2
  item = cJSON_GetObjectItem(soc_data_obj, "analog_in_2");
  m2nvr_data.io.analog2 =
      (item->valuedouble) ? static_cast<float>(item->valuedouble) : -1.0f;

  cJSON_Delete(soc_data_obj);
}

void redis_data::get_all() {
  RedisClient &redis = RedisClient::getInstance();

  // Parse GPS data
  std::string json_gps = redis.getString("gps_data", "{}");
  parse_gps_json(json_gps.c_str());

  // Parse USB data
  std::string json_usb = redis.getString("usb_data", "{}");
  parse_usb_json(json_usb.c_str());

  // Parse modem data
  std::string json_modem = redis.getString("modem_data", "{}");
  parse_modem_json(json_modem.c_str());

  // Parse CAN Data
  std::string json_can = redis.getString("can_data", "{}");
  parse_can_json(json_can.c_str());

  // Parse cam status
  std::string cam_json = redis.getString("cam_status", "{}");
  parse_cam_status_json(cam_json.c_str());

  // Parse SOC data (epoch_time, uptime_sec, cpu_usage, cpu_temp_avg)
  std::string json_soc = redis.getString("soc_data", "{}");
  parse_soc_data_json(json_soc.c_str());

  // Calculate uptime string from parsed uptime_sec
  calculate_uptime_string(m2nvr_data.device_info.os_uptime_seconds,
                          m2nvr_data.device_info.os_uptime);
}

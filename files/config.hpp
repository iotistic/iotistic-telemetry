#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include <stdint.h>
#include <cjson/cJSON.h>
#include <stdbool.h>
typedef enum
{
  QOS_0_AT_MOST_ONCE = 0,  // Fire and forget
  QOS_1_AT_LEAST_ONCE = 1, // Acknowledged delivery
  QOS_2_EXACTLY_ONCE = 2   // Assured delivery
} mqtt_qos_t;

/** @brief Default MQTT broker address for Flespi connection */
#define DF_MQTT_BROKER "mqtt.flespi.io"
/** @brief Default MQTT port number */
#define DF_MQTT_PORT 1883
/** @brief Default MQTT topic for device publishing */
#define DF_MQTT_PUBLISH_TOPIC_1 "m2nvr/prod/messages"

#define DF_MQTT_COMMAND_TOPIC "m2nvr/cmd"

#define DF_MQTT_QOS QOS_0_AT_MOST_ONCE

/** @brief Default MQTT keep-alive interval in minutes */
#define DF_MQTT_KEEP_ALIVE 60
/** @brief Default username for Flespi MQTT connection */
#define DF_MQTT_FLESPI_USERNAME "BiLNxt9GLdqbHxF9ff9oNMvvuk2GuXf7Gsg4Zt0YiiGb4IegfPgEZ5Fwlsf4i3uc"
/** @brief Default password for Flespi MQTT connection (empty) */
#define DF_MQTT_FLESPI_PASSWORD ""

#define DF_MQTT_FLESPI_TIMEOUT 1000L

/** @brief Flag to disable Flespi short names feature (0=disabled) */
#define DF_MQTT_FLISPI_SHORT_NAMES_FALSE 0
/** @brief Default interval for basic status message publishing in seconds (range: 5-15) */
#define DF_MQTT_PUBLISH_BASIC_MESSAGE_SENDING_RATE_SEC 15
/** @brief Default interval for running state message publishing in seconds (range: 420-900) */
#define DF_MQTT_PUBLISH_RUNNNING_MESSAGE_SENDING_RATE_SEC 30
/** @brief Default interval for checking MQTT subscription topic in seconds */
#define DF_MQTT_SUBSICRIBE_TOPIC_1_SENDING_RATE_SEC 10

#define DF_MQTT_IOTISTIC_STATUS_UPDATE_SENDING_RATE_SEC 5

/** @brief Default topic for receiving commands from Flespi trace/cmd/<IMEI>   */
#define DF_MQTT_FLESPI_SUBSICRIBE_COMMANDS_TOPIC "trace/cmd"

/** @brief Default topic for responding to commands on Flespi */
#define DF_MQTT_FLESPI_PUBLILSH_COMMANDS_TOPIC "trace/cmd-update"

#define DF_MQTT_FLESPI_CLIENTID ""

/**
 * @brief MQTT configuration parameters
 *
 * Contains all parameters required for MQTT broker connection
 * and message publishing/subscription behavior.
 */
typedef struct
{
  char name[10];            /**< for debugging  */
  char Broker[40];          /**< MQTT broker hostname or IP address */
  char publish_Topic[32];   /**< Topic for publishing device data */
  char lastwill_Topic[32];  /**< Topic for publishing device data */
  char jobUpdate_Topic[32]; /**< Topic for publishing device data */
  char commands_Topic[32];  /**< Topic for receiving commands */
  char username[80];        /**< Authentication username */
  char password[32];        /**< Authentication password */
  int client_id;
  uint16_t Port;              /**< MQTT broker port number */
  uint8_t flespi_short_names; /**< Flag for using short field names in Flespi messages */
  uint16_t keepalive;         /**< Keep-alive interval for MQTT connection (seconds) */
  uint32_t timeout_ms;        /**< Connection timeout (milliseconds) */
  mqtt_qos_t qos_level;
  bool use_clean_session; /**< Clean session flag */
  bool auto_reconnect;    /**< Auto-reconnect flag */
  bool require_ack;       /**< If true, wait for publish confirmation */

} st_MQTT_Config;

typedef struct
{
  char address[32];
  uint16_t Port;
} st_redis_Config;

typedef struct
{
  bool aqs;
  bool canbus;
  bool gps;
  bool gsm;
  bool mqtt;
  bool taxi_meter;
  bool event_messages;
  bool status_messages;
} st_services;

typedef struct
{
  uint16_t period;  // in seconds
  uint8_t IsActive; // flage
} st_timer;

typedef struct
{
  st_timer publish_basic_message;
  st_timer publish_run_message;
  st_timer publish_stauts_update_message;
  st_timer check_log_messages;
  st_timer publish_iotistic_run_message;
} st_timers;

typedef struct
{
  st_MQTT_Config flespi;
  st_redis_Config redis;
  st_services services_en; // Enable or Disable the services
  st_timers timer;

} st_config;

extern st_config config; // You must define this instance somewhere

void config_set_default_config(st_config *config);
void config_read_json_configuration_file(void);
cJSON *config_read_json_file(const char *filename);

#endif
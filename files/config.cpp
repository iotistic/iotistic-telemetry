#include "config.hpp"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <string.h>
#include "main.hpp"
#include "utils.hpp"

const char *filename = "/m2nvr_Config/device_config.json";
st_config config;

// Function to read the JSON file
cJSON *config_read_json_file(const char *filename)
{
    FILE *file = fopen(filename, "rb"); // binary read mode
    if (!file)
    {
        log_message(LOG_ERROR,"Error opening file : %s",filename);
        return NULL;
    }
    // Determining File Size
    fseek(file, 0, SEEK_END); // Moves to the end of the file
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET); // Returns to the beginning of the file  , Ready to read the file from the start

    char *buffer = (char *)malloc(file_size + 1);

    if (buffer)
    {
        size_t bytes_read = fread(buffer, 1, file_size, file);
        if (bytes_read != static_cast<size_t>(file_size))
        {
            log_message(LOG_ERROR, " Error reading file, expected %ld bytes, but read %zu bytes.", file_size, bytes_read);

            free(buffer);
            fclose(file);
            return NULL;
        }
        buffer[file_size] = '\0';
    }

    fclose(file);

    cJSON *json = cJSON_Parse(buffer);
    free(buffer);
    if (json == NULL)
    {
        log_message(LOG_ERROR, " Error parsing JSON: %s", cJSON_GetErrorPtr());

        return NULL;
    }

    return json;
}

// Default fallback function
void config_set_default_config(st_config *config)
{
    // Redis defaults
    strncpy(config->redis.address, "127.0.0.1", sizeof(config->redis.address));
    config->redis.Port = 6379;

    // MQTT  flespi defaults
    strncpy(config->flespi.Broker, DF_MQTT_BROKER, sizeof(config->flespi.Broker));
    config->flespi.Port = DF_MQTT_PORT;
    strncpy(config->flespi.name, "Flespi", sizeof(config->flespi.name));
    strncpy(config->flespi.username, DF_MQTT_FLESPI_USERNAME, sizeof(config->flespi.username));
    strncpy(config->flespi.password, DF_MQTT_FLESPI_PASSWORD, sizeof(config->flespi.password));
    config->flespi.keepalive = DF_MQTT_KEEP_ALIVE;
    strncpy(config->flespi.publish_Topic, DF_MQTT_PUBLISH_TOPIC_1, sizeof(config->flespi.publish_Topic));
    strncpy(config->flespi.commands_Topic, DF_MQTT_FLESPI_SUBSICRIBE_COMMANDS_TOPIC, sizeof(config->flespi.commands_Topic));
    config->timer.publish_basic_message.period = DF_MQTT_PUBLISH_BASIC_MESSAGE_SENDING_RATE_SEC;
    config->timer.publish_run_message.period = DF_MQTT_PUBLISH_RUNNNING_MESSAGE_SENDING_RATE_SEC;
    // config->timer..period = DF_MQTT_SUBSICRIBE_TOPIC_1_SENDING_RATE_SEC;
    config->flespi.flespi_short_names = DF_MQTT_FLISPI_SHORT_NAMES_FALSE;
    config->flespi.qos_level = DF_MQTT_QOS;
    config->flespi.client_id = 1;

    config->timer.check_log_messages.period = 1;
    config->timer.check_log_messages.IsActive = false;
    config->timer.publish_basic_message.IsActive = false;
    config->timer.publish_run_message.IsActive = false;

    // Services
    config->services_en.aqs = false;
    config->services_en.canbus = true;
    config->services_en.event_messages = false;
    config->services_en.gps = true;
    config->services_en.gsm = false;
    config->services_en.mqtt = false;
    config->services_en.status_messages = false;
    config->services_en.taxi_meter = true;
}


#define MQTT_PROFILES_PATH "/m2nvr_Config/mqtt_profiles.json"
#define MESSAGING_SETTINGS_PATH "/m2nvr_Config/messaging_mqtt_settings.json"
#define MGMT_SETTINGS_PATH "/m2nvr_Config/management_mqtt_settings.json"

void config_read_json_configuration_file(void)
{
    // Start with default values for everything
    config_set_default_config(&config);

    // --- MQTT Profiles ---
    cJSON *mqtt_profiles = config_read_json_file(MQTT_PROFILES_PATH);
    if (!mqtt_profiles)
    {
        log_message(LOG_ERROR, "Failed to read mqtt_profiles.json file. Using default config only.");
        return;
    }
   // log_message(LOG_DEBUG,"Mqtt_profile : \n%s",mqtt_profiles);

    cJSON *flespi = cJSON_GetObjectItemCaseSensitive(mqtt_profiles, "flespi");


    if (cJSON_IsObject(flespi))
    {
        cJSON *host = cJSON_GetObjectItemCaseSensitive(flespi, "host");
        if (cJSON_IsString(host)) strncpy(config.flespi.Broker, host->valuestring, sizeof(config.flespi.Broker));

        cJSON *port = cJSON_GetObjectItemCaseSensitive(flespi, "port");
        if (cJSON_IsNumber(port)) config.flespi.Port = (uint16_t)port->valuedouble;

        cJSON *username = cJSON_GetObjectItemCaseSensitive(flespi, "username");
        if (cJSON_IsString(username)) strncpy(config.flespi.username, username->valuestring, sizeof(config.flespi.username));

        cJSON *password = cJSON_GetObjectItemCaseSensitive(flespi, "password");
        if (cJSON_IsString(password)) strncpy(config.flespi.password, password->valuestring, sizeof(config.flespi.password));

        cJSON *keepalive = cJSON_GetObjectItemCaseSensitive(flespi, "keepalive");
        if (cJSON_IsNumber(keepalive)) config.flespi.keepalive = (uint16_t)keepalive->valuedouble;

        cJSON *qos = cJSON_GetObjectItemCaseSensitive(flespi, "QOS");
if (cJSON_IsNumber(qos))
{
    int qos_val = static_cast<int>(qos->valuedouble);
    if (qos_val < 0) qos_val = 0;
    if (qos_val > 2) qos_val = 2; // clamp valid MQTT QoS
    config.flespi.qos_level = static_cast<mqtt_qos_t>(qos_val);
}    }

    log_message(LOG_INFO, "Finished reading mqtt_profiles.json");

    // --- Messaging Settings ---
    cJSON *messaging_mqtt_settings = config_read_json_file(MESSAGING_SETTINGS_PATH);
    if (!messaging_mqtt_settings)
    {
        log_message(LOG_ERROR, "Failed to read messaging_mqtt_settings.json file. Using default config only.");
        cJSON_Delete(mqtt_profiles);
        return;
    }
    //log_message(LOG_DEBUG,"messaging_mqtt_settings : \n%s",messaging_mqtt_settings);

    if (cJSON_IsObject(messaging_mqtt_settings))
    {
        cJSON *topic = cJSON_GetObjectItemCaseSensitive(messaging_mqtt_settings, "topic");
        if (cJSON_IsString(topic)) strncpy(config.flespi.publish_Topic, topic->valuestring, sizeof(config.flespi.publish_Topic));

        cJSON *basic = cJSON_GetObjectItemCaseSensitive(messaging_mqtt_settings, "basic_sending_period");
        if (cJSON_IsNumber(basic)) config.timer.publish_basic_message.period = (uint16_t)basic->valuedouble;

        cJSON *run = cJSON_GetObjectItemCaseSensitive(messaging_mqtt_settings, "running_sending_period");
        if (cJSON_IsNumber(run)) config.timer.publish_run_message.period = (uint16_t)run->valuedouble;

        cJSON *short_names = cJSON_GetObjectItemCaseSensitive(messaging_mqtt_settings, "short_names");
        if (cJSON_IsBool(short_names)) config.flespi.flespi_short_names = cJSON_IsTrue(short_names) ? 1 : 0;
    }

    // --- Management MQTT Settings ---
    cJSON *management_mqtt_settings = config_read_json_file(MGMT_SETTINGS_PATH);
    if (!management_mqtt_settings)
    {
        log_message(LOG_ERROR, "Failed to read management_mqtt_settings.json file. Using default config only.");
        cJSON_Delete(mqtt_profiles);
        cJSON_Delete(management_mqtt_settings);
        return;
    }
    //log_message(LOG_DEBUG,"management_mqtt_settings : \n%s",management_mqtt_settings);

   // log_message(LOG_DEBUG, "Finished reading configuration.");

    // Cleanup
    cJSON_Delete(mqtt_profiles);
    cJSON_Delete(messaging_mqtt_settings);
    cJSON_Delete(management_mqtt_settings);
}


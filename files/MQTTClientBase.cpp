/**
 * @file MQTTClientBase.cpp
 * @brief Implementation of base MQTT client class
 */

#include "MQTTClientBase.hpp"
#include "main.hpp"
#include <cstring>

// ============================================================================
// Constructor / Destructor
// ============================================================================

MQTTClientBase::MQTTClientBase(
    const char* broker_name,
    const char* broker_address,
    int port,
    const char* client_id,
    const char* username,
    const char* password,
    int keepalive,
    short int qos)
    : client_(nullptr),
      broker_name_(broker_name),
      broker_address_(broker_address),
      port_(port),
      client_id_(client_id),
      username_(username ? username : ""),
      password_(password ? password : ""),
      keepalive_(keepalive),
      is_connected_(false),
      qos_(qos)
{
}

MQTTClientBase::~MQTTClientBase()
{
    disconnect();
}

// ============================================================================
// Core MQTT Operations
// ============================================================================

int MQTTClientBase::connect(const char* last_will_topic,
                            const char* last_will_message,
                            int qos)
{
    std::lock_guard<std::mutex> lock(mqtt_mutex_);

    // Build broker address
    char address[128];
    snprintf(address, sizeof(address), "tcp://%s:%d", 
             broker_address_.c_str(), port_);

    // log_message(LOG_INFO, "%s: Connecting to %s:%d with client ID '%s'", 
    //             broker_name_.c_str(), broker_address_.c_str(), port_, 
    //             client_id_.c_str());

    // Create MQTT client
    int rc = MQTTClient_create(&client_, address, client_id_.c_str(), 
                               MQTTCLIENT_PERSISTENCE_NONE, NULL);
    if (rc != MQTTCLIENT_SUCCESS)
    {
        logError("MQTTClient_create", rc);
        return rc;
    }

    // Set callback
    rc = MQTTClient_setCallbacks(client_, this, NULL, messageCallbackWrapper, NULL);
    if (rc != MQTTCLIENT_SUCCESS)
    {
        logError("MQTTClient_setCallbacks", rc);
        MQTTClient_destroy(&client_);
        client_ = nullptr;
        return rc;
    }

    // Setup connection options
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    
    // Set credentials if provided
    if (!username_.empty())
        conn_opts.username = username_.c_str();
    if (!password_.empty())
        conn_opts.password = password_.c_str();
    
    conn_opts.keepAliveInterval = keepalive_;
    conn_opts.cleansession = 1;

    // Setup Last Will (if provided)
    MQTTClient_willOptions will_opts = MQTTClient_willOptions_initializer;
    std::string lwt_msg;
    
    if (last_will_topic)
    {
        // Use provided message or build default
        if (last_will_message)
        {
            lwt_msg = last_will_message;
        }
        else
        {
            lwt_msg = buildDefaultLastWillMessage();
        }
        
        will_opts.topicName = last_will_topic;
        will_opts.message = lwt_msg.c_str();
        will_opts.qos = qos;
        will_opts.retained = 1;
        conn_opts.will = &will_opts;
    }

    // Connect to broker
    rc = MQTTClient_connect(client_, &conn_opts);
    if (rc != MQTTCLIENT_SUCCESS)
    {
        logError("MQTTClient_connect", rc);
        MQTTClient_destroy(&client_);
        client_ = nullptr;
        return rc;
    }

    is_connected_ = true;
  //  log_message(LOG_INFO, "%s: Connected to MQTT broker at %s:%d", broker_name_.c_str(), broker_address_.c_str(), port_);
    
    return MQTTCLIENT_SUCCESS;
}

int MQTTClientBase::publish(const char* topic, const char* payload, int qos, bool retained)
{
    std::lock_guard<std::mutex> lock(mqtt_mutex_);

    if (!client_ || !payload)
    {
        log_message(LOG_ERROR, "%s: NULL parameter in publish to topic %s", 
                    broker_name_.c_str(), topic ? topic : "NULL");
        return MQTTCLIENT_FAILURE;
    }

    if (!MQTTClient_isConnected(client_))
    {
        log_message(LOG_ERROR, "%s: Client is not connected", broker_name_.c_str());
        return MQTTCLIENT_DISCONNECTED;
    }

    // Setup message
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    pubmsg.payload = (void*)payload;
    pubmsg.payloadlen = (int)strlen(payload);
    pubmsg.qos = qos;
    pubmsg.retained = retained ? 1 : 0;

    // Publish
    MQTTClient_deliveryToken token;
    int rc = MQTTClient_publishMessage(client_, topic, &pubmsg, &token);
    if (rc != MQTTCLIENT_SUCCESS)
    {
        logError("MQTTClient_publishMessage", rc);
        return rc;
    }

    // Wait for completion
    const int TIMEOUT_MS = 1000;
    rc = MQTTClient_waitForCompletion(client_, token, TIMEOUT_MS);
    if (rc != MQTTCLIENT_SUCCESS) {
        log_message(LOG_ERROR, "%s: publish: waitForCompletion failed on topic '%s' (rc=%d)",
                    broker_name_.c_str(), topic, rc);
    }

    return rc;
}

int MQTTClientBase::subscribe(const char* topic, int qos)
{
    std::lock_guard<std::mutex> lock(mqtt_mutex_);

    if (!client_)
    {
        log_message(LOG_ERROR, "%s: NULL client - cannot subscribe", broker_name_.c_str());
        return MQTTCLIENT_FAILURE;
    }

    if (!MQTTClient_isConnected(client_))
    {
        log_message(LOG_ERROR, "%s: Cannot subscribe - client disconnected", 
                    broker_name_.c_str());
        return MQTTCLIENT_DISCONNECTED;
    }

    int rc = MQTTClient_subscribe(client_, topic, qos);

    if (rc == MQTTCLIENT_SUCCESS)
    {
        log_message(LOG_INFO, "%s: Successfully subscribed to topic: %s", 
                    broker_name_.c_str(), topic);
    }
    else
    {
        logError("MQTTClient_subscribe", rc);
    }

    return rc;
}

int MQTTClientBase::reconnect(const char* last_will_topic,
                              const char* last_will_message,
                              int qos)
{
   // log_message(LOG_INFO, "%s: Attempting reconnection...", broker_name_.c_str());

    // Disconnect and cleanup existing connection
    disconnect();

    // Wait a moment before reconnecting
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Reconnect
    return connect(last_will_topic, last_will_message, qos);
}

void MQTTClientBase::disconnect()
{
    std::lock_guard<std::mutex> lock(mqtt_mutex_);

    if (client_)
    {
        if (MQTTClient_isConnected(client_))
        {
            MQTTClient_disconnect(client_, 10000);
            log_message(LOG_INFO, "%s: Disconnected from MQTT broker", broker_name_.c_str());
        }
        
        MQTTClient_destroy(&client_);
        client_ = nullptr;
    }

    is_connected_ = false;
}

bool MQTTClientBase::isConnected() const
{
    std::lock_guard<std::mutex> lock(mqtt_mutex_);
    return client_ && MQTTClient_isConnected(client_);
}

// ============================================================================
// Callback Handling
// ============================================================================

int MQTTClientBase::messageCallbackWrapper(void* context, char* topicName,
                                           [[maybe_unused]] int topicLen, MQTTClient_message* message)
{
    if (!context)
    {
        log_message(LOG_ERROR, "NULL context in message callback");
        MQTTClient_freeMessage(&message);
        MQTTClient_free(topicName);
        return 1;
    }

    // Cast context back to MQTTClientBase and call virtual function
    MQTTClientBase* instance = static_cast<MQTTClientBase*>(context);
    return instance->handleMessage(topicName, message);
}

// ============================================================================
// Protected Helper Functions
// ============================================================================

std::string MQTTClientBase::buildDefaultLastWillMessage()
{
    // Default Last Will message
    char lwt[128];
    snprintf(lwt, sizeof(lwt), "{\"imei\":\"%s\"}", client_id_.c_str());
    return std::string(lwt);
}

void MQTTClientBase::logError(const char* function_name, int error_code) const
{
    log_message(LOG_ERROR, "%s: %s failed - %s", 
                broker_name_.c_str(), function_name, getErrorString(error_code));
}

const char* MQTTClientBase::getErrorString(int error_code) const
{
    switch (error_code)
    {
    case 0:  return "SUCCESS (0): Operation completed successfully";
    case -1: return "FAILURE (-1): Generic error";
    case -2: return "PERSISTENCE_ERROR (-2): Persistent store problem";
    case -3: return "DISCONNECTED (-3): Client is disconnected";
    case -4: return "MAX_MESSAGES_INFLIGHT (-4): Max messages in-flight reached";
    case -5: return "BAD_UTF8_STRING (-5): Invalid UTF-8 string";
    case -6: return "NULL_PARAMETER (-6): NULL parameter supplied";
    case -7: return "TOPICNAME_TRUNCATED (-7): Topic truncated";
    case -8: return "BAD_STRUCTURE (-8): Invalid structure";
    case -9: return "BAD_QOS (-9): QoS out of range";
    case -10: return "SSL_NOT_SUPPORTED (-10): SSL not supported";
    case -11: return "BAD_MQTT_VERSION (-11): Unrecognized MQTT version";
    case -12: return "BAD_PROTOCOL (-12): Protocol error";
    case -13: return "BAD_MQTT_OPTION (-13): Invalid MQTT option";
    case -14: return "WRONG_MQTT_VERSION (-14): Wrong MQTT version";
    case -15: return "0_LEN_WILL_TOPIC (-15): Zero length will topic";
    case 1:  return "BAD_PROTOCOL_VERSION (1): Incorrect protocol version";
    case 2:  return "BAD_CLIENT_ID (2): Invalid client identifier";
    case 3:  return "BROKER_UNAVAILABLE (3): Server unavailable";
    case 4:  return "BAD_USERNAME_OR_PASSWORD (4): Bad credentials";
    case 5:  return "NOT_AUTHORIZED (5): Not authorized";
    default: return "UNKNOWN_ERROR: Unknown MQTT error code";
    }
}
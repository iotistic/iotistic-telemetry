#include "utils.hpp"
#include "config.hpp"
#include "main.hpp"
#include "redis.hpp"

//todo impreve logging
static int signal_counter = 0;

void setup_timer(timer_t *timer_id, int initial_sec, int interval_sec, void (*handler)(int))
{
    struct sigaction sa;
    struct itimerspec timer_spec;
    struct sigevent sev;

    // Set up signal handler
    sa.sa_flags = 0;
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);

    // Increment signal_counter for a unique signal
    signal_counter++;
    int signal_num = SIGRTMIN + signal_counter;

    if (signal_num > SIGRTMAX)
    {
        log_message(LOG_ERROR, "Exceeded available real-time signals");

        return;
    }

    if (sigaction(signal_num, &sa, NULL) == -1)
    {
        log_message(LOG_ERROR, "Error setting up signal action");

        return;
    }

    // Configure sigevent for timer
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = signal_num;
    sev.sigev_value.sival_ptr = timer_id;

    if (timer_create(CLOCK_REALTIME, &sev, timer_id) == -1)
    {
        log_message(LOG_ERROR, "Error creating timer");
        return;
    }

    timer_spec.it_value.tv_sec = initial_sec;
    timer_spec.it_value.tv_nsec = 0;
    timer_spec.it_interval.tv_sec = interval_sec;
    timer_spec.it_interval.tv_nsec = 0;

    if (timer_settime(*timer_id, 0, &timer_spec, NULL) == -1)
    {
        log_message(LOG_ERROR, "Error setting timer");
        return;
    }

    //log_message(LOG_DEBUG, "Timer %d set with %d sec initial delay and %d sec interval", signal_counter, initial_sec, interval_sec);
}

void runing_payload_timer_callback([[maybe_unused]] int signum)
{
    config.timer.publish_run_message.IsActive = true;
}

void basic_payload_timer_callback([[maybe_unused]] int signum)
{
    config.timer.publish_basic_message.IsActive = true;
}



void setup_timers(void)
{
    timer_t runing_payload_timer_id, basic_payload_timer_id;
    setup_timer(&runing_payload_timer_id,config.timer.publish_run_message.period, config.timer.publish_run_message.period, runing_payload_timer_callback);
    setup_timer(&basic_payload_timer_id, config.timer.publish_basic_message.period, config.timer.publish_basic_message.period, basic_payload_timer_callback);
}

/**
 * @brief Logging function with level support
 * @param level Log level (DEBUG, INFO, ERROR, WARNING)
 * @param format Printf-style format string
 * @param ... Variable arguments for format string
 */
void log_message(LogLevel level, const char *format, ...)
{
    va_list args;
    va_start(args, format);

    switch (level)
    {
    case LOG_DEBUG:
        std::cerr << "[DEBUG] ";
        break;
    case LOG_INFO:
        std::cerr << "[INFO] ";
        break;
    case LOG_ERROR:
        std::cerr << "[ERROR] ";
        break;
    case LOG_WARNING:
        std::cerr << "[WARNING] ";
        break;
    }

    vfprintf(stderr, format, args);
    std::cerr << std::endl;
    va_end(args);
    std::cerr.flush();
}


void convert_to_epoch_string(const char *gps_date, const char *gps_utc, char *output_buf, size_t buf_size)
{
    struct tm t = {};
    time_t epoch_time = 0;

    if (sscanf(gps_date, "%2d%2d%2d", &t.tm_mday, &t.tm_mon, &t.tm_year) == 3)
    {
        if (sscanf(gps_utc, "%2d%2d%2d", &t.tm_hour, &t.tm_min, &t.tm_sec) == 3)
        {
            t.tm_mon -= 1;
            t.tm_year += 100;
            epoch_time = mktime(&t);
        }
        else
        {
            log_message(LOG_ERROR, "Failed parsing time: %s", gps_utc);
        }
    }
    else
    {
        log_message(LOG_ERROR, "Failed parsing date: %s", gps_date);
    }

    snprintf(output_buf, buf_size, "%ld", epoch_time);
}

void calculate_uptime_string(int uptime_seconds, std::string &out)
{
    int days = uptime_seconds / 86400;
    int hours = (uptime_seconds % 86400) / 3600;
    int minutes = (uptime_seconds % 3600) / 60;

    char tmp[64];
    snprintf(tmp, sizeof(tmp), "%d days %02d:%02d", days, hours, minutes);

    out = tmp;
}

int validate_signal_strength(const char *signalStrengthStr)
{
    if (signalStrengthStr == NULL)
    {
        log_message(LOG_ERROR, "Signal strength is NULL");

        return -1; // Return an invalid value
    }

    char *endptr;
    long signalStrength = strtol(signalStrengthStr, &endptr, 10);

    // Check for conversion errors
    if (*endptr != '\0' || signalStrength < 0 || signalStrength > 31)
    {
        log_message(LOG_ERROR, "Invalid signal strength value '%s'", signalStrengthStr);

        return -1; // Return an invalid value
    }

    // Validate range
    if (signalStrength >= 0 && signalStrength <= 31)
    {

        log_message(LOG_DEBUG, "Signal strength is valid: %ld", signalStrength);
        return (int)signalStrength;
    }
    else
    {
        log_message(LOG_ERROR, "Signal strength out of range (0-31): %ld", signalStrength);
        return -1; // Return an invalid value
    }
}

uint8_t input_bitmask()
{
  uint8_t input_mask = 0;                // Initialize to zero before setting bits
   input_mask |= m2nvr_data.io.input1 << 0 ; //set bit zero
   input_mask |=m2nvr_data.io.input2 <<1 ; //set bit one
  return input_mask;
}


uint8_t output_bitmask()
{
  uint8_t output_mask = 0;                // Initialize to zero before setting bits
   output_mask |= m2nvr_data.io.out1 << 0 ; //set bit zero
   output_mask |=m2nvr_data.io.out2 <<1 ; //set bit one
  return output_mask;
}


std::string get_signal_quality(int rssi)
{
    // Handle valid RSSI range (0-31)
    if (rssi >= 0 && rssi <= 31)
    {
      ////RSRP (Reference Signal Received Power)
        int rsrp = -113 + (rssi * 2);

        if (rsrp >= -80)
            return "Excellent";
        else if (rsrp >= -100)
            return "Good";
        else if (rsrp >= -120)
            return "Poor";
        else
            return "VeryPoor";
    }
    // Handle unknown/invalid RSSI (99 or out of range)
    else
    {
        return "VeryPoor";
    }
}


#include <string.h>

int extract_value_from_brackets(const char* str, const char* key) {
    const char* pos = strstr(str, key);
    if (!pos) return -1;
    pos = strchr(pos, '[');
    if (!pos) return -1;
    int value = 0;
    sscanf(pos, "[%d]", &value);
    return value;
}
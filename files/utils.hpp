#ifndef UTILS_H
#define UTILS_H

#include <signal.h>
#include <time.h>
#include <unistd.h> // For usleep
#include <stdlib.h>
#include "main.hpp"
#include <iostream>
#include <cmath>  // Add this at the top


void setup_timer(timer_t *timer_id, int initial_sec, int interval_sec, void (*handler)(int));
void setup_timers(void);
void publish_off_line_payload_timer_callback(int signum);
void runing_payload_timer_callback(int signum);
void basic_payload_timer_callback(int signum);
void iotistic_run_message_timer_callback(int signum);

void calculate_uptime_string(int uptime_seconds, std::string &out);

// Log level enum
typedef enum
{
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
} LogLevel;

#include <stdarg.h>
// Centralized logging function
void log_message(LogLevel level, const char *format, ...);


uint8_t input_bitmask();
uint8_t output_bitmask();
std::string get_signal_quality(int rssi); 
int extract_value_from_brackets(const char* str, const char* key);

#endif 
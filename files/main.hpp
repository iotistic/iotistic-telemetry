#ifndef MAIN_H
#define MAIN_H
#include <stdbool.h>
#include <stdint.h>
#include "utils.hpp"
#include "redis.hpp"
#include <iostream>
#include <thread>          
#include <chrono> 

// Redis value definitions
#define REDIS_VALUE_DOUT1_ON "{\"dout1_data\": 49}"
#define REDIS_VALUE_DOUT1_OFF "{\"dout1_data\": 48}" // redis-cli SET usb_command '{"dout1_data": 48}'
#define REDIS_VALUE_DOUT2_ON "{\"dout2_data\": 49}"
#define REDIS_VALUE_DOUT2_OFF "{\"dout2_data\": 48}"
#define REDIS_VALUE_SET_IMU "{\"gyro_conf\": \"27690960100501\"}" // redis-cli SET usb_command '{"gyro_conf": "27690960100501"}'

typedef struct
{
    uint8_t rssi;
    char net_operator[20];
    char access_technology[25];
    std::string signal_quality;
    char ICCID[25];
} st_simCard;

typedef struct
{
    char IMEI[20];
    char IMSI[20];
    char type[10];
    char revision[25];
} st_moduel;

typedef struct
{
    float Longitude;
    float Latitude;
    float Altitude;
    float Speed_kmh; // km/H
    float direction;
    uint8_t Number_Of_Satellites;
    float Horizontal_Precision; //hdop
    char gps_utc[10];
    char gps_date[10];
} st_GPS;

typedef struct
{
    int os_uptime_seconds;
    std::string os_uptime;
    float cpu_usage;
    float cpu_temp;
    int64_t epoic_time;
    bool internetConnection; 
    int rx_bytes;
    int tx_bytes;
    short int shutdown_counter; //it's will chagne when ignition off countdown 
    short int shutdown_timer; // its; fix and accutal timer config //todo will be remvoe form flespi 
} st_device_info;

typedef struct
{
    // float vehicle_mileage_km;
    // float vehicle_speed_kmph;
    // float fuel_level;
    // char Diagnostic_trouble_code[10];
    short int speed_kmh; 
    short int coolant_temp; 
    short int engine_rpm; 
    short int engine_load; 
} st_canbus;

typedef struct
{
    float accel_x;
    float accel_y;
    float accel_z;
    float gyro_x;
    float gyro_y;
    float gyro_z;
    float gyro_temp;
} st_IMU_Data;

typedef struct
{
    int gyro_range;
    float gyro_sampling_freq;
    int accelerometer_range;
    float accelerometer_sampling_freq;
    int gyroscope_temp_bandwidth;
    int gyroscope_bandwidth;
    int accelerometer_bandwidth;
} st_IMU_Config;

typedef struct
{
    float bat_voltage;
    float soc_input_voltage;
    float soc_output_voltage;
    float controller_temp;
    float usb_voltage;
} st_Power_Data;

typedef struct
{
    int quectel_power;
    int main_power;
    int out1;
    int out2;
    int ignition;
    int input1;
    int input2;
    bool LDR_status; 
    float analog2; 
} st_IO_Data;

typedef struct
{
    float initial_fee;
    float initial_distance;
    int initial_time;
    float tariff_per_km;
    float tariff_per_hour;
} st_Tariff;

typedef struct
{
    float time;
    float distance;
    float total_distance;
    float fare;
    int time_pulses;
    int distance_pulses;
    char status[20]; // "Stopped" or "Running"
} st_Trip_Info;

typedef struct
{
    char current[10];    // "Day" or "Night"
    char start_time[20]; // "01:54:00 PM"
    char end_time[20];   // "01:54:59 PM"
    float cross_over_speed;
} st_DayNight_Info;

typedef struct
{
    char time[20]; // "06:46:11 AM"
    char date[20]; // "2025-07-08"
    int day;       // 2 (Tuesday)
} st_RTC;

typedef struct
{
    st_Tariff day_tariff;
    st_Tariff night_tariff;
    st_Trip_Info trip;
    st_DayNight_Info daynight;

    int shutdown_counter;
    int flash_counter;
    int k_constant;
    int calc_flag;
    char mode[10];               // "Double"
    char calculation_method[20]; // "time.dist"
    float speed;

    double totalizer_fare;
    double totalizer_distance_hired;
    double totalizer_distance;
    int totalizer_number_of_journeys;

    char time_block_status[10]; 
    char time_block_output[10]; 
    short int signal_error_flag; 
    short int voltage_error_flag; 
    short int rtc_error_flag; 
} st_Taximeter;


#define MAX_CAMS 16 // Define a reasonable maximum number of cameras
// typedef struct
// {
//     uint8_t cams_status[MAX_CAMS]; // online/offline status for each camera
// } st_cams;

typedef struct
{
    st_simCard simcard;
    st_GPS gps;
    st_device_info device_info;
    st_moduel moduel;
    st_canbus canbus;
    st_Taximeter taximeter;
    uint8_t cams_status[MAX_CAMS]; // A more direct way to store camera statuses
    st_IMU_Data imu;
    st_IMU_Config imu_config;
    st_Power_Data power;
    st_IO_Data io;
    st_RTC RTC;

} st_m2nvr_data;

extern st_m2nvr_data m2nvr_data;


#endif
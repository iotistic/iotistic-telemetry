#ifndef IOTIST_MESSAGING_PROTOCOL_H
#define IOTIST_MESSAGING_PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// All these Topecs follows IOTISTIC Messaging Protocol V2.3
// Basic Message Format
#define IDENTIFIER_TOPIC "ident"               // Device unique identifier (IMEI)
#define IDENTIFIER_SH_TOPIC "idt"              // Device unique identifier (IMEI)
#define MESSAGE_TYPE_TOPIC "message.type"               //"messageType"  // Message type: basic(B), event(E), running(R)
#define MESSAGE_TYPE_SH_TOPIC "mty"            // Message type: basic(B), event(E), running(R)
#define TIMESTAMP_TOPIC "timestamp"            // Message timestamp
#define TIMESTAMP_SH_TOPIC "tms"               // Message timestamp
#define ALTITUDE_TOPIC "position.altitude"     // Altitude value for position
#define ALTITUDE_SH_TOPIC "p.a"                // Altitude value for position
#define LATITUDE_TOPIC "position.latitude"     // Latitude coordinate value
#define LATITUDE_SH_TOPIC "p.la"               // Latitude coordinate value
#define LONGITUDE_TOPIC "position.longitude"   // Longitude coordinate value
#define LONGITUDE_SH_TOPIC "p.lo"              // Longitude coordinate value
#define DIRECTION_TOPIC "position.direction"   // Heading angle at position detection moment
#define DIRECTION_SH_TOPIC "p.di"              // Heading angle at position detection moment
#define TRIP_MILEAGE_TOPIC "trip.mileage"      // Distance driven since engine start
#define TRIP_MILEAGE_SH_TOPIC "t.ds"           // Distance driven since engine start
#define SPEED_TOPIC "position.speed"           // Instant speed at position detection moment
#define SPEED_SH_TOPIC "p.sp"                  // Instant speed at position detection moment
#define SATELLITES_TOPIC "position.satellites" // Quantity of the GPS satellites
#define SATELLITES_SH_TOPIC "p.sa"             // Quantity of the GPS satellites
#define HDOP_TOPIC "position.hdop" // Quantity of the GPS satellites
#define HDOP_SH_TOPIC "p.hd"             // Quantity of the GPS satellites
// Running Message Format
#define CAN_MILEAGE_TOPIC "can.vehicle.mileage"                 // CAN data on distance driven
#define CAN_MILEAGE_SH_TOPIC "c.ds"                            // CAN data on distance driven
#define CAN_SPEED_TOPIC "can.vehicle.speed"                    // CAN data on current speed
#define CAN_SPEED_SH_TOPIC "c.sp"                              // CAN data on current speed
#define X_AXIS_ANGLE_TOPIC "gz"                      // X-axis angle to horizon (Troubleshooting)
#define X_AXIS_ANGLE_SH_TOPIC "xa.a"                           // X-axis angle to horizon (Troubleshooting)
#define Y_AXIS_ANGLE_TOPIC "gy"                      // Y-axis angle to horizon (Troubleshooting)
#define Y_AXIS_ANGLE_SH_TOPIC "ya.a"                           // Y-axis angle to horizon (Troubleshooting)
#define Z_AXIS_ANGLE_TOPIC "gz"                      // Z-axis angle to horizon (Troubleshooting)
#define Z_AXIS_ANGLE_SH_TOPIC "za.a"                           // Z-axis angle to horizon (Troubleshooting)
#define Y_ACCELERATION_TOPIC "y.acceleration"                  // Accelerometer value Y (Troubleshooting)
#define Y_ACCELERATION_SH_TOPIC "y.ac"                         // Accelerometer value Y (Troubleshooting)
#define Z_ACCELERATION_TOPIC "z.acceleration"                  // Accelerometer value Z (Troubleshooting)
#define Z_ACCELERATION_SH_TOPIC "z.ac"                         // Accelerometer value Z (Troubleshooting)
#define X_ACCELERATION_TOPIC "x.acceleration"                  // Accelerometer value X (Troubleshooting)
#define X_ACCELERATION_SH_TOPIC "x.ac"                         // Accelerometer value X (Troubleshooting)
#define ACCESS_TECH_TOPIC "access.technology"                  // The type of the Signal Coverage
#define ACCESS_TECH_SH_TOPIC "act"                             // The type of the Signal Coverage
#define AXLE_WEIGHT_TOPIC "weight.axle"                        // Weight sensor value
#define AXLE_WEIGHT_SH_TOPIC "w.ax"                            // Weight sensor value
#define CPU_TEMP_TOPIC "cpu.temperature"                       // The current CPU temperature
#define CPU_TEMP_SH_TOPIC "cp.t"                               // The current CPU temperature
#define DEVICE_TEMP_TOPIC "device.temperature"                 // The current device temperature
#define DEVICE_TEMP_SH_TOPIC "de.t"                            // The current device temperature
#define EXT_POWER_VOLTAGE_TOPIC "external.powersource.voltage" // The voltage of the external power (Battery)
#define EXT_POWER_VOLTAGE_SH_TOPIC "ex.v"                      // The voltage of the external power (Battery)
#define CPU_USAGE_TOPIC "cpu.utilization"                      // The current CPU usage in percentage
#define CPU_USAGE_SH_TOPIC "cp.ut"                             // The current CPU usage in percentage
#define SIGNAL_LEVEL_TOPIC "gsm.signal.level"                      // 4G signal level
#define SIGNAL_LEVEL_SH_TOPIC "s.lev"                          // 4G signal level
#define SIGNAL_QUALITY_TOPIC "gsm.signal.quality"                      // 4G QUALITY level
#define SIGNAL_QUALITY_SH_TOPIC "s.qua"                          // 4G QUALITY level
#define DEVICE_UPTIME_TOPIC "device.uptime"                    // Up time from ignition on
#define DEVICE_UPTIME_SH_TOPIC "u.time"                        // Up time from ignition on
#define HUMIDITY_TOPIC "humidity"                    // Up time from ignition on
#define HUMIDITY_SH_TOPIC "hum"                        // Up time from ignition on
#define ANALOG_IN_TOPIC "analog.in"                    // List of Analog input Voltage
#define ANALOG_IN_SH_TOPIC "a.in"                        // List of Analog input Voltage

// Info Message Format
#define SIM_IMSI_TOPIC "gsm.sim.imsi" // IMSI of the SIM card
#define SIM_ICCID_TOPIC "gsm.sim.iccid" // Integrated Circuit Card ID of SIM card

// Event Message Format
#define E_GSM_SIGNAL_LEVEL_TOPIC "gsm.signal.level"
#define E_GSM_SIGNAL_LEVEL_SH_TOPIC "gsl"
#define E_HARSH_CORNERING_TOPIC "harsh.cornering"
#define E_HARSH_CORNERING_SH_TOPIC "h.ce"
#define E_HARSH_BRAKING_TOPIC "harsh.braking"
#define E_HARSH_BRAKING_SH_TOPIC "h.be"
#define E_HARSH_ACCELERATION_TOPIC "harsh.acceleration"
#define E_HARSH_ACCELERATION_SH_TOPIC "h.ae"
#define E_CRASH_TOPIC "crash"
#define E_CRASH_SH_TOPIC "Cev"
#define E_DIGITAL_INPUT_TOPIC "din" // Digital inputs bitmask
#define E_DIGITAL_OUTPUT_TOPIC "dout" // Digital inputs bitmask
#define E_ANALOG_INPUT_TOPIC "ain"  // Analog inputs 
#define E_ENGINE_EGNITION_STATUS_TOPIC "engine.ignition.status"
#define E_ENGINE_EGNITION_STATUS_SH_TOPIC "Ign"

// Define the structure for the basic message format
typedef struct
{
    char identifier[30];
    char message_type[30];
    char timestamp[30];
    char altitude[30];
    char latitude[30];
    char longitude[30];
    char direction[30];
    char hdop[30];
    char speed[30];
    char satellites[30];
} basic_attributes;

// Define the structure for the info message format
typedef struct
{
    char imsi[20];
    char iccid[25];
    char device_name[64];
} info_attributes;

// Define the structure for the run message format
typedef struct
{
    char x_axis_angle[25];
    char y_axis_angle[25];
    char z_axis_angle[25];
    char x_acceleration[25];
    char y_acceleration[25];
    char z_acceleration[25];
    char access_tech[25];
    char device_temp[30];
    char device_uptime[30];
    char ext_power_voltage[30];
    char signal_level[25];
    char signal_quality[25];
    char humidity[20];  //they useit on event but it's didn't clarify yet so it on runing now
    char cpu_temp[20];
    char cpu_usage[20];
    char analog_in[20];
} run_attributes;

typedef struct
{   char harsh_cornering[30];
    char harsh_breaking[30];
    char harsh_acceleration[30];
    char crash[30];
    char digitalInputBitmask[30];
    char ignition_status[30];
} event_attributes;

// Define the main structure that combines all message formats
typedef struct
{
    info_attributes info;
    basic_attributes basic;
    run_attributes run;
    event_attributes event;
} IMP_attributes;
extern IMP_attributes IMP_attribute;

void IMP_use_short_attributes(IMP_attributes *attribute);
void IMP_use_full_attributes(IMP_attributes *attribute);

#ifdef __cplusplus
}
#endif

#endif
/**
 * @file IOTISTIC_Messaging_Protocol.cpp
 * @brief Implementation of Flespi Messaging Protocol
 * 
 * This file is compatible with both C and C++ compilation
 */

#include "IOTISTIC_Messaging_Protocol.hpp"

#ifdef __cplusplus
extern "C" {
#endif

// use short message in case received a request from http server
void IMP_use_short_attributes(IMP_attributes *attribute)
{
    // INFO
    strcpy(attribute->info.imsi, SIM_IMSI_TOPIC);
    strcpy(attribute->info.iccid, SIM_ICCID_TOPIC);

    // BASIC
    strcpy(attribute->basic.identifier, IDENTIFIER_TOPIC);
    strcpy(attribute->basic.message_type, MESSAGE_TYPE_SH_TOPIC);
    strcpy(attribute->basic.timestamp, TIMESTAMP_SH_TOPIC);
    strcpy(attribute->basic.altitude, ALTITUDE_SH_TOPIC);
    strcpy(attribute->basic.latitude, LATITUDE_SH_TOPIC);
    strcpy(attribute->basic.longitude, LONGITUDE_SH_TOPIC);
    strcpy(attribute->basic.direction, DIRECTION_SH_TOPIC);
    strcpy(attribute->basic.speed, SPEED_SH_TOPIC);
    strcpy(attribute->basic.satellites, SATELLITES_SH_TOPIC);
    strcpy(attribute->basic.hdop, HDOP_SH_TOPIC);

    // RUN
    strcpy(attribute->run.x_axis_angle, X_AXIS_ANGLE_SH_TOPIC);
    strcpy(attribute->run.y_axis_angle, Y_AXIS_ANGLE_SH_TOPIC);
    strcpy(attribute->run.z_axis_angle, Z_AXIS_ANGLE_SH_TOPIC);
    strcpy(attribute->run.x_acceleration, X_ACCELERATION_SH_TOPIC);
    strcpy(attribute->run.y_acceleration, Y_ACCELERATION_SH_TOPIC);
    strcpy(attribute->run.z_acceleration, Z_ACCELERATION_SH_TOPIC);
    strcpy(attribute->run.access_tech, ACCESS_TECH_SH_TOPIC);
    strcpy(attribute->run.device_temp, DEVICE_TEMP_SH_TOPIC);
    strcpy(attribute->run.ext_power_voltage, EXT_POWER_VOLTAGE_SH_TOPIC);
    strcpy(attribute->run.signal_level, SIGNAL_LEVEL_SH_TOPIC);
    strcpy(attribute->run.signal_quality, SIGNAL_QUALITY_SH_TOPIC);
    strcpy(attribute->run.cpu_temp, CPU_TEMP_SH_TOPIC);
    strcpy(attribute->run.cpu_usage, CPU_USAGE_SH_TOPIC);
    strcpy(attribute->run.humidity, HUMIDITY_SH_TOPIC);
    strcpy(attribute->run.analog_in, ANALOG_IN_SH_TOPIC);
    strcpy(attribute->run.device_uptime, DEVICE_UPTIME_SH_TOPIC);

    // event
    strcpy(attribute->event.crash, E_CRASH_TOPIC);
    strcpy(attribute->event.harsh_acceleration, E_HARSH_ACCELERATION_TOPIC);
    strcpy(attribute->event.harsh_breaking, E_HARSH_BRAKING_TOPIC);
    strcpy(attribute->event.harsh_cornering, E_HARSH_CORNERING_TOPIC);
    strcpy(attribute->event.digitalInputBitmask, E_DIGITAL_INPUT_TOPIC);
    strcpy(attribute->event.ignition_status, E_ENGINE_EGNITION_STATUS_TOPIC);
}

// use full message by default
void IMP_use_full_attributes(IMP_attributes *attribute)
{
    // INFO
    strcpy(attribute->info.imsi, SIM_IMSI_TOPIC);
    strcpy(attribute->info.iccid, SIM_ICCID_TOPIC);

    // BASIC
    strcpy(attribute->basic.identifier, IDENTIFIER_TOPIC);
    strcpy(attribute->basic.message_type, MESSAGE_TYPE_TOPIC);
    strcpy(attribute->basic.timestamp, TIMESTAMP_TOPIC);
    strcpy(attribute->basic.altitude, ALTITUDE_TOPIC);
    strcpy(attribute->basic.latitude, LATITUDE_TOPIC);
    strcpy(attribute->basic.longitude, LONGITUDE_TOPIC);
    strcpy(attribute->basic.direction, DIRECTION_TOPIC);
    strcpy(attribute->basic.speed, SPEED_TOPIC);
    strcpy(attribute->basic.satellites, SATELLITES_TOPIC);
    strcpy(attribute->basic.hdop, HDOP_TOPIC);

    // RUN
    strcpy(attribute->run.x_axis_angle, X_AXIS_ANGLE_TOPIC);
    strcpy(attribute->run.y_axis_angle, Y_AXIS_ANGLE_TOPIC);
    strcpy(attribute->run.z_axis_angle, Z_AXIS_ANGLE_TOPIC);
    strcpy(attribute->run.x_acceleration, X_ACCELERATION_TOPIC);
    strcpy(attribute->run.y_acceleration, Y_ACCELERATION_TOPIC);
    strcpy(attribute->run.z_acceleration, Z_ACCELERATION_TOPIC);
    strcpy(attribute->run.access_tech, ACCESS_TECH_TOPIC);
    strcpy(attribute->run.device_temp, DEVICE_TEMP_TOPIC);
    strcpy(attribute->run.ext_power_voltage, EXT_POWER_VOLTAGE_TOPIC);
    strcpy(attribute->run.signal_level, SIGNAL_LEVEL_TOPIC);
    strcpy(attribute->run.signal_quality, SIGNAL_QUALITY_TOPIC);
    strcpy(attribute->run.cpu_temp, CPU_TEMP_TOPIC);
    strcpy(attribute->run.cpu_usage, CPU_USAGE_TOPIC);
    strcpy(attribute->run.humidity, HUMIDITY_TOPIC);
    strcpy(attribute->run.analog_in, ANALOG_IN_TOPIC);
    strcpy(attribute->run.device_uptime, DEVICE_UPTIME_TOPIC);

    // event
    strcpy(attribute->event.crash, E_CRASH_TOPIC);
    strcpy(attribute->event.harsh_acceleration, E_HARSH_ACCELERATION_TOPIC);
    strcpy(attribute->event.harsh_breaking, E_HARSH_BRAKING_TOPIC);
    strcpy(attribute->event.harsh_cornering, E_HARSH_CORNERING_TOPIC);
    strcpy(attribute->event.digitalInputBitmask, E_DIGITAL_INPUT_TOPIC);
    strcpy(attribute->event.ignition_status, E_ENGINE_EGNITION_STATUS_TOPIC);
}

#ifdef __cplusplus
}
#endif
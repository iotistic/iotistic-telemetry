cmake_minimum_required(VERSION 3.10)

message("Building IOTISTIC MQTT Project (C++)")

project(iotistic_mqtt CXX)

# Set C++ standard (matching BitBake recipe)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set output binary name
set(EXECUTABLE_NAME iotistic-mqtt)

# Add source files
set(SOURCES
    main.cpp
    redis.cpp
    SqlDatabase.cpp
    MQTTClientBase.cpp
    IOTISTIC_Messaging_Protocol.cpp
    FlespiMQTT.cpp
    config.cpp
    utils.cpp
)

# Add header include path
include_directories(${CMAKE_SOURCE_DIR})

# Find required libraries
find_library(PAHO_MQTT_LIBRARIES paho-mqtt3c)
find_library(CJSON_LIBRARIES cjson)
find_library(HIREDIS_LIBRARIES hiredis)
find_library(SQLITE3_LIBRARIES sqlite3)

# Find Threads (recommended way for modern CMake)
find_package(Threads REQUIRED)

# Add required libraries
set(LIBS
    ${PAHO_MQTT_LIBRARIES}
    ${HIREDIS_LIBRARIES}
    ${CJSON_LIBRARIES}
    ${SQLITE3_LIBRARIES}
    m          # Required for math functions
    pthread    # Required for threading
    rt         # Required for timers/shared memory
)

# Add include directories from pkg-config (optional)
include_directories(
    ${PAHO_MQTT_INCLUDE_DIRS}
    ${HIREDIS_INCLUDE_DIRS}
    ${CJSON_INCLUDE_DIRS}
    ${SQLITE3_INCLUDE_DIRS}
)

# Create the executable
add_executable(${EXECUTABLE_NAME} ${SOURCES})

# Link with required libraries
target_link_libraries(${EXECUTABLE_NAME}
    ${LIBS}
    Threads::Threads
)

# Enable compiler warnings (best practice)
target_compile_options(${EXECUTABLE_NAME} PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

message("CMake configuration completed successfully")

# Installation (optional - useful for testing)
install(TARGETS ${EXECUTABLE_NAME}
    RUNTIME DESTINATION bin
)

## Required packages for Ubuntu/Debian:
## sudo apt install libpaho-mqtt-dev libhiredis-dev libcjson-dev libsqlite3-dev cmake pkg-config build-essential g++
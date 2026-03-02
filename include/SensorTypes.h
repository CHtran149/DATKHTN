// Shared sensor message types and externs
#pragma once

#include <Arduino.h>

typedef struct {
    float temperature;
    float humidity;
    float pressure;
    float altitude;
    float wind_speed;
    float rainfall;

    int lat_deg, lat_min;
    float lat_sec;
    char lat_dir;

    int lon_deg, lon_min;
    float lon_sec;
    char lon_dir;

    uint32_t timestamp;
} SensorRaw_t;

// Queue handle declared in main.cpp
extern QueueHandle_t Queue_SensorRaw;

// Processed sensor data sent from Task_Processing to Task_FSM
typedef struct {
    float t_avg;
    float h_avg;
    float p_avg;
    float w_avg;
    float r_avg;
    float heat_index_c;
    int rain_index;
    int wind_index;
    uint32_t timestamp;
} ProcessedSensor_t;

// Alert message
typedef enum { ALERT_INFO=0, ALERT_WARNING, ALERT_DANGER, ALERT_ERROR } AlertLevel_t;

typedef struct {
    AlertLevel_t level;
    char message[128];
    uint32_t timestamp;
} Alert_t;

// Queues and EventGroup used across tasks
extern QueueHandle_t Queue_FSM_Input; // carries ProcessedSensor_t
extern QueueHandle_t Queue_Alert;     // carries Alert_t
extern EventGroupHandle_t Event_Weather;

// Additional queues and synchronization
extern QueueHandle_t Queue_Data_Blynk; // processed data for Blynk
extern QueueHandle_t Queue_Data_Cloud; // processed data for Cloud
extern QueueHandle_t Queue_Config;     // carries Config_t from Blynk to Processing
extern SemaphoreHandle_t Config_Mutex; // protects g_config

// Global configuration structure
typedef struct {
    uint32_t sample_interval_ms;
    float temp_warn;
    float temp_danger;
    float humi_warn;
    float humi_danger;
    float wind_danger;
    float rain_danger;
} Config_t;

extern Config_t g_config;

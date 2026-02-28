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

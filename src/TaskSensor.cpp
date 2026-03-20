#include "TaskSensor.h"
#include "DHT11.h"
#include "BMP180.h"
#include "WindSpeed.h"
#include "Rainfall.h"
#include "GPS.h"
#include "SensorTypes.h"

/* ===== extern object từ main.cpp ===== */
extern DHT11 dht;
extern BMP180 bmp;
extern WindSpeed wind;
extern Rainfall rain;
extern GPS gps;

/* ===== extern queue từ main.cpp ===== */
extern QueueHandle_t Queue_SensorRaw;

void Task_Sensor(void *pvParameters) {
    SensorRaw_t data;

    Serial.println("[Task_Sensor] Started");

    while (1) {
        /* ---- DHT11 ---- */
        auto dhtData = dht.read();

        if (dhtData.valid) {
            data.temperature = dhtData.temperature;
            data.humidity    = dhtData.humidity;

            Serial.print("Temp: ");
            Serial.print(data.temperature);
            Serial.print(" | Humi: ");
            Serial.println(data.humidity);
        } else {
            Serial.println("DHT11 invalid");
        }

        /* ---- BMP180 ---- */
        auto bmpData = bmp.read();

        if (bmpData.valid) {
            data.pressure = bmpData.pressure;
            data.altitude = bmpData.altitude;

            Serial.print("Pressure: ");
            Serial.print(data.pressure);
            Serial.print(" | Altitude: ");
            Serial.println(data.altitude);
        } else {
            Serial.println("BMP180 invalid");
        }

        /* ---- Wind ---- */
        auto windData = wind.read(1000);

        if (windData.valid) {
            data.wind_speed = windData.toc_do_gio;
            Serial.print("Wind: ");
            Serial.println(data.wind_speed);
        } else {
            Serial.println("Wind invalid");
        }

        /* ---- Rain ---- */
        auto rainData = rain.read();
        Serial.println("Rain read");

        if (rainData.valid) {
            data.rainfall = rainData.luongMua;
            Serial.print("Rainfall: ");
            Serial.println(data.rainfall);
        } else {
            Serial.println("Rain invalid");
        }

        /* ---- GPS ---- */
        Serial.println("Reading GPS...");
        auto gpsData = gps.read();
        Serial.println("GPS read");

        if (gpsData.valid) {
            data.lat_deg = gpsData.lat_deg;
            data.lat_min = gpsData.lat_min;
            data.lat_sec = gpsData.lat_sec;
            data.lat_dir = gpsData.lat_dir;

            data.lon_deg = gpsData.lon_deg;
            data.lon_min = gpsData.lon_min;
            data.lon_sec = gpsData.lon_sec;
            data.lon_dir = gpsData.lon_dir;

            Serial.print("Lat: ");
            Serial.print(gpsData.lat_deg);
            Serial.print("°");
            Serial.print(gpsData.lat_min);
            Serial.print("'");
            Serial.print(gpsData.lat_sec);
            Serial.print(gpsData.lat_dir);

            Serial.print(" | Lon: ");
            Serial.print(gpsData.lon_deg);
            Serial.print("°");
            Serial.print(gpsData.lon_min);
            Serial.print("'");
            Serial.print(gpsData.lon_sec);
            Serial.println(gpsData.lon_dir);
        } else {
            Serial.println("GPS invalid");
        }

        data.timestamp = millis();

        xQueueSend(Queue_SensorRaw, &data, 0);

        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}
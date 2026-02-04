#include <Arduino.h>
#include <Wire.h>

/* ===== Include module ===== */
#include "DHT11.h"
#include "BMP180.h"
#include "WindSpeed.h"
#include "Rainfall.h"
#include "GPS.h"

/* ===== PIN DEFINE ===== */
#define PIN_DHT        10
#define PIN_WIND       5
#define PIN_RAIN       7

#define SDA_PIN        13
#define SCL_PIN        12

#define GPS_RX         18
#define GPS_TX         17

/* ===== Object ===== */
DHT11 dht(PIN_DHT);
BMP180 bmp;
WindSpeed wind(PIN_WIND, 0.1f);
Rainfall rain(PIN_RAIN, 150.0f, 12000.0f, 10000);
GPS gps(Serial1);

/* ===== Queue ===== */
QueueHandle_t Queue_SensorRaw;

/* ===== Struct ===== */
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

void Task_Sensor(void *pvParameters) {
    SensorRaw_t data;

    //Serial.println("[Task_Sensor] Started");

    while (1) {
        //Serial.println("---- New Sensor Cycle ----");

        /* ---- DHT11 ---- */
        //Serial.println("Reading DHT11...");
        auto dhtData = dht.read();
        //Serial.println("DHT11 read ");

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
        //Serial.println("Reading BMP180...");
        auto bmpData = bmp.read();
        //Serial.println("BMP180 read ");

        if (bmpData.valid) {
            data.pressure = bmpData.pressure;
            data.altitude = bmpData.altitude;

            Serial.print("Pressure: ");
            Serial.print(data.pressure);
            Serial.print(" | Altitude: ");
            Serial.println(data.altitude);
        }

        /* ---- Wind ---- */
        //Serial.println("Reading Wind...");
        auto windData = wind.read(1000);
        //Serial.println("Wind read ");

        if (windData.valid) {
            data.wind_speed = windData.toc_do_gio;
            Serial.print("Wind: ");
            Serial.println(data.wind_speed);
        }

        /* ---- Rain ---- */
        //Serial.println("Reading Rain...");
        auto rainData = rain.read();
        //Serial.println("Rain read ");

        if (rainData.valid) {
            data.rainfall = rainData.luongMua;
            Serial.print("Rainfall: ");
            Serial.println(data.rainfall);
        }

        /* ---- GPS ---- */
        //Serial.println("Reading GPS...");
        auto gpsData = gps.read();
        //Serial.println("GPS read ");

        if (gpsData.valid) {
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
        }

        data.timestamp = millis();

        xQueueSend(Queue_SensorRaw, &data, 0);

        //Serial.println("Cycle done\n");

        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}
void setup() {
    Serial.begin(9600);
    delay(1000);

    Wire.begin(SDA_PIN, SCL_PIN);

    bmp.begin();
    wind.begin();
    rain.begin();
    gps.begin(115200);

    Queue_SensorRaw = xQueueCreate(5, sizeof(SensorRaw_t));

    xTaskCreatePinnedToCore(
        Task_Sensor,
        "Task_Sensor",
        8192,
        NULL,
        2,
        NULL,
        1
    );

    Serial.println("Task_Sensor created");
}
void loop() {
    // Empty. Tasks are running in FreeRTOS.
}